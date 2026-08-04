# Testing

## Backend build and smoke test

Run the bootstrap once to install the pinned SDK, compile the module, and generate the Unreal bindings:

```bash
./Tools/SpacetimeDB/bootstrap.sh
```

Start SpacetimeDB in one terminal:

```bash
spacetime start
```

Publish and inspect the local shard in another:

```bash
spacetime publish aetherfront-dev --server local --module-path ./spacetimedb --yes
spacetime sql aetherfront-dev "SELECT * FROM world_config" --server local
spacetime logs aetherfront-dev --server local -f
```

Expected initial state is one `world_config` row, one repeating private simulation timer, 160 deterministic resource nodes, and the cells containing those resources. Connecting a fresh UE client should add one commander, one complete Citadel, and six Fabricators. Reconnecting with the saved token must reuse the same commander and must not create another starter base.

## SpacetimeDB-connected editor smoke test

1. Complete the backend smoke test above.
2. Generate project files and build the `AetherfrontEditor` Development target.
3. Open the project in Unreal Engine 5.6.
4. Press Play and confirm the log reports `persistent shard is ready`.
5. Stop and restart PIE; confirm the same identity token reconnects.
6. Inspect `commander`, `building`, and `unit` with `spacetime sql` and confirm the starter state remains durable.

The current checkpoint synchronizes the generated client cache but does not yet project SpacetimeDB rows into gameplay Actors or route the controller through generated reducers. Those end-to-end command assertions belong to the next checkpoint.

## Offline Unreal fallback smoke test

Set `bAutoConnect=False` under `[/Script/Aetherfront.AetherfrontSpacetimeSubsystem]` in a local config override, then:

1. Choose **Play → Net Mode: Play As Listen Server** and start two players.
2. Confirm each player receives a differently colored Citadel and six units.
3. Drag-select owned units, right-click the ground, and confirm only the owning client can command them.
4. Shift-right-click multiple positions and confirm the unit formations traverse the queued waypoints.
5. Press `B`, click a clear location, and confirm 250 Alloy is removed, the Relay constructs over time, and both clients see it.
6. Stop PIE, restart it, and confirm fallback buildings return from `Saved/Aetherfront/world-v1.json`.

Development-only fallback identities may still be supplied as URL options:

```text
127.0.0.1?PlayerId=commander-alpha
127.0.0.1?PlayerId=commander-beta
```

Those values never establish ownership in SpacetimeDB. Connected mode derives ownership only from the backend sender identity.

## Unreal automation tests

From an Unreal Engine installation, run:

```bash
UnrealEditor-Cmd Aetherfront.uproject \
  -ExecCmds="Automation RunTests Aetherfront; Quit" \
  -unattended -nop4 -NullRHI -log
```

The initial Unreal tests cover the economy catalog and versioned offline save-schema round trip. Backend reducer integration and reconnect tests are still required.

## Dedicated-server fallback

Build `AetherfrontServer`, then run a server and two clients:

```bash
AetherfrontServer /Engine/Maps/Entry?listen -log
Aetherfront 127.0.0.1?PlayerId=commander-alpha -windowed -ResX=1440 -ResY=900 -log
Aetherfront 127.0.0.1?PlayerId=commander-beta -windowed -ResX=1440 -ResY=900 -log
```

This is an offline/fallback topology, not the production source of truth.

## Current verification boundary

This workspace does not contain Unreal Editor, Blender, Rust, or the SpacetimeDB CLI. Source, configuration, scripts, JSON, Python, and repository hygiene can be checked here, but a successful SpacetimeDB module build, generated-binding pass, UE5.6 compile, and editor smoke test are required before this checkpoint is called runtime-verified.
