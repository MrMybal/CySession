// Copyright Atlas XR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CySessionTypes.h"
#include "Engine/DeveloperSettings.h"
#include "CySessionSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Cy Session"))
class CYSESSION_API UCySessionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCySessionSettings();

	/** Profile selected for this build. Override at launch with -CySessionProfile=ProfileName. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profiles")
	FName ActiveProfileName = NAME_None;

	/** Ready-to-edit presets. SDK plugins remain optional and are not linked by CySession. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profiles", meta = (TitleProperty = "ProfileName"))
	TArray<FCySessionPlatformProfile> PlatformProfiles;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profiles")
	bool bValidateConfigurationOnStartup = true;

	/** Empty uses [OnlineSubsystem] DefaultPlatformService. Examples: EOS, EOSPlus, Steam, NULL. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Provider")
	FName SubsystemName = NAME_None;

	/** Optional stable platform family used by Same Platform Only. Empty uses the packaged platform name. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cross-play")
	FString PlatformTagOverride;

	/** Optional typed platform override. Custom Platform Tag takes precedence when set. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cross-play")
	ECySessionPlatformType PlatformTypeOverride = ECySessionPlatformType::Auto;

	/** Store is not always detectable from EOS. Set this per store build (GOG, Meta, Apple, etc.). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cross-play")
	FString StoreTagOverride;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cross-play")
	ECySessionStoreType StoreTypeOverride = ECySessionStoreType::Auto;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	FName DefaultSessionName = TEXT("GameSession");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bVerboseLogging = false;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	const FCySessionPlatformProfile* FindProfile(FName ProfileName) const;
};
