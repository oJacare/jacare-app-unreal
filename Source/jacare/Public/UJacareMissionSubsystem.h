// Copyright 2026, Lisboon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UJacareMissionSubsystem.generated.h"

/**
 * Jacare plugin settings exposed in Project Settings → Plugins → Jacare.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Jacare"))
class JACARE_API UJacareSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Backend", meta = (Tooltip = "Base URL of the Jacare NestJS backend. No trailing slash."))
	FString BackendUrl = TEXT("http://localhost:3001");

	UPROPERTY(Config, EditAnywhere, Category = "Backend", meta = (Tooltip = "M2M API key from the Jacare web editor. Keep this secret — do not commit to source control."))
	FString EngineApiKey;
};

USTRUCT(BlueprintType)
struct FJacareTargetActor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString class_path;

	UPROPERTY(BlueprintReadOnly)
	FVector spawn_location = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FJacareMissionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString mission_id;

	UPROPERTY(BlueprintReadOnly)
	FJacareTargetActor target_actor;
};

UCLASS()
class JACARE_API UJacareMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/**
	 * Fetches the active version of the given mission from the Jacare backend
	 * and spawns the described actor in the world. The operation is fully async
	 * and does not block the Game Thread.
	 *
	 * @param MissionId  The mission ID as configured in the Jacare web editor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Jacare")
	void LoadAndSpawnMission(const FString& MissionId);

private:
	void OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnTargetLoaded(FJacareMissionData MissionData);
};
