// Copyright Atlas XR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CySteamTypes.h"
#include "OnlineStats.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CySteamSubsystem.generated.h"

class IOnlineSubsystem;
class FOnlineUserPresence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamFriendsReadComplete, bool, bSuccess, const TArray<FCySteamFriend>&, Friends, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamPresenceComplete, bool, bSuccess, const FString&, UserId, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamAchievementsReadComplete, bool, bSuccess, const TArray<FCySteamAchievement>&, Achievements, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamAchievementWriteComplete, bool, bSuccess, const FString&, AchievementId, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCySteamLeaderboardReadComplete, bool, bSuccess, const FString&, LeaderboardName, const TArray<FCySteamLeaderboardRow>&, Rows, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamLeaderboardWriteComplete, bool, bSuccess, const FString&, LeaderboardName, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamCloudFilesEnumerated, bool, bSuccess, const TArray<FCySteamCloudFile>&, Files, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCySteamCloudFileReadComplete, bool, bSuccess, const FString&, FileName, const TArray<uint8>&, Data, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySteamCloudActionComplete, bool, bSuccess, const FString&, FileName, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCySteamExternalUIChanged, bool, bIsVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCySteamTalkingStateChanged, const FString&, UserId, bool, bIsTalking);

/**
 * Blueprint access to the Steam services implemented by UE 5.5 OnlineSubsystemSteam.
 * No EOS or Steamworks header is linked: the Steam provider remains optional on desktop targets.
 */
UCLASS(BlueprintType)
class CYSESSION_API UCySteamSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Friends")
	FCySteamFriendsReadComplete OnFriendsReadComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Presence")
	FCySteamPresenceComplete OnPresenceSetComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Presence")
	FCySteamPresenceComplete OnPresenceQueryComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Achievements")
	FCySteamAchievementsReadComplete OnAchievementsReadComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Achievements")
	FCySteamAchievementWriteComplete OnAchievementWriteComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Leaderboards")
	FCySteamLeaderboardReadComplete OnLeaderboardReadComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Leaderboards")
	FCySteamLeaderboardWriteComplete OnLeaderboardWriteComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Cloud")
	FCySteamCloudFilesEnumerated OnCloudFilesEnumerated;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Cloud")
	FCySteamCloudFileReadComplete OnCloudFileReadComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Cloud")
	FCySteamCloudActionComplete OnCloudFileWriteComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Cloud")
	FCySteamCloudActionComplete OnCloudFileDeleteComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Overlay")
	FCySteamExternalUIChanged OnExternalUIChanged;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Steam|Voice")
	FCySteamTalkingStateChanged OnTalkingStateChanged;

	/** Selects Steam_Windows_Full in CySession. NetDriver changes still require the supplied .ini before startup. */
	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Configuration")
	bool ActivateFullSteamMode();

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Configuration")
	bool IsSteamAvailable() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Configuration")
	bool IsSteamActiveProvider() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Configuration")
	FCySteamCapabilities GetSteamCapabilities() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Configuration")
	FCySteamValidationReport ValidateSteamConfiguration() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Configuration")
	FString GetLastSteamError() const { return LastSteamError; }

	/** Provider token exposed by IOnlineIdentitySteam for authentication with a game backend. */
	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Identity")
	FString GetSteamAuthToken(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Friends")
	bool ReadFriends(int32 LocalUserNum = 0, const FString& ListName = TEXT("default"));

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Friends")
	TArray<FCySteamFriend> GetCachedFriends(int32 LocalUserNum = 0, const FString& ListName = TEXT("default")) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Presence")
	bool SetPresence(int32 LocalUserNum, ECySteamPresenceState State, const FString& Status, const TMap<FString, FString>& Properties);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Presence")
	bool QueryPresence(const FString& UserId);

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Presence")
	bool GetCachedPresence(const FString& UserId, FCySteamPresence& Presence) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Achievements")
	bool QueryAchievements(int32 LocalUserNum = 0);

	/** OnlineSubsystemSteam treats any value above zero as an unlock; zero only resets in non-Shipping builds. */
	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Achievements")
	bool WriteAchievement(const FString& AchievementId, double Progress = 100.0, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Achievements")
	TArray<FCySteamAchievement> GetCachedAchievements(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Leaderboards")
	bool ReadLeaderboardAroundUser(const FString& LeaderboardName, const FString& StatName, int32 Range = 10, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Leaderboards")
	bool ReadLeaderboardAroundRank(const FString& LeaderboardName, const FString& StatName, int32 Rank = 1, int32 Range = 10);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Leaderboards")
	bool ReadFriendsLeaderboard(const FString& LeaderboardName, const FString& StatName, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Leaderboards")
	bool WriteLeaderboardScore(const FString& LeaderboardName, const FString& StatName, int32 Score, ECySteamLeaderboardUpdateMethod UpdateMethod = ECySteamLeaderboardUpdateMethod::KeepBest, FName SessionName = TEXT("GameSession"), int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Cloud")
	bool EnumerateCloudFiles(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Cloud")
	bool ReadCloudFile(const FString& FileName, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Cloud")
	bool WriteCloudFile(const FString& FileName, const TArray<uint8>& Data, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Cloud")
	bool DeleteCloudFile(const FString& FileName, bool bDeleteLocalCopy = true, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Overlay")
	bool ShowFriendsOverlay(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Overlay")
	bool ShowInviteOverlay(int32 LocalUserNum = 0, FName SessionName = TEXT("GameSession"));

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Overlay")
	bool ShowAchievementsOverlay(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Overlay")
	bool ShowProfileOverlay(const FString& UserId, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Overlay")
	bool ShowStoreOverlay(const FString& SteamAppId, bool bAddToCart = false, int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Overlay")
	bool ShowWebOverlay(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Voice")
	bool RegisterLocalTalker(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Voice")
	void StartVoiceTransmission(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Voice")
	void StopVoiceTransmission(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintPure, Category = "Cy Session|Steam|Voice")
	bool IsLocalPlayerTalking(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Steam|Voice")
	bool MuteRemotePlayer(const FString& UserId, bool bMuted, int32 LocalUserNum = 0);

private:
	IOnlineSubsystem* GetSteamSubsystem() const;
	TSharedPtr<const FUniqueNetId> GetLocalUserId(int32 LocalUserNum, FString* Error = nullptr) const;
	TSharedPtr<const FUniqueNetId> ParseUserId(const FString& UserId, FString* Error = nullptr) const;
	void SetError(const FString& Error);
	void EnsureNotificationBindings();
	void ClearNotificationBindings();

	FCySteamPresence MakePresence(const FString& UserId, const FOnlineUserPresence& NativePresence) const;
	TArray<FCySteamAchievement> BuildAchievements(const FUniqueNetId& UserId) const;
	bool BeginLeaderboardRead(const FString& LeaderboardName, const FString& StatName);
	void HandleLeaderboardReadComplete(bool bSuccess);
	void HandleLeaderboardFlushComplete(FName SessionName, bool bSuccess);
	void HandleEnumerateCloudFilesComplete(bool bSuccess, const FUniqueNetId& UserId);
	void HandleReadCloudFileComplete(bool bSuccess, const FUniqueNetId& UserId, const FString& FileName);
	void HandleWriteCloudFileComplete(bool bSuccess, const FUniqueNetId& UserId, const FString& FileName);
	void HandleDeleteCloudFileComplete(bool bSuccess, const FUniqueNetId& UserId, const FString& FileName);
	void HandleExternalUIChanged(bool bIsVisible);
	void HandleTalkingStateChanged(TSharedRef<const FUniqueNetId> UserId, bool bIsTalking);

	FString LastSteamError;
	FString ActiveAchievementId;
	FString ActiveLeaderboardReadName;
	FString ActiveLeaderboardReadStat;
	FString ActiveLeaderboardWriteName;
	FName ActiveLeaderboardSessionName = NAME_None;
	TSharedPtr<FOnlineAchievementsWrite, ESPMode::ThreadSafe> ActiveAchievementWrite;
	TSharedPtr<FOnlineLeaderboardRead, ESPMode::ThreadSafe> ActiveLeaderboardRead;
	TSharedPtr<FOnlineLeaderboardWrite, ESPMode::ThreadSafe> ActiveLeaderboardWrite;
	TArray<uint8> PendingCloudWriteData;

	TSharedPtr<class IOnlineLeaderboards, ESPMode::ThreadSafe> BoundLeaderboards;
	TSharedPtr<class IOnlineUserCloud, ESPMode::ThreadSafe> BoundCloud;
	TSharedPtr<class IOnlineExternalUI, ESPMode::ThreadSafe> BoundExternalUI;
	TSharedPtr<class IOnlineVoice, ESPMode::ThreadSafe> BoundVoice;
	FDelegateHandle LeaderboardReadHandle;
	FDelegateHandle LeaderboardFlushHandle;
	FDelegateHandle EnumerateCloudHandle;
	FDelegateHandle ReadCloudHandle;
	FDelegateHandle WriteCloudHandle;
	FDelegateHandle DeleteCloudHandle;
	FDelegateHandle ExternalUIHandle;
	FDelegateHandle TalkingStateHandle;
};
