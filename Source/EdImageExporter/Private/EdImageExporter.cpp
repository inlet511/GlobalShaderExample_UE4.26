// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "EdImageExporter.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include <ShaderCore.h>

#define LOCTEXT_NAMESPACE "FEdImageExporterModule"

void FEdImageExporterModule::StartupModule()
{

}

void FEdImageExporterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEdImageExporterModule, EdImageExporter)