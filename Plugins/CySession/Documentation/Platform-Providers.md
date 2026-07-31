# Providers Meta, Apple, GOG et mobile

CySession sépare volontairement :

- le **provider réseau**, qui fournit `IOnlineIdentity` et `IOnlineSession` ;
- la **plateforme matérielle**, par exemple Windows, iOS, Android ou Meta Quest ;
- le **store**, par exemple GOG, Apple App Store, Google Play ou Meta Horizon.

Un jeu GOG sous Windows et un jeu Steam sous Windows partagent la même plateforme, mais pas le même store. Un build Meta Quest est techniquement Android, mais reçoit le type de plateforme `MetaQuest` afin que les règles cross-play restent explicites.

## Réglages communs

Dans `Project Settings > Plugins > Cy Session`, ou dans `DefaultGame.ini` :

```ini
[/Script/CySession.CySessionSettings]
SubsystemName=EOS
PlatformTypeOverride=Auto
StoreTypeOverride=Auto
PlatformTagOverride=
StoreTagOverride=
DefaultSessionName=GameSession
```

Les overrides typés couvrent les stores connus. Les tags texte permettent d'intégrer immédiatement un store ou un provider futur sans recompiler CySession.

`Get Provider Capabilities` permet de vérifier depuis Blueprint si un provider optionnel est installé et s'il expose réellement l'identité et les sessions.

## Meta Quest

Meta Quest est une cible Android. Le plugin Meta Platform/Oculus n'est pas livré dans toutes les installations Unreal et CySession n'en dépend pas.

Deux architectures sont possibles :

1. un Online Subsystem Meta/Oculus tiers fournit directement `IOnlineIdentity` et `IOnlineSession` ; sélectionner son nom via `Subsystem Name` ;
2. le SDK Meta fournit l'identité native et `OnlineSubsystemEOS` fournit les sessions cross-play communes.

Profil de build recommandé avec EOS :

```ini
[/Script/CySession.CySessionSettings]
SubsystemName=EOS
PlatformTypeOverride=MetaQuest
StoreTypeOverride=MetaHorizon
```

Si le provider actif s'appelle `Oculus` ou `Meta`, le store Meta est détecté automatiquement. Avec EOS, l'override reste nécessaire, car EOS est un backend et non un store.

## Apple, iPhone et iPad

UE 5.5 contient deux plugins distincts :

- `OnlineSubsystemIOS` fournit Game Center, y compris une interface de sessions iOS ;
- `OnlineSubsystemApple` fournit Sign in with Apple, mais pas d'interface de sessions.

Pour des sessions limitées à Game Center :

```ini
[OnlineSubsystem]
DefaultPlatformService=IOS

[/Script/CySession.CySessionSettings]
SubsystemName=IOS
PlatformTypeOverride=IOS
StoreTypeOverride=AppleAppStore
```

Pour jouer avec Android, PC, Meta ou consoles, utiliser un backend commun tel qu'EOS :

```ini
[/Script/CySession.CySessionSettings]
SubsystemName=EOS
PlatformTypeOverride=IOS
StoreTypeOverride=AppleAppStore
```

Le projet iOS doit toujours activer les entitlements Game Center/Sign in with Apple appropriés et être signé par l'équipe Apple du jeu.

## Android et Google Play

Dans UE 5.5, `OnlineSubsystemGooglePlay` fournit login, achievements, leaderboards et achats, mais retourne une interface de sessions nulle. Il ne peut donc pas être le provider de sessions de CySession.

Utiliser EOS ou un backend tiers pour les sessions :

```ini
[/Script/CySession.CySessionSettings]
SubsystemName=EOS
PlatformTypeOverride=Android
StoreTypeOverride=GooglePlay
```

Le provider EOS peut ensuite utiliser l'identité native configurée pour créer le Product User ID. Les réglages Google Play, OAuth, signatures et App ID restent propres au projet Android.

## GOG Galaxy

UE 5.5 ne livre pas d'Online Subsystem GOG/Galaxy dans l'installation auditée. CySession accepte néanmoins :

- un plugin tiers qui enregistre un Online Subsystem nommé `GOG` ou `Galaxy` ;
- EOS comme backend commun, avec Galaxy uniquement pour l'identité/store ;
- tout provider custom implémentant les interfaces Unreal standard.

Profil GOG avec EOS :

```ini
[/Script/CySession.CySessionSettings]
SubsystemName=EOS
PlatformTypeOverride=Windows
StoreTypeOverride=GOG
```

Le SDK Galaxy et ses identifiants ne sont pas copiés dans CySession. Ils doivent être fournis et autorisés par le projet distribué sur GOG.

## Amazon et autres stores Android

`AmazonAppstore` est disponible comme type de store. Pour Huawei, Samsung ou un futur store, utiliser `StoreTypeOverride=Other` et un `StoreTagOverride` stable :

```ini
[/Script/CySession.CySessionSettings]
SubsystemName=EOS
PlatformTypeOverride=Android
StoreTypeOverride=Other
StoreTagOverride=HuaweiAppGallery
```

## Règles de recherche

`CySessionFindOptions` propose :

- `Any` : respecte les interdictions annoncées par l'hôte, mais accepte le cross-play/cross-store autorisé ;
- `Crossplay Enabled Only` ;
- `Same Platform Only` ;
- `Same Store Only` ;
- `Same Platform And Store Only` ;
- `Required Platform Tag` et `Required Store Tag` pour un filtrage custom exact.

Ces règles sont vérifiées côté client même si le backend ignore un filtre. Une restriction de sécurité, de certification ou de licence doit également être appliquée côté backend/provider.
