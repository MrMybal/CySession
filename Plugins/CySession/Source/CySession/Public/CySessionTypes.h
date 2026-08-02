// Copyright Atlas XR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CySessionTypes.generated.h"

/** Hardware/runtime family. Auto is resolved from the packaged target and optional provider hints. */
UENUM(BlueprintType)
enum class ECySessionPlatformType : uint8
{
	Auto,
	Windows,
	MacOS,
	Linux,
	IOS,
	Android,
	MetaQuest,
	PlayStation,
	Xbox,
	NintendoSwitch,
	Other
};

/** Distribution store, kept separate from the networking provider. */
UENUM(BlueprintType)
enum class ECySessionStoreType : uint8
{
	Auto,
	EpicGamesStore,
	Steam,
	GOG,
	AppleAppStore,
	GooglePlay,
	MetaHorizon,
	AmazonAppstore,
	PlayStationStore,
	MicrosoftStore,
	NintendoEshop,
	Other
};

/** Login state exposed without leaking provider-specific Online Subsystem types. */
UENUM(BlueprintType)
enum class ECySessionLoginStatus : uint8
{
	NotLoggedIn,
	UsingLocalProfile,
	LoggedIn
};

UENUM(BlueprintType)
enum class ECySessionJoinResult : uint8
{
	Success,
	SessionIsFull,
	SessionDoesNotExist,
	CouldNotRetrieveAddress,
	AlreadyInSession,
	UnknownError
};

UENUM(BlueprintType)
enum class ECySessionState : uint8
{
	NoSession,
	Creating,
	Pending,
	Starting,
	InProgress,
	Ending,
	Ended,
	Destroying
};

/** Controls which advertised sessions are returned by Find Sessions. */
UENUM(BlueprintType)
enum class ECySessionCrossplayPolicy : uint8
{
	/** Return same-platform and cross-platform sessions. */
	Any,
	/** Return only sessions whose host explicitly enabled cross-play. */
	CrossplayEnabledOnly,
	/** Return only sessions hosted on this client's platform tag. */
	SamePlatformOnly,
	/** Return only sessions distributed through the same store tag. */
	SameStoreOnly,
	/** Require both the same hardware platform and the same store. */
	SamePlatformAndStoreOnly
};

UENUM(BlueprintType)
enum class ECySessionAsyncOperation : uint8
{
	Login,
	Logout,
	Create,
	Find,
	CancelFind,
	Join,
	Update,
	Start,
	End,
	Destroy
};

/** Provider credentials. For EOS, Type is commonly accountportal, developer, exchange_code or persistentauth. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionCredentials
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Identity")
	int32 LocalUserNum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Identity")
	FString Type = TEXT("accountportal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Identity")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Identity")
	FString Token;
};

/** Runtime identity advertised with sessions. Provider, hardware and store are deliberately independent. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionPlatformInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	FName ProviderName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	ECySessionPlatformType PlatformType = ECySessionPlatformType::Other;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	ECySessionStoreType StoreType = ECySessionStoreType::Auto;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	FString PlatformTag;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	FString StoreTag;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	bool bIsMobile = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	bool bIsConsole = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Platform")
	bool bIsXR = false;
};

/** Lets Blueprint verify an optional provider without adding a compile-time dependency to its SDK. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionProviderCapabilities
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	FName ProviderName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bProviderAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bIdentityAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bSessionsAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bFriendsAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bPresenceAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bAchievementsAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bLeaderboardsAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bUserCloudAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bVoiceAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bExternalUIAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Provider")
	bool bIsActive = false;
};

/** One deployable store/platform configuration selectable from Project Settings or -CySessionProfile=. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionPlatformProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FName ProfileName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	bool bEnabled = true;

	/** Online Subsystem instance used for both identity and game sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Provider")
	FName SubsystemName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	ECySessionPlatformType PlatformType = ECySessionPlatformType::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	ECySessionStoreType StoreType = ECySessionStoreType::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FString PlatformTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FString StoreTag;

	/** Default credentials type used by Make Default Credentials. Tokens are never stored in config. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FString DefaultLoginType = TEXT("accountportal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session", meta = (ClampMin = "1"))
	int32 DefaultPublicConnections = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bAllowCrossplayByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bAllowCrossStoreByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bUseLobbiesByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bUseStatsByDefault = false;
};

/** Result of validating the selected profile and its runtime Online Subsystem. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionConfigurationReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Configuration")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Configuration")
	FName ActiveProfileName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Configuration")
	FCySessionPlatformInfo PlatformInfo;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Configuration")
	FCySessionProviderCapabilities ProviderCapabilities;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Configuration")
	TArray<FString> Errors;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Configuration")
	TArray<FString> Warnings;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionCreateOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	int32 LocalUserNum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FName SessionName = TEXT("GameSession");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session", meta = (ClampMin = "1"))
	int32 PublicConnections = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session", meta = (ClampMin = "0"))
	int32 PrivateConnections = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FString MapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FString GameMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FString MatchType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FString Region;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAllowCrossplay = true;

	/** Allows users from another store on the same hardware family (for example GOG and Steam on Windows). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAllowCrossStore = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bLAN = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bDedicated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAdvertise = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAllowJoinInProgress = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAllowInvites = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bUsesPresence = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bUsesStats = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bUseLobbies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bUseLobbyVoiceChat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bStartSessionAfterCreate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Travel")
	bool bTravelOnSuccess = false;

	/** Long package name such as /Game/Maps/Lobby. Falls back to MapName. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Travel")
	FString TravelMap;

	/** Appended to host travel. "listen" is added automatically for non-dedicated hosts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Travel")
	FString TravelOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bDestroyExistingSession = true;

	/** Zero lets the provider/project select the build ID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session", meta = (ClampMin = "0"))
	int32 BuildUniqueId = 0;

	/** String attributes advertised to the backend and exposed in search results. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	TMap<FName, FString> CustomSettings;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionFindOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	int32 LocalUserNum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 MaxResults = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bLAN = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bUseLobbies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bRequireOpenPublicSlot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	ECySessionCrossplayPolicy CrossplayPolicy = ECySessionCrossplayPolicy::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FString MatchType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FString Region;

	/** Zero disables client-side ping filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session", meta = (ClampMin = "0"))
	int32 MaxPingMs = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	TMap<FName, FString> CustomFilters;

	/** Optional exact tag filter; useful for custom providers and stores not represented by the enum. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Platform")
	FString RequiredPlatformTag;

	/** Optional exact store filter, for example GOG, AppleAppStore, GooglePlay or MetaHorizon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session|Platform")
	FString RequiredStoreTag;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionJoinOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	int32 LocalUserNum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FName SessionName = TEXT("GameSession");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bTravelOnSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bDestroyExistingSession = true;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionUpdateOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	FName SessionName = TEXT("GameSession");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAdvertise = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAllowJoinInProgress = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	bool bAllowInvites = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cy Session")
	TMap<FName, FString> CustomSettings;
};

/** A Blueprint-safe snapshot. ResultHandle is the opaque key used by Join Session. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString ResultHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString SessionId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString HostUserId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString HostDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString Platform;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString Store;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FName Provider = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString GameMode;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString MatchType;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	FString Region;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	bool bCrossplayEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	bool bCrossStoreEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	bool bMobile = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	bool bDedicated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	int32 OpenPublicConnections = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	int32 MaxPublicConnections = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	TMap<FName, FString> Settings;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session")
	bool bIsValid = false;
};
