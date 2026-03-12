// Copyright 2026, Lisboon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UJacareMissionSubsystem.generated.h"

/**
 * Configurações do Jacare Flow expostas no Project Settings.
 * Acesse em: Project Settings → Plugins → Jacare Flow
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Jacare Flow"))
class JACARE_API UJacareSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** URL base do Jacare Maestro (backend NestJS) */
	UPROPERTY(Config, EditAnywhere, Category = "Backend")
	FString BackendUrl = TEXT("http://localhost:3000");

	/** API Key gerada no painel web (POST /api-keys). Formato: jcr_live_<hex> */
	UPROPERTY(Config, EditAnywhere, Category = "Backend")
	FString EngineApiKey;
};

USTRUCT(BlueprintType)
struct FJacareTargetActor
{
	GENERATED_BODY()

	UPROPERTY()
	FString class_path;

	UPROPERTY()
	FVector spawn_location;
};

USTRUCT(BlueprintType)
struct FJacareMissionData
{
	GENERATED_BODY()

	UPROPERTY()
	FString mission_id;

	UPROPERTY()
	FJacareTargetActor target_actor;
};

UCLASS()
class JACARE_API UUJacareMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, category = "Jacare")
	void LoadAndSpawnMission();

private:
	void OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnTargetLoaded(FJacareMissionData MissionData);
};
