// Copyright Atlas XR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CySessionTypes.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CySessionSubsystem.generated.h"

class IOnlineSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCySessionLoginComplete, bool, bSuccess, int32, LocalUserNum, const FString&, UserId, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySessionLogoutComplete, bool, bSuccess, int32, LocalUserNum, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCySessionLoginStatusChanged, int32, LocalUserNum, ECySessionLoginStatus, OldStatus, ECySessionLoginStatus, NewStatus, const FString&, UserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySessionActionComplete, bool, bSuccess, FName, SessionName, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySessionFindComplete, bool, bSuccess, const TArray<FCySessionSearchResult>&, Results, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCySessionJoinComplete, bool, bSuccess, FName, SessionName, ECySessionJoinResult, Result, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySessionInviteAccepted, int32, LocalUserNum, const FString&, UserId, const FCySessionSearchResult&, Session);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySessionInviteReceived, const FString&, UserId, const FString&, FromUserId, const FCySessionSearchResult&, Session);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCySessionParticipantChanged, FName, SessionName, const FString&, UserId, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCySessionFailure, const FString&, UserId, const FString&, Error);

/**
 * Persistent Blueprint entry point for authentication and multiplayer sessions.
 * In Blueprint: Get Game Instance Subsystem -> Cy Session Subsystem.
 */
UCLASS(BlueprintType)
class CYSESSION_API UCySessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Identity")
	FCySessionLoginComplete OnLoginComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Identity")
	FCySessionLogoutComplete OnLogoutComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Identity")
	FCySessionLoginStatusChanged OnLoginStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionActionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionActionComplete OnUpdateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionActionComplete OnStartSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionActionComplete OnEndSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionActionComplete OnDestroySessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Search")
	FCySessionFindComplete OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Search")
	FCySessionActionComplete OnCancelFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionJoinComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Invites")
	FCySessionInviteAccepted OnSessionInviteAccepted;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Invites")
	FCySessionInviteReceived OnSessionInviteReceived;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionParticipantChanged OnSessionParticipantJoined;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionParticipantChanged OnSessionParticipantLeft;

	UPROPERTY(BlueprintAssignable, Category = "Cy Session|Session")
	FCySessionFailure OnSessionFailure;

	/** Overrides Project Settings for this Game Instance. NAME_None restores the configured default. */
	UFUNCTION(BlueprintCallable, Category = "Cy Session|Provider")
	bool SetSubsystemOverride(FName SubsystemName);

	/** Selects a configured profile for this Game Instance. NAME_None restores command-line/project selection. */
	UFUNCTION(BlueprintCallable, Category = "Cy Session|Configuration")
	bool SetActiveProfileOverride(FName ProfileName);

	UFUNCTION(BlueprintPure, Category = "Cy Session|Configuration")
	FName GetActiveProfileName() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Configuration")
	bool GetActivePlatformProfile(FCySessionPlatformProfile& Profile) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Configuration")
	FCySessionConfigurationReport ValidateConfiguration() const;

	/** Creates provider-appropriate credentials without storing a token in config. */
	UFUNCTION(BlueprintPure, Category = "Cy Session|Configuration")
	FCySessionCredentials MakeDefaultCredentials(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Configuration")
	FCySessionCreateOptions MakeDefaultCreateOptions(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	bool IsSubsystemAvailable() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	FName GetActiveSubsystemName() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	FString GetPlatformTag() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	FString GetStoreTag() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	ECySessionPlatformType GetPlatformType() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	ECySessionStoreType GetStoreType() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	FCySessionPlatformInfo GetPlatformInfo() const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Provider")
	FCySessionProviderCapabilities GetProviderCapabilities(FName ProviderName) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Debug")
	FString GetLastError() const { return LastError; }

	UFUNCTION(BlueprintPure, Category = "Cy Session|Debug")
	bool IsOperationInProgress(ECySessionAsyncOperation Operation) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Identity")
	bool AutoLogin(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Identity")
	bool Login(const FCySessionCredentials& Credentials);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Identity")
	bool Logout(int32 LocalUserNum = 0);

	UFUNCTION(BlueprintPure, Category = "Cy Session|Identity")
	ECySessionLoginStatus GetLoginStatus(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Identity")
	bool IsLoggedIn(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Identity")
	FString GetLocalUserId(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Identity")
	FString GetLocalUserDisplayName(int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Session")
	bool CreateSession(const FCySessionCreateOptions& Options);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Session")
	bool UpdateSession(const FCySessionUpdateOptions& Options);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Session")
	bool StartSession(FName SessionName = TEXT("GameSession"));

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Session")
	bool EndSession(FName SessionName = TEXT("GameSession"));

	/** Destroys the local named session. This is also the normal Leave Session operation for clients. */
	UFUNCTION(BlueprintCallable, Category = "Cy Session|Session")
	bool DestroySession(FName SessionName = TEXT("GameSession"));

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Search")
	bool FindSessions(const FCySessionFindOptions& Options);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Search")
	bool CancelFindSessions();

	UFUNCTION(BlueprintPure, Category = "Cy Session|Search")
	TArray<FCySessionSearchResult> GetCachedSearchResults() const { return CachedBlueprintResults; }

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Session")
	bool JoinSession(const FString& ResultHandle, const FCySessionJoinOptions& Options);

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Invites")
	bool SendSessionInvite(const FString& FriendUserId, int32 LocalUserNum = 0, FName SessionName = TEXT("GameSession"));

	UFUNCTION(BlueprintPure, Category = "Cy Session|Session")
	ECySessionState GetSessionState(FName SessionName = TEXT("GameSession")) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Session")
	bool GetCurrentSession(FName SessionName, FCySessionSearchResult& Result) const;

	UFUNCTION(BlueprintPure, Category = "Cy Session|Travel")
	bool GetResolvedConnectString(FName SessionName, FString& ConnectString) const;

	UFUNCTION(BlueprintCallable, Category = "Cy Session|Travel")
	bool TravelToSession(FName SessionName = TEXT("GameSession"), int32 LocalPlayerIndex = 0);

private:
	enum class EPendingAfterDestroy : uint8
	{
		None,
		Create,
		Join
	};

	IOnlineSubsystem* GetOnlineSubsystem() const;
	IOnlineIdentityPtr GetIdentityInterface() const;
	IOnlineSessionPtr GetSessionInterface() const;
	FName GetRequestedSubsystemName() const;
	const FCySessionPlatformProfile* GetActiveProfile() const;
	bool HasAnyOperationInProgress() const;
	void EnsureNotificationBindings();
	void ClearNotificationBindings();
	void SetError(const FString& Error);

	bool BeginCreateSession(const FCySessionCreateOptions& Options);
	bool BeginJoinSession(const FOnlineSessionSearchResult& NativeResult, const FCySessionJoinOptions& Options);
	bool BeginDestroySession(FName SessionName);
	void HostTravel(const FCySessionCreateOptions& Options);
	FCySessionSearchResult MakeBlueprintResult(const FOnlineSessionSearchResult& NativeResult, const FString& PreferredHandle = FString()) const;
	FString CacheNativeResult(const FOnlineSessionSearchResult& NativeResult);
	bool MatchesFindOptions(const FOnlineSessionSearchResult& NativeResult, const FCySessionFindOptions& Options) const;

	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void HandleLogoutComplete(int32 LocalUserNum, bool bWasSuccessful);
	void HandleLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& UserId);
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleUpdateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleEndSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleCancelFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleSessionInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult);
	void HandleSessionParticipantJoined(FName SessionName, const FUniqueNetId& UserId);
	void HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UserId, EOnSessionParticipantLeftReason Reason);
	void HandleSessionFailure(const FUniqueNetId& UserId, ESessionFailure::Type FailureType);

	FName RuntimeSubsystemOverride = NAME_None;
	FName RuntimeProfileOverride = NAME_None;
	FString LastError;
	IOnlineIdentityPtr BoundIdentity;
	IOnlineSessionPtr BoundSessions;
	TArray<FDelegateHandle> LoginStatusChangedHandles;
	FDelegateHandle InviteAcceptedHandle;
	FDelegateHandle InviteReceivedHandle;
	FDelegateHandle ParticipantJoinedHandle;
	FDelegateHandle ParticipantLeftHandle;
	FDelegateHandle SessionFailureHandle;

	FDelegateHandle LoginHandle;
	FDelegateHandle LogoutHandle;
	FDelegateHandle CreateHandle;
	FDelegateHandle UpdateHandle;
	FDelegateHandle StartHandle;
	FDelegateHandle EndHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle CancelFindHandle;
	FDelegateHandle JoinHandle;

	bool bLoginInProgress = false;
	bool bLogoutInProgress = false;
	bool bCreateInProgress = false;
	bool bUpdateInProgress = false;
	bool bStartInProgress = false;
	bool bEndInProgress = false;
	bool bDestroyInProgress = false;
	bool bFindInProgress = false;
	bool bCancelFindInProgress = false;
	bool bJoinInProgress = false;
	int32 ActiveLoginUserNum = 0;
	int32 ActiveLogoutUserNum = 0;

	TSharedPtr<FOnlineSessionSearch> ActiveSearch;
	TMap<FString, FOnlineSessionSearchResult> CachedNativeResults;
	TArray<FCySessionSearchResult> CachedBlueprintResults;
	FCySessionFindOptions ActiveFindOptions;
	FCySessionCreateOptions ActiveCreateOptions;
	FCySessionJoinOptions ActiveJoinOptions;
	EPendingAfterDestroy PendingAfterDestroy = EPendingAfterDestroy::None;
	FCySessionCreateOptions PendingCreateOptions;
	FCySessionJoinOptions PendingJoinOptions;
	FOnlineSessionSearchResult PendingJoinResult;
};
