/****************************************************************
*
* Copyright 2022 � Leia Inc.
*
****************************************************************
*/

#include "LeiaCamera.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FLeiaCameraModule"

//Implimented in the pawn for access to some classes conveniently
#if 0

void FLeiaCameraModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("LeiaCamera"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/LeiaCamera"), PluginShaderDir);
}

void FLeiaCameraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLeiaCameraModule, LeiaCamera)

#endif