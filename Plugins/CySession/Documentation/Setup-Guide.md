# Guide de configuration CySession

Ce guide configure la couche de sessions sans dépendre du plugin Redpoint/EOS placé dans le dossier d'exemple. CySession dépend uniquement des interfaces Online Subsystem publiques d'Unreal Engine.

## 1. Installer le plugin

Copier le dossier `CySession` dans `YourProject/Plugins/CySession`, puis activer :

- `CySession` ;
- `Online Subsystem` ;
- `Online Subsystem Utils` ;
- le provider choisi pour le build.

Le descriptor CySession ne déclare jamais EOS, Steam, Meta, Apple ou GOG comme dépendance obligatoire. Un package peut donc être compilé sans les SDK absents.

## 2. Choisir un profil

Ouvrir `Project Settings > Plugins > Cy Session` et renseigner `Active Profile Name`.

| Distribution | Profil conseillé | Provider de sessions |
|---|---|---|
| Développement local | `Local_NULL` | NULL |
| Epic Games Store / Windows | `EOS_Windows_Epic` | EOS Epic natif |
| Steam avec cross-play EOS | `EOS_Windows_Steam` | EOS Epic natif |
| Steam natif sessions uniquement | `Steam_Windows` | Steam |
| Steam natif avec services Blueprint | `Steam_Windows_Full` | Steam, sans EOS |
| GOG avec cross-play EOS | `EOS_Windows_GOG` | EOS Epic natif |
| GOG Galaxy natif | `GOG_Galaxy_Windows` | Plugin Galaxy tiers |
| Android / Google Play | `EOS_Android_GooglePlay` | EOS ou backend tiers |
| Meta Quest avec cross-play | `EOS_MetaQuest` | EOS Epic natif |
| Meta Quest natif | `Oculus_MetaQuest` | Plugin Meta/Oculus tiers |
| iPhone/iPad avec cross-play | `EOS_iOS_Apple` | EOS Epic natif |
| iPhone/iPad Game Center | `IOS_GameCenter` | OnlineSubsystemIOS |

Le profil peut être remplacé sans modifier les fichiers packagés :

```text
-CySessionProfile=EOS_Windows_GOG
```

Blueprint expose également `Set Active Profile Override`. Le changement est refusé pendant une opération asynchrone afin de ne pas perdre les callbacks du provider précédent.

## 3. Valider avant le menu multijoueur

Appeler `Validate Configuration` sur `CySessionSubsystem`.

Le rapport indique :

- le profil réellement sélectionné ;
- le provider, la plateforme et le store annoncés ;
- la présence de `IOnlineIdentity` ;
- la présence de `IOnlineSession` ;
- les erreurs bloquantes et avertissements.

La même validation est exécutée au démarrage lorsque `Validate Configuration On Startup` est activé. Les erreurs apparaissent dans la catégorie de log `LogCySession`.

`Get Provider Capabilities` peut sonder un provider facultatif par nom, par exemple `EOS`, `Steam`, `IOS`, `Oculus`, `Galaxy` ou `NULL`.

## 4. Initialiser les structs Blueprint

Utiliser `Make Default Credentials` au lieu de construire manuellement les credentials. Le type de login vient du profil actif ; aucun token n'est enregistré dans les `.ini`.

Utiliser `Make Default Create Options` pour récupérer :

- le nom de session par défaut ;
- le nombre de places ;
- les valeurs cross-play/cross-store ;
- la préférence Lobby du profil.

Les champs du struct peuvent ensuite être modifiés avant `Login` ou `Create Session`.

## 5. Fichiers de configuration

Des modèles copiables sont fournis dans [Documentation/Config](Config) :

- `Local-NULL.ini.example` ;
- `Windows-EOS.ini.example` ;
- `Windows-Steam.ini.example` ;
- `Windows-Steam-Full.ini.example` ;
- `Windows-GOG.ini.example` ;
- `Android-GooglePlay.ini.example` ;
- `Android-MetaQuest.ini.example` ;
- `IOS-Apple.ini.example`.

Les réglages `CySessionSettings` appartiennent à `DefaultGame.ini`. Les réglages du provider et du NetDriver appartiennent généralement à `DefaultEngine.ini`.

Pour une surcharge par plateforme Unreal :

- `Config/Windows/WindowsGame.ini` ;
- `Config/Android/AndroidGame.ini` ;
- `Config/IOS/IOSGame.ini`.

Deux stores utilisant la même cible, comme Google Play et Meta Quest sous Android, doivent utiliser des configs/targets de build distincts ou l'argument `-CySessionProfile`.

## 6. Plugins par cible

### EOS Epic natif

Activer `OnlineSubsystemEOS`. Suivre [EOS-Setup.md](EOS-Setup.md). Le plugin Redpoint d'exemple n'est pas nécessaire.

### Steam

Activer `OnlineSubsystemSteam`. Pour un jeu Steam autonome qui ne charge pas EOS, sélectionner `Steam_Windows_Full` et suivre [Steam-Full-Setup.md](Steam-Full-Setup.md). `CySteamSubsystem` expose en Blueprint les amis, la présence, les achievements, les leaderboards, Steam Cloud, l'overlay et la voix. Pour Steam vers GOG/console/mobile, sélectionner au contraire un profil EOS commun.

### Apple

Activer `OnlineSubsystemIOS` pour Game Center et ses sessions. `OnlineSubsystemApple` concerne Sign in with Apple mais ne fournit pas de sessions UE 5.5.

### Android

`OnlineSubsystemGooglePlay` fournit l'identité, achievements, leaderboards et achats, mais pas `IOnlineSession` dans UE 5.5. Les sessions doivent venir d'EOS ou d'un provider tiers.

### Meta

Ajouter le SDK/plugin Meta autorisé pour le projet si l'identité Meta est requise. CySession détecte les providers `Oculus`/`Meta` mais ne redistribue pas leur SDK.

### GOG

Ajouter un Online Subsystem Galaxy tiers si les sessions Galaxy natives sont souhaitées. Sinon, utiliser EOS comme backend et le profil `EOS_Windows_GOG`.

## 7. Cross-play et cross-store

Une session annonce séparément :

- `CY_PLATFORM` : Windows, Android, iOS, MetaQuest, etc. ;
- `CY_STORE` : Steam, GOG, AppleAppStore, GooglePlay, MetaHorizon, etc. ;
- `CY_PROVIDER` : EOS, Steam, IOS, Galaxy, etc. ;
- `CY_ALLOWCROSSPLAY` ;
- `CY_ALLOWCROSSSTORE` ;
- `CY_MOBILE`.

Le client respecte toujours les interdictions de l'hôte. Les politiques de recherche peuvent ensuite exiger la même plateforme, le même store, les deux ou une session cross-play.

## 8. Checklist de livraison

- `Validate Configuration` ne retourne aucune erreur.
- Deux comptes distincts peuvent se connecter.
- La session est trouvée entre deux machines/processus.
- Le `ClientTravel` rejoint réellement la map.
- Les tags plateforme/store correspondent au build distribué.
- Le provider et le NetDriver sont inclus dans Shipping.
- Les règles backend/certification reproduisent les restrictions importantes côté serveur.
- iOS est recompilé sur Mac avec Xcode, provisioning et entitlements.
