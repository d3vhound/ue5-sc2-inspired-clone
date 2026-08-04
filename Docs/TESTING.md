# Testing

## Editor smoke test

1. Generate project files and build the `AetherfrontEditor` Development target.
2. Open the project in Unreal Engine 5.6.
3. Choose **Play → Net Mode: Play As Listen Server** and start two players.
4. Confirm each player receives a differently colored Citadel and six units.
5. Drag-select owned units, right-click the ground, and confirm only the owning client can command them.
6. Press `B`, click a clear location, and confirm 250 Alloy is removed, the Relay constructs over time, and both clients see it.
7. Stop PIE, restart it, and confirm buildings return from `Saved/Aetherfront/world-v1.json`.

Use unique URL options for stable development identities when launching standalone clients:

```text
127.0.0.1?PlayerId=commander-alpha
127.0.0.1?PlayerId=commander-beta
```

`PlayerId` is a development-only identity hook. A production server must derive ownership from authenticated session claims, never a client-selected URL value.

## Automation tests

From an Unreal Engine installation, run:

```bash
UnrealEditor-Cmd Aetherfront.uproject \
  -ExecCmds="Automation RunTests Aetherfront; Quit" \
  -unattended -nop4 -NullRHI -log
```

The initial tests cover the economy catalog and versioned save-schema round trip.

## Dedicated server

Build `AetherfrontServer`, then run a server and two clients:

```bash
AetherfrontServer /Engine/Maps/Entry?listen -log
Aetherfront 127.0.0.1?PlayerId=commander-alpha -windowed -ResX=1440 -ResY=900 -log
Aetherfront 127.0.0.1?PlayerId=commander-beta -windowed -ResX=1440 -ResY=900 -log
```

## Current verification boundary

The code in the first remote checkpoints is statically checked here, but this workspace does not contain Unreal Editor or Blender. A successful local UE5.6 compile and the smoke test above are required before the checkpoint should be called editor-verified.
