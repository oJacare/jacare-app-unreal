# jacare-app-unreal

UE5 C++ Plugin · HttpModule · FJsonObjectConverter · FStreamableManager. The runtime executor of Jacare Flow.

## Mandatory Rule: Keep Skills Up to Date

**After ANY interaction with this project** — reading code, adding handlers, creating UStructs, fixing bugs, or answering questions — **you MUST update the corresponding skill** in `.claude/skills/`.

This includes:
- **New Handler (INodeHandler)** → update `/jacare-runtime`
- **New USTRUCT or field** → update `/jacare-runtime` AND `/new-jacare-mission-type`
- **New UE5 interface created** → update `/jacare-runtime`
- **HTTP or spawn flow change** → update `/jacare-runtime`
- **New dependency in Build.cs** → update `/jacare-runtime`

**Skills are the living source of truth. Stale skills corrupt future interactions.**

---

## Context

**Jacare Flow** is an open-source visual state machine engine for UE5. This plugin (Jacare Runtime) is the executor: it fetches the compiled mission JSON from the backend via a secure HTTP M2M call, parses it, and executes the state machine node by node without blocking the Game Thread.

Current phase: **Phase 1 complete** (`Spawn.Actor` works end-to-end). Next: Handler Registry + Objective Handlers (Phase 4).

---

## Architecture

```
Source/jacare/
├── Public/
│   ├── jacare.h                    # DECLARE_LOG_CATEGORY_EXTERN(LogJacare)
│   └── UJacareMissionSubsystem.h   # Settings, UStructs, public Subsystem API
└── Private/
    ├── jacare.cpp                  # DEFINE_LOG_CATEGORY(LogJacare) + module startup
    └── UJacareMissionSubsystem.cpp # Subsystem implementation
```

### Key classes

**`UJacareSettings : UDeveloperSettings`**
- Exposed at: Project Settings → Plugins → Jacare
- `BackendUrl` — backend base URL, no trailing slash
- `EngineApiKey` — M2M API key from the Canvas editor. **Never commit this value.**

**`UJacareMissionSubsystem : UGameInstanceSubsystem`**
- One instance per game session
- Blueprint-callable: `LoadAndSpawnMission(const FString& MissionId)`
- Fully async — the Game Thread is never blocked

### Execution flow (Phase 1)

```
Blueprint → LoadAndSpawnMission(MissionId)
  → GET /missions/engine/{id}/active  [x-api-key header]
  → OnHttpResponse: parse JSON → FJacareMissionData via FJsonObjectConverter
  → RequestAsyncLoad(FSoftClassPath) via FStreamableManager
  → OnTargetLoaded: TSoftClassPtr::Get() → IsChildOf(AActor) → SpawnActor
```

---

## Non-Negotiable Rules

### Never block the Game Thread
```cpp
// FORBIDDEN
UClass* C = StaticLoadObject(...);       // Blocks the thread
UClass* C = LoadObject<UClass>(...);     // Blocks the thread
UObject* O = SoftPath.ResolveObject();  // Deprecated + async-unsafe

// CORRECT
UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftPath, Delegate);
UClass* C = TSoftClassPtr<AActor>(Path).Get(); // Inside the delegate only
```

### Guard every async callback
Any method not running immediately (HTTP response, streamable delegate) must start with:
```cpp
if (!IsValid(this)) return;
UGameInstance* GI = GetGameInstance();
if (!IsValid(GI)) return;
UWorld* World = GI->GetWorld();
if (!IsValid(World)) return;
```

### Always use LogJacare — never LogTemp
```cpp
UE_LOG(LogJacare, Log,     TEXT("..."));
UE_LOG(LogJacare, Warning, TEXT("..."));
UE_LOG(LogJacare, Error,   TEXT("..."));
```

### USTRUCT field order (avoid padding waste)
```cpp
USTRUCT(BlueprintType)
struct FJacareSomething
{
    GENERATED_BODY()

    // Order by size, largest first:
    // 1. FVector, FRotator, TArray, FString (8-16 bytes)
    // 2. float, int32, uint32 (4 bytes)
    // 3. bool, uint8 (1 byte) — always last

    UPROPERTY(BlueprintReadOnly, meta=(Tooltip="..."))
    FVector SpawnLocation = FVector::ZeroVector;  // PascalCase required

    UPROPERTY(BlueprintReadOnly, meta=(Tooltip="..."))
    int32 Count = 0;
};
```

### PascalCase in C++ ↔ snake_case in JSON
`FJsonObjectConverter` maps automatically. `SpawnLocation` ↔ `"spawn_location"`.
Never use snake_case in C++ field names.

### Zero EventTick
No Handler may use `Tick`. All execution is event-driven via Delegates.

---

## Current UStructs (Phase 1)

```cpp
USTRUCT(BlueprintType) struct FJacareTargetActor {
    FVector SpawnLocation;  // JSON: "spawn_location"
    FString ClassPath;      // JSON: "class_path"
};

USTRUCT(BlueprintType) struct FJacareMissionData {
    FString MissionId;              // JSON: "mission_id"
    FJacareTargetActor TargetActor; // JSON: "target_actor"
};
```

These structs are Phase 1. They will be expanded in Phase 4 to represent the full node graph.

---

## Build.cs Dependencies

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine",
    "Json", "JsonUtilities",  // JSON contract parsing
    "HTTP",                    // Backend requests
    "DeveloperSettings"        // UJacareSettings
});
```

---

## M2M Authentication

All requests include the `x-api-key` header read from `UJacareSettings::EngineApiKey`. The backend hashes the received key and compares it against the stored SHA-256 — the plaintext is never persisted on the server.

`DefaultGame.ini` stores plugin settings and **must not be committed** if it contains the API key. Add it to `.gitignore`.

---

## What is implemented (Phase 1)

- `LoadAndSpawnMission()` — async HTTP GET + JSON parse + Soft Reference spawn
- `UJacareSettings` — configuration via Project Settings UI
- `LogJacare` — dedicated log category
- `IsValid` guards on all async callbacks
- `IsChildOf(AActor)` validation before spawn

## Next steps (Phase 4)

- `INodeHandler` — base interface for node execution
- Handler Registry — `TMap<FGameplayTag, TUniquePtr<INodeHandler>>`
- Handlers: `Objective.Kill`, `Objective.Goto`, `Objective.Collect`, `Objective.Interact`
- Handlers: `Audio.Play`, `Reward.Give`, `Flag.Set`, `Flow.Wait`, `Flow.Branch`
- `ReportObjective(MissionId, NodeId)` — advances the in-memory graph
- `FOnMissionSpawned` delegate — Blueprint reacts to success or failure

---

## Setup

1. Copy this repo into `{YourProject}/Plugins/jacare/`
2. Right-click `.uproject` → **Generate Visual Studio project files**
3. Build the project
4. **Project Settings → Plugins → Jacare** → set `BackendUrl` and `EngineApiKey`

---

## Skills (Slash Commands)

| Skill | When to use |
|-------|-------------|
| `/review` | Full architecture review before a PR |
| `/commit` | Generate a Conventional Commits message |
| `/new-jacare-mission-type` | Cross-ecosystem scaffold for a new Node Type (includes C++ steps) |

---

## Rules (Contextual)

Path-specific rules to create in `.claude/rules/`:
- `ue5-conventions.md` — C++/UE5 rules (`Source/**/*.cpp`, `Source/**/*.h`)
- `handlers.md` — INodeHandler implementation pattern (`Source/**/Handlers/**`)
