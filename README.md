# CySession

Unreal Engine Blueprint plugin for multiplayer sessions, authentication and optional native Steam services.

The repository contains the `CySession` runtime plugin (version `1.1.0`) and an Unreal Engine 5.5 host project.

## Capabilities

- Blueprint-accessible Game Instance subsystem for login, logout and identity information.
- Create, update, start, end, destroy, find and join multiplayer sessions.
- Session invitations, participant events and travel to a resolved session connection.
- Provider and platform profiles, configuration validation and runtime overrides.
- Optional Steam integration through `CySteamSubsystem`.

## Repository layout

- `Plugins/CySession/`: plugin source and descriptor.
- `Source/`: host project source.
- `Config/`: project configuration.
- `CySession_Plugin.uproject`: Unreal Engine 5.5 host project.

## Getting started

Open `CySession_Plugin.uproject` with Unreal Engine 5.5 and build the C++ project with the matching toolchain.
In Blueprint, use **Get Game Instance Subsystem → Cy Session Subsystem**.
Configure the appropriate online provider for your project; service availability depends on the provider and its setup.
