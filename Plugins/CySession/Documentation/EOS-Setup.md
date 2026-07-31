# Configuration EOS pour CySession (Unreal Engine 5.5)

CySession orchestre les sessions, mais les comptes EOS, les Product User IDs, le backend et les sockets P2P sont fournis par le plugin Epic `OnlineSubsystemEOS` livré avec Unreal Engine.

Le plugin Redpoint/EOS présent dans le dossier d'exemple n'est jamais requis, lié ou copié par CySession. Il peut être retiré entièrement du projet final.

## 1. Activer les plugins

Activer au minimum dans le projet :

- `CySession` ;
- `Online Subsystem` ;
- `Online Subsystem Utils` ;
- `Online Subsystem EOS`.

`OnlineSubsystemEOS` déclare lui-même ses dépendances `EOSShared`, `EOSVoiceChat`, `SocketSubsystemEOS` et `EOSOverlayInputProvider`.

## 2. Renseigner l'artifact EOS

Créer le produit, le sandbox, le deployment et le client dans l'Epic Developer Portal. Dans Unreal, ouvrir `Project Settings > Plugins > EOS` puis renseigner l'artifact et les options de login.

Équivalent indicatif dans `DefaultEngine.ini` :

```ini
[OnlineSubsystem]
DefaultPlatformService=EOS

[/Script/OnlineSubsystemEOS.EOSSettings]
DefaultArtifactName=YourArtifact
bUseEAS=True
bUseEOSConnect=True
bUseNewLoginFlow=True
bEnableSocialOverlay=True
bEnableEditorOverlay=True
+Artifacts=(ArtifactName="YourArtifact",ClientId="...",ClientSecret="...",ProductId="...",SandboxId="...",DeploymentId="...",ClientEncryptionKey="...")
```

`bUseEOSConnect=True` fournit les PUID utilisés par les Game Services et le cross-play. `bUseEAS=True` active l'identité Epic Account et la couche sociale. Pour un titre utilisant uniquement Connect avec l'identité native de la plateforme, adapter le type de login et les identity providers configurés dans le Developer Portal.

`bUseEOSSessions` concerne surtout l'ancien chemin EOSPlus qui miroir un Online Subsystem natif vers EOS. Avec EOS comme `DefaultPlatformService`, les sessions sont déjà créées directement dans EOS.

## 3. Transport réseau EOS P2P

Pour des parties peer-to-peer sans adresse IP publique, utiliser le NetDriver EOS :

```ini
[/Script/Engine.GameEngine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemEOS.NetDriverEOS",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[/Script/OnlineSubsystemEOS.NetDriverEOS]
bIsUsingP2PSockets=True
```

Pour un dedicated server adressable en IP, conserver/configurer l'`IpNetDriver` selon l'infrastructure du projet.

## 4. Blueprint

Pour un login interactif EOS, appeler `Login` avec :

- `Type = accountportal` ;
- `Id` et `Token` vides ;
- `Local User Num = 0`.

Le Developer Auth Tool utilise généralement `Type = developer`, `Id = localhost:port` et `Token = nom_du_credential`.

Après `On Login Complete`, les flux `Create Session`, `Find Sessions` et `Join Session` sont identiques à ceux du provider NULL ou Steam.

## 5. Tests réalistes

- Ne pas valider le multiplayer avec un unique process et un unique compte EOS.
- Lancer deux clients avec deux identités distinctes.
- Vérifier d'abord une session cross-play activée, puis une session désactivée et `Same Platform Only`.
- Pour tester deux stores PC comme plateformes distinctes, renseigner `Platform Tag Override` différemment par build/configuration.
- Pour Steam, GOG, Meta, Apple ou Google Play utilisant EOS comme backend commun, conserver `Subsystem Name = EOS` et définir le `Store Type Override` propre à chaque build.
- Vérifier le travel réel, car une création/recherche réussie ne garantit pas à elle seule la configuration du NetDriver ou du firewall.
