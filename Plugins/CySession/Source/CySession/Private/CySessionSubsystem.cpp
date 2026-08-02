// Copyright Atlas XR. All Rights Reserved.

#include "CySessionSubsystem.h"

#include "CySession.h"
#include "CySessionSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProperties.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

namespace CySessionKeys
{
	static const FName AllowCrossplay(TEXT("CY_ALLOWCROSSPLAY"));
	static const FName AllowCrossStore(TEXT("CY_ALLOWCROSSSTORE"));
	static const FName Platform(TEXT("CY_PLATFORM"));
	static const FName Store(TEXT("CY_STORE"));
	static const FName Provider(TEXT("CY_PROVIDER"));
	static const FName Mobile(TEXT("CY_MOBILE"));
	static const FName MatchType(TEXT("CY_MATCHTYPE"));
	static const FName Region(TEXT("CY_REGION"));
}

namespace
{
	FString GetSettingString(const FOnlineSessionSettings& Settings, const FName Key)
	{
		if (const FOnlineSessionSetting* Setting = Settings.Settings.Find(Key))
		{
			return Setting->Data.ToString();
		}
		return FString();
	}

	bool GetSettingBool(const FOnlineSessionSettings& Settings, const FName Key, const bool DefaultValue = false)
	{
		if (const FOnlineSessionSetting* Setting = Settings.Settings.Find(Key))
		{
			if (Setting->Data.GetType() == EOnlineKeyValuePairDataType::Bool)
			{
				bool Value = DefaultValue;
				Setting->Data.GetValue(Value);
				return Value;
			}

			const FString Value = Setting->Data.ToString();
			return Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1");
		}
		return DefaultValue;
	}

	ECySessionLoginStatus ToBlueprintLoginStatus(const ELoginStatus::Type Status)
	{
		switch (Status)
		{
		case ELoginStatus::UsingLocalProfile:
			return ECySessionLoginStatus::UsingLocalProfile;
		case ELoginStatus::LoggedIn:
			return ECySessionLoginStatus::LoggedIn;
		default:
			return ECySessionLoginStatus::NotLoggedIn;
		}
	}

	ECySessionJoinResult ToBlueprintJoinResult(const EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success:
			return ECySessionJoinResult::Success;
		case EOnJoinSessionCompleteResult::SessionIsFull:
			return ECySessionJoinResult::SessionIsFull;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			return ECySessionJoinResult::SessionDoesNotExist;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			return ECySessionJoinResult::CouldNotRetrieveAddress;
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			return ECySessionJoinResult::AlreadyInSession;
		default:
			return ECySessionJoinResult::UnknownError;
		}
	}

	ECySessionState ToBlueprintSessionState(const EOnlineSessionState::Type State)
	{
		switch (State)
		{
		case EOnlineSessionState::Creating: return ECySessionState::Creating;
		case EOnlineSessionState::Pending: return ECySessionState::Pending;
		case EOnlineSessionState::Starting: return ECySessionState::Starting;
		case EOnlineSessionState::InProgress: return ECySessionState::InProgress;
		case EOnlineSessionState::Ending: return ECySessionState::Ending;
		case EOnlineSessionState::Ended: return ECySessionState::Ended;
		case EOnlineSessionState::Destroying: return ECySessionState::Destroying;
		default: return ECySessionState::NoSession;
		}
	}

	FString NormalizeTravelOptions(FString Options)
	{
		while (Options.StartsWith(TEXT("?")))
		{
			Options.RightChopInline(1);
		}
		return Options;
	}

	FString PlatformTypeToTag(const ECySessionPlatformType Type)
	{
		switch (Type)
		{
		case ECySessionPlatformType::Windows: return TEXT("Windows");
		case ECySessionPlatformType::MacOS: return TEXT("macOS");
		case ECySessionPlatformType::Linux: return TEXT("Linux");
		case ECySessionPlatformType::IOS: return TEXT("iOS");
		case ECySessionPlatformType::Android: return TEXT("Android");
		case ECySessionPlatformType::MetaQuest: return TEXT("MetaQuest");
		case ECySessionPlatformType::PlayStation: return TEXT("PlayStation");
		case ECySessionPlatformType::Xbox: return TEXT("Xbox");
		case ECySessionPlatformType::NintendoSwitch: return TEXT("NintendoSwitch");
		default: return FString();
		}
	}

	FString StoreTypeToTag(const ECySessionStoreType Type)
	{
		switch (Type)
		{
		case ECySessionStoreType::EpicGamesStore: return TEXT("EpicGamesStore");
		case ECySessionStoreType::Steam: return TEXT("Steam");
		case ECySessionStoreType::GOG: return TEXT("GOG");
		case ECySessionStoreType::AppleAppStore: return TEXT("AppleAppStore");
		case ECySessionStoreType::GooglePlay: return TEXT("GooglePlay");
		case ECySessionStoreType::MetaHorizon: return TEXT("MetaHorizon");
		case ECySessionStoreType::AmazonAppstore: return TEXT("AmazonAppstore");
		case ECySessionStoreType::PlayStationStore: return TEXT("PlayStationStore");
		case ECySessionStoreType::MicrosoftStore: return TEXT("MicrosoftStore");
		case ECySessionStoreType::NintendoEshop: return TEXT("NintendoEshop");
		default: return FString();
		}
	}
}

void UCySessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureNotificationBindings();

	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	if (Settings && Settings->bValidateConfigurationOnStartup)
	{
		const FCySessionConfigurationReport Report = ValidateConfiguration();
		for (const FString& Error : Report.Errors)
		{
			UE_LOG(LogCySession, Error, TEXT("Configuration: %s"), *Error);
		}
		for (const FString& Warning : Report.Warnings)
		{
			UE_LOG(LogCySession, Warning, TEXT("Configuration: %s"), *Warning);
		}
	}
}

void UCySessionSubsystem::Deinitialize()
{
	ClearNotificationBindings();
	CachedNativeResults.Reset();
	CachedBlueprintResults.Reset();
	ActiveSearch.Reset();
	Super::Deinitialize();
}

FName UCySessionSubsystem::GetRequestedSubsystemName() const
{
	if (!RuntimeSubsystemOverride.IsNone())
	{
		return RuntimeSubsystemOverride;
	}
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		if (!Profile->SubsystemName.IsNone())
		{
			return Profile->SubsystemName;
		}
	}

	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	return Settings ? Settings->SubsystemName : NAME_None;
}

FName UCySessionSubsystem::GetActiveProfileName() const
{
	if (!RuntimeProfileOverride.IsNone())
	{
		return RuntimeProfileOverride;
	}

	FString CommandLineProfile;
	if (FParse::Value(FCommandLine::Get(), TEXT("CySessionProfile="), CommandLineProfile) && !CommandLineProfile.IsEmpty())
	{
		return FName(*CommandLineProfile);
	}

	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	return Settings ? Settings->ActiveProfileName : NAME_None;
}

const FCySessionPlatformProfile* UCySessionSubsystem::GetActiveProfile() const
{
	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	return Settings ? Settings->FindProfile(GetActiveProfileName()) : nullptr;
}

bool UCySessionSubsystem::GetActivePlatformProfile(FCySessionPlatformProfile& Profile) const
{
	if (const FCySessionPlatformProfile* ActiveProfile = GetActiveProfile())
	{
		Profile = *ActiveProfile;
		return true;
	}
	Profile = FCySessionPlatformProfile();
	return false;
}

bool UCySessionSubsystem::HasAnyOperationInProgress() const
{
	return bLoginInProgress || bLogoutInProgress || bCreateInProgress || bUpdateInProgress || bStartInProgress ||
		bEndInProgress || bDestroyInProgress || bFindInProgress || bCancelFindInProgress || bJoinInProgress;
}

IOnlineSubsystem* UCySessionSubsystem::GetOnlineSubsystem() const
{
	return Online::GetSubsystem(GetWorld(), GetRequestedSubsystemName());
}

IOnlineIdentityPtr UCySessionSubsystem::GetIdentityInterface() const
{
	if (IOnlineSubsystem* Subsystem = GetOnlineSubsystem())
	{
		return Subsystem->GetIdentityInterface();
	}
	return nullptr;
}

IOnlineSessionPtr UCySessionSubsystem::GetSessionInterface() const
{
	if (IOnlineSubsystem* Subsystem = GetOnlineSubsystem())
	{
		return Subsystem->GetSessionInterface();
	}
	return nullptr;
}

void UCySessionSubsystem::EnsureNotificationBindings()
{
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Identity == BoundIdentity && Sessions == BoundSessions)
	{
		return;
	}

	ClearNotificationBindings();
	BoundIdentity = Identity;
	BoundSessions = Sessions;

	if (BoundIdentity.IsValid())
	{
		LoginStatusChangedHandles.SetNum(MAX_LOCAL_PLAYERS);
		for (int32 LocalUserNum = 0; LocalUserNum < MAX_LOCAL_PLAYERS; ++LocalUserNum)
		{
			LoginStatusChangedHandles[LocalUserNum] = BoundIdentity->AddOnLoginStatusChangedDelegate_Handle(
				LocalUserNum,
				FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleLoginStatusChanged));
		}
	}

	if (BoundSessions.IsValid())
	{
		InviteAcceptedHandle = BoundSessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleSessionUserInviteAccepted));
		InviteReceivedHandle = BoundSessions->AddOnSessionInviteReceivedDelegate_Handle(
			FOnSessionInviteReceivedDelegate::CreateUObject(this, &ThisClass::HandleSessionInviteReceived));
		ParticipantJoinedHandle = BoundSessions->AddOnSessionParticipantJoinedDelegate_Handle(
			FOnSessionParticipantJoinedDelegate::CreateUObject(this, &ThisClass::HandleSessionParticipantJoined));
		ParticipantLeftHandle = BoundSessions->AddOnSessionParticipantLeftDelegate_Handle(
			FOnSessionParticipantLeftDelegate::CreateUObject(this, &ThisClass::HandleSessionParticipantLeft));
		SessionFailureHandle = BoundSessions->AddOnSessionFailureDelegate_Handle(
			FOnSessionFailureDelegate::CreateUObject(this, &ThisClass::HandleSessionFailure));
	}
}

void UCySessionSubsystem::ClearNotificationBindings()
{
	if (BoundIdentity.IsValid())
	{
		for (int32 LocalUserNum = 0; LocalUserNum < LoginStatusChangedHandles.Num(); ++LocalUserNum)
		{
			if (LoginStatusChangedHandles[LocalUserNum].IsValid())
			{
				BoundIdentity->ClearOnLoginStatusChangedDelegate_Handle(LocalUserNum, LoginStatusChangedHandles[LocalUserNum]);
			}
		}
		if (LoginHandle.IsValid())
		{
			BoundIdentity->ClearOnLoginCompleteDelegate_Handle(ActiveLoginUserNum, LoginHandle);
		}
		if (LogoutHandle.IsValid())
		{
			BoundIdentity->ClearOnLogoutCompleteDelegate_Handle(ActiveLogoutUserNum, LogoutHandle);
		}
	}
	LoginStatusChangedHandles.Reset();
	LoginHandle.Reset();
	LogoutHandle.Reset();

	if (BoundSessions.IsValid())
	{
		if (InviteAcceptedHandle.IsValid()) BoundSessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteAcceptedHandle);
		if (InviteReceivedHandle.IsValid()) BoundSessions->ClearOnSessionInviteReceivedDelegate_Handle(InviteReceivedHandle);
		if (ParticipantJoinedHandle.IsValid()) BoundSessions->ClearOnSessionParticipantJoinedDelegate_Handle(ParticipantJoinedHandle);
		if (ParticipantLeftHandle.IsValid()) BoundSessions->ClearOnSessionParticipantLeftDelegate_Handle(ParticipantLeftHandle);
		if (SessionFailureHandle.IsValid()) BoundSessions->ClearOnSessionFailureDelegate_Handle(SessionFailureHandle);
		if (CreateHandle.IsValid()) BoundSessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		if (UpdateHandle.IsValid()) BoundSessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
		if (StartHandle.IsValid()) BoundSessions->ClearOnStartSessionCompleteDelegate_Handle(StartHandle);
		if (EndHandle.IsValid()) BoundSessions->ClearOnEndSessionCompleteDelegate_Handle(EndHandle);
		if (DestroyHandle.IsValid()) BoundSessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		if (FindHandle.IsValid()) BoundSessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		if (CancelFindHandle.IsValid()) BoundSessions->ClearOnCancelFindSessionsCompleteDelegate_Handle(CancelFindHandle);
		if (JoinHandle.IsValid()) BoundSessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}

	InviteAcceptedHandle.Reset();
	InviteReceivedHandle.Reset();
	ParticipantJoinedHandle.Reset();
	ParticipantLeftHandle.Reset();
	SessionFailureHandle.Reset();
	CreateHandle.Reset();
	UpdateHandle.Reset();
	StartHandle.Reset();
	EndHandle.Reset();
	DestroyHandle.Reset();
	FindHandle.Reset();
	CancelFindHandle.Reset();
	JoinHandle.Reset();
	BoundIdentity.Reset();
	BoundSessions.Reset();

	bLoginInProgress = false;
	bLogoutInProgress = false;
	bCreateInProgress = false;
	bUpdateInProgress = false;
	bStartInProgress = false;
	bEndInProgress = false;
	bDestroyInProgress = false;
	bFindInProgress = false;
	bCancelFindInProgress = false;
	bJoinInProgress = false;
}

void UCySessionSubsystem::SetError(const FString& Error)
{
	LastError = Error;
	if (!Error.IsEmpty())
	{
		UE_LOG(LogCySession, Warning, TEXT("%s"), *Error);
	}
}

bool UCySessionSubsystem::SetSubsystemOverride(const FName SubsystemName)
{
	if (HasAnyOperationInProgress())
	{
		SetError(TEXT("Cannot change Online Subsystem while an asynchronous operation is running."));
		return false;
	}

	const FName Previous = RuntimeSubsystemOverride;
	ClearNotificationBindings();
	RuntimeSubsystemOverride = SubsystemName;
	if (!GetOnlineSubsystem())
	{
		RuntimeSubsystemOverride = Previous;
		EnsureNotificationBindings();
		SetError(FString::Printf(TEXT("Online Subsystem '%s' is not available."), *SubsystemName.ToString()));
		return false;
	}

	CachedNativeResults.Reset();
	CachedBlueprintResults.Reset();
	EnsureNotificationBindings();
	SetError(FString());
	return true;
}

bool UCySessionSubsystem::SetActiveProfileOverride(const FName ProfileName)
{
	if (HasAnyOperationInProgress())
	{
		SetError(TEXT("Cannot change CySession profile while an asynchronous operation is running."));
		return false;
	}

	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	if (!ProfileName.IsNone() && (!Settings || !Settings->FindProfile(ProfileName)))
	{
		SetError(FString::Printf(TEXT("CySession profile '%s' does not exist or is disabled."), *ProfileName.ToString()));
		return false;
	}

	ClearNotificationBindings();
	RuntimeProfileOverride = ProfileName;
	RuntimeSubsystemOverride = NAME_None;
	CachedNativeResults.Reset();
	CachedBlueprintResults.Reset();
	EnsureNotificationBindings();
	SetError(FString());
	return true;
}

bool UCySessionSubsystem::IsSubsystemAvailable() const
{
	const IOnlineSubsystem* Subsystem = GetOnlineSubsystem();
	return Subsystem && Subsystem->GetIdentityInterface().IsValid() && Subsystem->GetSessionInterface().IsValid();
}

FName UCySessionSubsystem::GetActiveSubsystemName() const
{
	if (const IOnlineSubsystem* Subsystem = GetOnlineSubsystem())
	{
		return Subsystem->GetSubsystemName();
	}
	return NAME_None;
}

FString UCySessionSubsystem::GetPlatformTag() const
{
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		if (!Profile->PlatformTag.IsEmpty()) return Profile->PlatformTag;
	}
	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	if (Settings && !Settings->PlatformTagOverride.IsEmpty())
	{
		return Settings->PlatformTagOverride;
	}
	const FString TypedTag = PlatformTypeToTag(GetPlatformType());
	return !TypedTag.IsEmpty() ? TypedTag : FString(FPlatformProperties::IniPlatformName());
}

ECySessionStoreType UCySessionSubsystem::GetStoreType() const
{
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		if (Profile->StoreType != ECySessionStoreType::Auto) return Profile->StoreType;
	}
	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	if (Settings && Settings->StoreTypeOverride != ECySessionStoreType::Auto)
	{
		return Settings->StoreTypeOverride;
	}

	const FString Provider = GetActiveSubsystemName().ToString().ToLower();
	if (Provider.Contains(TEXT("steam"))) return ECySessionStoreType::Steam;
	if (Provider.Contains(TEXT("gog")) || Provider.Contains(TEXT("galaxy"))) return ECySessionStoreType::GOG;
	if (Provider.Contains(TEXT("oculus")) || Provider.Contains(TEXT("meta"))) return ECySessionStoreType::MetaHorizon;
	if (Provider.Contains(TEXT("googleplay"))) return ECySessionStoreType::GooglePlay;
	if (Provider.Contains(TEXT("amazon"))) return ECySessionStoreType::AmazonAppstore;
	if (Provider.Contains(TEXT("apple")) || Provider == TEXT("ios")) return ECySessionStoreType::AppleAppStore;
	if (Provider.Contains(TEXT("psn")) || Provider.Contains(TEXT("playstation"))) return ECySessionStoreType::PlayStationStore;
	if (Provider.Contains(TEXT("xbox")) || Provider.Contains(TEXT("live"))) return ECySessionStoreType::MicrosoftStore;
	if (Provider.Contains(TEXT("switch")) || Provider.Contains(TEXT("nintendo"))) return ECySessionStoreType::NintendoEshop;
	return ECySessionStoreType::Auto;
}

FString UCySessionSubsystem::GetStoreTag() const
{
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		if (!Profile->StoreTag.IsEmpty()) return Profile->StoreTag;
	}
	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	if (Settings && !Settings->StoreTagOverride.IsEmpty())
	{
		return Settings->StoreTagOverride;
	}
	const FString TypedTag = StoreTypeToTag(GetStoreType());
	return !TypedTag.IsEmpty() ? TypedTag : TEXT("Unknown");
}

ECySessionPlatformType UCySessionSubsystem::GetPlatformType() const
{
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		if (Profile->PlatformType != ECySessionPlatformType::Auto) return Profile->PlatformType;
	}
	const UCySessionSettings* Settings = GetDefault<UCySessionSettings>();
	if (Settings && Settings->PlatformTypeOverride != ECySessionPlatformType::Auto)
	{
		return Settings->PlatformTypeOverride;
	}

	const FString Platform = FString(FPlatformProperties::IniPlatformName()).ToLower();
	if (Platform.Contains(TEXT("win"))) return ECySessionPlatformType::Windows;
	if (Platform.Contains(TEXT("mac"))) return ECySessionPlatformType::MacOS;
	if (Platform.Contains(TEXT("linux"))) return ECySessionPlatformType::Linux;
	if (Platform.Contains(TEXT("ios")) || Platform.Contains(TEXT("tvos"))) return ECySessionPlatformType::IOS;
	if (Platform.Contains(TEXT("android")))
	{
		return GetStoreType() == ECySessionStoreType::MetaHorizon ? ECySessionPlatformType::MetaQuest : ECySessionPlatformType::Android;
	}
	if (Platform.Contains(TEXT("ps4")) || Platform.Contains(TEXT("ps5")) || Platform.Contains(TEXT("playstation"))) return ECySessionPlatformType::PlayStation;
	if (Platform.Contains(TEXT("xbox")) || Platform.Contains(TEXT("gdk"))) return ECySessionPlatformType::Xbox;
	if (Platform.Contains(TEXT("switch"))) return ECySessionPlatformType::NintendoSwitch;
	return ECySessionPlatformType::Other;
}

FCySessionPlatformInfo UCySessionSubsystem::GetPlatformInfo() const
{
	FCySessionPlatformInfo Info;
	Info.ProviderName = GetActiveSubsystemName();
	Info.PlatformType = GetPlatformType();
	Info.StoreType = GetStoreType();
	Info.PlatformTag = GetPlatformTag();
	Info.StoreTag = GetStoreTag();
	Info.bIsMobile = Info.PlatformType == ECySessionPlatformType::IOS || Info.PlatformType == ECySessionPlatformType::Android || Info.PlatformType == ECySessionPlatformType::MetaQuest;
	Info.bIsConsole = Info.PlatformType == ECySessionPlatformType::PlayStation || Info.PlatformType == ECySessionPlatformType::Xbox || Info.PlatformType == ECySessionPlatformType::NintendoSwitch;
	Info.bIsXR = Info.PlatformType == ECySessionPlatformType::MetaQuest;
	return Info;
}

FCySessionProviderCapabilities UCySessionSubsystem::GetProviderCapabilities(FName ProviderName) const
{
	FCySessionProviderCapabilities Capabilities;
	if (ProviderName.IsNone()) ProviderName = GetRequestedSubsystemName();
	IOnlineSubsystem* Provider = Online::GetSubsystem(GetWorld(), ProviderName);
	Capabilities.ProviderName = Provider ? Provider->GetSubsystemName() : ProviderName;
	Capabilities.bProviderAvailable = Provider != nullptr;
	Capabilities.bIdentityAvailable = Provider && Provider->GetIdentityInterface().IsValid();
	Capabilities.bSessionsAvailable = Provider && Provider->GetSessionInterface().IsValid();
	Capabilities.bFriendsAvailable = Provider && Provider->GetFriendsInterface().IsValid();
	Capabilities.bPresenceAvailable = Provider && Provider->GetPresenceInterface().IsValid();
	Capabilities.bAchievementsAvailable = Provider && Provider->GetAchievementsInterface().IsValid();
	Capabilities.bLeaderboardsAvailable = Provider && Provider->GetLeaderboardsInterface().IsValid();
	Capabilities.bUserCloudAvailable = Provider && Provider->GetUserCloudInterface().IsValid();
	Capabilities.bVoiceAvailable = Provider && Provider->GetVoiceInterface().IsValid();
	Capabilities.bExternalUIAvailable = Provider && Provider->GetExternalUIInterface().IsValid();
	Capabilities.bIsActive = Provider && Provider == GetOnlineSubsystem();
	return Capabilities;
}

FCySessionCredentials UCySessionSubsystem::MakeDefaultCredentials(const int32 LocalUserNum) const
{
	FCySessionCredentials Credentials;
	Credentials.LocalUserNum = LocalUserNum;
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		Credentials.Type = Profile->DefaultLoginType;
	}
	return Credentials;
}

FCySessionCreateOptions UCySessionSubsystem::MakeDefaultCreateOptions(const int32 LocalUserNum) const
{
	FCySessionCreateOptions Options;
	Options.LocalUserNum = LocalUserNum;
	if (const UCySessionSettings* Settings = GetDefault<UCySessionSettings>())
	{
		Options.SessionName = Settings->DefaultSessionName;
	}
	if (const FCySessionPlatformProfile* Profile = GetActiveProfile())
	{
		Options.PublicConnections = FMath::Max(1, Profile->DefaultPublicConnections);
		Options.bAllowCrossplay = Profile->bAllowCrossplayByDefault;
		Options.bAllowCrossStore = Profile->bAllowCrossStoreByDefault;
		Options.bUseLobbies = Profile->bUseLobbiesByDefault;
		Options.bUsesStats = Profile->bUseStatsByDefault;
	}
	return Options;
}

FCySessionConfigurationReport UCySessionSubsystem::ValidateConfiguration() const
{
	FCySessionConfigurationReport Report;
	Report.ActiveProfileName = GetActiveProfileName();
	Report.PlatformInfo = GetPlatformInfo();
	Report.ProviderCapabilities = GetProviderCapabilities(GetRequestedSubsystemName());

	if (!Report.ActiveProfileName.IsNone() && !GetActiveProfile())
	{
		Report.Errors.Add(FString::Printf(TEXT("Profile '%s' is missing or disabled."), *Report.ActiveProfileName.ToString()));
	}
	if (!Report.ProviderCapabilities.bProviderAvailable)
	{
		Report.Errors.Add(FString::Printf(TEXT("Online Subsystem '%s' is not installed, enabled or initialized."), *Report.ProviderCapabilities.ProviderName.ToString()));
	}
	else
	{
		if (!Report.ProviderCapabilities.bIdentityAvailable)
		{
			Report.Errors.Add(TEXT("The selected Online Subsystem does not expose IOnlineIdentity."));
		}
		if (!Report.ProviderCapabilities.bSessionsAvailable)
		{
			Report.Errors.Add(TEXT("The selected Online Subsystem does not expose IOnlineSession. GooglePlay and Sign in with Apple require EOS or another session backend."));
		}
	}
	if (Report.PlatformInfo.StoreTag.IsEmpty() || Report.PlatformInfo.StoreTag == TEXT("Unknown"))
	{
		Report.Warnings.Add(TEXT("Store could not be detected. Select a profile or configure Store Type/Tag Override for reliable cross-store filtering."));
	}
	if (Report.ActiveProfileName.IsNone())
	{
		Report.Warnings.Add(TEXT("No CySession profile is selected; legacy provider and platform settings are being used."));
	}

	Report.bValid = Report.Errors.IsEmpty();
	return Report;
}

bool UCySessionSubsystem::IsOperationInProgress(const ECySessionAsyncOperation Operation) const
{
	switch (Operation)
	{
	case ECySessionAsyncOperation::Login: return bLoginInProgress;
	case ECySessionAsyncOperation::Logout: return bLogoutInProgress;
	case ECySessionAsyncOperation::Create: return bCreateInProgress;
	case ECySessionAsyncOperation::Find: return bFindInProgress;
	case ECySessionAsyncOperation::CancelFind: return bCancelFindInProgress;
	case ECySessionAsyncOperation::Join: return bJoinInProgress;
	case ECySessionAsyncOperation::Update: return bUpdateInProgress;
	case ECySessionAsyncOperation::Start: return bStartInProgress;
	case ECySessionAsyncOperation::End: return bEndInProgress;
	case ECySessionAsyncOperation::Destroy: return bDestroyInProgress;
	default: return false;
	}
}

bool UCySessionSubsystem::AutoLogin(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid() || LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS || bLoginInProgress)
	{
		SetError(!Identity.IsValid() ? TEXT("The active Online Subsystem has no identity interface.") : TEXT("Login request is invalid or already in progress."));
		OnLoginComplete.Broadcast(false, LocalUserNum, FString(), LastError);
		return false;
	}

	SetError(FString());
	bLoginInProgress = true;
	ActiveLoginUserNum = LocalUserNum;
	LoginHandle = Identity->AddOnLoginCompleteDelegate_Handle(
		LocalUserNum,
		FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginComplete));
	if (!Identity->AutoLogin(LocalUserNum))
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginHandle);
		LoginHandle.Reset();
		bLoginInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Auto Login."));
		OnLoginComplete.Broadcast(false, LocalUserNum, FString(), LastError);
		return false;
	}
	return true;
}

bool UCySessionSubsystem::Login(const FCySessionCredentials& Credentials)
{
	EnsureNotificationBindings();
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid() || Credentials.LocalUserNum < 0 || Credentials.LocalUserNum >= MAX_LOCAL_PLAYERS || bLoginInProgress)
	{
		SetError(!Identity.IsValid() ? TEXT("The active Online Subsystem has no identity interface.") : TEXT("Login request is invalid or already in progress."));
		OnLoginComplete.Broadcast(false, Credentials.LocalUserNum, FString(), LastError);
		return false;
	}

	SetError(FString());
	bLoginInProgress = true;
	ActiveLoginUserNum = Credentials.LocalUserNum;
	LoginHandle = Identity->AddOnLoginCompleteDelegate_Handle(
		Credentials.LocalUserNum,
		FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginComplete));
	const FOnlineAccountCredentials NativeCredentials(Credentials.Type, Credentials.Id, Credentials.Token);
	if (!Identity->Login(Credentials.LocalUserNum, NativeCredentials))
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(Credentials.LocalUserNum, LoginHandle);
		LoginHandle.Reset();
		bLoginInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Login."));
		OnLoginComplete.Broadcast(false, Credentials.LocalUserNum, FString(), LastError);
		return false;
	}
	return true;
}

bool UCySessionSubsystem::Logout(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid() || LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS || bLogoutInProgress)
	{
		SetError(!Identity.IsValid() ? TEXT("The active Online Subsystem has no identity interface.") : TEXT("Logout request is invalid or already in progress."));
		OnLogoutComplete.Broadcast(false, LocalUserNum, LastError);
		return false;
	}

	SetError(FString());
	bLogoutInProgress = true;
	ActiveLogoutUserNum = LocalUserNum;
	LogoutHandle = Identity->AddOnLogoutCompleteDelegate_Handle(
		LocalUserNum,
		FOnLogoutCompleteDelegate::CreateUObject(this, &ThisClass::HandleLogoutComplete));
	if (!Identity->Logout(LocalUserNum))
	{
		Identity->ClearOnLogoutCompleteDelegate_Handle(LocalUserNum, LogoutHandle);
		LogoutHandle.Reset();
		bLogoutInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Logout."));
		OnLogoutComplete.Broadcast(false, LocalUserNum, LastError);
		return false;
	}
	return true;
}

ECySessionLoginStatus UCySessionSubsystem::GetLoginStatus(const int32 LocalUserNum) const
{
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	return Identity.IsValid() ? ToBlueprintLoginStatus(Identity->GetLoginStatus(LocalUserNum)) : ECySessionLoginStatus::NotLoggedIn;
}

bool UCySessionSubsystem::IsLoggedIn(const int32 LocalUserNum) const
{
	return GetLoginStatus(LocalUserNum) == ECySessionLoginStatus::LoggedIn;
}

FString UCySessionSubsystem::GetLocalUserId(const int32 LocalUserNum) const
{
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (Identity.IsValid())
	{
		const FUniqueNetIdPtr UserId = Identity->GetUniquePlayerId(LocalUserNum);
		return UserId.IsValid() ? UserId->ToString() : FString();
	}
	return FString();
}

FString UCySessionSubsystem::GetLocalUserDisplayName(const int32 LocalUserNum) const
{
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	return Identity.IsValid() ? Identity->GetPlayerNickname(LocalUserNum) : FString();
}

void UCySessionSubsystem::HandleLoginComplete(const int32 LocalUserNum, const bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (Identity.IsValid() && LoginHandle.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginHandle);
	}
	LoginHandle.Reset();
	bLoginInProgress = false;
	SetError(bWasSuccessful ? FString() : (Error.IsEmpty() ? TEXT("Login failed.") : Error));
	OnLoginComplete.Broadcast(bWasSuccessful, LocalUserNum, UserId.ToString(), LastError);
}

void UCySessionSubsystem::HandleLogoutComplete(const int32 LocalUserNum, const bool bWasSuccessful)
{
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (Identity.IsValid() && LogoutHandle.IsValid())
	{
		Identity->ClearOnLogoutCompleteDelegate_Handle(LocalUserNum, LogoutHandle);
	}
	LogoutHandle.Reset();
	bLogoutInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("Logout failed."));
	OnLogoutComplete.Broadcast(bWasSuccessful, LocalUserNum, LastError);
}

void UCySessionSubsystem::HandleLoginStatusChanged(const int32 LocalUserNum, const ELoginStatus::Type OldStatus, const ELoginStatus::Type NewStatus, const FUniqueNetId& UserId)
{
	OnLoginStatusChanged.Broadcast(LocalUserNum, ToBlueprintLoginStatus(OldStatus), ToBlueprintLoginStatus(NewStatus), UserId.ToString());
}

bool UCySessionSubsystem::CreateSession(const FCySessionCreateOptions& Options)
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	FCySessionCreateOptions Normalized = Options;
	if (Normalized.SessionName.IsNone())
	{
		Normalized.SessionName = GetDefault<UCySessionSettings>()->DefaultSessionName;
	}

	if (!Sessions.IsValid() || Normalized.PublicConnections < 1 || bCreateInProgress || bDestroyInProgress)
	{
		SetError(!Sessions.IsValid() ? TEXT("The active Online Subsystem has no session interface.") : TEXT("Create Session request is invalid or another create/destroy is in progress."));
		OnCreateSessionComplete.Broadcast(false, Normalized.SessionName, LastError);
		return false;
	}

	if (Sessions->GetNamedSession(Normalized.SessionName))
	{
		if (!Normalized.bDestroyExistingSession)
		{
			SetError(TEXT("A local session with this name already exists."));
			OnCreateSessionComplete.Broadcast(false, Normalized.SessionName, LastError);
			return false;
		}

		PendingAfterDestroy = EPendingAfterDestroy::Create;
		PendingCreateOptions = Normalized;
		if (!BeginDestroySession(Normalized.SessionName))
		{
			PendingAfterDestroy = EPendingAfterDestroy::None;
			OnCreateSessionComplete.Broadcast(false, Normalized.SessionName, LastError);
			return false;
		}
		return true;
	}

	return BeginCreateSession(Normalized);
}

bool UCySessionSubsystem::BeginCreateSession(const FCySessionCreateOptions& Options)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		SetError(TEXT("Session interface became unavailable before Create Session."));
		OnCreateSessionComplete.Broadcast(false, Options.SessionName, LastError);
		return false;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = FMath::Max(1, Options.PublicConnections);
	Settings.NumPrivateConnections = FMath::Max(0, Options.PrivateConnections);
	Settings.bShouldAdvertise = Options.bAdvertise;
	Settings.bAllowJoinInProgress = Options.bAllowJoinInProgress;
	Settings.bIsLANMatch = Options.bLAN;
	Settings.bIsDedicated = Options.bDedicated;
	Settings.bUsesStats = Options.bUsesStats;
	Settings.bAllowInvites = Options.bAllowInvites;
	const bool bSteamProvider = GetActiveSubsystemName().ToString().Equals(TEXT("Steam"), ESearchCase::IgnoreCase);
	const bool bUsesPresence = bSteamProvider ? Options.bUseLobbies : Options.bUsesPresence;
	Settings.bUsesPresence = bUsesPresence;
	Settings.bAllowJoinViaPresence = bUsesPresence;
	Settings.bAllowJoinViaPresenceFriendsOnly = false;
	Settings.bUseLobbiesIfAvailable = Options.bUseLobbies;
	Settings.bUseLobbiesVoiceChatIfAvailable = Options.bUseLobbyVoiceChat;
	if (Options.BuildUniqueId > 0)
	{
		Settings.BuildUniqueId = Options.BuildUniqueId;
	}

	Settings.Set(CySessionKeys::AllowCrossplay, Options.bAllowCrossplay, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(CySessionKeys::AllowCrossStore, Options.bAllowCrossStore, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(CySessionKeys::Platform, GetPlatformTag(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(CySessionKeys::Store, GetStoreTag(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(CySessionKeys::Provider, GetActiveSubsystemName().ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(CySessionKeys::Mobile, GetPlatformInfo().bIsMobile, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Options.MapName.IsEmpty()) Settings.Set(SETTING_MAPNAME, Options.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Options.GameMode.IsEmpty()) Settings.Set(SETTING_GAMEMODE, Options.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Options.MatchType.IsEmpty()) Settings.Set(CySessionKeys::MatchType, Options.MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Options.Region.IsEmpty()) Settings.Set(CySessionKeys::Region, Options.Region, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	for (const TPair<FName, FString>& Pair : Options.CustomSettings)
	{
		if (!Pair.Key.IsNone())
		{
			Settings.Set(Pair.Key, Pair.Value, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		}
	}

	SetError(FString());
	ActiveCreateOptions = Options;
	bCreateInProgress = true;
	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));
	if (!Sessions->CreateSession(Options.LocalUserNum, Options.SessionName, Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		CreateHandle.Reset();
		bCreateInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Create Session."));
		OnCreateSessionComplete.Broadcast(false, Options.SessionName, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && CreateHandle.IsValid()) Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	CreateHandle.Reset();
	bCreateInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("Create Session failed."));
	OnCreateSessionComplete.Broadcast(bWasSuccessful, SessionName, LastError);

	if (bWasSuccessful)
	{
		if (ActiveCreateOptions.bStartSessionAfterCreate)
		{
			StartSession(SessionName);
		}
		if (ActiveCreateOptions.bTravelOnSuccess)
		{
			HostTravel(ActiveCreateOptions);
		}
	}
}

void UCySessionSubsystem::HostTravel(const FCySessionCreateOptions& Options)
{
	const FString Map = !Options.TravelMap.IsEmpty() ? Options.TravelMap : Options.MapName;
	if (Map.IsEmpty())
	{
		SetError(TEXT("Host travel was requested but neither Travel Map nor Map Name is set."));
		return;
	}

	FString TravelOptions = NormalizeTravelOptions(Options.TravelOptions);
	if (!Options.bDedicated)
	{
		TravelOptions = TravelOptions.IsEmpty() ? TEXT("listen") : FString::Printf(TEXT("listen?%s"), *TravelOptions);
	}
	UGameplayStatics::OpenLevel(this, FName(*Map), true, TravelOptions);
}

bool UCySessionSubsystem::UpdateSession(const FCySessionUpdateOptions& Options)
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(Options.SessionName) : nullptr;
	if (!Sessions.IsValid() || !NamedSession || bUpdateInProgress)
	{
		SetError(!Sessions.IsValid() ? TEXT("The active Online Subsystem has no session interface.") : TEXT("No local session exists, or Update Session is already running."));
		OnUpdateSessionComplete.Broadcast(false, Options.SessionName, LastError);
		return false;
	}

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.bShouldAdvertise = Options.bAdvertise;
	UpdatedSettings.bAllowJoinInProgress = Options.bAllowJoinInProgress;
	UpdatedSettings.bAllowInvites = Options.bAllowInvites;
	for (const TPair<FName, FString>& Pair : Options.CustomSettings)
	{
		if (!Pair.Key.IsNone()) UpdatedSettings.Set(Pair.Key, Pair.Value, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

	SetError(FString());
	bUpdateInProgress = true;
	UpdateHandle = Sessions->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleUpdateSessionComplete));
	if (!Sessions->UpdateSession(Options.SessionName, UpdatedSettings, true))
	{
		Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
		UpdateHandle.Reset();
		bUpdateInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Update Session."));
		OnUpdateSessionComplete.Broadcast(false, Options.SessionName, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleUpdateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && UpdateHandle.IsValid()) Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
	UpdateHandle.Reset();
	bUpdateInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("Update Session failed."));
	OnUpdateSessionComplete.Broadcast(bWasSuccessful, SessionName, LastError);
}

bool UCySessionSubsystem::StartSession(const FName SessionName)
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || bStartInProgress)
	{
		SetError(!Sessions.IsValid() ? TEXT("The active Online Subsystem has no session interface.") : TEXT("Start Session is already running."));
		OnStartSessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}

	SetError(FString());
	bStartInProgress = true;
	StartHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleStartSessionComplete));
	if (!Sessions->StartSession(SessionName))
	{
		Sessions->ClearOnStartSessionCompleteDelegate_Handle(StartHandle);
		StartHandle.Reset();
		bStartInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Start Session."));
		OnStartSessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleStartSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && StartHandle.IsValid()) Sessions->ClearOnStartSessionCompleteDelegate_Handle(StartHandle);
	StartHandle.Reset();
	bStartInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("Start Session failed."));
	OnStartSessionComplete.Broadcast(bWasSuccessful, SessionName, LastError);
}

bool UCySessionSubsystem::EndSession(const FName SessionName)
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || bEndInProgress)
	{
		SetError(!Sessions.IsValid() ? TEXT("The active Online Subsystem has no session interface.") : TEXT("End Session is already running."));
		OnEndSessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}

	SetError(FString());
	bEndInProgress = true;
	EndHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleEndSessionComplete));
	if (!Sessions->EndSession(SessionName))
	{
		Sessions->ClearOnEndSessionCompleteDelegate_Handle(EndHandle);
		EndHandle.Reset();
		bEndInProgress = false;
		SetError(TEXT("The Online Subsystem rejected End Session."));
		OnEndSessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleEndSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && EndHandle.IsValid()) Sessions->ClearOnEndSessionCompleteDelegate_Handle(EndHandle);
	EndHandle.Reset();
	bEndInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("End Session failed."));
	OnEndSessionComplete.Broadcast(bWasSuccessful, SessionName, LastError);
}

bool UCySessionSubsystem::DestroySession(const FName SessionName)
{
	EnsureNotificationBindings();
	if (bDestroyInProgress)
	{
		SetError(TEXT("Destroy Session is already running."));
		OnDestroySessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}
	PendingAfterDestroy = EPendingAfterDestroy::None;
	return BeginDestroySession(SessionName);
}

bool UCySessionSubsystem::BeginDestroySession(const FName SessionName)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Sessions->GetNamedSession(SessionName))
	{
		SetError(!Sessions.IsValid() ? TEXT("The active Online Subsystem has no session interface.") : TEXT("No local session with this name exists."));
		OnDestroySessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}

	SetError(FString());
	bDestroyInProgress = true;
	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
	if (!Sessions->DestroySession(SessionName))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		DestroyHandle.Reset();
		bDestroyInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Destroy Session."));
		OnDestroySessionComplete.Broadcast(false, SessionName, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleDestroySessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && DestroyHandle.IsValid()) Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	DestroyHandle.Reset();
	bDestroyInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("Destroy Session failed."));

	const EPendingAfterDestroy Pending = PendingAfterDestroy;
	PendingAfterDestroy = EPendingAfterDestroy::None;
	OnDestroySessionComplete.Broadcast(bWasSuccessful, SessionName, LastError);
	if (!bWasSuccessful)
	{
		if (Pending == EPendingAfterDestroy::Create) OnCreateSessionComplete.Broadcast(false, PendingCreateOptions.SessionName, LastError);
		if (Pending == EPendingAfterDestroy::Join) OnJoinSessionComplete.Broadcast(false, PendingJoinOptions.SessionName, ECySessionJoinResult::UnknownError, LastError);
		return;
	}

	if (Pending == EPendingAfterDestroy::Create)
	{
		BeginCreateSession(PendingCreateOptions);
	}
	else if (Pending == EPendingAfterDestroy::Join)
	{
		BeginJoinSession(PendingJoinResult, PendingJoinOptions);
	}
}

bool UCySessionSubsystem::FindSessions(const FCySessionFindOptions& Options)
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || bFindInProgress)
	{
		SetError(!Sessions.IsValid() ? TEXT("The active Online Subsystem has no session interface.") : TEXT("Find Sessions is already running."));
		OnFindSessionsComplete.Broadcast(false, TArray<FCySessionSearchResult>(), LastError);
		return false;
	}

	CachedNativeResults.Reset();
	CachedBlueprintResults.Reset();
	ActiveFindOptions = Options;
	ActiveSearch = MakeShared<FOnlineSessionSearch>();
	ActiveSearch->MaxSearchResults = FMath::Clamp(Options.MaxResults, 1, 1000);
	ActiveSearch->bIsLanQuery = Options.bLAN;
	ActiveSearch->PingBucketSize = 50;
	if (Options.bUseLobbies) ActiveSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	if (Options.bRequireOpenPublicSlot) ActiveSearch->QuerySettings.Set(SEARCH_MINSLOTSAVAILABLE, 1, EOnlineComparisonOp::GreaterThanEquals);
	if (!Options.MatchType.IsEmpty()) ActiveSearch->QuerySettings.Set(CySessionKeys::MatchType, Options.MatchType, EOnlineComparisonOp::Equals);
	if (!Options.Region.IsEmpty()) ActiveSearch->QuerySettings.Set(CySessionKeys::Region, Options.Region, EOnlineComparisonOp::Equals);
	if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::CrossplayEnabledOnly)
	{
		ActiveSearch->QuerySettings.Set(CySessionKeys::AllowCrossplay, true, EOnlineComparisonOp::Equals);
	}
	else if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::SamePlatformOnly)
	{
		ActiveSearch->QuerySettings.Set(CySessionKeys::Platform, GetPlatformTag(), EOnlineComparisonOp::Equals);
	}
	else if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::SameStoreOnly)
	{
		ActiveSearch->QuerySettings.Set(CySessionKeys::Store, GetStoreTag(), EOnlineComparisonOp::Equals);
	}
	else if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::SamePlatformAndStoreOnly)
	{
		ActiveSearch->QuerySettings.Set(CySessionKeys::Platform, GetPlatformTag(), EOnlineComparisonOp::Equals);
		ActiveSearch->QuerySettings.Set(CySessionKeys::Store, GetStoreTag(), EOnlineComparisonOp::Equals);
	}
	if (!Options.RequiredPlatformTag.IsEmpty()) ActiveSearch->QuerySettings.Set(CySessionKeys::Platform, Options.RequiredPlatformTag, EOnlineComparisonOp::Equals);
	if (!Options.RequiredStoreTag.IsEmpty()) ActiveSearch->QuerySettings.Set(CySessionKeys::Store, Options.RequiredStoreTag, EOnlineComparisonOp::Equals);
	for (const TPair<FName, FString>& Pair : Options.CustomFilters)
	{
		if (!Pair.Key.IsNone()) ActiveSearch->QuerySettings.Set(Pair.Key, Pair.Value, EOnlineComparisonOp::Equals);
	}

	SetError(FString());
	bFindInProgress = true;
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));
	if (!Sessions->FindSessions(Options.LocalUserNum, ActiveSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		FindHandle.Reset();
		bFindInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Find Sessions."));
		OnFindSessionsComplete.Broadcast(false, CachedBlueprintResults, LastError);
		return false;
	}
	return true;
}

bool UCySessionSubsystem::MatchesFindOptions(const FOnlineSessionSearchResult& NativeResult, const FCySessionFindOptions& Options) const
{
	if (!NativeResult.IsValid()) return false;
	const FOnlineSession& Session = NativeResult.Session;
	if (Options.bRequireOpenPublicSlot && Session.NumOpenPublicConnections <= 0) return false;
	if (Options.MaxPingMs > 0 && NativeResult.PingInMs > Options.MaxPingMs) return false;
	if (!Options.MatchType.IsEmpty() && GetSettingString(Session.SessionSettings, CySessionKeys::MatchType) != Options.MatchType) return false;
	if (!Options.Region.IsEmpty() && GetSettingString(Session.SessionSettings, CySessionKeys::Region) != Options.Region) return false;
	const FString SessionPlatform = GetSettingString(Session.SessionSettings, CySessionKeys::Platform);
	const FString SessionStore = GetSettingString(Session.SessionSettings, CySessionKeys::Store);
	if (Session.SessionSettings.Settings.Contains(CySessionKeys::AllowCrossplay) && !GetSettingBool(Session.SessionSettings, CySessionKeys::AllowCrossplay) && SessionPlatform != GetPlatformTag()) return false;
	if (Session.SessionSettings.Settings.Contains(CySessionKeys::AllowCrossStore) && !GetSettingBool(Session.SessionSettings, CySessionKeys::AllowCrossStore) && SessionStore != GetStoreTag()) return false;
	if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::CrossplayEnabledOnly && !GetSettingBool(Session.SessionSettings, CySessionKeys::AllowCrossplay)) return false;
	if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::SamePlatformOnly && SessionPlatform != GetPlatformTag()) return false;
	if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::SameStoreOnly && SessionStore != GetStoreTag()) return false;
	if (Options.CrossplayPolicy == ECySessionCrossplayPolicy::SamePlatformAndStoreOnly && (SessionPlatform != GetPlatformTag() || SessionStore != GetStoreTag())) return false;
	if (!Options.RequiredPlatformTag.IsEmpty() && SessionPlatform != Options.RequiredPlatformTag) return false;
	if (!Options.RequiredStoreTag.IsEmpty() && SessionStore != Options.RequiredStoreTag) return false;
	for (const TPair<FName, FString>& Pair : Options.CustomFilters)
	{
		if (GetSettingString(Session.SessionSettings, Pair.Key) != Pair.Value) return false;
	}
	return true;
}

void UCySessionSubsystem::HandleFindSessionsComplete(const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && FindHandle.IsValid()) Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	FindHandle.Reset();
	bFindInProgress = false;
	CachedNativeResults.Reset();
	CachedBlueprintResults.Reset();
	if (bWasSuccessful && ActiveSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& NativeResult : ActiveSearch->SearchResults)
		{
			if (MatchesFindOptions(NativeResult, ActiveFindOptions))
			{
				const FString Handle = CacheNativeResult(NativeResult);
				CachedBlueprintResults.Add(MakeBlueprintResult(NativeResult, Handle));
			}
		}
	}
	SetError(bWasSuccessful ? FString() : TEXT("Find Sessions failed."));
	OnFindSessionsComplete.Broadcast(bWasSuccessful, CachedBlueprintResults, LastError);
}

bool UCySessionSubsystem::CancelFindSessions()
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !bFindInProgress || bCancelFindInProgress)
	{
		SetError(TEXT("There is no cancellable Find Sessions request."));
		OnCancelFindSessionsComplete.Broadcast(false, NAME_None, LastError);
		return false;
	}

	bCancelFindInProgress = true;
	CancelFindHandle = Sessions->AddOnCancelFindSessionsCompleteDelegate_Handle(FOnCancelFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleCancelFindSessionsComplete));
	if (!Sessions->CancelFindSessions())
	{
		Sessions->ClearOnCancelFindSessionsCompleteDelegate_Handle(CancelFindHandle);
		CancelFindHandle.Reset();
		bCancelFindInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Cancel Find Sessions."));
		OnCancelFindSessionsComplete.Broadcast(false, NAME_None, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleCancelFindSessionsComplete(const bool bWasSuccessful)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && CancelFindHandle.IsValid()) Sessions->ClearOnCancelFindSessionsCompleteDelegate_Handle(CancelFindHandle);
	CancelFindHandle.Reset();
	bCancelFindInProgress = false;
	SetError(bWasSuccessful ? FString() : TEXT("Cancel Find Sessions failed."));
	OnCancelFindSessionsComplete.Broadcast(bWasSuccessful, NAME_None, LastError);
}

FString UCySessionSubsystem::CacheNativeResult(const FOnlineSessionSearchResult& NativeResult)
{
	FString Handle = NativeResult.GetSessionIdStr();
	if (Handle.IsEmpty())
	{
		Handle = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	}
	CachedNativeResults.Add(Handle, NativeResult);
	return Handle;
}

FCySessionSearchResult UCySessionSubsystem::MakeBlueprintResult(const FOnlineSessionSearchResult& NativeResult, const FString& PreferredHandle) const
{
	FCySessionSearchResult Result;
	Result.SessionId = NativeResult.GetSessionIdStr();
	Result.ResultHandle = !PreferredHandle.IsEmpty() ? PreferredHandle : Result.SessionId;
	if (Result.ResultHandle.IsEmpty()) Result.ResultHandle = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	Result.HostUserId = NativeResult.Session.OwningUserId.IsValid() ? NativeResult.Session.OwningUserId->ToString() : FString();
	Result.HostDisplayName = NativeResult.Session.OwningUserName;
	Result.Platform = GetSettingString(NativeResult.Session.SessionSettings, CySessionKeys::Platform);
	Result.Store = GetSettingString(NativeResult.Session.SessionSettings, CySessionKeys::Store);
	Result.Provider = FName(*GetSettingString(NativeResult.Session.SessionSettings, CySessionKeys::Provider));
	Result.MapName = GetSettingString(NativeResult.Session.SessionSettings, SETTING_MAPNAME);
	Result.GameMode = GetSettingString(NativeResult.Session.SessionSettings, SETTING_GAMEMODE);
	Result.MatchType = GetSettingString(NativeResult.Session.SessionSettings, CySessionKeys::MatchType);
	Result.Region = GetSettingString(NativeResult.Session.SessionSettings, CySessionKeys::Region);
	Result.bCrossplayEnabled = GetSettingBool(NativeResult.Session.SessionSettings, CySessionKeys::AllowCrossplay);
	Result.bCrossStoreEnabled = GetSettingBool(NativeResult.Session.SessionSettings, CySessionKeys::AllowCrossStore);
	Result.bMobile = GetSettingBool(NativeResult.Session.SessionSettings, CySessionKeys::Mobile);
	Result.bDedicated = NativeResult.Session.SessionSettings.bIsDedicated;
	Result.OpenPublicConnections = NativeResult.Session.NumOpenPublicConnections;
	Result.MaxPublicConnections = NativeResult.Session.SessionSettings.NumPublicConnections;
	Result.PingMs = NativeResult.PingInMs;
	for (const TPair<FName, FOnlineSessionSetting>& Pair : NativeResult.Session.SessionSettings.Settings)
	{
		Result.Settings.Add(Pair.Key, Pair.Value.Data.ToString());
	}
	Result.bIsValid = NativeResult.IsValid();
	return Result;
}

bool UCySessionSubsystem::JoinSession(const FString& ResultHandle, const FCySessionJoinOptions& Options)
{
	EnsureNotificationBindings();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	const FOnlineSessionSearchResult* NativeResult = CachedNativeResults.Find(ResultHandle);
	if (!Sessions.IsValid() || !NativeResult || !NativeResult->IsValid() || bJoinInProgress || bDestroyInProgress)
	{
		SetError(!NativeResult ? TEXT("The search result handle is unknown or expired.") : TEXT("Join Session is invalid or another join/destroy is in progress."));
		OnJoinSessionComplete.Broadcast(false, Options.SessionName, ECySessionJoinResult::UnknownError, LastError);
		return false;
	}

	if (Sessions->GetNamedSession(Options.SessionName))
	{
		if (!Options.bDestroyExistingSession)
		{
			SetError(TEXT("A local session with this name already exists."));
			OnJoinSessionComplete.Broadcast(false, Options.SessionName, ECySessionJoinResult::AlreadyInSession, LastError);
			return false;
		}
		PendingAfterDestroy = EPendingAfterDestroy::Join;
		PendingJoinOptions = Options;
		PendingJoinResult = *NativeResult;
		if (!BeginDestroySession(Options.SessionName))
		{
			PendingAfterDestroy = EPendingAfterDestroy::None;
			OnJoinSessionComplete.Broadcast(false, Options.SessionName, ECySessionJoinResult::UnknownError, LastError);
			return false;
		}
		return true;
	}

	return BeginJoinSession(*NativeResult, Options);
}

bool UCySessionSubsystem::BeginJoinSession(const FOnlineSessionSearchResult& NativeResult, const FCySessionJoinOptions& Options)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		SetError(TEXT("Session interface became unavailable before Join Session."));
		OnJoinSessionComplete.Broadcast(false, Options.SessionName, ECySessionJoinResult::UnknownError, LastError);
		return false;
	}

	SetError(FString());
	ActiveJoinOptions = Options;
	bJoinInProgress = true;
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
	if (!Sessions->JoinSession(Options.LocalUserNum, Options.SessionName, NativeResult))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		JoinHandle.Reset();
		bJoinInProgress = false;
		SetError(TEXT("The Online Subsystem rejected Join Session."));
		OnJoinSessionComplete.Broadcast(false, Options.SessionName, ECySessionJoinResult::UnknownError, LastError);
		return false;
	}
	return true;
}

void UCySessionSubsystem::HandleJoinSessionComplete(const FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && JoinHandle.IsValid()) Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	JoinHandle.Reset();
	bJoinInProgress = false;
	const bool bSuccess = Result == EOnJoinSessionCompleteResult::Success;
	SetError(bSuccess ? FString() : FString::Printf(TEXT("Join Session failed: %s."), LexToString(Result)));
	OnJoinSessionComplete.Broadcast(bSuccess, SessionName, ToBlueprintJoinResult(Result), LastError);
	if (bSuccess && ActiveJoinOptions.bTravelOnSuccess)
	{
		TravelToSession(SessionName, ActiveJoinOptions.LocalUserNum);
	}
}

bool UCySessionSubsystem::SendSessionInvite(const FString& FriendUserId, const int32 LocalUserNum, const FName SessionName)
{
	EnsureNotificationBindings();
	const IOnlineIdentityPtr Identity = GetIdentityInterface();
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Identity.IsValid() || !Sessions.IsValid() || FriendUserId.IsEmpty())
	{
		SetError(TEXT("Identity/session interface is unavailable or Friend User ID is empty."));
		return false;
	}

	const FUniqueNetIdPtr FriendId = Identity->CreateUniquePlayerId(FriendUserId);
	if (!FriendId.IsValid())
	{
		SetError(TEXT("The active Online Subsystem could not parse Friend User ID."));
		return false;
	}

	const bool bStarted = Sessions->SendSessionInviteToFriend(LocalUserNum, SessionName, *FriendId);
	SetError(bStarted ? FString() : TEXT("The Online Subsystem rejected Send Session Invite."));
	return bStarted;
}

void UCySessionSubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, const FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	FString Handle;
	if (bWasSuccessful && InviteResult.IsValid()) Handle = CacheNativeResult(InviteResult);
	const FCySessionSearchResult Result = MakeBlueprintResult(InviteResult, Handle);
	OnSessionInviteAccepted.Broadcast(ControllerId, UserId.IsValid() ? UserId->ToString() : FString(), Result);
}

void UCySessionSubsystem::HandleSessionInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult)
{
	const FString Handle = InviteResult.IsValid() ? CacheNativeResult(InviteResult) : FString();
	OnSessionInviteReceived.Broadcast(UserId.ToString(), FromId.ToString(), MakeBlueprintResult(InviteResult, Handle));
}

void UCySessionSubsystem::HandleSessionParticipantJoined(const FName SessionName, const FUniqueNetId& UserId)
{
	OnSessionParticipantJoined.Broadcast(SessionName, UserId.ToString(), TEXT("Joined"));
}

void UCySessionSubsystem::HandleSessionParticipantLeft(const FName SessionName, const FUniqueNetId& UserId, const EOnSessionParticipantLeftReason Reason)
{
	OnSessionParticipantLeft.Broadcast(SessionName, UserId.ToString(), ToLogString(Reason));
}

void UCySessionSubsystem::HandleSessionFailure(const FUniqueNetId& UserId, const ESessionFailure::Type FailureType)
{
	const FString Error = LexToString(FailureType);
	SetError(Error);
	OnSessionFailure.Broadcast(UserId.ToString(), Error);
}

ECySessionState UCySessionSubsystem::GetSessionState(const FName SessionName) const
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	return Sessions.IsValid() ? ToBlueprintSessionState(Sessions->GetSessionState(SessionName)) : ECySessionState::NoSession;
}

bool UCySessionSubsystem::GetCurrentSession(const FName SessionName, FCySessionSearchResult& Result) const
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	const FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(SessionName) : nullptr;
	if (!NamedSession)
	{
		Result = FCySessionSearchResult();
		return false;
	}

	FOnlineSessionSearchResult NativeResult;
	NativeResult.Session = static_cast<const FOnlineSession&>(*NamedSession);
	Result = MakeBlueprintResult(NativeResult);
	return true;
}

bool UCySessionSubsystem::GetResolvedConnectString(const FName SessionName, FString& ConnectString) const
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	ConnectString.Reset();
	return Sessions.IsValid() && Sessions->GetResolvedConnectString(SessionName, ConnectString) && !ConnectString.IsEmpty();
}

bool UCySessionSubsystem::TravelToSession(const FName SessionName, const int32 LocalPlayerIndex)
{
	FString ConnectString;
	if (!GetResolvedConnectString(SessionName, ConnectString))
	{
		SetError(TEXT("Could not resolve the session connect string."));
		return false;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, LocalPlayerIndex);
	if (!PlayerController)
	{
		SetError(TEXT("No local Player Controller is available for client travel."));
		return false;
	}

	SetError(FString());
	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	return true;
}
