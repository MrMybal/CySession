# Mode Steam natif complet

Ce mode utilise `OnlineSubsystemSteam` pour l'identité, les lobbies/sessions et le transport réseau. EOS, EOSPlus et le plugin EOS d'exemple ne sont ni chargés ni requis.

Tous les clients d'une partie Steam native doivent utiliser Steam. Ce mode ne permet pas à un client GOG, Epic, console ou mobile de rejoindre directement le lobby Steam ; un backend commun comme EOS reste nécessaire pour ce type de cross-play.

## Activation

1. Activer `Online Subsystem Steam` dans le projet Unreal.
2. Copier les sections de [`Config/Windows-Steam-Full.ini.example`](Config/Windows-Steam-Full.ini.example) dans les fichiers `DefaultEngine.ini` et `DefaultGame.ini` du jeu.
3. Remplacer l'App ID de test `480` par l'App ID Steamworks du jeu.
4. Dans `Project Settings > Plugins > Cy Session`, sélectionner `Steam_Windows_Full`.
5. Au démarrage du menu multijoueur, appeler `Validate Steam Configuration` sur `CySteamSubsystem`.

Entrée attendue dans le `.uproject` :

```json
{
  "Name": "OnlineSubsystemSteam",
  "Enabled": true
}
```

Le Blueprint `Activate Full Steam Mode` sélectionne le profil pendant l'exécution, mais il ne peut pas remplacer le NetDriver après le démarrage du moteur. Le `.ini` reste donc obligatoire.

## Blueprints

Deux Game Instance Subsystems sont disponibles :

- `CySessionSubsystem` : login, lobby/session, recherche, invitation, join et travel ;
- `CySteamSubsystem` : validation Steam, amis, rich presence, achievements, leaderboards, Steam Cloud, overlay et voix.

Le flux minimal est :

1. `Get Game Instance Subsystem -> CySessionSubsystem` ;
2. `Auto Login` puis attendre `On Login Complete` ;
3. créer/rechercher/rejoindre la session normalement ;
4. utiliser `CySteamSubsystem` pour les services sociaux Steam.

`Make Default Create Options` active automatiquement `Uses Stats` avec le profil `Steam_Windows_Full`, ce qui prépare la session aux écritures de leaderboard.

## Correspondance fonctions/événements

| Appel Blueprint | Résultat asynchrone |
|---|---|
| `Read Friends` | `On Friends Read Complete` |
| `Set Presence` | `On Presence Set Complete` |
| `Query Presence` | `On Presence Query Complete`, puis `Get Cached Presence` |
| `Query Achievements` | `On Achievements Read Complete` |
| `Write Achievement` | `On Achievement Write Complete` |
| lectures de leaderboard | `On Leaderboard Read Complete` |
| `Write Leaderboard Score` | `On Leaderboard Write Complete` |
| `Enumerate Cloud Files` | `On Cloud Files Enumerated` |
| `Read Cloud File` | `On Cloud File Read Complete` |
| `Write Cloud File` | `On Cloud File Write Complete` |
| `Delete Cloud File` | `On Cloud File Delete Complete` |

La valeur booléenne retournée par un appel asynchrone indique uniquement si l'opération a pu démarrer. Le succès final vient toujours de l'événement associé.

## Services exposés

- liste d'amis et présence associée ;
- lecture et écriture de rich presence ;
- lecture des achievements et déverrouillage/progression ;
- lecture des classements autour du joueur, d'un rang ou des amis ;
- écriture de scores Steam ;
- énumération, lecture, écriture et suppression Steam Remote Storage ;
- overlay amis, invitations, achievements, profil, store et Web ;
- enregistrement du talker, push-to-talk et mute local des joueurs distants.
- récupération du token d'authentification Steam destiné au backend du jeu.

Avec l'implémentation Steam d'Unreal 5.5, `Write Achievement` déverrouille le succès dès que la valeur est supérieure à zéro. Une progression statistique détaillée doit être écrite via un leaderboard/stat Steam configuré.

## Limites réelles d'OnlineSubsystemSteam UE 5.5

CySession expose tout le socle Steam fourni publiquement par l'Online Subsystem. Quelques services Steamworks ne sont pas implémentés par ce provider Unreal : Workshop/UGC, Inventory Service, parties persistantes et chat de groupes. Ils demandent un module Steamworks direct séparé.

L'interface d'achat UE 5.5 existe, mais le checkout Steam nécessite un lien serveur sécurisé fourni par le jeu. Elle n'est donc pas présentée comme prête pour la production par CySession. L'overlay store fonctionne, et les licences/DLC doivent être validées avec le backend ou l'API Steam appropriée.

## Tests

- lancer Steam avant l'exécutable ;
- utiliser deux comptes Steam et deux machines pour tester lobby/invitations ;
- vérifier que l'overlay est activé pour le jeu ;
- ne pas tester deux clients avec le même compte ;
- confirmer que `Validate Steam Configuration` ne retourne aucune erreur ;
- tester un build Shipping lancé depuis Steam avant livraison.
