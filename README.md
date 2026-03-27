<p align="center">
  <a href="#">
    <img src="https://avatars.githubusercontent.com/u/271130245?s=400&u=2259ff9edea9b7ffd9c1151df6cfaea18ed15027&v=4" height="96">
    <h3 align="center">Jacare</h3>
  </a>
</p>

<p align="center">
  Design. Compile. Play. 
</p>
<p align="center">
  <a href="#installation"><strong>Installation</strong></a> ·
  <a href="#usage"><strong>Usage</strong></a> ·
  <a href="#ecosystem"><strong>Ecosystem</strong></a> ·
  <a href="#contributing"><strong>Contributing</strong></a>
</p>

<br/>

## What is this?

Jacare-app-unreal is the UE5 plugin of [Jacare](https://github.com/oJacare) — an open-source visual state machine engine for Unreal Engine 5.

It fetches the compiled mission JSON from the backend, parses it, and executes each node asynchronously — without blocking the Game Thread, without `.uasset` mission files, and without recompiling when a mission changes.

## Ecosystem

| Repository | Role |
|------------|------|
| [jacare-app-backend](https://github.com/oJacare/jacare-app-backend) | Compiler, versioning, and REST API |
| [jacare-app-frontend](https://github.com/oJacare/jacare-app-frontend) | Visual graph editor for Game Designers |
| **jacare-app-unreal** ← you are here | UE5 C++ plugin that executes the missions |

## Prerequisites

- Unreal Engine 5.7+
- A running instance of [jacare-app-backend](https://github.com/oJacare/jacare-app-backend)
- An engine API key — generated in the Canvas editor under **Settings → API Keys**

## Installation

1. Copy this repository into your project's `Plugins/` directory and name the folder `jacare`.
2. Right-click your `.uproject` → **Generate Visual Studio project files**.
3. Build the project.
4. Go to **Project Settings → Plugins → Jacare** and set:
   - `Backend URL` — e.g. `http://localhost:3001`
   - `Engine API Key` — the M2M key from the Canvas editor

> **Note:** Do not commit `DefaultGame.ini` with the API key set. Add it to `.gitignore`.

## Usage

Call `Load And Spawn Mission` from any Blueprint or C++ class:

```cpp
// C++
UJacareMissionSubsystem* Jacare = GetGameInstance()->GetSubsystem<UJacareMissionSubsystem>();
Jacare->LoadAndSpawnMission(TEXT("qst_old_country"));
```

The subsystem fires an async HTTP request to the backend, parses the JSON contract, and loads the described actor via Soft Referencing — the Game Thread is never blocked.

## How It Works

```
LoadAndSpawnMission("qst_old_country")
  → GET /missions/engine/qst_old_country/active  [x-api-key]
  → Parse JSON into FJacareMissionData
  → RequestAsyncLoad via FStreamableManager
  → SpawnActor at configured world location
```

## Supported Node Types

| Node Type | Status |
|-----------|--------|
| `Spawn.Actor` | ✅ Phase 1 — fully implemented |
| `Objective.*` · `Condition.*` · `Flow.*` | 🔜 Phase 4 — Handler Registry |
| `Dialogue.*` · `Cinematic.*` | 🔜 Phase 5 |

## Stack

C++ · Unreal Engine 5.7 · HttpModule · FJsonObjectConverter · FStreamableManager

## Contributing

Contributions are welcome. Please read [CLAUDE.md](./CLAUDE.md) for C++ and UE5 conventions before submitting a pull request.
