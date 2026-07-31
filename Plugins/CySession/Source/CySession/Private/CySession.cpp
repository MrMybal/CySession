// Copyright Epic Games, Inc. All Rights Reserved.

#include "CySession.h"

DEFINE_LOG_CATEGORY(LogCySession);

#define LOCTEXT_NAMESPACE "FCySessionModule"

void FCySessionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FCySessionModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCySessionModule, CySession)
