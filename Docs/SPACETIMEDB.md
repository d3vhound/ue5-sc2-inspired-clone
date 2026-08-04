# SpacetimeDB backend

Aetherfront pins the Rust module crate and the official Unreal client SDK to **SpacetimeDB 2.7.1**. SpacetimeDB is the authoritative durable shard, not a cache behind Unreal replication.

## Source layout

| Path | Purpose |
| --- | --- |
| `spacetimedb/Cargo.toml` | Pinned Rust module dependencies |
| `spacetimedb/src/lib.rs` | Tables, reducers, validation, generation, and fixed-step simulation |
| `Tools/SpacetimeDB/bootstrap.*` | Fetch pinned Unreal SDK, build module, generate typed bindings |
| `Source/Aetherfront/*SpacetimeSubsystem*` | UE connection, token reuse, subscription lifecycle, Blueprint status |
| `Source/Aetherfront/**/ModuleBindings` | Generated client API after bootstrap; commit these when the schema changes |

The third-party SDK itself is ignored by Git and fetched from the official `clockworklabs/SpacetimeDB` `v2.7.1` tag. No credentials are committed.

## Local setup

Install the CLI from the [official installer](https://spacetimedb.com/install), select the pinned version, then run:

```bash
spacetime version install 2.7.1 --use --yes
```

Then bootstrap the project:

```bash
./Tools/SpacetimeDB/bootstrap.sh
```

PowerShell:

```powershell
./Tools/SpacetimeDB/bootstrap.ps1
```

The bootstrap is deliberately non-destructive. If another `Plugins/SpacetimeDbSdk` directory exists, it stops instead of replacing it.

Start the local service in its own terminal:

```bash
spacetime start
```

Publish from the repository root:

```bash
spacetime publish aetherfront-dev \
  --server local \
  --module-path ./spacetimedb \
  --yes
```

Inspect it:

```bash
spacetime sql aetherfront-dev "SELECT * FROM world_config" --server local
spacetime logs aetherfront-dev --server local -f
```

Do not use `--delete-data=always` against a persistent shard. It is appropriate only when a disposable local database must be rebuilt from zero. Production schema changes require a migration, backup, and restore plan.

## Authority and command flow

1. The UE subsystem connects to the configured URI and database.
2. A saved server-issued token restores the same SpacetimeDB `Identity` on reconnect.
3. The `client_connected` lifecycle reducer restores presence or creates a commander, Citadel, and six Fabricators transactionally.
4. UE invokes semantic reducers such as `issue_move` and `place_building`.
5. Reducers validate identity, ownership, bounds, command ordering, costs, overlap, and queue capacity.
6. A scheduled reducer advances construction and movement at 20 Hz.
7. Subscriptions update the UE client cache; the presentation adapter creates or updates local Actors and Mass representations.

Command IDs are per identity, monotonically increasing, and durable. The client must persist its next command ID alongside the identity token. Replayed IDs produce a duplicate receipt without reapplying the mutation.

## Current schema

| Table/event | Role |
| --- | --- |
| `world_config` | Seed, schema, cell size, boundary, fixed-step rate, authoritative tick |
| `commander` | Identity, durable economy, home, presence, command watermark |
| `world_cell` | Lazily materialized deterministic generation cells |
| `resource_node` | Seeded Alloy deposits and remaining quantity |
| `building` | Durable structures and construction state |
| `unit` | Durable unit transform, health, and current destination |
| `waypoint` | Per-unit queued destinations |
| `command_receipt` | Ephemeral accepted/rejected/duplicate feedback |
| `simulation_timer` | Private repeating 50 ms scheduled reducer trigger |

## Security boundary

Reducer sender identity is authoritative; UE URL parameters and client-provided owner IDs are not. The development schema exposes gameplay tables publicly so the initial Unreal subscription can synchronize everything. That is not suitable for competitive production play.

Before production:

- replace broad public tables with identity-scoped and visibility-scoped views;
- subscribe only to the commander's visible cells and owned private state;
- authenticate through a managed OIDC flow instead of treating a locally saved development token as an account system;
- rate-limit reducer calls and retain abuse telemetry;
- make fog-of-war filtering a server-side data-access rule;
- test schema migrations, backups, restores, disconnects, and regional failure.

## Integration boundary

This checkpoint establishes the real backend module and the Unreal connection/subscription lifecycle. Generated bindings are produced locally because they depend on the installed 2.7.1 toolchain. The next code checkpoint wires generated table callbacks into Actor/Mass projection and routes controller commands through generated reducers. Until then, the existing Unreal replicated loop is the playable offline fallback.
