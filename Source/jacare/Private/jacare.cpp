// Copyright Epic Games, Inc. All Rights Reserved.

#include "jacare.h"

DEFINE_LOG_CATEGORY(LogJacare);

#define LOCTEXT_NAMESPACE "FjacareModule"

void FjacareModule::StartupModule()
{
	// This code will execute after your module is loaded into memory...
}

void FjacareModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module...
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FjacareModule, jacare)