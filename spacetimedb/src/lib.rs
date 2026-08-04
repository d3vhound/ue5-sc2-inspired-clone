use spacetimedb::{Identity, ReducerContext, ScheduleAt, Table, Timestamp};
use std::collections::BTreeSet;
use std::time::Duration;

const SCHEMA_VERSION: u32 = 1;
const WORLD_SEED: u32 = 20_260_804;
const WORLD_HALF_EXTENT_CM: i32 = 900_000;
const CELL_SIZE_CM: i32 = 10_000;
const SIMULATION_HZ: u16 = 20;
const SIMULATION_STEP_MS: u64 = 1_000 / SIMULATION_HZ as u64;
const UNIT_STEP_CM: i32 = 30;
const STARTING_ALLOY: i64 = 1_200;
const STARTING_FLUX: i64 = 200;
const RELAY_COST_ALLOY: i64 = 250;
const RELAY_BUILD_TICKS: u16 = SIMULATION_HZ * 9;
const MAX_UNITS_PER_COMMAND: usize = 200;
const MAX_WAYPOINTS_PER_UNIT: usize = 16;
const FORMATION_SPACING_CM: i32 = 180;
const RESOURCE_NODE_COUNT: u64 = 160;
const MAX_STARTER_SLOTS: u64 = 169;

const BUILDING_CITADEL: u8 = 0;
const BUILDING_RELAY: u8 = 1;
const UNIT_FABRICATOR: u8 = 0;
const RESOURCE_ALLOY: u8 = 0;

const COMMAND_MOVE: u8 = 0;
const COMMAND_PLACE_BUILDING: u8 = 1;
const RECEIPT_REJECTED: u8 = 0;
const RECEIPT_ACCEPTED: u8 = 1;
const RECEIPT_DUPLICATE: u8 = 2;

#[spacetimedb::table(accessor = world_config, public)]
pub struct WorldConfig {
    #[primary_key]
    pub id: u8,
    pub schema_version: u32,
    pub world_seed: u32,
    pub cell_size_cm: i32,
    pub world_half_extent_cm: i32,
    pub simulation_hz: u16,
    pub authoritative_tick: u64,
}

#[spacetimedb::table(accessor = commander, public)]
pub struct Commander {
    #[primary_key]
    pub identity: Identity,
    #[unique]
    #[auto_inc]
    pub commander_id: u64,
    pub display_name: String,
    pub alloy: i64,
    pub flux: i64,
    pub home_x_cm: i32,
    pub home_y_cm: i32,
    pub team_hue: u8,
    pub created_at: Timestamp,
    pub last_seen_at: Timestamp,
    pub is_online: bool,
    pub last_command_id: u64,
}

#[spacetimedb::table(accessor = world_cell, public)]
pub struct WorldCell {
    #[primary_key]
    pub cell_id: i64,
    pub cell_x: i32,
    pub cell_y: i32,
    pub seed: u32,
    pub generation_version: u16,
}

#[spacetimedb::table(accessor = building, public)]
pub struct Building {
    #[primary_key]
    #[auto_inc]
    pub entity_id: u64,
    #[index(btree)]
    pub owner: Identity,
    pub kind: u8,
    pub x_cm: i32,
    pub y_cm: i32,
    pub z_cm: i32,
    pub yaw_milliradians: i32,
    #[index(btree)]
    pub cell_id: i64,
    pub build_progress_basis_points: u16,
    pub build_ticks_remaining: u16,
    pub hit_points: i32,
    pub updated_at: Timestamp,
}

#[spacetimedb::table(accessor = unit, public)]
pub struct Unit {
    #[primary_key]
    #[auto_inc]
    pub entity_id: u64,
    #[index(btree)]
    pub owner: Identity,
    pub kind: u8,
    pub x_cm: i32,
    pub y_cm: i32,
    pub z_cm: i32,
    pub yaw_milliradians: i32,
    #[index(btree)]
    pub cell_id: i64,
    pub hit_points: i32,
    pub target_x_cm: i32,
    pub target_y_cm: i32,
    pub has_target: bool,
    pub command_sequence: u64,
    pub updated_at: Timestamp,
}

#[spacetimedb::table(accessor = waypoint, public)]
pub struct Waypoint {
    #[primary_key]
    #[auto_inc]
    pub waypoint_id: u64,
    #[index(btree)]
    pub unit_id: u64,
    pub ordinal: u16,
    pub x_cm: i32,
    pub y_cm: i32,
}

#[spacetimedb::table(accessor = resource_node, public)]
pub struct ResourceNode {
    #[primary_key]
    pub resource_id: u64,
    pub kind: u8,
    pub x_cm: i32,
    pub y_cm: i32,
    pub z_cm: i32,
    #[index(btree)]
    pub cell_id: i64,
    pub remaining_units: i64,
}

#[spacetimedb::table(accessor = command_receipt, public, event)]
pub struct CommandReceipt {
    pub identity: Identity,
    pub command_id: u64,
    pub command_kind: u8,
    pub status: u8,
    pub message: String,
}

#[spacetimedb::table(accessor = simulation_timer, scheduled(simulation_tick))]
pub struct SimulationTimer {
    #[primary_key]
    #[auto_inc]
    pub scheduled_id: u64,
    pub scheduled_at: ScheduleAt,
}

#[spacetimedb::reducer(init)]
pub fn init(ctx: &ReducerContext) -> Result<(), String> {
    ctx.db.world_config().try_insert(WorldConfig {
        id: 0,
        schema_version: SCHEMA_VERSION,
        world_seed: WORLD_SEED,
        cell_size_cm: CELL_SIZE_CM,
        world_half_extent_cm: WORLD_HALF_EXTENT_CM,
        simulation_hz: SIMULATION_HZ,
        authoritative_tick: 0,
    })?;

    ctx.db.simulation_timer().try_insert(SimulationTimer {
        scheduled_id: 0,
        scheduled_at: ScheduleAt::Interval(Duration::from_millis(SIMULATION_STEP_MS).into()),
    })?;

    seed_resource_nodes(ctx)?;
    log::info!("Aetherfront shard initialized with schema {}", SCHEMA_VERSION);
    Ok(())
}

#[spacetimedb::reducer(client_connected)]
pub fn client_connected(ctx: &ReducerContext) -> Result<(), String> {
    if let Some(mut commander) = ctx.db.commander().identity().find(ctx.sender()) {
        commander.is_online = true;
        commander.last_seen_at = ctx.timestamp;
        ctx.db.commander().identity().update(commander);
        return Ok(());
    }

    let slot = ctx.db.commander().count();
    if slot >= MAX_STARTER_SLOTS {
        return Err("This shard has reached its current starter-base capacity".into());
    }
    let (home_x_cm, home_y_cm) = home_for_slot(slot);
    let commander = ctx.db.commander().try_insert(Commander {
        identity: ctx.sender(),
        commander_id: 0,
        display_name: format!("Commander {}", slot + 1),
        alloy: STARTING_ALLOY,
        flux: STARTING_FLUX,
        home_x_cm,
        home_y_cm,
        team_hue: ((slot * 47) % 255) as u8,
        created_at: ctx.timestamp,
        last_seen_at: ctx.timestamp,
        is_online: true,
        last_command_id: 0,
    })?;

    spawn_starter_base(ctx, commander.identity, home_x_cm, home_y_cm)?;
    log::info!("Created commander {}", commander.commander_id);
    Ok(())
}

#[spacetimedb::reducer(client_disconnected)]
pub fn client_disconnected(ctx: &ReducerContext) -> Result<(), String> {
    if let Some(mut commander) = ctx.db.commander().identity().find(ctx.sender()) {
        commander.is_online = false;
        commander.last_seen_at = ctx.timestamp;
        ctx.db.commander().identity().update(commander);
    }
    Ok(())
}

#[spacetimedb::reducer]
pub fn set_display_name(ctx: &ReducerContext, display_name: String) -> Result<(), String> {
    let clean_name = display_name.trim();
    let character_count = clean_name.chars().count();
    if !(2..=24).contains(&character_count) || clean_name.chars().any(char::is_control) {
        return Err("Display name must contain 2-24 visible characters".into());
    }

    let mut commander = ctx
        .db
        .commander()
        .identity()
        .find(ctx.sender())
        .ok_or("Commander profile not found")?;
    commander.display_name = clean_name.to_owned();
    commander.last_seen_at = ctx.timestamp;
    ctx.db.commander().identity().update(commander);
    Ok(())
}

#[spacetimedb::reducer]
pub fn place_building(
    ctx: &ReducerContext,
    kind: u8,
    x_cm: i32,
    y_cm: i32,
    yaw_milliradians: i32,
    command_id: u64,
) -> Result<(), String> {
    let mut commander = ctx
        .db
        .commander()
        .identity()
        .find(ctx.sender())
        .ok_or("Commander profile not found")?;

    if command_id == 0 || command_id <= commander.last_command_id {
        emit_receipt(
            ctx,
            command_id,
            COMMAND_PLACE_BUILDING,
            RECEIPT_DUPLICATE,
            "Command was already processed",
        );
        return Ok(());
    }

    if kind != BUILDING_RELAY {
        reject_command(
            ctx,
            commander,
            command_id,
            COMMAND_PLACE_BUILDING,
            "Only Assembly Relays are currently buildable",
        );
        return Ok(());
    }

    let footprint_cm = building_footprint_cm(kind);
    if !is_in_world_with_margin(x_cm, y_cm, footprint_cm) {
        reject_command(
            ctx,
            commander,
            command_id,
            COMMAND_PLACE_BUILDING,
            "Placement is outside the shard boundary",
        );
        return Ok(());
    }

    if commander.alloy < RELAY_COST_ALLOY {
        reject_command(
            ctx,
            commander,
            command_id,
            COMMAND_PLACE_BUILDING,
            "Not enough Alloy",
        );
        return Ok(());
    }

    if placement_overlaps(ctx, x_cm, y_cm, footprint_cm) {
        reject_command(
            ctx,
            commander,
            command_id,
            COMMAND_PLACE_BUILDING,
            "Placement overlaps an existing structure or resource",
        );
        return Ok(());
    }

    let cell_id = ensure_world_cell(ctx, x_cm, y_cm)?;
    commander.alloy -= RELAY_COST_ALLOY;
    commander.last_command_id = command_id;
    commander.last_seen_at = ctx.timestamp;
    ctx.db.commander().identity().update(commander);
    ctx.db.building().try_insert(Building {
        entity_id: 0,
        owner: ctx.sender(),
        kind,
        x_cm,
        y_cm,
        z_cm: 0,
        yaw_milliradians: normalize_yaw(yaw_milliradians),
        cell_id,
        build_progress_basis_points: 0,
        build_ticks_remaining: RELAY_BUILD_TICKS,
        hit_points: 1_200,
        updated_at: ctx.timestamp,
    })?;
    emit_receipt(
        ctx,
        command_id,
        COMMAND_PLACE_BUILDING,
        RECEIPT_ACCEPTED,
        "Relay construction started",
    );
    Ok(())
}

#[spacetimedb::reducer]
pub fn issue_move(
    ctx: &ReducerContext,
    unit_ids: Vec<u64>,
    target_x_cm: i32,
    target_y_cm: i32,
    queue: bool,
    command_id: u64,
) -> Result<(), String> {
    let mut commander = ctx
        .db
        .commander()
        .identity()
        .find(ctx.sender())
        .ok_or("Commander profile not found")?;

    if command_id == 0 || command_id <= commander.last_command_id {
        emit_receipt(
            ctx,
            command_id,
            COMMAND_MOVE,
            RECEIPT_DUPLICATE,
            "Command was already processed",
        );
        return Ok(());
    }

    let unique_ids: BTreeSet<u64> = unit_ids.into_iter().collect();
    if unique_ids.is_empty() || unique_ids.len() > MAX_UNITS_PER_COMMAND {
        reject_command(
            ctx,
            commander,
            command_id,
            COMMAND_MOVE,
            "Move commands require 1-200 unique units",
        );
        return Ok(());
    }

    if !is_in_world_with_margin(target_x_cm, target_y_cm, 0) {
        reject_command(
            ctx,
            commander,
            command_id,
            COMMAND_MOVE,
            "Move target is outside the shard boundary",
        );
        return Ok(());
    }

    let mut owned_units = Vec::with_capacity(unique_ids.len());
    for unit_id in unique_ids {
        let Some(unit) = ctx.db.unit().entity_id().find(unit_id) else {
            reject_command(
                ctx,
                commander,
                command_id,
                COMMAND_MOVE,
                "At least one selected unit no longer exists",
            );
            return Ok(());
        };
        if unit.owner != ctx.sender() {
            reject_command(
                ctx,
                commander,
                command_id,
                COMMAND_MOVE,
                "At least one selected unit is not owned by the sender",
            );
            return Ok(());
        }
        if queue {
            let queued_count = ctx.db.waypoint().unit_id().filter(unit_id).count();
            let order_count = queued_count + usize::from(unit.has_target);
            if order_count >= MAX_WAYPOINTS_PER_UNIT {
                reject_command(
                    ctx,
                    commander,
                    command_id,
                    COMMAND_MOVE,
                    "A unit has reached the 16-waypoint queue limit",
                );
                return Ok(());
            }
        }
        owned_units.push(unit);
    }

    let unit_count = owned_units.len();
    for (index, mut unit) in owned_units.into_iter().enumerate() {
        let (destination_x_cm, destination_y_cm) =
            formation_destination(index, unit_count, target_x_cm, target_y_cm);
        if queue && unit.has_target {
            let ordinal = next_waypoint_ordinal(ctx, unit.entity_id);
            ctx.db.waypoint().try_insert(Waypoint {
                waypoint_id: 0,
                unit_id: unit.entity_id,
                ordinal,
                x_cm: destination_x_cm,
                y_cm: destination_y_cm,
            })?;
        } else {
            if !queue {
                clear_waypoints(ctx, unit.entity_id);
            }
            unit.target_x_cm = destination_x_cm;
            unit.target_y_cm = destination_y_cm;
            unit.has_target = true;
        }
        unit.command_sequence = command_id;
        unit.updated_at = ctx.timestamp;
        ctx.db.unit().entity_id().update(unit);
    }

    commander.last_command_id = command_id;
    commander.last_seen_at = ctx.timestamp;
    ctx.db.commander().identity().update(commander);
    emit_receipt(
        ctx,
        command_id,
        COMMAND_MOVE,
        RECEIPT_ACCEPTED,
        "Move accepted",
    );
    Ok(())
}

#[spacetimedb::reducer]
pub fn simulation_tick(ctx: &ReducerContext, _timer: SimulationTimer) -> Result<(), String> {
    let mut config = ctx
        .db
        .world_config()
        .id()
        .find(0)
        .ok_or("World config not found")?;
    config.authoritative_tick = config.authoritative_tick.saturating_add(1);
    ctx.db.world_config().id().update(config);

    for mut building in ctx.db.building().iter() {
        if building.build_ticks_remaining == 0 {
            continue;
        }
        building.build_ticks_remaining -= 1;
        building.build_progress_basis_points = (10_000u32
            - (u32::from(building.build_ticks_remaining) * 10_000
                / u32::from(RELAY_BUILD_TICKS))) as u16;
        building.updated_at = ctx.timestamp;
        ctx.db.building().entity_id().update(building);
    }

    for mut unit in ctx.db.unit().iter() {
        if !unit.has_target {
            continue;
        }

        let dx = i64::from(unit.target_x_cm) - i64::from(unit.x_cm);
        let dy = i64::from(unit.target_y_cm) - i64::from(unit.y_cm);
        let distance_squared = dx * dx + dy * dy;
        if distance_squared <= i64::from(UNIT_STEP_CM * UNIT_STEP_CM) {
            unit.x_cm = unit.target_x_cm;
            unit.y_cm = unit.target_y_cm;
            if let Some(next) = take_next_waypoint(ctx, unit.entity_id) {
                unit.target_x_cm = next.x_cm;
                unit.target_y_cm = next.y_cm;
                unit.has_target = true;
            } else {
                unit.has_target = false;
            }
        } else {
            let distance = (distance_squared as f64).sqrt();
            let step_x = (dx as f64 / distance * f64::from(UNIT_STEP_CM)).round() as i32;
            let step_y = (dy as f64 / distance * f64::from(UNIT_STEP_CM)).round() as i32;
            unit.x_cm = clamp_world(unit.x_cm.saturating_add(step_x));
            unit.y_cm = clamp_world(unit.y_cm.saturating_add(step_y));
            unit.yaw_milliradians =
                ((dy as f64).atan2(dx as f64) * 1_000.0).round() as i32;
        }
        unit.cell_id = ensure_world_cell(ctx, unit.x_cm, unit.y_cm)?;
        unit.updated_at = ctx.timestamp;
        ctx.db.unit().entity_id().update(unit);
    }

    Ok(())
}

fn seed_resource_nodes(ctx: &ReducerContext) -> Result<(), String> {
    let mut random_state = WORLD_SEED;
    for resource_id in 1..=RESOURCE_NODE_COUNT {
        let mut location = None;
        for _attempt in 0..64 {
            let candidate_x_cm = sample_world_coordinate(&mut random_state);
            let candidate_y_cm = sample_world_coordinate(&mut random_state);
            if !is_reserved_home_area(candidate_x_cm, candidate_y_cm)
                && !placement_overlaps(ctx, candidate_x_cm, candidate_y_cm, 180)
            {
                location = Some((candidate_x_cm, candidate_y_cm));
                break;
            }
        }
        let (x_cm, y_cm) = location.ok_or("Unable to place deterministic resource node")?;
        let remaining_units = 8_000 + i64::from(next_random(&mut random_state) % 8_001);
        let cell_id = ensure_world_cell(ctx, x_cm, y_cm)?;
        ctx.db.resource_node().try_insert(ResourceNode {
            resource_id,
            kind: RESOURCE_ALLOY,
            x_cm,
            y_cm,
            z_cm: 0,
            cell_id,
            remaining_units,
        })?;
    }
    Ok(())
}

fn spawn_starter_base(
    ctx: &ReducerContext,
    owner: Identity,
    home_x_cm: i32,
    home_y_cm: i32,
) -> Result<(), String> {
    let cell_id = ensure_world_cell(ctx, home_x_cm, home_y_cm)?;
    ctx.db.building().try_insert(Building {
        entity_id: 0,
        owner,
        kind: BUILDING_CITADEL,
        x_cm: home_x_cm,
        y_cm: home_y_cm,
        z_cm: 0,
        yaw_milliradians: 0,
        cell_id,
        build_progress_basis_points: 10_000,
        build_ticks_remaining: 0,
        hit_points: 3_500,
        updated_at: ctx.timestamp,
    })?;

    const OFFSETS: [(i32, i32); 6] = [
        (-420, -300),
        (0, -360),
        (420, -300),
        (-420, 300),
        (0, 360),
        (420, 300),
    ];
    for (offset_x_cm, offset_y_cm) in OFFSETS {
        let x_cm = home_x_cm + offset_x_cm;
        let y_cm = home_y_cm + offset_y_cm;
        let unit_cell_id = ensure_world_cell(ctx, x_cm, y_cm)?;
        ctx.db.unit().try_insert(Unit {
            entity_id: 0,
            owner,
            kind: UNIT_FABRICATOR,
            x_cm,
            y_cm,
            z_cm: 0,
            yaw_milliradians: 0,
            cell_id: unit_cell_id,
            hit_points: 100,
            target_x_cm: x_cm,
            target_y_cm: y_cm,
            has_target: false,
            command_sequence: 0,
            updated_at: ctx.timestamp,
        })?;
    }
    Ok(())
}

fn reject_command(
    ctx: &ReducerContext,
    mut commander: Commander,
    command_id: u64,
    command_kind: u8,
    message: &str,
) {
    commander.last_command_id = command_id;
    commander.last_seen_at = ctx.timestamp;
    ctx.db.commander().identity().update(commander);
    emit_receipt(ctx, command_id, command_kind, RECEIPT_REJECTED, message);
}

fn emit_receipt(
    ctx: &ReducerContext,
    command_id: u64,
    command_kind: u8,
    status: u8,
    message: &str,
) {
    ctx.db.command_receipt().insert(CommandReceipt {
        identity: ctx.sender(),
        command_id,
        command_kind,
        status,
        message: message.to_owned(),
    });
}

fn ensure_world_cell(ctx: &ReducerContext, x_cm: i32, y_cm: i32) -> Result<i64, String> {
    let cell_x = x_cm.div_euclid(CELL_SIZE_CM);
    let cell_y = y_cm.div_euclid(CELL_SIZE_CM);
    let cell_id = pack_cell_id(cell_x, cell_y);
    if ctx.db.world_cell().cell_id().find(cell_id).is_none() {
        ctx.db.world_cell().try_insert(WorldCell {
            cell_id,
            cell_x,
            cell_y,
            seed: cell_seed(cell_x, cell_y),
            generation_version: 1,
        })?;
    }
    Ok(cell_id)
}

fn placement_overlaps(ctx: &ReducerContext, x_cm: i32, y_cm: i32, radius_cm: i32) -> bool {
    let center_cell_x = x_cm.div_euclid(CELL_SIZE_CM);
    let center_cell_y = y_cm.div_euclid(CELL_SIZE_CM);
    for cell_y in (center_cell_y - 1)..=(center_cell_y + 1) {
        for cell_x in (center_cell_x - 1)..=(center_cell_x + 1) {
            let cell_id = pack_cell_id(cell_x, cell_y);
            for building in ctx.db.building().cell_id().filter(cell_id) {
                if circles_overlap(
                    x_cm,
                    y_cm,
                    radius_cm,
                    building.x_cm,
                    building.y_cm,
                    building_footprint_cm(building.kind),
                ) {
                    return true;
                }
            }
            for resource in ctx.db.resource_node().cell_id().filter(cell_id) {
                if circles_overlap(
                    x_cm,
                    y_cm,
                    radius_cm,
                    resource.x_cm,
                    resource.y_cm,
                    180,
                ) {
                    return true;
                }
            }
        }
    }
    false
}

fn clear_waypoints(ctx: &ReducerContext, unit_id: u64) {
    let waypoint_ids: Vec<u64> = ctx
        .db
        .waypoint()
        .unit_id()
        .filter(unit_id)
        .map(|waypoint| waypoint.waypoint_id)
        .collect();
    for waypoint_id in waypoint_ids {
        ctx.db.waypoint().waypoint_id().delete(waypoint_id);
    }
}

fn next_waypoint_ordinal(ctx: &ReducerContext, unit_id: u64) -> u16 {
    ctx.db
        .waypoint()
        .unit_id()
        .filter(unit_id)
        .map(|waypoint| waypoint.ordinal)
        .max()
        .map_or(0, |ordinal| ordinal.saturating_add(1))
}

fn take_next_waypoint(ctx: &ReducerContext, unit_id: u64) -> Option<Waypoint> {
    let next = ctx
        .db
        .waypoint()
        .unit_id()
        .filter(unit_id)
        .min_by_key(|waypoint| waypoint.ordinal);
    if let Some(ref waypoint) = next {
        ctx.db.waypoint().waypoint_id().delete(waypoint.waypoint_id);
    }
    next
}

fn formation_destination(
    index: usize,
    count: usize,
    target_x_cm: i32,
    target_y_cm: i32,
) -> (i32, i32) {
    let columns = (count as f64).sqrt().ceil() as i32;
    let rows = ((count as i32) + columns - 1) / columns;
    let column = index as i32 % columns;
    let row = index as i32 / columns;
    let offset_x_cm = (column * 2 - (columns - 1)) * FORMATION_SPACING_CM / 2;
    let offset_y_cm = (row * 2 - (rows - 1)) * FORMATION_SPACING_CM / 2;
    (
        clamp_world(target_x_cm.saturating_add(offset_x_cm)),
        clamp_world(target_y_cm.saturating_add(offset_y_cm)),
    )
}

fn home_for_slot(slot: u64) -> (i32, i32) {
    const GRID_SPACING_CM: i32 = 120_000;
    let target_step = slot % MAX_STARTER_SLOTS;
    let mut grid_x = 0i32;
    let mut grid_y = 0i32;
    let mut visited = 0u64;
    let mut leg_length = 1i32;
    let directions = [(1, 0), (0, 1), (-1, 0), (0, -1)];

    if target_step == 0 {
        return (0, 0);
    }

    loop {
        for (direction_x, direction_y) in directions {
            for _ in 0..leg_length {
                grid_x += direction_x;
                grid_y += direction_y;
                visited += 1;
                if visited == target_step {
                    return (grid_x * GRID_SPACING_CM, grid_y * GRID_SPACING_CM);
                }
            }
            if direction_y != 0 {
                leg_length += 1;
            }
        }
    }
}

fn is_reserved_home_area(x_cm: i32, y_cm: i32) -> bool {
    const START_CLEARANCE_CM: i64 = 2_500;
    for slot in 0..MAX_STARTER_SLOTS {
        let (home_x_cm, home_y_cm) = home_for_slot(slot);
        let dx = i64::from(x_cm) - i64::from(home_x_cm);
        let dy = i64::from(y_cm) - i64::from(home_y_cm);
        if dx * dx + dy * dy < START_CLEARANCE_CM * START_CLEARANCE_CM {
            return true;
        }
    }
    false
}

fn pack_cell_id(cell_x: i32, cell_y: i32) -> i64 {
    (i64::from(cell_x) << 32) | i64::from(cell_y as u32)
}

fn cell_seed(cell_x: i32, cell_y: i32) -> u32 {
    let mut seed = WORLD_SEED
        ^ (cell_x as u32).wrapping_mul(0x9E37_79B9)
        ^ (cell_y as u32).wrapping_mul(0x85EB_CA6B);
    next_random(&mut seed)
}

fn next_random(state: &mut u32) -> u32 {
    let mut value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    value
}

fn sample_world_coordinate(state: &mut u32) -> i32 {
    const RESOURCE_MARGIN_CM: i32 = 35_000;
    let half_range = WORLD_HALF_EXTENT_CM - RESOURCE_MARGIN_CM;
    let range = (half_range * 2 + 1) as u32;
    (next_random(state) % range) as i32 - half_range
}

fn building_footprint_cm(kind: u8) -> i32 {
    match kind {
        BUILDING_CITADEL => 520,
        BUILDING_RELAY => 300,
        _ => 300,
    }
}

fn circles_overlap(
    ax_cm: i32,
    ay_cm: i32,
    ar_cm: i32,
    bx_cm: i32,
    by_cm: i32,
    br_cm: i32,
) -> bool {
    let dx = i64::from(ax_cm) - i64::from(bx_cm);
    let dy = i64::from(ay_cm) - i64::from(by_cm);
    let radius = i64::from(ar_cm) + i64::from(br_cm);
    dx * dx + dy * dy < radius * radius
}

fn is_in_world_with_margin(x_cm: i32, y_cm: i32, margin_cm: i32) -> bool {
    let limit = i64::from(WORLD_HALF_EXTENT_CM - margin_cm);
    i64::from(x_cm).abs() <= limit && i64::from(y_cm).abs() <= limit
}

fn clamp_world(value_cm: i32) -> i32 {
    value_cm.clamp(-WORLD_HALF_EXTENT_CM, WORLD_HALF_EXTENT_CM)
}

fn normalize_yaw(yaw_milliradians: i32) -> i32 {
    const FULL_TURN: i32 = 6_283;
    yaw_milliradians.rem_euclid(FULL_TURN)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cell_ids_preserve_signed_coordinates() {
        assert_ne!(pack_cell_id(-1, 0), pack_cell_id(0, -1));
        assert_ne!(pack_cell_id(-7, 3), pack_cell_id(7, -3));
    }

    #[test]
    fn formation_is_centered_for_four_units() {
        let destinations: Vec<(i32, i32)> = (0..4)
            .map(|index| formation_destination(index, 4, 0, 0))
            .collect();
        assert_eq!(destinations, vec![(-90, -90), (90, -90), (-90, 90), (90, 90)]);
    }

    #[test]
    fn placement_uses_strict_footprint_overlap() {
        assert!(circles_overlap(0, 0, 300, 599, 0, 300));
        assert!(!circles_overlap(0, 0, 300, 600, 0, 300));
    }
}
