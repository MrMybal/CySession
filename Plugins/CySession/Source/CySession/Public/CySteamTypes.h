// Copyright Atlas XR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CySteamTypes.generated.h"

/** Presence states shared by Steam and Unreal's Online Subsystem. */
UENUM(BlueprintType)
enum class ECySteamPresenceState : uint8
{
	Online,
	Offline,
	Away,
	ExtendedAway,
	DoNotDisturb,
	Chat
};

/** Steam leaderboard score replacement policy. */
UENUM(BlueprintType)
enum class ECySteamLeaderboardUpdateMethod : uint8
{
	KeepBest,
	Force
};

/** Runtime view of every Steam service exposed by OnlineSubsystemSteam in UE 5.5. */
USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamCapabilities
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bSteamAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bSteamIsActiveProvider = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bIdentity = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bSessions = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bFriends = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bPresence = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bAchievements = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bLeaderboards = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bUserCloud = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bVoice = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bExternalUI = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bStore = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bPurchase = false;

	/** True when a Steam/SteamSockets net driver is present in the resolved engine config. */
	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bSteamNetDriverConfigured = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bSteamEnabledInConfig = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bInitServerOnClient = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bUseSteamNetworking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bAllowP2PPacketRelay = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString AppId;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamValidationReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FCySteamCapabilities Capabilities;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	TArray<FString> Errors;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	TArray<FString> Warnings;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamPresence
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString UserId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	ECySteamPresenceState State = ECySteamPresenceState::Offline;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bOnline = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bPlaying = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bPlayingThisGame = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bJoinable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bHasVoiceSupport = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString SessionId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	TMap<FString, FString> Properties;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamFriend
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString UserId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString RealName;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FCySteamPresence Presence;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamAchievement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString AchievementId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	double Progress = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FText LockedDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FText UnlockedDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	bool bHidden = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FDateTime UnlockTime;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamLeaderboardRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString PlayerId;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	TMap<FString, FString> Columns;
};

USTRUCT(BlueprintType)
struct CYSESSION_API FCySteamCloudFile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString FileName;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	int32 FileSize = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Cy Session|Steam")
	FString Hash;
};
