// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BRPlugins.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FBRPluginsModule"

void FBRPluginsModule::StartupModule()
{
	FString ShaderPath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("BRPlugins"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/BRPlugins"), ShaderPath);
}

void FBRPluginsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBRPluginsModule, BRPlugins)