# jacare-app-unreal

UE5 C++ Plugin. Executor do Jacare Flow: consome JSON do backend via HTTP M2M e executa a state machine sem bloquear a Game Thread.

## REGRA OBRIGATORIA: Manter Skills Atualizadas

**Apos QUALQUER interacao com o projeto** — leitura de codigo, adicao de handlers, novos UStructs, correcao de bugs ou resposta a perguntas — **voce DEVE atualizar a skill correspondente** em `.claude/skills/`.

Isso inclui:
- **Novo Handler (INodeHandler)** → atualizar `/jacare-runtime`
- **Novo USTRUCT ou campo** → atualizar `/jacare-runtime` E `/new-jacare-mission-type`
- **Nova interface UE5 criada** → atualizar `/jacare-runtime`
- **Mudanca no fluxo HTTP ou de spawn** → atualizar `/jacare-runtime`
- **Nova dependencia em Build.cs** → atualizar `/jacare-runtime`

**As skills sao a fonte de verdade viva. Skills desatualizadas corrompem futuras interacoes.**

---

## Contexto

**Jacare Flow** e uma State Machine Engine open-source para UE5. Este plugin (Jacare Runtime) e o executor: faz HTTP GET no backend com API Key M2M, recebe o `missionData` JSON compilado e executa a state machine no a no sem bloquear a Game Thread.

Fase atual: **Fase 1 concluida** (Spawn.Actor funciona end-to-end). Proximo: Handler Registry + Objective Handlers (Fase 4).

---

## Arquitetura

```
Source/jacare/
├── Public/
│   ├── jacare.h                    # DECLARE_LOG_CATEGORY_EXTERN(LogJacare)
│   └── UJacareMissionSubsystem.h   # Settings, UStructs, Subsystem API publica
└── Private/
    ├── jacare.cpp                  # DEFINE_LOG_CATEGORY(LogJacare) + module startup
    └── UJacareMissionSubsystem.cpp # Implementacao do subsystem
```

### Classes principais

**`UJacareSettings : UDeveloperSettings`**
- Aparece em: Project Settings → Plugins → Jacare
- `BackendUrl` — URL base do backend (sem trailing slash)
- `EngineApiKey` — API Key M2M do Canvas. **Nunca commitar este valor**

**`UJacareMissionSubsystem : UGameInstanceSubsystem`**
- Uma instancia por sessao de jogo
- Blueprint-callable: `LoadAndSpawnMission(const FString& MissionId)`
- Toda execucao e assincrona — zero bloqueio na Game Thread

### Fluxo de execucao atual (Fase 1)

```
Blueprint → LoadAndSpawnMission(MissionId)
    → HTTP GET /missions/engine/{id}/active  [header: x-api-key]
    → OnHttpResponse: parse JSON → FJacareMissionData via FJsonObjectConverter
    → RequestAsyncLoad(FSoftClassPath)
    → OnTargetLoaded: TSoftClassPtr::Get() → IsChildOf(AActor) → SpawnActor
```

---

## Convencoes Inviolaveis

### Nunca bloquear a Game Thread
```cpp
// PROIBIDO
UClass* Cls = StaticLoadObject(...);      // Bloqueia thread
UClass* Cls = LoadObject<UClass>(...);    // Bloqueia thread
UObject* Obj = SoftPath.ResolveObject(); // Deprecated + unsafe

// CORRETO
UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftPath, Delegate);
// Resolve dentro do delegate, apos carregamento assincrono
UClass* Cls = TSoftClassPtr<AActor>(Path).Get();
```

### Guard obrigatorio em todo callback assincrono
Todo metodo que nao executa imediatamente (HTTP response, streamable delegate) comeca com:
```cpp
if (!IsValid(this)) return;
UGameInstance* GI = GetGameInstance();
if (!IsValid(GI)) return;
UWorld* World = GI->GetWorld();
if (!IsValid(World)) return;
```
O objeto pode ter sido destruido entre o disparo e a execucao do callback.

### Log category propria — nunca LogTemp
```cpp
UE_LOG(LogJacare, Log,     TEXT("..."));
UE_LOG(LogJacare, Warning, TEXT("..."));
UE_LOG(LogJacare, Error,   TEXT("..."));
// LogTemp e proibido neste plugin
```

### USTRUCT — naming e ordenacao
```cpp
USTRUCT(BlueprintType)
struct FJacareAlgumaCoisa
{
    GENERATED_BODY()

    // Regra de ordenacao (evita padding desnecessario):
    // 1. FVector, FRotator, TArray, FString (8-16 bytes)
    // 2. float, int32, uint32 (4 bytes)
    // 3. bool, uint8 (1 byte) — sempre por ultimo

    UPROPERTY(BlueprintReadOnly, meta=(Tooltip="..."))
    FVector SpawnLocation = FVector::ZeroVector;  // PascalCase obrigatorio

    UPROPERTY(BlueprintReadOnly, meta=(Tooltip="..."))
    FString ClassPath;

    UPROPERTY(BlueprintReadOnly, meta=(Tooltip="..."))
    int32 Count = 0;
};
```
`FJsonObjectConverter` mapeia `PascalCase` C++ ↔ `snake_case` JSON automaticamente.
`SpawnLocation` ↔ `"spawn_location"`. Nunca usar snake_case em C++.

### Soft Referencing — sempre
Assets chegam do JSON como `class_path` string. Sempre `TSoftClassPtr`, nunca hardcoded:
```cpp
const TSoftClassPtr<AActor> SoftClass(MissionData.TargetActor.ClassPath);
// Carregar via FStreamableManager, nunca StaticLoadObject
```

### Zero EventTick
Nenhum Handler pode usar `Tick`. Toda execucao e orientada a eventos e Delegates.

### Modulo HTTP — editor only
```cpp
// Se o modulo HTTP for isolado em builds de producao:
#if WITH_EDITOR
    // HTTP requests aqui
#endif
```
Em builds de distribuicao, missoes sao baked como `.json` em `Content/JacareFlow/`.

---

## UStructs Atuais (Fase 1)

```cpp
USTRUCT(BlueprintType) struct FJacareTargetActor {
    FVector SpawnLocation;   // JSON: "spawn_location"
    FString ClassPath;       // JSON: "class_path"
};

USTRUCT(BlueprintType) struct FJacareMissionData {
    FString MissionId;           // JSON: "mission_id"
    FJacareTargetActor TargetActor;  // JSON: "target_actor"
};
```

Estes structs sao Fase 1. Na Fase 4 serao expandidos para o grafo completo de nos.

---

## Build.cs — Dependencias

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine",
    "Json", "JsonUtilities",   // Parse do contrato JSON
    "HTTP",                     // Requisicoes ao backend
    "DeveloperSettings"         // UJacareSettings
});
```

---

## Auth M2M

Header `x-api-key` em todas as requisicoes. Lido de `UJacareSettings::EngineApiKey`.
O backend hasha o key recebido e compara — o texto puro nunca e persistido no servidor.

**Atencao**: `DefaultGame.ini` onde o Unreal persiste as configuracoes **nao deve ser commitado** se contiver `EngineApiKey`. Adicionar ao `.gitignore`.

---

## O que esta implementado (Fase 1)

- `LoadAndSpawnMission()` — HTTP GET + parse JSON + Soft Reference async spawn
- `UJacareSettings` — configuracao via Project Settings UI
- `LogJacare` — categoria de log propria
- Guards de `IsValid` nos callbacks assincronos
- Validacao `IsChildOf(AActor)` antes do spawn

## Proximo passo (Fase 4)

- `INodeHandler` — interface base para execucao de nos
- Handler Registry — `TMap<FGameplayTag, TUniquePtr<INodeHandler>>`
- Handlers: `Objective.Kill`, `Objective.Goto`, `Objective.Collect`, `Objective.Interact`
- Handlers: `Audio.Play`, `Reward.Give`, `Flag.Set`, `Flow.Wait`, `Flow.Branch`
- `ReportObjective(MissionId, NodeId)` — avanca o grafo em memoria
- `FOnMissionSpawned` delegate — Blueprint reage ao sucesso/falha

---

## Comandos

```
# Setup inicial
1. Copiar Source/jacare/ para {SeuProjeto}/Plugins/jacare/
2. Right-click no .uproject → Generate Visual Studio files
3. Build o projeto (Ctrl+Shift+B no Rider ou VS)
4. Project Settings → Plugins → Jacare → configurar BackendUrl e EngineApiKey
```

---

## Skills (Slash Commands)

| Skill | Quando usar |
|-------|-------------|
| `/review` | Review completo de arquitetura antes de PR |
| `/commit` | Gerar commit message em Conventional Commits |
| `/new-jacare-mission-type` | Scaffold cross-ecosystem para novo Node Type (inclui steps C++) |

---

## Rules (Contextuais)

Rules path-specific a criar em `.claude/rules/`:
- `ue5-conventions.md` — Regras C++/UE5 (`Source/**/*.cpp`, `Source/**/*.h`)
- `handlers.md` — Padrao de implementacao de INodeHandler (`Source/**/Handlers/**`)
