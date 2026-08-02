// Copyright Atlas XR. All Rights Reserved.

#include "CySteamSubsystem.h"

#include "CySession.h"
#include "CySessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Interfaces/OnlineUserCloudInterface.h"
#include "Interfaces/VoiceInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

namespace
{
	const FName SteamSubsystemName(TEXT("Steam"));

	EOnlinePresenceState::Type ToNativePresenceState(const ECySteamPresenceState State)
	{
		switch (State)
		{
		case ECySteamPresenceState::Online: return EOnlinePresenceState::Online;
		case ECySteamPresenceState::Away: return EOnlinePresenceState::Away;
		case ECySteamPresenceState::ExtendedAway: return EOnlinePresenceState::ExtendedAway;
		case ECySteamPresenceState::DoNotDisturb: return EOnlinePresenceState::DoNotDisturb;
		case ECySteamPresenceState::Chat: return EOnlinePresenceState::Chat;
		default: return EOnlinePresenceState::Offline;
		}
	}

	ECySteamPresenceState ToBlueprintPresenceState(const EOnlinePresenceState::Type State)
	{
		switch (State)
		{
		case EOnlinePresenceState::Online: return ECySteamPresenceState::Online;
		case EOnlinePresenceState::Away: return ECySteamPresenceState::Away;
		case EOnlinePresenceState::ExtendedAway: return ECySteamPresenceState::ExtendedAway;
		case EOnlinePresenceState::DoNotDisturb: return ECySteamPresenceState::DoNotDisturb;
		case EOnlinePresenceState::Chat: return ECySteamPresenceState::Chat;
		default: return ECySteamPresenceState::Offline;
		}
	}

	FString FailureText(const TCHAR* Operation)
	{
		return FString::Printf(TEXT("Steam failed to start %s."), Operation);
	}
}

void UCySteamSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureNotificationBindings();
}

void UCySteamSubsystem::Deinitialize()
{
	ClearNotificationBindings();
	ActiveAchievementWrite.Reset();
	ActiveLeaderboardRead.Reset();
	ActiveLeaderboardWrite.Reset();
	PendingCloudWriteData.Reset();
	Super::Deinitialize();
}

IOnlineSubsystem* UCySteamSubsystem::GetSteamSubsystem() const
{
	return GetWorld() ? Online::GetSubsystem(GetWorld(), SteamSubsystemName) : IOnlineSubsystem::Get(SteamSubsystemName);
}

TSharedPtr<const FUniqueNetId> UCySteamSubsystem::GetLocalUserId(const int32 LocalUserNum, FString* Error) const
{
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineIdentityPtr Identity = Steam ? Steam->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(LocalUserNum) : nullptr;
	if (!UserId.IsValid() && Error)
	{
		*Error = TEXT("Steam user is not logged in. Call CySession Auto Login first.");
	}
	return UserId;
}

TSharedPtr<const FUniqueNetId> UCySteamSubsystem::ParseUserId(const FString& UserId, FString* Error) const
{
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineIdentityPtr Identity = Steam ? Steam->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr NativeId = Identity.IsValid() ? Identity->CreateUniquePlayerId(UserId) : nullptr;
	if (!NativeId.IsValid() && Error)
	{
		*Error = FString::Printf(TEXT("'%s' is not a valid Steam user id."), *UserId);
	}
	return NativeId;
}

void UCySteamSubsystem::SetError(const FString& Error)
{
	LastSteamError = Error;
	if (!Error.IsEmpty())
	{
		UE_LOG(LogCySession, Warning, TEXT("Steam: %s"), *Error);
	}
}

bool UCySteamSubsystem::ActivateFullSteamMode()
{
	UGameInstance* GameInstance = GetGameInstance();
	UCySessionSubsystem* Sessions = GameInstance ? GameInstance->GetSubsystem<UCySessionSubsystem>() : nullptr;
	if (!Sessions)
	{
		SetError(TEXT("CySession subsystem is unavailable."));
		return false;
	}
	if (!Sessions->SetActiveProfileOverride(TEXT("Steam_Windows_Full")))
	{
		SetError(Sessions->GetLastError());
		return false;
	}
	EnsureNotificationBindings();
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::IsSteamAvailable() const
{
	return GetSteamSubsystem() != nullptr;
}

bool UCySteamSubsystem::IsSteamActiveProvider() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UCySessionSubsystem* Sessions = GameInstance ? GameInstance->GetSubsystem<UCySessionSubsystem>() : nullptr;
	if (Sessions)
	{
		return Sessions->GetActiveSubsystemName() == SteamSubsystemName;
	}
	const IOnlineSubsystem* Active = GetWorld() ? Online::GetSubsystem(GetWorld()) : nullptr;
	return Active && Active->GetSubsystemName() == SteamSubsystemName;
}

FCySteamCapabilities UCySteamSubsystem::GetSteamCapabilities() const
{
	FCySteamCapabilities Result;
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	Result.bSteamAvailable = Steam != nullptr;
	Result.bSteamIsActiveProvider = IsSteamActiveProvider();
	if (!Steam)
	{
		return Result;
	}

	Result.AppId = Steam->GetAppId();
	Result.bIdentity = Steam->GetIdentityInterface().IsValid();
	Result.bSessions = Steam->GetSessionInterface().IsValid();
	Result.bFriends = Steam->GetFriendsInterface().IsValid();
	Result.bPresence = Steam->GetPresenceInterface().IsValid();
	Result.bAchievements = Steam->GetAchievementsInterface().IsValid();
	Result.bLeaderboards = Steam->GetLeaderboardsInterface().IsValid();
	Result.bUserCloud = Steam->GetUserCloudInterface().IsValid();
	Result.bVoice = Steam->GetVoiceInterface().IsValid();
	Result.bExternalUI = Steam->GetExternalUIInterface().IsValid();
	Result.bStore = Steam->GetStoreV2Interface().IsValid();
	Result.bPurchase = Steam->GetPurchaseInterface().IsValid();

	TArray<FString> NetDriverDefinitions;
	if (GConfig)
	{
		GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bEnabled"), Result.bSteamEnabledInConfig, GEngineIni);
		GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bInitServerOnClient"), Result.bInitServerOnClient, GEngineIni);
		GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bUseSteamNetworking"), Result.bUseSteamNetworking, GEngineIni);
		GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bAllowP2PPacketRelay"), Result.bAllowP2PPacketRelay, GEngineIni);
		GConfig->GetArray(TEXT("/Script/Engine.GameEngine"), TEXT("NetDriverDefinitions"), NetDriverDefinitions, GEngineIni);
		if (NetDriverDefinitions.IsEmpty())
		{
			GConfig->GetArray(TEXT("/Script/Engine.Engine"), TEXT("NetDriverDefinitions"), NetDriverDefinitions, GEngineIni);
		}
	}
	Result.bSteamNetDriverConfigured = NetDriverDefinitions.ContainsByPredicate([](const FString& Definition)
	{
		return Definition.Contains(TEXT("SteamNetDriver"), ESearchCase::IgnoreCase)
			|| Definition.Contains(TEXT("SteamSocketsNetDriver"), ESearchCase::IgnoreCase);
	});
	return Result;
}

FCySteamValidationReport UCySteamSubsystem::ValidateSteamConfiguration() const
{
	FCySteamValidationReport Report;
	Report.Capabilities = GetSteamCapabilities();
	if (!Report.Capabilities.bSteamAvailable)
	{
		Report.Errors.Add(TEXT("OnlineSubsystemSteam is not enabled or Steam failed to initialize."));
	}
	if (Report.Capabilities.bSteamAvailable && !Report.Capabilities.bSteamIsActiveProvider)
	{
		Report.Errors.Add(TEXT("Steam is available but is not the active CySession provider. Select Steam_Windows_Full or set DefaultPlatformService=Steam."));
	}
	if (!Report.Capabilities.bIdentity) Report.Errors.Add(TEXT("Steam identity interface is unavailable."));
	if (!Report.Capabilities.bSessions) Report.Errors.Add(TEXT("Steam session/lobby interface is unavailable."));
	if (!Report.Capabilities.bSteamEnabledInConfig) Report.Errors.Add(TEXT("[OnlineSubsystemSteam] bEnabled=True is missing."));
	if (!Report.Capabilities.bInitServerOnClient) Report.Errors.Add(TEXT("[OnlineSubsystemSteam] bInitServerOnClient=True is required for client-hosted Steam sessions."));
	if (!Report.Capabilities.bUseSteamNetworking) Report.Errors.Add(TEXT("[OnlineSubsystemSteam] bUseSteamNetworking=True is required by the supplied native Steam NetDriver configuration."));
	if (!Report.Capabilities.bSteamNetDriverConfigured) Report.Errors.Add(TEXT("No Steam net driver is configured. Apply Windows-Steam-Full.ini.example before packaging."));
	if (Report.Capabilities.AppId.IsEmpty()) Report.Errors.Add(TEXT("Steam App ID is empty."));
	if (Report.Capabilities.AppId == TEXT("480")) Report.Warnings.Add(TEXT("Steam App ID 480 is Spacewar and must only be used for development."));
	if (!Report.Capabilities.bFriends) Report.Warnings.Add(TEXT("Steam friends are unavailable."));
	if (!Report.Capabilities.bPresence) Report.Warnings.Add(TEXT("Steam rich presence is unavailable."));
	if (!Report.Capabilities.bAchievements) Report.Warnings.Add(TEXT("Steam achievements are unavailable."));
	if (!Report.Capabilities.bLeaderboards) Report.Warnings.Add(TEXT("Steam leaderboards are unavailable."));
	if (!Report.Capabilities.bUserCloud) Report.Warnings.Add(TEXT("Steam Remote Storage is unavailable."));
	if (!Report.Capabilities.bExternalUI) Report.Warnings.Add(TEXT("Steam overlay is unavailable."));
	if (!Report.Capabilities.bVoice) Report.Warnings.Add(TEXT("Steam voice is unavailable or could not initialize."));
	if (!Report.Capabilities.bAllowP2PPacketRelay) Report.Warnings.Add(TEXT("Steam P2P relay is disabled; NAT traversal may fail for some players."));
	if (Report.Capabilities.bPurchase)
	{
		Report.Warnings.Add(TEXT("UE 5.5 Steam checkout still requires a game-provided secure purchasing server link; use the overlay/store Web API for production purchases."));
	}
	Report.bValid = Report.Errors.IsEmpty();
	return Report;
}

FString UCySteamSubsystem::GetSteamAuthToken(const int32 LocalUserNum) const
{
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineIdentityPtr Identity = Steam ? Steam->GetIdentityInterface() : nullptr;
	return Identity.IsValid() ? Identity->GetAuthToken(LocalUserNum) : FString();
}

FCySteamPresence UCySteamSubsystem::MakePresence(const FString& UserId, const FOnlineUserPresence& NativePresence) const
{
	FCySteamPresence Result;
	Result.UserId = UserId;
	Result.Status = NativePresence.Status.StatusStr;
	Result.State = ToBlueprintPresenceState(NativePresence.Status.State);
	Result.bOnline = NativePresence.bIsOnline;
	Result.bPlaying = NativePresence.bIsPlaying;
	Result.bPlayingThisGame = NativePresence.bIsPlayingThisGame;
	Result.bJoinable = NativePresence.bIsJoinable;
	Result.bHasVoiceSupport = NativePresence.bHasVoiceSupport;
	Result.SessionId = NativePresence.SessionId.IsValid() ? NativePresence.SessionId->ToString() : FString();
	for (const TPair<FString, FVariantData>& Property : NativePresence.Status.Properties)
	{
		Result.Properties.Add(Property.Key, Property.Value.ToString());
	}
	return Result;
}

TArray<FCySteamFriend> UCySteamSubsystem::GetCachedFriends(const int32 LocalUserNum, const FString& ListName) const
{
	TArray<FCySteamFriend> Result;
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineFriendsPtr Friends = Steam ? Steam->GetFriendsInterface() : nullptr;
	if (!Friends.IsValid())
	{
		return Result;
	}
	TArray<TSharedRef<FOnlineFriend>> NativeFriends;
	if (!Friends->GetFriendsList(LocalUserNum, ListName, NativeFriends))
	{
		return Result;
	}
	Result.Reserve(NativeFriends.Num());
	for (const TSharedRef<FOnlineFriend>& NativeFriend : NativeFriends)
	{
		FCySteamFriend& Friend = Result.Emplace_GetRef();
		Friend.UserId = NativeFriend->GetUserId()->ToString();
		Friend.DisplayName = NativeFriend->GetDisplayName();
		Friend.RealName = NativeFriend->GetRealName();
		Friend.Presence = MakePresence(Friend.UserId, NativeFriend->GetPresence());
	}
	return Result;
}

bool UCySteamSubsystem::ReadFriends(const int32 LocalUserNum, const FString& ListName)
{
	EnsureNotificationBindings();
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineFriendsPtr Friends = Steam ? Steam->GetFriendsInterface() : nullptr;
	if (!Friends.IsValid())
	{
		SetError(TEXT("Steam friends interface is unavailable."));
		return false;
	}
	const TWeakObjectPtr<UCySteamSubsystem> WeakThis(this);
	const bool bStarted = Friends->ReadFriendsList(LocalUserNum, ListName,
		FOnReadFriendsListComplete::CreateLambda([WeakThis](const int32 UserNum, const bool bSuccess, const FString& CompletedList, const FString& Error)
		{
			if (UCySteamSubsystem* Self = WeakThis.Get())
			{
				const TArray<FCySteamFriend> Results = bSuccess ? Self->GetCachedFriends(UserNum, CompletedList) : TArray<FCySteamFriend>();
				Self->SetError(bSuccess ? FString() : Error);
				Self->OnFriendsReadComplete.Broadcast(bSuccess, Results, Error);
			}
		}));
	if (!bStarted) SetError(FailureText(TEXT("ReadFriends")));
	return bStarted;
}

bool UCySteamSubsystem::SetPresence(const int32 LocalUserNum, const ECySteamPresenceState State, const FString& Status, const TMap<FString, FString>& Properties)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlinePresencePtr Presence = Steam ? Steam->GetPresenceInterface() : nullptr;
	if (!UserId.IsValid() || !Presence.IsValid())
	{
		SetError(UserId.IsValid() ? TEXT("Steam presence interface is unavailable.") : Error);
		return false;
	}
	FOnlineUserPresenceStatus NativeStatus;
	NativeStatus.State = ToNativePresenceState(State);
	NativeStatus.StatusStr = Status;
	for (const TPair<FString, FString>& Property : Properties)
	{
		FVariantData Value;
		Value.SetValue(Property.Value);
		NativeStatus.Properties.Add(Property.Key, MoveTemp(Value));
	}
	const TWeakObjectPtr<UCySteamSubsystem> WeakThis(this);
	Presence->SetPresence(*UserId, NativeStatus,
		IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateLambda([WeakThis](const FUniqueNetId& CompletedUser, const bool bSuccess)
		{
			if (UCySteamSubsystem* Self = WeakThis.Get())
			{
				const FString CompletedError = bSuccess ? FString() : TEXT("Steam failed to set presence.");
				Self->SetError(CompletedError);
				Self->OnPresenceSetComplete.Broadcast(bSuccess, CompletedUser.ToString(), CompletedError);
			}
		}));
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::QueryPresence(const FString& UserId)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr NativeId = ParseUserId(UserId, &Error);
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlinePresencePtr Presence = Steam ? Steam->GetPresenceInterface() : nullptr;
	if (!NativeId.IsValid() || !Presence.IsValid())
	{
		SetError(NativeId.IsValid() ? TEXT("Steam presence interface is unavailable.") : Error);
		return false;
	}
	const TWeakObjectPtr<UCySteamSubsystem> WeakThis(this);
	Presence->QueryPresence(*NativeId,
		IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateLambda([WeakThis](const FUniqueNetId& CompletedUser, const bool bSuccess)
		{
			if (UCySteamSubsystem* Self = WeakThis.Get())
			{
				const FString CompletedError = bSuccess ? FString() : TEXT("Steam failed to query presence.");
				Self->SetError(CompletedError);
				Self->OnPresenceQueryComplete.Broadcast(bSuccess, CompletedUser.ToString(), CompletedError);
			}
		}));
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::GetCachedPresence(const FString& UserId, FCySteamPresence& PresenceResult) const
{
	PresenceResult = FCySteamPresence();
	const FUniqueNetIdPtr NativeId = ParseUserId(UserId);
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlinePresencePtr Presence = Steam ? Steam->GetPresenceInterface() : nullptr;
	TSharedPtr<FOnlineUserPresence> NativePresence;
	if (!NativeId.IsValid() || !Presence.IsValid()
		|| Presence->GetCachedPresence(*NativeId, NativePresence) != EOnlineCachedResult::Success
		|| !NativePresence.IsValid())
	{
		return false;
	}
	PresenceResult = MakePresence(UserId, *NativePresence);
	return true;
}

TArray<FCySteamAchievement> UCySteamSubsystem::BuildAchievements(const FUniqueNetId& UserId) const
{
	TArray<FCySteamAchievement> Result;
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineAchievementsPtr Achievements = Steam ? Steam->GetAchievementsInterface() : nullptr;
	TArray<FOnlineAchievement> NativeAchievements;
	if (!Achievements.IsValid() || Achievements->GetCachedAchievements(UserId, NativeAchievements) != EOnlineCachedResult::Success)
	{
		return Result;
	}
	Result.Reserve(NativeAchievements.Num());
	for (const FOnlineAchievement& NativeAchievement : NativeAchievements)
	{
		FCySteamAchievement& Achievement = Result.Emplace_GetRef();
		Achievement.AchievementId = NativeAchievement.Id;
		Achievement.Progress = NativeAchievement.Progress;
		Achievement.bUnlocked = NativeAchievement.Progress >= 100.0;
		FOnlineAchievementDesc Description;
		if (Achievements->GetCachedAchievementDescription(NativeAchievement.Id, Description) == EOnlineCachedResult::Success)
		{
			Achievement.Title = Description.Title;
			Achievement.LockedDescription = Description.LockedDesc;
			Achievement.UnlockedDescription = Description.UnlockedDesc;
			Achievement.bHidden = Description.bIsHidden;
			Achievement.UnlockTime = Description.UnlockTime;
		}
	}
	return Result;
}

TArray<FCySteamAchievement> UCySteamSubsystem::GetCachedAchievements(const int32 LocalUserNum) const
{
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum);
	return UserId.IsValid() ? BuildAchievements(*UserId) : TArray<FCySteamAchievement>();
}

bool UCySteamSubsystem::QueryAchievements(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineAchievementsPtr Achievements = Steam ? Steam->GetAchievementsInterface() : nullptr;
	if (!UserId.IsValid() || !Achievements.IsValid())
	{
		SetError(UserId.IsValid() ? TEXT("Steam achievements interface is unavailable.") : Error);
		return false;
	}
	const TWeakObjectPtr<UCySteamSubsystem> WeakThis(this);
	Achievements->QueryAchievements(*UserId,
		FOnQueryAchievementsCompleteDelegate::CreateLambda([WeakThis](const FUniqueNetId& CompletedUser, const bool bSuccess)
		{
			UCySteamSubsystem* Self = WeakThis.Get();
			IOnlineSubsystem* SteamSubsystem = Self ? Self->GetSteamSubsystem() : nullptr;
			const IOnlineAchievementsPtr Interface = SteamSubsystem ? SteamSubsystem->GetAchievementsInterface() : nullptr;
			if (!Self || !bSuccess || !Interface.IsValid())
			{
				if (Self)
				{
					const FString QueryError = TEXT("Steam failed to query achievements.");
					Self->SetError(QueryError);
					Self->OnAchievementsReadComplete.Broadcast(false, TArray<FCySteamAchievement>(), QueryError);
				}
				return;
			}
			Interface->QueryAchievementDescriptions(CompletedUser,
				FOnQueryAchievementsCompleteDelegate::CreateLambda([WeakThis](const FUniqueNetId& DescriptionUser, const bool bDescriptionsSuccess)
				{
					if (UCySteamSubsystem* InnerSelf = WeakThis.Get())
					{
						const TArray<FCySteamAchievement> Results = InnerSelf->BuildAchievements(DescriptionUser);
						const FString QueryError = bDescriptionsSuccess ? FString() : TEXT("Steam failed to query achievement descriptions.");
						InnerSelf->SetError(QueryError);
						InnerSelf->OnAchievementsReadComplete.Broadcast(bDescriptionsSuccess, Results, QueryError);
					}
				}));
		}));
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::WriteAchievement(const FString& AchievementId, const double Progress, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineAchievementsPtr Achievements = Steam ? Steam->GetAchievementsInterface() : nullptr;
	if (!UserId.IsValid() || !Achievements.IsValid() || AchievementId.IsEmpty())
	{
		SetError(!UserId.IsValid() ? Error : TEXT("Steam achievements are unavailable or Achievement Id is empty."));
		return false;
	}
	FOnlineAchievement Cached;
	if (Achievements->GetCachedAchievement(*UserId, AchievementId, Cached) != EOnlineCachedResult::Success)
	{
		SetError(TEXT("Query Achievements must complete before writing a Steam achievement."));
		return false;
	}
	ActiveAchievementId = AchievementId;
	ActiveAchievementWrite = MakeShared<FOnlineAchievementsWrite, ESPMode::ThreadSafe>();
	ActiveAchievementWrite->SetFloatStat(AchievementId, static_cast<float>(FMath::Clamp(Progress, 0.0, 100.0)));
	FOnlineAchievementsWriteRef WriteRef = ActiveAchievementWrite.ToSharedRef();
	const TWeakObjectPtr<UCySteamSubsystem> WeakThis(this);
	Achievements->WriteAchievements(*UserId, WriteRef,
		FOnAchievementsWrittenDelegate::CreateLambda([WeakThis](const FUniqueNetId&, const bool bSuccess)
		{
			if (UCySteamSubsystem* Self = WeakThis.Get())
			{
				const FString CompletedId = Self->ActiveAchievementId;
				const FString WriteError = bSuccess ? FString() : TEXT("Steam failed to write the achievement.");
				Self->ActiveAchievementWrite.Reset();
				Self->ActiveAchievementId.Reset();
				Self->SetError(WriteError);
				Self->OnAchievementWriteComplete.Broadcast(bSuccess, CompletedId, WriteError);
			}
		}));
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::BeginLeaderboardRead(const FString& LeaderboardName, const FString& StatName)
{
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineLeaderboardsPtr Leaderboards = Steam ? Steam->GetLeaderboardsInterface() : nullptr;
	if (!Leaderboards.IsValid() || LeaderboardName.IsEmpty() || StatName.IsEmpty())
	{
		SetError(TEXT("Steam leaderboards are unavailable or leaderboard/stat name is empty."));
		return false;
	}
	if (ActiveLeaderboardRead.IsValid())
	{
		SetError(TEXT("A Steam leaderboard read is already in progress."));
		return false;
	}
	EnsureNotificationBindings();
	ActiveLeaderboardReadName = LeaderboardName;
	ActiveLeaderboardReadStat = StatName;
	ActiveLeaderboardRead = MakeShared<FOnlineLeaderboardRead, ESPMode::ThreadSafe>();
	ActiveLeaderboardRead->LeaderboardName = LeaderboardName;
	ActiveLeaderboardRead->SortedColumn = StatName;
	ActiveLeaderboardRead->ColumnMetadata.Emplace(StatName, EOnlineKeyValuePairDataType::Int32);
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::ReadLeaderboardAroundUser(const FString& LeaderboardName, const FString& StatName, const int32 Range, const int32 LocalUserNum)
{
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	if (!UserId.IsValid())
	{
		SetError(Error);
		return false;
	}
	if (!BeginLeaderboardRead(LeaderboardName, StatName)) return false;
	FOnlineLeaderboardReadRef ReadRef = ActiveLeaderboardRead.ToSharedRef();
	const bool bStarted = BoundLeaderboards->ReadLeaderboardsAroundUser(UserId.ToSharedRef(), FMath::Max(0, Range), ReadRef);
	if (!bStarted)
	{
		ActiveLeaderboardRead.Reset();
		SetError(FailureText(TEXT("ReadLeaderboardAroundUser")));
	}
	return bStarted;
}

bool UCySteamSubsystem::ReadLeaderboardAroundRank(const FString& LeaderboardName, const FString& StatName, const int32 Rank, const int32 Range)
{
	if (!BeginLeaderboardRead(LeaderboardName, StatName)) return false;
	FOnlineLeaderboardReadRef ReadRef = ActiveLeaderboardRead.ToSharedRef();
	const bool bStarted = BoundLeaderboards->ReadLeaderboardsAroundRank(FMath::Max(1, Rank), FMath::Max(0, Range), ReadRef);
	if (!bStarted)
	{
		ActiveLeaderboardRead.Reset();
		SetError(FailureText(TEXT("ReadLeaderboardAroundRank")));
	}
	return bStarted;
}

bool UCySteamSubsystem::ReadFriendsLeaderboard(const FString& LeaderboardName, const FString& StatName, const int32 LocalUserNum)
{
	if (!BeginLeaderboardRead(LeaderboardName, StatName)) return false;
	FOnlineLeaderboardReadRef ReadRef = ActiveLeaderboardRead.ToSharedRef();
	const bool bStarted = BoundLeaderboards->ReadLeaderboardsForFriends(LocalUserNum, ReadRef);
	if (!bStarted)
	{
		ActiveLeaderboardRead.Reset();
		SetError(FailureText(TEXT("ReadLeaderboardsForFriends")));
	}
	return bStarted;
}

void UCySteamSubsystem::HandleLeaderboardReadComplete(const bool bSuccess)
{
	TArray<FCySteamLeaderboardRow> Rows;
	if (bSuccess && ActiveLeaderboardRead.IsValid())
	{
		Rows.Reserve(ActiveLeaderboardRead->Rows.Num());
		for (const FOnlineStatsRow& NativeRow : ActiveLeaderboardRead->Rows)
		{
			FCySteamLeaderboardRow& Row = Rows.Emplace_GetRef();
			Row.PlayerId = NativeRow.PlayerId.IsValid() ? NativeRow.PlayerId->ToString() : FString();
			Row.Nickname = NativeRow.NickName;
			Row.Rank = NativeRow.Rank;
			for (const TPair<FString, FVariantData>& Column : NativeRow.Columns)
			{
				Row.Columns.Add(Column.Key, Column.Value.ToString());
			}
		}
	}
	const FString CompletedLeaderboard = ActiveLeaderboardReadName;
	const FString Error = bSuccess ? FString() : TEXT("Steam failed to read the leaderboard.");
	ActiveLeaderboardRead.Reset();
	ActiveLeaderboardReadName.Reset();
	ActiveLeaderboardReadStat.Reset();
	SetError(Error);
	OnLeaderboardReadComplete.Broadcast(bSuccess, CompletedLeaderboard, Rows, Error);
}

bool UCySteamSubsystem::WriteLeaderboardScore(const FString& LeaderboardName, const FString& StatName, const int32 Score, const ECySteamLeaderboardUpdateMethod UpdateMethod, const FName SessionName, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineLeaderboardsPtr Leaderboards = Steam ? Steam->GetLeaderboardsInterface() : nullptr;
	if (!UserId.IsValid() || !Leaderboards.IsValid() || LeaderboardName.IsEmpty() || StatName.IsEmpty())
	{
		SetError(!UserId.IsValid() ? Error : TEXT("Steam leaderboards are unavailable or leaderboard/stat name is empty."));
		return false;
	}
	if (ActiveLeaderboardWrite.IsValid())
	{
		SetError(TEXT("A Steam leaderboard write is already in progress."));
		return false;
	}
	ActiveLeaderboardWriteName = LeaderboardName;
	ActiveLeaderboardSessionName = SessionName;
	ActiveLeaderboardWrite = MakeShared<FOnlineLeaderboardWrite, ESPMode::ThreadSafe>();
	ActiveLeaderboardWrite->LeaderboardNames.Add(LeaderboardName);
	ActiveLeaderboardWrite->RatedStat = StatName;
	ActiveLeaderboardWrite->DisplayFormat = ELeaderboardFormat::Number;
	ActiveLeaderboardWrite->SortMethod = ELeaderboardSort::Descending;
	ActiveLeaderboardWrite->UpdateMethod = UpdateMethod == ECySteamLeaderboardUpdateMethod::Force ? ELeaderboardUpdateMethod::Force : ELeaderboardUpdateMethod::KeepBest;
	ActiveLeaderboardWrite->SetIntStat(StatName, Score);
	if (!Leaderboards->WriteLeaderboards(SessionName, *UserId, *ActiveLeaderboardWrite)
		|| !Leaderboards->FlushLeaderboards(SessionName))
	{
		ActiveLeaderboardWrite.Reset();
		ActiveLeaderboardWriteName.Reset();
		ActiveLeaderboardSessionName = NAME_None;
		SetError(FailureText(TEXT("WriteLeaderboardScore")));
		return false;
	}
	SetError(FString());
	return true;
}

void UCySteamSubsystem::HandleLeaderboardFlushComplete(const FName SessionName, const bool bSuccess)
{
	if (!ActiveLeaderboardWrite.IsValid() || SessionName != ActiveLeaderboardSessionName)
	{
		return;
	}
	const FString CompletedLeaderboard = ActiveLeaderboardWriteName;
	const FString Error = bSuccess ? FString() : TEXT("Steam failed to flush the leaderboard score.");
	ActiveLeaderboardWrite.Reset();
	ActiveLeaderboardWriteName.Reset();
	ActiveLeaderboardSessionName = NAME_None;
	SetError(Error);
	OnLeaderboardWriteComplete.Broadcast(bSuccess, CompletedLeaderboard, Error);
}

bool UCySteamSubsystem::EnumerateCloudFiles(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	if (!UserId.IsValid() || !BoundCloud.IsValid())
	{
		SetError(UserId.IsValid() ? TEXT("Steam Cloud interface is unavailable.") : Error);
		return false;
	}
	BoundCloud->EnumerateUserFiles(*UserId);
	SetError(FString());
	return true;
}

bool UCySteamSubsystem::ReadCloudFile(const FString& FileName, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	if (!UserId.IsValid() || !BoundCloud.IsValid() || FileName.IsEmpty())
	{
		SetError(!UserId.IsValid() ? Error : TEXT("Steam Cloud is unavailable or File Name is empty."));
		return false;
	}
	const bool bStarted = BoundCloud->ReadUserFile(*UserId, FileName);
	if (!bStarted) SetError(FailureText(TEXT("ReadCloudFile")));
	return bStarted;
}

bool UCySteamSubsystem::WriteCloudFile(const FString& FileName, const TArray<uint8>& Data, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	if (!UserId.IsValid() || !BoundCloud.IsValid() || FileName.IsEmpty())
	{
		SetError(!UserId.IsValid() ? Error : TEXT("Steam Cloud is unavailable or File Name is empty."));
		return false;
	}
	PendingCloudWriteData = Data;
	const bool bStarted = BoundCloud->WriteUserFile(*UserId, FileName, PendingCloudWriteData, false);
	if (!bStarted)
	{
		PendingCloudWriteData.Reset();
		SetError(FailureText(TEXT("WriteCloudFile")));
	}
	return bStarted;
}

bool UCySteamSubsystem::DeleteCloudFile(const FString& FileName, const bool bDeleteLocalCopy, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FString Error;
	const FUniqueNetIdPtr UserId = GetLocalUserId(LocalUserNum, &Error);
	if (!UserId.IsValid() || !BoundCloud.IsValid() || FileName.IsEmpty())
	{
		SetError(!UserId.IsValid() ? Error : TEXT("Steam Cloud is unavailable or File Name is empty."));
		return false;
	}
	const bool bStarted = BoundCloud->DeleteUserFile(*UserId, FileName, true, bDeleteLocalCopy);
	if (!bStarted) SetError(FailureText(TEXT("DeleteCloudFile")));
	return bStarted;
}

void UCySteamSubsystem::HandleEnumerateCloudFilesComplete(const bool bSuccess, const FUniqueNetId& UserId)
{
	TArray<FCySteamCloudFile> Files;
	if (bSuccess && BoundCloud.IsValid())
	{
		TArray<FCloudFileHeader> NativeFiles;
		BoundCloud->GetUserFileList(UserId, NativeFiles);
		Files.Reserve(NativeFiles.Num());
		for (const FCloudFileHeader& NativeFile : NativeFiles)
		{
			FCySteamCloudFile& File = Files.Emplace_GetRef();
			File.FileName = NativeFile.FileName;
			File.FileSize = NativeFile.FileSize;
			File.Hash = NativeFile.Hash;
		}
	}
	const FString Error = bSuccess ? FString() : TEXT("Steam failed to enumerate cloud files.");
	SetError(Error);
	OnCloudFilesEnumerated.Broadcast(bSuccess, Files, Error);
}

void UCySteamSubsystem::HandleReadCloudFileComplete(const bool bSuccess, const FUniqueNetId& UserId, const FString& FileName)
{
	TArray<uint8> Data;
	const bool bHasData = bSuccess && BoundCloud.IsValid() && BoundCloud->GetFileContents(UserId, FileName, Data);
	const FString Error = bHasData ? FString() : TEXT("Steam failed to read the cloud file.");
	SetError(Error);
	OnCloudFileReadComplete.Broadcast(bHasData, FileName, Data, Error);
}

void UCySteamSubsystem::HandleWriteCloudFileComplete(const bool bSuccess, const FUniqueNetId&, const FString& FileName)
{
	PendingCloudWriteData.Reset();
	const FString Error = bSuccess ? FString() : TEXT("Steam failed to write the cloud file.");
	SetError(Error);
	OnCloudFileWriteComplete.Broadcast(bSuccess, FileName, Error);
}

void UCySteamSubsystem::HandleDeleteCloudFileComplete(const bool bSuccess, const FUniqueNetId&, const FString& FileName)
{
	const FString Error = bSuccess ? FString() : TEXT("Steam failed to delete the cloud file.");
	SetError(Error);
	OnCloudFileDeleteComplete.Broadcast(bSuccess, FileName, Error);
}

bool UCySteamSubsystem::ShowFriendsOverlay(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	return BoundExternalUI.IsValid() && BoundExternalUI->ShowFriendsUI(LocalUserNum);
}

bool UCySteamSubsystem::ShowInviteOverlay(const int32 LocalUserNum, const FName SessionName)
{
	EnsureNotificationBindings();
	return BoundExternalUI.IsValid() && BoundExternalUI->ShowInviteUI(LocalUserNum, SessionName);
}

bool UCySteamSubsystem::ShowAchievementsOverlay(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	return BoundExternalUI.IsValid() && BoundExternalUI->ShowAchievementsUI(LocalUserNum);
}

bool UCySteamSubsystem::ShowProfileOverlay(const FString& UserId, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	const FUniqueNetIdPtr LocalId = GetLocalUserId(LocalUserNum);
	const FUniqueNetIdPtr TargetId = ParseUserId(UserId);
	return BoundExternalUI.IsValid() && LocalId.IsValid() && TargetId.IsValid()
		&& BoundExternalUI->ShowProfileUI(*LocalId, *TargetId);
}

bool UCySteamSubsystem::ShowStoreOverlay(const FString& SteamAppId, const bool bAddToCart, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	FShowStoreParams Params;
	Params.ProductId = SteamAppId;
	Params.bAddToCart = bAddToCart;
	return BoundExternalUI.IsValid() && BoundExternalUI->ShowStoreUI(LocalUserNum, Params);
}

bool UCySteamSubsystem::ShowWebOverlay(const FString& Url)
{
	EnsureNotificationBindings();
	return BoundExternalUI.IsValid() && BoundExternalUI->ShowWebURL(Url, FShowWebUrlParams());
}

void UCySteamSubsystem::HandleExternalUIChanged(const bool bIsVisible)
{
	OnExternalUIChanged.Broadcast(bIsVisible);
}

bool UCySteamSubsystem::RegisterLocalTalker(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	return BoundVoice.IsValid() && LocalUserNum >= 0 && LocalUserNum <= MAX_uint8
		&& BoundVoice->RegisterLocalTalker(static_cast<uint32>(LocalUserNum));
}

void UCySteamSubsystem::StartVoiceTransmission(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	if (BoundVoice.IsValid() && LocalUserNum >= 0 && LocalUserNum <= MAX_uint8)
	{
		BoundVoice->StartNetworkedVoice(static_cast<uint8>(LocalUserNum));
	}
}

void UCySteamSubsystem::StopVoiceTransmission(const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	if (BoundVoice.IsValid() && LocalUserNum >= 0 && LocalUserNum <= MAX_uint8)
	{
		BoundVoice->StopNetworkedVoice(static_cast<uint8>(LocalUserNum));
	}
}

bool UCySteamSubsystem::IsLocalPlayerTalking(const int32 LocalUserNum) const
{
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	const IOnlineVoicePtr Voice = Steam ? Steam->GetVoiceInterface() : nullptr;
	return Voice.IsValid() && LocalUserNum >= 0 && Voice->IsLocalPlayerTalking(static_cast<uint32>(LocalUserNum));
}

bool UCySteamSubsystem::MuteRemotePlayer(const FString& UserId, const bool bMuted, const int32 LocalUserNum)
{
	EnsureNotificationBindings();
	const FUniqueNetIdPtr TargetId = ParseUserId(UserId);
	if (!BoundVoice.IsValid() || !TargetId.IsValid() || LocalUserNum < 0 || LocalUserNum > MAX_uint8)
	{
		return false;
	}
	return bMuted
		? BoundVoice->MuteRemoteTalker(static_cast<uint8>(LocalUserNum), *TargetId, false)
		: BoundVoice->UnmuteRemoteTalker(static_cast<uint8>(LocalUserNum), *TargetId, false);
}

void UCySteamSubsystem::HandleTalkingStateChanged(const TSharedRef<const FUniqueNetId> UserId, const bool bIsTalking)
{
	OnTalkingStateChanged.Broadcast(UserId->ToString(), bIsTalking);
}

void UCySteamSubsystem::EnsureNotificationBindings()
{
	IOnlineSubsystem* Steam = GetSteamSubsystem();
	if (!Steam)
	{
		return;
	}

	const IOnlineLeaderboardsPtr Leaderboards = Steam->GetLeaderboardsInterface();
	if (Leaderboards != BoundLeaderboards)
	{
		if (BoundLeaderboards.IsValid())
		{
			if (LeaderboardReadHandle.IsValid()) BoundLeaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadHandle);
			if (LeaderboardFlushHandle.IsValid()) BoundLeaderboards->ClearOnLeaderboardFlushCompleteDelegate_Handle(LeaderboardFlushHandle);
		}
		BoundLeaderboards = Leaderboards;
		LeaderboardReadHandle.Reset();
		LeaderboardFlushHandle.Reset();
		if (BoundLeaderboards.IsValid())
		{
			LeaderboardReadHandle = BoundLeaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateUObject(this, &ThisClass::HandleLeaderboardReadComplete));
			LeaderboardFlushHandle = BoundLeaderboards->AddOnLeaderboardFlushCompleteDelegate_Handle(FOnLeaderboardFlushCompleteDelegate::CreateUObject(this, &ThisClass::HandleLeaderboardFlushComplete));
		}
	}

	const IOnlineUserCloudPtr Cloud = Steam->GetUserCloudInterface();
	if (Cloud != BoundCloud)
	{
		if (BoundCloud.IsValid())
		{
			if (EnumerateCloudHandle.IsValid()) BoundCloud->ClearOnEnumerateUserFilesCompleteDelegate_Handle(EnumerateCloudHandle);
			if (ReadCloudHandle.IsValid()) BoundCloud->ClearOnReadUserFileCompleteDelegate_Handle(ReadCloudHandle);
			if (WriteCloudHandle.IsValid()) BoundCloud->ClearOnWriteUserFileCompleteDelegate_Handle(WriteCloudHandle);
			if (DeleteCloudHandle.IsValid()) BoundCloud->ClearOnDeleteUserFileCompleteDelegate_Handle(DeleteCloudHandle);
		}
		BoundCloud = Cloud;
		EnumerateCloudHandle.Reset();
		ReadCloudHandle.Reset();
		WriteCloudHandle.Reset();
		DeleteCloudHandle.Reset();
		if (BoundCloud.IsValid())
		{
			EnumerateCloudHandle = BoundCloud->AddOnEnumerateUserFilesCompleteDelegate_Handle(FOnEnumerateUserFilesCompleteDelegate::CreateUObject(this, &ThisClass::HandleEnumerateCloudFilesComplete));
			ReadCloudHandle = BoundCloud->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateUObject(this, &ThisClass::HandleReadCloudFileComplete));
			WriteCloudHandle = BoundCloud->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateUObject(this, &ThisClass::HandleWriteCloudFileComplete));
			DeleteCloudHandle = BoundCloud->AddOnDeleteUserFileCompleteDelegate_Handle(FOnDeleteUserFileCompleteDelegate::CreateUObject(this, &ThisClass::HandleDeleteCloudFileComplete));
		}
	}

	const IOnlineExternalUIPtr ExternalUI = Steam->GetExternalUIInterface();
	if (ExternalUI != BoundExternalUI)
	{
		if (BoundExternalUI.IsValid() && ExternalUIHandle.IsValid()) BoundExternalUI->ClearOnExternalUIChangeDelegate_Handle(ExternalUIHandle);
		BoundExternalUI = ExternalUI;
		ExternalUIHandle.Reset();
		if (BoundExternalUI.IsValid()) ExternalUIHandle = BoundExternalUI->AddOnExternalUIChangeDelegate_Handle(FOnExternalUIChangeDelegate::CreateUObject(this, &ThisClass::HandleExternalUIChanged));
	}

	const IOnlineVoicePtr Voice = Steam->GetVoiceInterface();
	if (Voice != BoundVoice)
	{
		if (BoundVoice.IsValid() && TalkingStateHandle.IsValid()) BoundVoice->ClearOnPlayerTalkingStateChangedDelegate_Handle(TalkingStateHandle);
		BoundVoice = Voice;
		TalkingStateHandle.Reset();
		if (BoundVoice.IsValid()) TalkingStateHandle = BoundVoice->AddOnPlayerTalkingStateChangedDelegate_Handle(FOnPlayerTalkingStateChangedDelegate::CreateUObject(this, &ThisClass::HandleTalkingStateChanged));
	}
}

void UCySteamSubsystem::ClearNotificationBindings()
{
	if (BoundLeaderboards.IsValid())
	{
		if (LeaderboardReadHandle.IsValid()) BoundLeaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadHandle);
		if (LeaderboardFlushHandle.IsValid()) BoundLeaderboards->ClearOnLeaderboardFlushCompleteDelegate_Handle(LeaderboardFlushHandle);
	}
	if (BoundCloud.IsValid())
	{
		if (EnumerateCloudHandle.IsValid()) BoundCloud->ClearOnEnumerateUserFilesCompleteDelegate_Handle(EnumerateCloudHandle);
		if (ReadCloudHandle.IsValid()) BoundCloud->ClearOnReadUserFileCompleteDelegate_Handle(ReadCloudHandle);
		if (WriteCloudHandle.IsValid()) BoundCloud->ClearOnWriteUserFileCompleteDelegate_Handle(WriteCloudHandle);
		if (DeleteCloudHandle.IsValid()) BoundCloud->ClearOnDeleteUserFileCompleteDelegate_Handle(DeleteCloudHandle);
	}
	if (BoundExternalUI.IsValid() && ExternalUIHandle.IsValid()) BoundExternalUI->ClearOnExternalUIChangeDelegate_Handle(ExternalUIHandle);
	if (BoundVoice.IsValid() && TalkingStateHandle.IsValid()) BoundVoice->ClearOnPlayerTalkingStateChangedDelegate_Handle(TalkingStateHandle);

	BoundLeaderboards.Reset();
	BoundCloud.Reset();
	BoundExternalUI.Reset();
	BoundVoice.Reset();
	LeaderboardReadHandle.Reset();
	LeaderboardFlushHandle.Reset();
	EnumerateCloudHandle.Reset();
	ReadCloudHandle.Reset();
	WriteCloudHandle.Reset();
	DeleteCloudHandle.Reset();
	ExternalUIHandle.Reset();
	TalkingStateHandle.Reset();
}
