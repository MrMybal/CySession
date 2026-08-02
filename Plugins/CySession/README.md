# CySession

`CySession` est une couche multijoueur maison, entièrement accessible en Blueprint, au-dessus des interfaces `OnlineSubsystem` d'Unreal Engine 5.5.

Le plugin ne dépend pas de Redpoint, du plugin EOS fourni en exemple, ni d'un SDK de store. Le provider reste interchangeable : EOS Epic natif, Steam, iOS/Game Center, Meta/Oculus, GOG Galaxy, un provider console, ou NULL/LAN. Les providers optionnels sont détectés au runtime par leur interface Unreal ; `CySession` continue donc à compiler lorsqu'ils ne sont pas installés.

## Fonctionnalités

- login, auto-login, logout et suivi du statut d'identité ;
- création, mise à jour, démarrage, fin et destruction/leave d'une session ;
- recherche avec filtres de mode, région, attributs, places, ping et plateforme ;
- cross-play activé, cross-play uniquement, ou même plateforme uniquement ;
- séparation explicite entre provider réseau, plateforme matérielle et store ;
- compatibilité cross-store contrôlable (par exemple Steam vers GOG sur Windows) ;
- join par handle Blueprint sûr et `ClientTravel` automatique ;
- host travel automatique avec `listen` ;
- invitations reçues/acceptées et envoi d'invitation par identifiant provider ;
- événements de participants et perte du service ;
- attributs custom `TMap<Name, String>` annoncés et retournés aux Blueprints ;
- changement de provider par réglage projet ou override du Game Instance.

## Point d'entrée Blueprint

Dans n'importe quel Blueprint disposant d'un contexte de jeu :

1. utiliser `Get Game Instance Subsystem` ;
2. choisir `CySessionSubsystem` ;
3. binder les événements voulus une seule fois, généralement dans le Game Instance ou un manager UI ;
4. appeler les fonctions asynchrones ; la valeur booléenne indique si la requête a démarré et l'événement correspondant donne le résultat final.

Les erreurs synchrones et asynchrones sont disponibles dans la sortie `Error`, ainsi que via `Get Last Error`.

Le guide complet d'installation, de sélection des presets et de validation se trouve dans [Documentation/Setup-Guide.md](Documentation/Setup-Guide.md).

## Profils intégrés

`Project Settings > Plugins > Cy Session > Profiles` contient des presets modifiables :

- `Local_NULL` ;
- `EOS_Windows_Epic`, `EOS_Windows_Steam`, `EOS_Windows_GOG` ;
- `Steam_Windows`, `Steam_Windows_Full`, `GOG_Galaxy_Windows` ;
- `EOS_Android_GooglePlay`, `EOS_MetaQuest`, `Oculus_MetaQuest` ;
- `EOS_iOS_Apple`, `IOS_GameCenter`.

Sélectionner `Active Profile Name`, ou lancer le jeu avec `-CySessionProfile=EOS_MetaQuest`. Un override Blueprint est également disponible par Game Instance.

## Flux hôte

1. `Auto Login` ou `Login`.
2. Attendre `On Login Complete`.
3. Construire `CySessionCreateOptions`.
4. Appeler `Create Session`.
5. Attendre `On Create Session Complete`.

Pour ouvrir immédiatement un listen server, renseigner `Travel Map`, activer `Travel On Success`, et laisser `Dedicated` désactivé. `listen` est ajouté automatiquement.

## Flux client

1. Se connecter au provider.
2. Construire `CySessionFindOptions`, puis appeler `Find Sessions`.
3. Afficher le tableau reçu par `On Find Sessions Complete`.
4. Passer le `Result Handle` choisi à `Join Session` avec `CySessionJoinOptions`.
5. Si `Travel On Success` est actif, le plugin résout l'adresse provider et exécute le travel client.

Les handles ne sont valables que tant que le `CySessionSubsystem` du Game Instance conserve son cache. Une nouvelle recherche remplace le cache précédent.

## Politique cross-play

À la création, `Allow Crossplay` et `Allow Cross Store` publient les attributs `CY_ALLOWCROSSPLAY`, `CY_ALLOWCROSSSTORE`, `CY_PLATFORM`, `CY_STORE`, `CY_PROVIDER` et `CY_MOBILE`.

- `Any` accepte les sessions cross-play, plus les sessions non cross-play de la même plateforme.
- `Crossplay Enabled Only` exige une session ayant explicitement activé le cross-play.
- `Same Platform Only` exige le même tag plateforme.
- `Same Store Only` exige le même store, indépendamment du matériel.
- `Same Platform And Store Only` exige les deux.

Le filtrage est refait côté client pour être identique sur les providers. Il protège donc un écosystème dont tous les clients passent par `CySession`. Pour une interdiction de plateforme hostile ou réglementaire, appliquer également les politiques d'accès côté backend/provider.

Les tags viennent de la cible packagée et du provider actif. Ils peuvent être imposés par build dans `Project Settings > Plugins > Cy Session`, par exemple `MetaQuest + MetaHorizon`, `iOS + AppleAppStore`, `Android + GooglePlay`, ou `Windows + GOG`.

## Providers

Le provider par défaut vient de `[OnlineSubsystem] DefaultPlatformService`. Il peut être imposé dans `Project Settings > Plugins > Cy Session > Subsystem Name`, ou temporairement avec `Set Subsystem Override`.

- Pour un test local : activer `OnlineSubsystemNull` et utiliser `DefaultPlatformService=NULL`.
- Pour Meta, Apple, GOG et Android : suivre [Documentation/Platform-Providers.md](Documentation/Platform-Providers.md).
- Pour EOS/cross-play : suivre [Documentation/EOS-Setup.md](Documentation/EOS-Setup.md).
- Pour Steam autonome sans EOS : suivre [Documentation/Steam-Full-Setup.md](Documentation/Steam-Full-Setup.md).
- Pour les consoles : activer et configurer le plugin Online Subsystem du SDK concerné ; les mêmes Blueprints CySession restent valables.

## Fichiers principaux

- `CySessionSubsystem.h/.cpp` : orchestration asynchrone et événements Blueprint.
- `CySessionTypes.h` : options, résultats et enums Blueprint.
- `CySessionSettings.h` : sélection du provider et tag plateforme.
- `CySteamSubsystem.h/.cpp` : amis, présence, achievements, leaderboards, Steam Cloud, overlay et voix Steam en Blueprint.

## Limite de cette première couche

Le socle commun couvre l'identité et les sessions de jeu. Le mode `Steam_Windows_Full` ajoute les services Steam pris en charge par `OnlineSubsystemSteam` : amis, rich presence, achievements, leaderboards, Remote Storage, overlay et voix. Les fonctions Steamworks sans interface Unreal publique, comme Workshop/UGC et Inventory Service, demandent encore un module Steamworks direct séparé.

Les SDK Meta et GOG ne sont pas redistribuables et doivent être ajoutés par le projet final. Sur Android, `OnlineSubsystemGooglePlay` fournit l'identité mais pas les sessions dans UE 5.5 : utiliser EOS ou un autre backend de session.
