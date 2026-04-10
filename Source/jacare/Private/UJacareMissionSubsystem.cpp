// Copyright 2026, oJacare. All Rights Reserved.

#include "UJacareMissionSubsystem.h"
#include "jacare.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void UJacareMissionSubsystem::LoadAndSpawnMission(const FString& MissionId)
{
	if (MissionId.IsEmpty())
	{
		UE_LOG(LogJacare, Error, TEXT("LoadAndSpawnMission: MissionId cannot be empty."));
		return;
	}

	const UJacareSettings* Settings = GetDefault<UJacareSettings>();

	if (Settings->EngineApiKey.IsEmpty())
	{
		UE_LOG(LogJacare, Error, TEXT("LoadAndSpawnMission: EngineApiKey is not configured. Set it in Project Settings → Plugins → Jacare."));
		return;
	}

	const FString Url = FString::Printf(TEXT("%s/missions/engine/%s/active"), *Settings->BackendUrl, *MissionId);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("x-api-key"), Settings->EngineApiKey);
	Request->SetTimeout(10.f);
	Request->OnProcessRequestComplete().BindUObject(this, &UJacareMissionSubsystem::OnHttpResponse);
	Request->ProcessRequest();

	UE_LOG(LogJacare, Log, TEXT("Fetching mission '%s' from %s"), *MissionId, *Url);
}

void UJacareMissionSubsystem::OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!IsValid(this)) return;

	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogJacare, Error, TEXT("HTTP request failed — could not reach backend."));
		OnMissionFailed.Broadcast(TEXT(""), TEXT("HTTP request failed")); 
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	if (StatusCode != 200)
	{
		UE_LOG(LogJacare, Error, TEXT("Backend returned HTTP %d. Body: %s"), StatusCode, *Response->GetContentAsString());
		OnMissionFailed.Broadcast(TEXT(""), FString::Printf(TEXT("HTTP %d"), StatusCode));
		return;
	}

	FJacareMissionData MissionData;
	const bool bParsed = FJsonObjectConverter::JsonObjectStringToUStruct(Response->GetContentAsString(), &MissionData, 0, 0);

	if (!bParsed)
	{
		UE_LOG(LogJacare, Error, TEXT("Failed to parse mission JSON. Raw response: %s"), *Response->GetContentAsString());
		OnMissionFailed.Broadcast(TEXT(""), TEXT("Failed to parse mission JSON"));
		return;
	}

	UE_LOG(LogJacare, Log, TEXT("Mission '%s' received. Async-loading actor class: %s"), *MissionData.MissionId, *MissionData.TargetActor.ClassPath);

	const FSoftClassPath SoftPath(MissionData.TargetActor.ClassPath);
	UAssetManager::GetStreamableManager().RequestAsyncLoad(
		SoftPath,
		FStreamableDelegate::CreateUObject(this, &UJacareMissionSubsystem::OnTargetLoaded, MissionData)
	);
}

void UJacareMissionSubsystem::OnTargetLoaded(FJacareMissionData MissionData)
{
	if (!IsValid(this))
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!IsValid(GI))
	{
		UE_LOG(LogJacare, Error, TEXT("OnTargetLoaded: GameInstance is no longer valid."));
		return;
	}

	UWorld* World = GI->GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogJacare, Error, TEXT("OnTargetLoaded: World is no longer valid."));
		return;
	}

	const TSoftClassPtr<AActor> SoftClass(MissionData.TargetActor.ClassPath);
	UClass* LoadedClass = SoftClass.Get();
	if (!LoadedClass)
	{
		UE_LOG(LogJacare, Error, TEXT("OnTargetLoaded: Failed to resolve class '%s'. Check that the path is correct and the asset is cooked."), *MissionData.TargetActor.ClassPath);
		return;
	}

	if (!LoadedClass->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(LogJacare, Error, TEXT("OnTargetLoaded: Class '%s' is not an AActor subclass. Spawn aborted."), *MissionData.TargetActor.ClassPath);
		return;
	}

	AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClass, MissionData.TargetActor.SpawnLocation, FRotator::ZeroRotator);
	if (SpawnedActor)
	{
		UE_LOG(LogJacare, Log, TEXT("Actor '%s' spawned for mission '%s'."), *LoadedClass->GetName(), *MissionData.MissionId);
		OnMissionSpawned.Broadcast(MissionData.MissionId, SpawnedActor);
	}
	else
	{
		UE_LOG(LogJacare, Warning, TEXT("SpawnActor returned null for mission '%s'."), *MissionData.MissionId);
		OnMissionFailed.Broadcast(MissionData.MissionId, TEXT("SpawnActor returned null"));
	}
}
