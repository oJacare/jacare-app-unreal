// Copyright 2026, Lisboon. All Rights Reserved.

#include "UJacareMissionSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void UUJacareMissionSubsystem::LoadAndSpawnMission()
{
	const UJacareSettings* Settings = GetDefault<UJacareSettings>();

	const FString MissionId = TEXT("qst_old_country");
	const FString Url = FString::Printf(TEXT("%s/missions/engine/%s/active"), *Settings->BackendUrl, *MissionId);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("x-api-key"), Settings->EngineApiKey);
	Request->OnProcessRequestComplete().BindUObject(this, &UUJacareMissionSubsystem::OnHttpResponse);
	Request->ProcessRequest();

	UE_LOG(LogTemp, Warning, TEXT("Jacare: Buscando missão %s no backend..."), *MissionId);
}

void UUJacareMissionSubsystem::OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Jacare: Falha ao conectar no backend!"));
		return;
	}

	FString JsonString = Response->GetContentAsString();
	UE_LOG(LogTemp, Warning, TEXT("Jacare: JSON recebido: %s"), *JsonString);

	FJacareMissionData MissionData;
	bool bParsed = FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &MissionData, 0, 0);

	if (bParsed)
	{
		UE_LOG(LogTemp, Warning, TEXT("Jacare: Missão %s carregada! Preparando spawn..."), *MissionData.mission_id);

		FSoftClassPath SoftPath(MissionData.target_actor.class_path);
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SoftPath,
			FStreamableDelegate::CreateUObject(this, &UUJacareMissionSubsystem::OnTargetLoaded, MissionData)
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Jacare: Falha ao parsear o JSON!"));
	}
}

void UUJacareMissionSubsystem::OnTargetLoaded(FJacareMissionData MissionData)
{
	FSoftClassPath SoftPath(MissionData.target_actor.class_path);
	UClass* LoadedClass = Cast<UClass>(SoftPath.ResolveObject());

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	
	if (LoadedClass && World)
	{
		FRotator SpawnRotation = FRotator::ZeroRotator;
		World->SpawnActor<AActor>(LoadedClass, MissionData.target_actor.spawn_location, SpawnRotation);
		UE_LOG(LogTemp, Warning, TEXT("Lisboon: MAGIA! Ator spawnado via JSON!"));
	}
}