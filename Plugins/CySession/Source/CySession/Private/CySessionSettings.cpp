// Copyright Atlas XR. All Rights Reserved.

#include "CySessionSettings.h"

namespace
{
	FCySessionPlatformProfile MakeProfile(
		const TCHAR* Name,
		const TCHAR* Subsystem,
		const ECySessionPlatformType Platform,
		const ECySessionStoreType Store,
		const TCHAR* LoginType,
		const bool bCrossplay,
		const bool bCrossStore,
		const bool bUseLobbies = true)
	{
		FCySessionPlatformProfile Profile;
		Profile.ProfileName = FName(Name);
		Profile.SubsystemName = FName(Subsystem);
		Profile.PlatformType = Platform;
		Profile.StoreType = Store;
		Profile.DefaultLoginType = LoginType;
		Profile.bAllowCrossplayByDefault = bCrossplay;
		Profile.bAllowCrossStoreByDefault = bCrossStore;
		Profile.bUseLobbiesByDefault = bUseLobbies;
		return Profile;
	}
}

UCySessionSettings::UCySessionSettings()
{
	PlatformProfiles.Reserve(11);
	PlatformProfiles.Add(MakeProfile(TEXT("Local_NULL"), TEXT("NULL"), ECySessionPlatformType::Auto, ECySessionStoreType::Other, TEXT(""), false, false, false));
	PlatformProfiles.Add(MakeProfile(TEXT("EOS_Windows_Epic"), TEXT("EOS"), ECySessionPlatformType::Windows, ECySessionStoreType::EpicGamesStore, TEXT("accountportal"), true, true));
	PlatformProfiles.Add(MakeProfile(TEXT("EOS_Windows_Steam"), TEXT("EOS"), ECySessionPlatformType::Windows, ECySessionStoreType::Steam, TEXT(""), true, true));
	PlatformProfiles.Add(MakeProfile(TEXT("EOS_Windows_GOG"), TEXT("EOS"), ECySessionPlatformType::Windows, ECySessionStoreType::GOG, TEXT(""), true, true));
	PlatformProfiles.Add(MakeProfile(TEXT("Steam_Windows"), TEXT("Steam"), ECySessionPlatformType::Windows, ECySessionStoreType::Steam, TEXT(""), false, false));
	PlatformProfiles.Add(MakeProfile(TEXT("GOG_Galaxy_Windows"), TEXT("Galaxy"), ECySessionPlatformType::Windows, ECySessionStoreType::GOG, TEXT(""), false, false));
	PlatformProfiles.Add(MakeProfile(TEXT("EOS_Android_GooglePlay"), TEXT("EOS"), ECySessionPlatformType::Android, ECySessionStoreType::GooglePlay, TEXT(""), true, true));
	PlatformProfiles.Add(MakeProfile(TEXT("EOS_MetaQuest"), TEXT("EOS"), ECySessionPlatformType::MetaQuest, ECySessionStoreType::MetaHorizon, TEXT(""), true, true));
	PlatformProfiles.Add(MakeProfile(TEXT("Oculus_MetaQuest"), TEXT("Oculus"), ECySessionPlatformType::MetaQuest, ECySessionStoreType::MetaHorizon, TEXT(""), false, false));
	PlatformProfiles.Add(MakeProfile(TEXT("EOS_iOS_Apple"), TEXT("EOS"), ECySessionPlatformType::IOS, ECySessionStoreType::AppleAppStore, TEXT(""), true, true));
	PlatformProfiles.Add(MakeProfile(TEXT("IOS_GameCenter"), TEXT("IOS"), ECySessionPlatformType::IOS, ECySessionStoreType::AppleAppStore, TEXT(""), false, false, false));
}

const FCySessionPlatformProfile* UCySessionSettings::FindProfile(const FName ProfileName) const
{
	if (ProfileName.IsNone())
	{
		return nullptr;
	}
	return PlatformProfiles.FindByPredicate([ProfileName](const FCySessionPlatformProfile& Profile)
	{
		return Profile.bEnabled && Profile.ProfileName == ProfileName;
	});
}
