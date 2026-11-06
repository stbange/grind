#include "grind_core/grind_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const GrindItemDef k_item_defs[ITEM_COUNT] = {
    /* ITEM_LEMON */          {"Lemon",              0.12f, 0.15f, 0.015f, 0.35f},
    /* ITEM_SUGAR */          {"Sugar Pack",         0.40f, 0.40f, 0.001f, 0.60f},
    /* ITEM_WATER_JUG */      {"Water Jug",          1.50f, 1.50f, 0.000f, 0.40f},
    /* ITEM_PAPER_CUPS */     {"Paper Cups (5pk)",   0.08f, 0.25f, 0.000f, 0.25f},
    /* ITEM_LEMONADE_CUP */   {"Fresh Lemonade",     0.35f, 0.35f, 0.060f, 1.50f},
    /* ITEM_FOLDING_TABLE */  {"Folding Table",      2.00f, 1.80f, 0.000f, 4.00f},
    /* ITEM_FLYER_BUNDLE */   {"Flyer Bundle (20)",  0.40f, 0.40f, 0.000f, 3.00f},
    /* ITEM_STREET_SNACK */   {"Instant Noodles",    0.25f, 0.30f, 0.002f, 2.50f},
    /* ITEM_FRESH_MEAL */     {"Artisan Meal & Esp", 0.55f, 0.60f, 0.050f, 12.00f},
    /* ITEM_BUSINESS_BOOK */  {"Business Handbook",  0.60f, 0.50f, 0.000f, 35.00f}
};

const GrindItemDef *grind_item_def(GrindItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return &k_item_defs[0];
    return &k_item_defs[type];
}

static void set_status(GrindWorld *world, const char *msg) {
    snprintf(world->status_message, GRIND_MAX_LOG_LEN, "%s", msg);
}

static void spawn_contract(GrindWorld *world, int idx) {
    GrindCourierContract *c = &world->contracts[idx];
    c->active = true;
    c->claimed_by_player = false;
    c->picked_up = false;
    c->id = (world->day_index * 100) + idx + (int)(grind_rng_u32(&world->rng) % 89);

    int p_zone = 1 + (int)(grind_rng_u32(&world->rng) % (ZONE_COUNT - 1));
    int d_zone = (p_zone + 1 + (int)(grind_rng_u32(&world->rng) % (ZONE_COUNT - 2))) % ZONE_COUNT;
    c->pickup_zone = (GrindZoneId)p_zone;
    c->dropoff_zone = (GrindZoneId)d_zone;
    c->pickup_pos = world->buildings[p_zone].pos;
    c->pickup_pos.z += 6.0f;
    c->dropoff_pos = world->buildings[d_zone].pos;
    c->dropoff_pos.z += 6.0f;

    c->mass_kg = grind_rng_range(&world->rng, 1.8f, 9.5f);
    c->volume_l = grind_rng_range(&world->rng, 3.5f, 14.0f);
    float dist = vec3_dist_xz(c->pickup_pos, c->dropoff_pos);
    c->reward_cash = 8.0f + (dist * 0.16f) + (c->mass_kg * 1.35f);
    c->deadline_hours = 4.5f;
}

void grind_world_init(GrindWorld *world, uint64_t seed) {
    memset(world, 0, sizeof(*world));
    grind_rng_seed(&world->rng, seed ? seed : 0xC0FFEE123456ULL);

    world->day_index = 1;
    world->time_of_day_hours = 7.5f; /* 07:30 AM */
    world->ambient_temp_c = 25.0f;
    world->weather = WEATHER_SUNNY;

    /* Initial found money on the ground right near the tenement alley */
    world->ground_money_available = true;
    world->ground_money_pos = vec3_make(-45.0f, 0.0f, -42.0f);
    world->ground_money_amount = 12.00f;

    /* Player starts broke in the tenement alley */
    GrindPlayer *p = &world->player;
    p->pos = vec3_make(-52.0f, 0.0f, -48.0f);
    p->yaw_rad = GRIND_DEG2RAD(45.0f);
    p->pitch_rad = 0.0f;
    p->cash = 0.0f;
    p->bank_balance = 0.0f;
    p->stamina = 100.0f;
    p->hunger = 10.0f;
    p->stress = 12.0f;
    p->stand_price = 1.60f;
    p->stand_lemonade_quality = 0.70f;
    p->recipe_lemons_per_batch = 1;
    p->recipe_sugar_per_batch = 1;
    world->rival_stand_active = false;
    world->rival_stand_pos = vec3_make(9.0f, 0.0f, -6.0f);
    world->rival_stand_price = 1.45f;
    world->rival_stand_quality = 0.68f;
    for (int i = 0; i < ITEM_COUNT; ++i) {
        p->item_freshness[i] = 1.0f;
    }

    /* Initialize 7 Primary City Landmarks / Zones */
    const GrindBuilding b_init[ZONE_COUNT] = {
        {ZONE_TENEMENT_ALLEY,   "Tenement Alley (Home)",    {-58.0f, 0.0f, -55.0f}, {11.0f, 12.0f, 10.0f}, {0.72f, 0.42f, 0.36f, 1.0f}, 0},
        {ZONE_WHOLESALE_MARKET, "Wholesale Fresh Market",   {-32.0f, 0.0f, -12.0f}, {13.0f,  9.5f, 11.0f}, {0.35f, 0.78f, 0.45f, 1.0f}, 2},
        {ZONE_CENTRAL_PLAZA,    "Central Sunny Plaza",      {  0.0f, 0.0f,   0.0f}, { 9.0f,  3.2f,  9.0f}, {0.92f, 0.86f, 0.65f, 1.0f}, 2},
        {ZONE_BIKE_VEHICLE_SHOP,"Metro Bike & Auto Works",  { 38.0f, 0.0f, -18.0f}, {13.0f, 10.0f, 11.0f}, {0.32f, 0.62f, 0.92f, 1.0f}, 3},
        {ZONE_DISPATCH_HUB,     "City Courier Dispatch",    { 32.0f, 0.0f,  34.0f}, {14.0f, 11.5f, 11.0f}, {0.94f, 0.64f, 0.24f, 1.0f}, 3},
        {ZONE_BANK_FINANCE,     "First National Bank",      {-12.0f, 0.0f,  48.0f}, {14.0f, 25.0f, 12.0f}, {0.55f, 0.68f, 0.82f, 1.0f}, 1},
        {ZONE_BOUTIQUE_LUXURY,  "Grand Luxe & Penthouse",   { 52.0f, 0.0f,  52.0f}, {15.0f, 28.0f, 13.0f}, {0.88f, 0.48f, 0.82f, 1.0f}, 1}
    };
    memcpy(world->buildings, b_init, sizeof(b_init));
    world->building_count = ZONE_COUNT;

    /* Procedurally generate a dense, walkable urban skyline of ~80 additional city buildings */
    GrindRng city_rng;
    grind_rng_seed(&city_rng, (seed ? seed : 0xC0FFEE123456ULL) ^ 0xA5A55A5AULL);

    for (int gx = -96; gx <= 96 && world->building_count < GRIND_MAX_BUILDINGS; gx += 16) {
        for (int gz = -96; gz <= 96 && world->building_count < GRIND_MAX_BUILDINGS; gz += 16) {
            float bx = (float)gx + grind_rng_range(&city_rng, -1.8f, 1.8f);
            float bz = (float)gz + grind_rng_range(&city_rng, -1.8f, 1.8f);

            /* Keep main avenues (X = ±22, Z = ±26) and Central Plaza clear for fast bike/van routing */
            if (fabsf(bx - 22.0f) < 9.5f || fabsf(bx + 22.0f) < 9.5f) continue;
            if (fabsf(bz - 26.0f) < 9.5f || fabsf(bz + 26.0f) < 9.5f) continue;
            if (fabsf(bx) < 16.0f && fabsf(bz) < 16.0f) continue;

            /* Keep player spawn, found money, and luxury courtyard clear */
            Vec3 cand = vec3_make(bx, 0.0f, bz);
            if (vec3_dist_xz(cand, world->player.pos) < 10.0f) continue;
            if (vec3_dist_xz(cand, world->ground_money_pos) < 9.0f) continue;
            if (vec3_dist_xz(cand, vec3_make(45.0f, 0.0f, 39.0f)) < 11.0f) continue;

            /* Avoid overlapping the 7 key landmark buildings and their front entrances */
            bool overlaps = false;
            for (int k = 0; k < ZONE_COUNT; ++k) {
                if (vec3_dist_xz(cand, world->buildings[k].pos) < 15.5f) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) continue;

            GrindBuilding *b = &world->buildings[world->building_count++];
            b->zone = ZONE_CENTRAL_PLAZA;
            b->name = "City Block";
            b->pos = cand;
            float w = grind_rng_range(&city_rng, 7.5f, 10.8f);
            float d = grind_rng_range(&city_rng, 7.5f, 10.8f);
            /* Taller skyscrapers downtown, mid-rise tenements on outskirts */
            float dist_center = sqrtf(bx * bx + bz * bz);
            float max_h = (dist_center < 55.0f) ? 34.0f : 20.0f;
            float h = grind_rng_range(&city_rng, 8.5f, max_h);
            b->size = vec3_make(w, h, d);
            b->tex_tile = (int)(grind_rng_u32(&city_rng) % 4); /* Facade tile 0..3 in top row of city_atlas */

            float tint = grind_rng_range(&city_rng, 0.72f, 1.0f);
            if (b->tex_tile == 0)      b->color = (Color4){0.88f * tint, 0.54f * tint, 0.46f * tint, 1.0f}; /* Brick */
            else if (b->tex_tile == 1) b->color = (Color4){0.58f * tint, 0.74f * tint, 0.92f * tint, 1.0f}; /* Glass office */
            else if (b->tex_tile == 2) b->color = (Color4){0.86f * tint, 0.78f * tint, 0.62f * tint, 1.0f}; /* Sandstone shop */
            else                       b->color = (Color4){0.68f * tint, 0.70f * tint, 0.74f * tint, 1.0f}; /* Concrete loft */
        }
    }

    /* Procedurally scatter 48 street & park trees along avenues and plazas */
    world->tree_count = 0;
    for (int t = 0; t < 160 && world->tree_count < GRIND_MAX_TREES; ++t) {
        float tx = grind_rng_range(&city_rng, -92.0f, 92.0f);
        float tz = grind_rng_range(&city_rng, -92.0f, 92.0f);
        Vec3 tp = vec3_make(tx, 0.0f, tz);
        if (vec3_dist_xz(tp, world->player.pos) < 6.0f) continue;
        bool hit_b = false;
        for (int bi = 0; bi < world->building_count; ++bi) {
            const GrindBuilding *b = &world->buildings[bi];
            if (fabsf(tx - b->pos.x) < (b->size.x * 0.5f + 2.2f) &&
                fabsf(tz - b->pos.z) < (b->size.z * 0.5f + 2.2f)) {
                hit_b = true;
                break;
            }
        }
        if (!hit_b) {
            world->tree_positions[world->tree_count++] = tp;
        }
    }

    /* Named Social Circle NPCs + General Citizens */
    world->npc_count = 24;
    for (int i = 0; i < world->npc_count; ++i) {
        GrindNpc *npc = &world->npcs[i];
        npc->id = i;
        npc->walk_speed = grind_rng_range(&world->rng, 2.5f, 4.2f);
        npc->cash = grind_rng_range(&world->rng, 12.0f, 95.0f);
        npc->thirst = grind_rng_range(&world->rng, 0.35f, 0.85f);
        npc->price_sensitivity = grind_rng_range(&world->rng, 0.40f, 0.90f);
        npc->brand_awareness = 0.15f;

        if (i == 0) {
            snprintf(npc->name, sizeof(npc->name), "Marcus (Best Friend)");
            npc->archetype = ARCHETYPE_LOYAL_FRIEND;
            npc->pos = vec3_make(-40.0f, 0.0f, -35.0f);
            npc->affection = 0.75f;
            npc->respect = 0.45f;
            npc->trust = 0.80f;
            npc->envy = 0.02f;
        } else if (i == 1) {
            snprintf(npc->name, sizeof(npc->name), "Devon (Street Peer)");
            npc->archetype = ARCHETYPE_ENVIOUS_PEER;
            npc->pos = vec3_make(-15.0f, 0.0f, -15.0f);
            npc->affection = 0.45f;
            npc->respect = 0.30f;
            npc->trust = 0.40f;
            npc->envy = 0.05f;
        } else if (i == 2) {
            snprintf(npc->name, sizeof(npc->name), "Chloe (Socialite)");
            npc->archetype = ARCHETYPE_OPPORTUNIST;
            npc->pos = vec3_make(20.0f, 0.0f, 20.0f);
            npc->affection = -0.25f;
            npc->respect = 0.10f;
            npc->trust = 0.15f;
            npc->envy = 0.10f;
        } else if (i == 3) {
            snprintf(npc->name, sizeof(npc->name), "Director Vance (Banker)");
            npc->archetype = ARCHETYPE_BANKER;
            npc->pos = vec3_make(-12.0f, 0.0f, 42.0f);
            npc->affection = 0.0f;
            npc->respect = 0.15f;
            npc->trust = 0.25f;
            npc->envy = 0.0f;
        } else {
            snprintf(npc->name, sizeof(npc->name), "Citizen #%d", i - 3);
            npc->archetype = ARCHETYPE_CITIZEN;
            npc->pos = vec3_make(
                grind_rng_range(&world->rng, -35.0f, 35.0f),
                0.0f,
                grind_rng_range(&world->rng, -35.0f, 35.0f)
            );
            npc->affection = 0.10f;
            npc->respect = 0.20f;
            npc->trust = 0.30f;
            npc->envy = 0.0f;
        }
        npc->target_pos = vec3_make(
            grind_rng_range(&world->rng, -20.0f, 20.0f),
            0.0f,
            grind_rng_range(&world->rng, -20.0f, 20.0f)
        );
    }

    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        spawn_contract(world, i);
    }

    grind_update_social_reactions(world);
    set_status(world, "You are broke ($0). Spot the $12 bill on the ground ahead and press [E]!");
}

float grind_player_carried_mass_kg(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float mass = 0.0f;
    for (int i = 0; i < ITEM_COUNT; ++i) {
        if (p->inventory[i] > 0) {
            mass += (float)p->inventory[i] * k_item_defs[i].mass_kg;
        }
    }
    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        const GrindCourierContract *c = &world->contracts[i];
        if (c->active && c->claimed_by_player && c->picked_up) {
            mass += c->mass_kg;
        }
    }
    return mass;
}

float grind_player_carried_volume_l(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float vol = 0.0f;
    for (int i = 0; i < ITEM_COUNT; ++i) {
        if (p->inventory[i] > 0) {
            vol += (float)p->inventory[i] * k_item_defs[i].volume_l;
        }
    }
    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        const GrindCourierContract *c = &world->contracts[i];
        if (c->active && c->claimed_by_player && c->picked_up) {
            vol += c->volume_l;
        }
    }
    return vol;
}

float grind_player_max_mass_kg(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float cap = 6.0f; /* Bare hands */
    if (p->has_satchel) cap += 18.0f;
    if (p->vehicle == VEHICLE_BICYCLE) cap += 15.0f;
    else if (p->vehicle == VEHICLE_CARGO_EBIKE) cap += 45.0f;
    else if (p->vehicle == VEHICLE_DELIVERY_VAN) cap += 250.0f;
    else if (p->vehicle == VEHICLE_SPORTS_CAR) cap += 30.0f;
    return cap;
}

float grind_player_max_volume_l(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float cap = 5.0f; /* Bare hands */
    if (p->has_satchel) cap += 30.0f;
    if (p->vehicle == VEHICLE_BICYCLE) cap += 20.0f;
    else if (p->vehicle == VEHICLE_CARGO_EBIKE) cap += 65.0f;
    else if (p->vehicle == VEHICLE_DELIVERY_VAN) cap += 400.0f;
    else if (p->vehicle == VEHICLE_SPORTS_CAR) cap += 35.0f;
    return cap;
}

float grind_player_effective_move_speed(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float base = 4.5f;
    if (p->vehicle == VEHICLE_BICYCLE) base = 11.5f;
    else if (p->vehicle == VEHICLE_CARGO_EBIKE) base = 15.0f;
    else if (p->vehicle == VEHICLE_DELIVERY_VAN) base = 18.5f;
    else if (p->vehicle == VEHICLE_SPORTS_CAR) base = 24.0f;

    if (p->meal_speed_buff_hours > 0.0f) {
        base *= 1.12f;
    }

    /* Stamina fatigue penalty */
    if (p->stamina < 20.0f) {
        base *= 0.60f;
    }

    /* Encumbrance penalty if carrying over 70% of max mass */
    float carried = grind_player_carried_mass_kg(world);
    float max_m = grind_player_max_mass_kg(world);
    float comfort_m = max_m * 0.70f;
    if (carried > comfort_m && max_m > 0.1f) {
        float over_ratio = (carried - comfort_m) / max_m;
        float mult = grind_clampf(1.0f - over_ratio * 1.4f, 0.25f, 1.0f);
        base *= mult;
    }
    return base;
}

void grind_player_move(GrindWorld *world, float forward_axis, float strafe_axis, float dt_sec) {
    if (fabsf(forward_axis) < 1e-4f && fabsf(strafe_axis) < 1e-4f) return;

    GrindPlayer *p = &world->player;
    float speed = grind_player_effective_move_speed(world);
    float fwd_x = sinf(p->yaw_rad);
    float fwd_z = -cosf(p->yaw_rad);
    float right_x = cosf(p->yaw_rad);
    float right_z = sinf(p->yaw_rad);

    Vec3 dir = vec3_make(
        fwd_x * forward_axis + right_x * strafe_axis,
        0.0f,
        fwd_z * forward_axis + right_z * strafe_axis
    );
    if (vec3_len_sq(dir) > 1.0f) {
        dir = vec3_norm(dir);
    }

    Vec3 delta = vec3_scale(dir, speed * dt_sec);
    float next_x = grind_clampf(p->pos.x + delta.x, -110.0f, 110.0f);
    float next_z = grind_clampf(p->pos.z + delta.z, -110.0f, 110.0f);

    /* Resolve AABB collision against decorative city blocks (index >= ZONE_COUNT) so streets feel solid */
    for (int bi = ZONE_COUNT; bi < world->building_count; ++bi) {
        const GrindBuilding *b = &world->buildings[bi];
        float hx = b->size.x * 0.5f + 0.65f;
        float hz = b->size.z * 0.5f + 0.65f;
        if (fabsf(next_x - b->pos.x) < hx && fabsf(next_z - b->pos.z) < hz) {
            /* Slide along X or Z axis */
            if (fabsf(p->pos.x - b->pos.x) >= hx) next_x = p->pos.x;
            else if (fabsf(p->pos.z - b->pos.z) >= hz) next_z = p->pos.z;
            else { next_x = p->pos.x; next_z = p->pos.z; }
        }
    }
    p->pos.x = next_x;
    p->pos.z = next_z;

    float dist_m = vec3_len(delta);
    float load_factor = 1.0f + (grind_player_carried_mass_kg(world) / grind_player_max_mass_kg(world));
    float drain_per_m = 0.035f;
    if (p->vehicle == VEHICLE_BICYCLE) drain_per_m = 0.018f;
    else if (p->vehicle >= VEHICLE_CARGO_EBIKE) drain_per_m = 0.004f;

    /* Logistics skill & Route Optimizer insight reduce physical strain */
    drain_per_m *= (1.0f / (1.0f + 0.08f * p->skill_logistics));
    if (p->unlocked_insights & GRIND_INSIGHT_ROUTE_OPTIMIZER) {
        drain_per_m *= 0.85f;
    }
    p->stamina = grind_clampf(p->stamina - dist_m * drain_per_m * load_factor, 0.0f, 100.0f);

    /* Moving under load teaches logistics */
    if (grind_player_carried_mass_kg(world) > 1.5f) {
        p->skill_logistics = grind_clampf(p->skill_logistics + dist_m * 0.0008f, 0.0f, 10.0f);
    }
}

bool grind_action_pickup_ground_money(GrindWorld *world) {
    if (!world->ground_money_available) return false;
    float d = vec3_dist_xz(world->player.pos, world->ground_money_pos);
    if (d > 8.0f) return false;

    world->player.cash += world->ground_money_amount;
    world->ground_money_available = false;
    world->sound_events_mask |= GRIND_SFX_COIN;
    set_status(world, "Found $12.00 on the street! Visit the Wholesale Market to buy table & lemons.");
    return true;
}

static float effective_item_cost(const GrindWorld *world, GrindItemType type) {
    float base = k_item_defs[type].base_price;
    /* Supplier equity reduces wholesale ingredient costs up to 35% */
    float discount = 1.0f - (world->player.supplier_equity_share * 0.70f);
    /* Persuasion skill grants up to 15% better bargaining terms */
    discount *= (1.0f - 0.015f * world->player.skill_persuasion);
    return base * grind_clampf(discount, 0.55f, 1.0f);
}

bool grind_action_buy_item(GrindWorld *world, GrindItemType type, int quantity) {
    if (type < 0 || type >= ITEM_COUNT || quantity <= 0) return false;
    GrindPlayer *p = &world->player;
    float unit_cost = effective_item_cost(world, type);
    float total_cost = unit_cost * (float)quantity;
    if (p->cash < total_cost) {
        set_status(world, "Not enough cash to buy item.");
        return false;
    }

    float add_mass = k_item_defs[type].mass_kg * (float)quantity;
    float add_vol = k_item_defs[type].volume_l * (float)quantity;
    if (grind_player_carried_mass_kg(world) + add_mass > grind_player_max_mass_kg(world) ||
        grind_player_carried_volume_l(world) + add_vol > grind_player_max_volume_l(world)) {
        set_status(world, "Carry capacity exceeded! Buy a Satchel or Bicycle, or deploy your Stand.");
        return false;
    }

    p->cash -= total_cost;
    p->inventory[type] += quantity;
    p->item_freshness[type] = 1.0f;

    /* If player already deployed a stand, raw ingredients can also auto-transfer when near stand */
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Bought %dx %s for $%.2f.", quantity, k_item_defs[type].name, total_cost);
    set_status(world, buf);
    return true;
}

bool grind_action_deploy_stand(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    if (p->stand_deployed) {
        /* Transfer held raw ingredients into the stand storage */
        p->stand_lemons += p->inventory[ITEM_LEMON]; p->inventory[ITEM_LEMON] = 0;
        p->stand_sugar += p->inventory[ITEM_SUGAR];   p->inventory[ITEM_SUGAR] = 0;
        p->stand_water += p->inventory[ITEM_WATER_JUG]; p->inventory[ITEM_WATER_JUG] = 0;
        p->stand_cups += p->inventory[ITEM_PAPER_CUPS] * 5; p->inventory[ITEM_PAPER_CUPS] = 0;
        set_status(world, "Restocked stand with carried ingredients.");
        return true;
    }
    if (p->inventory[ITEM_FOLDING_TABLE] <= 0) {
        set_status(world, "Need a Folding Table in inventory to set up a stand!");
        return false;
    }
    p->inventory[ITEM_FOLDING_TABLE]--;
    p->stand_deployed = true;
    p->stand_pos = p->pos;
    p->stand_lemons += p->inventory[ITEM_LEMON]; p->inventory[ITEM_LEMON] = 0;
    p->stand_sugar += p->inventory[ITEM_SUGAR];   p->inventory[ITEM_SUGAR] = 0;
    p->stand_water += p->inventory[ITEM_WATER_JUG]; p->inventory[ITEM_WATER_JUG] = 0;
    p->stand_cups += p->inventory[ITEM_PAPER_CUPS] * 5; p->inventory[ITEM_PAPER_CUPS] = 0;
    grind_update_social_reactions(world);
    set_status(world, "Lemonade Stand deployed! Craft lemonade batches to start selling.");
    return true;
}

static float calc_recipe_quality(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    int lem = p->recipe_lemons_per_batch > 0 ? p->recipe_lemons_per_batch : 1;
    int sug = p->recipe_sugar_per_batch > 0 ? p->recipe_sugar_per_batch : 1;
    float richness = (float)(lem + sug) * 0.14f; /* 1+1 = 0.28, 2+2 = 0.56 */
    float imbalance = (float)abs(lem - sug) * 0.12f;
    float base = 0.42f + richness - imbalance + 0.035f * p->skill_production;
    return grind_clampf(base * p->item_freshness[ITEM_LEMON], 0.25f, 1.0f);
}

bool grind_action_craft_lemonade(GrindWorld *world, int batches) {
    GrindPlayer *p = &world->player;
    if (!p->stand_deployed && !p->owns_storefront) {
        set_status(world, "Deploy a Folding Table or lease a Storefront first!");
        return false;
    }
    /* Pull from carried inventory into stand if needed */
    p->stand_lemons += p->inventory[ITEM_LEMON]; p->inventory[ITEM_LEMON] = 0;
    p->stand_sugar += p->inventory[ITEM_SUGAR];   p->inventory[ITEM_SUGAR] = 0;
    p->stand_water += p->inventory[ITEM_WATER_JUG]; p->inventory[ITEM_WATER_JUG] = 0;
    p->stand_cups += p->inventory[ITEM_PAPER_CUPS] * 5; p->inventory[ITEM_PAPER_CUPS] = 0;

    int req_lem = p->recipe_lemons_per_batch > 0 ? p->recipe_lemons_per_batch : 1;
    int req_sug = p->recipe_sugar_per_batch > 0 ? p->recipe_sugar_per_batch : 1;

    int crafted = 0;
    for (int i = 0; i < batches; ++i) {
        if (p->stand_lemons >= req_lem && p->stand_sugar >= req_sug && p->stand_water >= 1 && p->stand_cups >= 5) {
            if (p->stamina < 3.0f) break;
            p->stand_lemons -= req_lem;
            p->stand_sugar -= req_sug;
            p->stand_water -= 1;
            p->stand_cups -= 5;
            p->stand_lemonade_stock += 5;
            p->stamina = grind_clampf(p->stamina - 3.5f, 0.0f, 100.0f);
            p->skill_production = grind_clampf(p->skill_production + 0.12f, 0.0f, 10.0f);
            crafted++;
        }
    }
    if (crafted == 0) {
        set_status(world, "Missing ingredients for recipe or low stamina!");
        return false;
    }
    p->stand_lemonade_quality = calc_recipe_quality(world);
    world->sound_events_mask |= GRIND_SFX_CRAFT;
    if (p->skill_production >= 0.20f) {
        p->unlocked_insights |= GRIND_INSIGHT_UNIT_ECONOMICS;
    }
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Crafted %d cups (Recipe %dL/%dS, Quality %.0f%%).",
             crafted * 5, req_lem, req_sug, p->stand_lemonade_quality * 100.0f);
    set_status(world, buf);
    return true;
}

float grind_player_unit_cost_per_cup(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    int lem = p->recipe_lemons_per_batch > 0 ? p->recipe_lemons_per_batch : 1;
    int sug = p->recipe_sugar_per_batch > 0 ? p->recipe_sugar_per_batch : 1;
    float batch_cost = (float)lem * effective_item_cost(world, ITEM_LEMON) +
                       (float)sug * effective_item_cost(world, ITEM_SUGAR) +
                       1.0f * effective_item_cost(world, ITEM_WATER_JUG) +
                       1.0f * effective_item_cost(world, ITEM_PAPER_CUPS);
    return batch_cost / 5.0f;
}

float grind_player_profit_margin_pct(const GrindWorld *world) {
    float price = world->player.stand_price;
    if (price <= 0.01f) return 0.0f;
    float cogs = grind_player_unit_cost_per_cup(world);
    return ((price - cogs) / price) * 100.0f;
}

void grind_action_set_lemonade_price(GrindWorld *world, float new_price) {
    world->player.stand_price = grind_clampf(new_price, 0.25f, 15.0f);
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Set Lemonade Price to $%.2f per cup.", world->player.stand_price);
    set_status(world, buf);
}

void grind_action_adjust_price(GrindWorld *world, float delta) {
    grind_action_set_lemonade_price(world, world->player.stand_price + delta);
}

void grind_action_set_recipe(GrindWorld *world, int lemons_per_batch, int sugar_per_batch) {
    GrindPlayer *p = &world->player;
    p->recipe_lemons_per_batch = (int)grind_clampf((float)lemons_per_batch, 1.0f, 4.0f);
    p->recipe_sugar_per_batch = (int)grind_clampf((float)sugar_per_batch, 1.0f, 4.0f);
    p->stand_lemonade_quality = calc_recipe_quality(world);
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Recipe set to %d Lemon / %d Sugar -> Quality %.0f%%.",
             p->recipe_lemons_per_batch, p->recipe_sugar_per_batch, p->stand_lemonade_quality * 100.0f);
    set_status(world, buf);
}

float grind_eval_customer_utility(const GrindWorld *world, const GrindNpc *npc, float price, float quality) {
    float temp_factor = grind_clampf((world->ambient_temp_c - 10.0f) / 15.0f, 0.5f, 1.8f);
    if (world->weather == WEATHER_HOT) temp_factor *= 1.25f;
    else if (world->weather == WEATHER_RAINY && !world->player.owns_storefront) temp_factor *= 0.45f;

    float awareness = grind_clampf(npc->brand_awareness + world->player.marketing_awareness, 0.0f, 1.0f);
    float persuasion_bonus = 1.0f + 0.03f * world->player.skill_persuasion;
    float desire = npc->thirst * temp_factor * (0.75f + 0.65f * quality) * (1.0f + 0.50f * awareness) * persuasion_bonus;
    float cost_pain = price * npc->price_sensitivity;
    return desire - cost_pain;
}

bool grind_action_buy_satchel(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    if (p->has_satchel) return false;
    if (p->cash < 25.0f) {
        set_status(world, "Need $25.00 to buy the Courier Satchel.");
        return false;
    }
    p->cash -= 25.0f;
    p->has_satchel = true;
    grind_update_social_reactions(world);
    set_status(world, "Bought Courier Satchel (+30L volume, +18kg carry capacity)!");
    return true;
}

bool grind_action_buy_vehicle(GrindWorld *world, GrindVehicleTier tier) {
    GrindPlayer *p = &world->player;
    if (tier <= p->vehicle) return false;
    float cost = 0.0f;
    const char *vname = "Vehicle";
    if (tier == VEHICLE_BICYCLE)          { cost = 85.0f;   vname = "Second-Hand Bicycle"; }
    else if (tier == VEHICLE_CARGO_EBIKE) { cost = 320.0f;  vname = "Cargo E-Bike"; }
    else if (tier == VEHICLE_DELIVERY_VAN){ cost = 1200.0f; vname = "Commercial Delivery Van"; }
    else if (tier == VEHICLE_SPORTS_CAR)  { cost = 4500.0f; vname = "Personal Sports Coupe"; }
    else return false;

    if (p->cash < cost) {
        set_status(world, "Not enough cash for vehicle upgrade.");
        return false;
    }
    p->cash -= cost;
    p->vehicle = tier;
    grind_update_social_reactions(world);
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Purchased %s for $%.0f!", vname, cost);
    set_status(world, buf);
    return true;
}

bool grind_action_claim_contract(GrindWorld *world, int contract_idx) {
    if (contract_idx < 0 || contract_idx >= GRIND_MAX_CONTRACTS) return false;
    GrindCourierContract *c = &world->contracts[contract_idx];
    if (!c->active || c->claimed_by_player) return false;
    c->claimed_by_player = true;
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Claimed Courier Contract #%d (%.1fkg, Reward $%.2f).",
             c->id, c->mass_kg, c->reward_cash);
    set_status(world, buf);
    return true;
}

bool grind_action_pickup_contract_parcel(GrindWorld *world, int contract_idx) {
    if (contract_idx < 0 || contract_idx >= GRIND_MAX_CONTRACTS) return false;
    GrindCourierContract *c = &world->contracts[contract_idx];
    if (!c->active || !c->claimed_by_player || c->picked_up) return false;

    if (grind_player_carried_mass_kg(world) + c->mass_kg > grind_player_max_mass_kg(world) ||
        grind_player_carried_volume_l(world) + c->volume_l > grind_player_max_volume_l(world)) {
        set_status(world, "Parcel too heavy/bulky! Equip a Satchel or Bicycle first.");
        return false;
    }
    c->picked_up = true;
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Picked up Parcel #%d -> Deliver to %s!",
             c->id, world->buildings[c->dropoff_zone].name);
    set_status(world, buf);
    return true;
}

bool grind_action_deliver_contract_parcel(GrindWorld *world, int contract_idx) {
    if (contract_idx < 0 || contract_idx >= GRIND_MAX_CONTRACTS) return false;
    GrindCourierContract *c = &world->contracts[contract_idx];
    if (!c->active || !c->claimed_by_player || !c->picked_up) return false;

    GrindPlayer *p = &world->player;
    float tip_mult = 1.0f + 0.04f * p->skill_logistics + 0.02f * p->skill_persuasion;
    float payout = c->reward_cash * tip_mult;
    p->cash += payout;
    world->revenue_today += payout;
    world->sound_events_mask |= GRIND_SFX_SALE;
    p->deliveries_completed_total++;
    p->skill_logistics = grind_clampf(p->skill_logistics + 0.28f, 0.0f, 10.0f);
    p->business_reputation = grind_clampf(p->business_reputation + 2.5f, 0.0f, 100.0f);
    if (p->deliveries_completed_total >= 3) {
        p->unlocked_insights |= GRIND_INSIGHT_ROUTE_OPTIMIZER;
    }
    world->sale_popup_timer = 2.5f;
    world->sale_popup_pos = vec3_make(c->dropoff_pos.x, 2.8f, c->dropoff_pos.z);
    world->sale_popup_amount = payout;

    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Delivered Parcel #%d! Earned $%.2f (+Logistics Skill).", c->id, payout);
    set_status(world, buf);

    spawn_contract(world, contract_idx);
    grind_update_social_reactions(world);
    return true;
}

bool grind_action_hire_employee(GrindWorld *world, GrindEmployeeRole role) {
    if (role == EMP_ROLE_NONE || world->employee_count >= GRIND_MAX_EMPLOYEES) return false;
    GrindPlayer *p = &world->player;
    float hiring_fee = (role == EMP_ROLE_STAND_OPERATOR) ? 45.0f : 65.0f;
    if (p->cash < hiring_fee) {
        set_status(world, "Not enough cash for hiring & onboarding fee.");
        return false;
    }

    p->cash -= hiring_fee;
    GrindEmployee *e = &world->employees[world->employee_count];
    e->hired = true;
    e->role = role;
    e->wage_per_hour = (role == EMP_ROLE_STAND_OPERATOR) ? 3.50f : 4.50f;
    e->skill = 0.75f + 0.08f * p->skill_management;
    e->morale = 0.85f;
    e->pos = p->pos;
    e->target_pos = world->buildings[ZONE_CENTRAL_PLAZA].pos;
    e->task_progress = 0.0f;
    e->revenue_generated_today = 0.0f;
    snprintf(e->name, sizeof(e->name), "%s #%d",
             role == EMP_ROLE_STAND_OPERATOR ? "Barista" : "Courier",
             world->employee_count + 1);
    world->employee_count++;
    p->skill_management = grind_clampf(p->skill_management + 0.35f, 0.0f, 10.0f);
    grind_update_social_reactions(world);

    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Hired %s ($%.2f/hr)! Task Graph delegated.", e->name, e->wage_per_hour);
    set_status(world, buf);
    return true;
}

bool grind_action_distribute_flyers(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    if (p->inventory[ITEM_FLYER_BUNDLE] <= 0) return false;
    p->inventory[ITEM_FLYER_BUNDLE]--;
    p->stamina = grind_clampf(p->stamina - 6.0f, 0.0f, 100.0f);
    p->marketing_awareness = grind_clampf(p->marketing_awareness + 0.14f, 0.0f, 1.0f);
    p->skill_persuasion = grind_clampf(p->skill_persuasion + 0.20f, 0.0f, 10.0f);
    for (int i = 0; i < world->npc_count; ++i) {
        world->npcs[i].brand_awareness = grind_clampf(world->npcs[i].brand_awareness + 0.12f, 0.0f, 1.0f);
    }
    set_status(world, "Handed out flyers! Local Brand Awareness & Persuasion increased.");
    return true;
}

bool grind_action_launch_marketing_campaign(GrindWorld *world, int tier) {
    GrindPlayer *p = &world->player;
    float cost = (tier == 1) ? 50.0f : 200.0f;
    float boost = (tier == 1) ? 0.25f : 0.60f;
    if (p->cash < cost) return false;
    p->cash -= cost;
    p->marketing_awareness = grind_clampf(p->marketing_awareness + boost, 0.0f, 1.0f);
    p->business_reputation = grind_clampf(p->business_reputation + boost * 20.0f, 0.0f, 100.0f);
    for (int i = 0; i < world->npc_count; ++i) {
        world->npcs[i].brand_awareness = grind_clampf(world->npcs[i].brand_awareness + boost, 0.0f, 1.0f);
    }
    set_status(world, "Launched Marketing Campaign! Citywide brand demand surged.");
    return true;
}

bool grind_action_lease_storefront(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    if (p->owns_storefront) return false;
    float cost = 350.0f;
    if (p->cash < cost) return false;
    p->cash -= cost;
    p->owns_storefront = true;
    p->stand_deployed = true;
    p->stand_pos = world->buildings[ZONE_CENTRAL_PLAZA].pos;
    grind_update_social_reactions(world);
    set_status(world, "Leased Flagship Storefront on Central Plaza! Weather-proof + high traffic.");
    return true;
}

bool grind_action_invest_supplier_equity(GrindWorld *world, float cash_amount) {
    GrindPlayer *p = &world->player;
    if (cash_amount <= 0.0f || p->cash < cash_amount) return false;
    if (p->supplier_equity_share >= 0.50f) return false;
    p->cash -= cash_amount;
    float added_share = cash_amount / 1000.0f; /* $500 = 50% cap */
    p->supplier_equity_share = grind_clampf(p->supplier_equity_share + added_share, 0.0f, 0.50f);
    grind_update_social_reactions(world);
    set_status(world, "Invested in Wholesale Supplier Equity! Raw material costs reduced + daily dividends.");
    return true;
}

bool grind_action_invest_index_fund(GrindWorld *world, float cash_amount) {
    GrindPlayer *p = &world->player;
    if (cash_amount <= 0.0f || p->cash < cash_amount) return false;
    p->cash -= cash_amount;
    p->index_fund_invested += cash_amount;
    grind_update_social_reactions(world);
    set_status(world, "Invested in Venture Index Fund for compounding passive returns.");
    return true;
}

bool grind_action_request_bank_loan(GrindWorld *world, float amount) {
    GrindPlayer *p = &world->player;
    grind_update_social_reactions(world);
    /* Director Vance (npc[3]) evaluates perceived wealth, respect, and trust */
    const GrindNpc *banker = &world->npcs[3];
    float score = p->perceived_wealth + banker->respect * 60.0f + banker->trust * 40.0f;
    if (score < 22.0f) {
        set_status(world, "Banker Vance declined loan: Build business reputation or dress professionally.");
        return false;
    }
    p->credit_limit = score * 25.0f;
    if (p->loan_debt + amount > p->credit_limit) {
        set_status(world, "Requested loan exceeds your approved credit limit.");
        return false;
    }
    p->cash += amount;
    p->loan_debt += amount;
    set_status(world, "Bank B2B Credit Line approved and disbursed!");
    return true;
}

bool grind_action_repay_bank_loan(GrindWorld *world, float amount) {
    GrindPlayer *p = &world->player;
    float pay = grind_clampf(amount, 0.0f, p->loan_debt);
    if (pay <= 0.0f || p->cash < pay) return false;
    p->cash -= pay;
    p->loan_debt -= pay;
    world->npcs[3].trust = grind_clampf(world->npcs[3].trust + 0.15f, 0.0f, 1.0f);
    world->npcs[3].respect = grind_clampf(world->npcs[3].respect + 0.10f, 0.0f, 1.0f);
    set_status(world, "Repaid bank loan! Banker trust and credit score increased.");
    return true;
}

bool grind_action_consume_item(GrindWorld *world, GrindItemType type) {
    GrindPlayer *p = &world->player;
    if (type < 0 || type >= ITEM_COUNT || p->inventory[type] <= 0) return false;

    if (type == ITEM_STREET_SNACK) {
        p->inventory[type]--;
        p->stamina = grind_clampf(p->stamina + 28.0f, 0.0f, 100.0f);
        p->hunger = grind_clampf(p->hunger - 35.0f, 0.0f, 100.0f);
        p->stress = grind_clampf(p->stress + 4.0f, 0.0f, 100.0f);
        set_status(world, "Ate Instant Noodles (+28 Stamina, +4 Stress from cheap sodium).");
        return true;
    } else if (type == ITEM_FRESH_MEAL) {
        p->inventory[type]--;
        p->stamina = grind_clampf(p->stamina + 68.0f, 0.0f, 100.0f);
        p->hunger = 0.0f;
        p->stress = grind_clampf(p->stress - 18.0f, 0.0f, 100.0f);
        p->meal_speed_buff_hours = 3.5f;
        set_status(world, "Enjoyed Artisan Meal & Espresso (+68 Stamina, -18 Stress, Speed Buff)!");
        return true;
    } else if (type == ITEM_BUSINESS_BOOK) {
        p->inventory[type]--;
        p->skill_management = grind_clampf(p->skill_management + 1.2f, 0.0f, 10.0f);
        p->skill_persuasion = grind_clampf(p->skill_persuasion + 0.8f, 0.0f, 10.0f);
        p->unlocked_insights |= (GRIND_INSIGHT_UNIT_ECONOMICS | GRIND_INSIGHT_DEMAND_ELASTICITY | GRIND_INSIGHT_COMPETITOR_INTEL);
        set_status(world, "Studied Business Handbook (+Management, +Persuasion, Unlocked Market Insights)!");
        return true;
    }
    return false;
}

bool grind_action_buy_apparel(GrindWorld *world, GrindApparelTier tier) {
    GrindPlayer *p = &world->player;
    if (tier <= p->apparel) return false;
    float cost = 0.0f;
    if (tier == APPAREL_CLEAN_CASUAL) cost = 45.0f;
    else if (tier == APPAREL_COURIER_GEAR) cost = 110.0f;
    else if (tier == APPAREL_TAILORED_SUIT) cost = 400.0f;
    else return false;

    if (p->cash < cost) return false;
    p->cash -= cost;
    p->apparel = tier;
    grind_update_social_reactions(world);
    set_status(world, "Upgraded Apparel! People treat you differently based on how you present yourself.");
    return true;
}

bool grind_action_buy_luxury_watch(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    if (p->has_luxury_watch || p->cash < 1200.0f) return false;
    p->cash -= 1200.0f;
    p->has_luxury_watch = true;
    grind_update_social_reactions(world);
    set_status(world, "Purchased Swiss Chronograph Watch (+25 Perceived Wealth, -10 Stress)!");
    p->stress = grind_clampf(p->stress - 10.0f, 0.0f, 100.0f);
    return true;
}

bool grind_action_upgrade_housing(GrindWorld *world, GrindHousingTier tier) {
    GrindPlayer *p = &world->player;
    if (tier <= p->housing) return false;
    float cost = (tier == HOUSING_STUDIO_APT) ? 250.0f : 2500.0f;
    if (p->cash < cost) return false;
    p->cash -= cost;
    p->housing = tier;
    world->sound_events_mask |= GRIND_SFX_UPGRADE;
    grind_update_social_reactions(world);
    set_status(world, tier == HOUSING_STUDIO_APT
        ? "Moved into Sunny Studio Apartment! Better sleep & lower stress."
        : "Purchased Luxury Penthouse Villa! Peak recovery & elite status.");
    return true;
}

float grind_calc_perceived_wealth(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float w = 0.0f;
    if (p->apparel == APPAREL_CLEAN_CASUAL) w += 10.0f;
    else if (p->apparel == APPAREL_COURIER_GEAR) w += 15.0f;
    else if (p->apparel == APPAREL_TAILORED_SUIT) w += 42.0f;

    if (p->has_luxury_watch) w += 22.0f;

    if (p->vehicle == VEHICLE_BICYCLE) w += 5.0f;
    else if (p->vehicle == VEHICLE_CARGO_EBIKE) w += 12.0f;
    else if (p->vehicle == VEHICLE_DELIVERY_VAN) w += 20.0f;
    else if (p->vehicle == VEHICLE_SPORTS_CAR) w += 48.0f;

    if (p->housing == HOUSING_STUDIO_APT) w += 15.0f;
    else if (p->housing == HOUSING_PENTHOUSE_VILLA) w += 42.0f;

    if (p->stand_deployed) w += 5.0f;
    if (p->owns_storefront) w += 20.0f;
    w += (float)world->employee_count * 7.0f;

    return grind_clampf(w, 0.0f, 100.0f);
}

float grind_calc_net_worth(const GrindWorld *world) {
    const GrindPlayer *p = &world->player;
    float worth = p->cash + p->bank_balance + p->index_fund_invested - p->loan_debt;
    worth += p->supplier_equity_share * 1000.0f;
    if (p->has_satchel) worth += 25.0f;
    if (p->has_luxury_watch) worth += 1200.0f;
    if (p->owns_storefront) worth += 350.0f;

    if (p->vehicle == VEHICLE_BICYCLE) worth += 85.0f;
    else if (p->vehicle == VEHICLE_CARGO_EBIKE) worth += 320.0f;
    else if (p->vehicle == VEHICLE_DELIVERY_VAN) worth += 1200.0f;
    else if (p->vehicle == VEHICLE_SPORTS_CAR) worth += 4500.0f;

    if (p->housing == HOUSING_STUDIO_APT) worth += 250.0f;
    else if (p->housing == HOUSING_PENTHOUSE_VILLA) worth += 2500.0f;
    return worth;
}

void grind_update_social_reactions(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    p->perceived_wealth = grind_calc_perceived_wealth(world);
    float pw = p->perceived_wealth;

    for (int i = 0; i < world->npc_count; ++i) {
        GrindNpc *npc = &world->npcs[i];
        if (npc->archetype == ARCHETYPE_LOYAL_FRIEND) {
            /* Marcus respects hard work, warns against burnout/ego */
            npc->respect = grind_clampf(0.45f + (p->business_reputation * 0.005f) + (pw * 0.003f), 0.0f, 1.0f);
            npc->envy = 0.02f;
            if (pw < 15.0f) {
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"Keep grinding! That stand in the plaza is a smart start.\"");
            } else if (pw < 55.0f) {
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"Proud of how far you've come. Don't forget to rest!\"");
            } else {
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"You built a real empire! Just stay grounded, my friend.\"");
            }
        } else if (npc->archetype == ARCHETYPE_ENVIOUS_PEER) {
            /* Devon gets increasingly envious as player's visible wealth rises, unless allied as franchise partner */
            if (world->rival_allied_franchise) {
                npc->envy = 0.05f;
                npc->affection = 0.78f;
                npc->respect = 0.85f;
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"Thanks for cutting me in as a franchise partner! We're crushing it!\"");
            } else if (pw < 12.0f) {
                npc->envy = 0.05f;
                npc->affection = 0.45f;
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"We're both hustling out here in the streets.\"");
            } else {
                npc->envy = grind_clampf((pw - 10.0f) * 0.015f, 0.10f, 0.98f);
                npc->affection = grind_clampf(0.45f - npc->envy * 0.85f, -0.80f, 0.45f);
                npc->respect = grind_clampf(0.30f + pw * 0.004f, 0.0f, 1.0f);
                if (npc->envy > 0.55f) {
                    snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                             "\"Must be nice showing off that luxury life. Changed on us.\"");
                } else {
                    snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                             "\"Oh, got a bike and employees now? Big shot, huh?\"");
                }
            }
        } else if (npc->archetype == ARCHETYPE_OPPORTUNIST) {
            /* Chloe ignores broke player, flatters wealthy player for handouts */
            if (pw < 18.0f) {
                npc->affection = -0.25f;
                npc->respect = 0.10f;
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"Excuse me, I'm busy. Don't block the sidewalk.\"");
            } else {
                npc->affection = grind_clampf(-0.20f + pw * 0.016f, -0.20f, 0.92f);
                npc->respect = grind_clampf(0.15f + pw * 0.008f, 0.0f, 1.0f);
                npc->envy = 0.25f;
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"I ALWAYS knew you were a visionary! Say, can you fund my startup?\"");
            }
        } else if (npc->archetype == ARCHETYPE_BANKER) {
            /* Director Vance respects professional attire, assets, and track record */
            npc->respect = grind_clampf(0.15f + (pw * 0.006f) + (p->business_reputation * 0.005f), 0.0f, 1.0f);
            npc->affection = (npc->respect > 0.55f) ? 0.50f : 0.05f;
            if (npc->respect < 0.40f) {
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"Come back when you have collateral and proper attire.\"");
            } else {
                snprintf(npc->last_reaction, sizeof(npc->last_reaction),
                         "\"Welcome! Our commercial credit & equity desks are at your service.\"");
            }
        }
    }
}

bool grind_action_talk_to_npc(GrindWorld *world, int npc_idx) {
    if (npc_idx < 0 || npc_idx >= world->npc_count) return false;
    grind_update_social_reactions(world);
    GrindNpc *npc = &world->npcs[npc_idx];
    GrindPlayer *p = &world->player;
    p->skill_persuasion = grind_clampf(p->skill_persuasion + 0.10f, 0.0f, 10.0f);

    if (npc->archetype == ARCHETYPE_LOYAL_FRIEND) {
        npc->affection = grind_clampf(npc->affection + 0.08f, -1.0f, 1.0f);
        p->stress = grind_clampf(p->stress - 6.0f, 0.0f, 100.0f);
    } else if (npc->archetype == ARCHETYPE_ENVIOUS_PEER && npc->envy > 0.5f) {
        p->stress = grind_clampf(p->stress + 5.0f, 0.0f, 100.0f);
    }

    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "%s: %s", npc->name, npc->last_reaction);
    set_status(world, buf);
    return true;
}

bool grind_action_social_choice(GrindWorld *world, int npc_idx, int choice_id) {
    if (npc_idx < 0 || npc_idx >= world->npc_count) return false;
    GrindPlayer *p = &world->player;
    GrindNpc *npc = &world->npcs[npc_idx];

    if (npc->archetype == ARCHETYPE_LOYAL_FRIEND) {
        /* Marcus: Share a meal / bonus ($15) -> boosts affection, lowers stress, teaches persuasion */
        if (choice_id == 1 && p->cash >= 15.0f) {
            p->cash -= 15.0f;
            npc->affection = grind_clampf(npc->affection + 0.20f, -1.0f, 1.0f);
            npc->trust = grind_clampf(npc->trust + 0.15f, 0.0f, 1.0f);
            p->stress = grind_clampf(p->stress - 22.0f, 0.0f, 100.0f);
            p->skill_persuasion = grind_clampf(p->skill_persuasion + 0.30f, 0.0f, 10.0f);
            world->sound_events_mask |= GRIND_SFX_UPGRADE;
            set_status(world, "Shared an artisan meal with Marcus! (-22 Stress, +Persuasion, +Trust)");
            return true;
        }
    } else if (npc->archetype == ARCHETYPE_ENVIOUS_PEER) {
        /* Devon: Offer Profit-Sharing Franchise Partnership ($80) -> converts Rival Stand into Allied Franchise! */
        if (choice_id == 1 && !world->rival_allied_franchise && p->cash >= 80.0f) {
            p->cash -= 80.0f;
            world->rival_stand_active = true;
            world->rival_allied_franchise = true;
            grind_update_social_reactions(world);
            world->sound_events_mask |= GRIND_SFX_UPGRADE;
            set_status(world, "Partnered with Devon! Rival stand is now an Allied Franchise paying royalties!");
            return true;
        }
    } else if (npc->archetype == ARCHETYPE_OPPORTUNIST) {
        /* Chloe: Choice 1 = Set firm boundaries ($0 lost, -10 Stress); Choice 2 = Sponsor PR Event ($120) */
        if (choice_id == 1) {
            p->stress = grind_clampf(p->stress - 10.0f, 0.0f, 100.0f);
            npc->respect = grind_clampf(npc->respect + 0.18f, 0.0f, 1.0f);
            set_status(world, "Politely declined Chloe's handout request. (+Respect, -10 Stress)");
            return true;
        } else if (choice_id == 2 && p->cash >= 120.0f) {
            p->cash -= 120.0f;
            p->marketing_awareness = grind_clampf(p->marketing_awareness + 0.30f, 0.0f, 1.0f);
            world->sound_events_mask |= GRIND_SFX_UPGRADE;
            set_status(world, "Sponsored Chloe's Gala PR Event! (+30% Citywide Brand Awareness)");
            return true;
        }
    } else if (npc->archetype == ARCHETYPE_BANKER) {
        if (choice_id == 1) {
            return grind_action_request_bank_loan(world, 250.0f);
        } else if (choice_id == 2) {
            return grind_action_repay_bank_loan(world, 250.0f);
        }
    }
    return false;
}

void grind_action_sleep_overnight(GrindWorld *world) {
    GrindPlayer *p = &world->player;
    world->day_index++;
    world->time_of_day_hours = 7.5f;

    /* Daily passive investment yields & dividends */
    float div = p->supplier_equity_share * 90.0f;
    float idx_yield = p->index_fund_invested * 0.03f;
    float loan_interest = p->loan_debt * 0.02f;
    p->cash += (div + idx_yield - loan_interest);
    world->passive_income_today = div + idx_yield;
    world->revenue_today = 0.0f;
    world->wages_paid_today = 0.0f;

    /* Housing tier determines sleep recovery & stress relief */
    if (p->housing == HOUSING_TENEMENT) {
        p->stamina = grind_clampf(p->stamina + 60.0f, 0.0f, 100.0f);
        p->stress = grind_clampf(p->stress - 12.0f, 0.0f, 100.0f);
    } else if (p->housing == HOUSING_STUDIO_APT) {
        p->stamina = grind_clampf(p->stamina + 88.0f, 0.0f, 100.0f);
        p->stress = grind_clampf(p->stress - 40.0f, 0.0f, 100.0f);
    } else {
        p->stamina = 100.0f;
        p->stress = grind_clampf(p->stress - 85.0f, 0.0f, 100.0f);
    }

    /* Weather roll for new day */
    uint32_t wroll = grind_rng_u32(&world->rng) % 10;
    if (wroll < 4)      { world->weather = WEATHER_SUNNY;  world->ambient_temp_c = 26.0f; }
    else if (wroll < 7) { world->weather = WEATHER_HOT;    world->ambient_temp_c = 32.0f; }
    else if (wroll < 9) { world->weather = WEATHER_CLOUDY; world->ambient_temp_c = 20.0f; }
    else                { world->weather = WEATHER_RAINY;  world->ambient_temp_c = 16.0f; }

    grind_update_social_reactions(world);
    char buf[GRIND_MAX_LOG_LEN];
    snprintf(buf, sizeof(buf), "Day %d dawned! Passive income: +$%.2f. Ready to grind.",
             world->day_index, div + idx_yield);
    set_status(world, buf);
}

bool grind_action_context_interact(GrindWorld *world) {
    GrindPlayer *p = &world->player;

    /* 1. Pick up initial found money if close */
    if (world->ground_money_available &&
        vec3_dist_xz(p->pos, world->ground_money_pos) <= 8.0f) {
        return grind_action_pickup_ground_money(world);
    }

    /* 2. Check active courier parcel deliveries or pickups */
    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        GrindCourierContract *c = &world->contracts[i];
        if (!c->active) continue;
        if (c->claimed_by_player && c->picked_up &&
            vec3_dist_xz(p->pos, c->dropoff_pos) <= 12.0f) {
            return grind_action_deliver_contract_parcel(world, i);
        }
        if (c->claimed_by_player && !c->picked_up &&
            vec3_dist_xz(p->pos, c->pickup_pos) <= 12.0f) {
            return grind_action_pickup_contract_parcel(world, i);
        }
    }

    /* 3. Check proximity to named NPCs for social interaction & relationship choices */
    for (int i = 0; i < 4; ++i) {
        if (vec3_dist_xz(p->pos, world->npcs[i].pos) <= 5.5f) {
            if (i == 1 && world->npcs[1].envy > 0.25f && !world->rival_allied_franchise && p->cash >= 80.0f) {
                return grind_action_social_choice(world, 1, 1);
            }
            if (i == 0 && p->stress > 30.0f && p->cash >= 15.0f) {
                return grind_action_social_choice(world, 0, 1);
            }
            if (i == 2 && p->perceived_wealth > 25.0f) {
                return grind_action_social_choice(world, 2, 1);
            }
            return grind_action_talk_to_npc(world, i);
        }
    }

    /* 4. Check proximity to city buildings for contextual business actions */
    if (vec3_dist_xz(p->pos, world->buildings[ZONE_WHOLESALE_MARKET].pos) <= 16.0f) {
        if (!p->stand_deployed && p->inventory[ITEM_FOLDING_TABLE] == 0 && p->cash >= 4.0f) {
            grind_action_buy_item(world, ITEM_FOLDING_TABLE, 1);
        }
        /* Buy lemonade batch bundle (2 lemons, 2 sugar, 2 water, 2 cup packs) */
        grind_action_buy_item(world, ITEM_LEMON, 2);
        grind_action_buy_item(world, ITEM_SUGAR, 2);
        grind_action_buy_item(world, ITEM_WATER_JUG, 1);
        grind_action_buy_item(world, ITEM_PAPER_CUPS, 2);
        return true;
    }

    if (vec3_dist_xz(p->pos, world->buildings[ZONE_CENTRAL_PLAZA].pos) <= 18.0f) {
        if (!p->stand_deployed && p->inventory[ITEM_FOLDING_TABLE] > 0) {
            return grind_action_deploy_stand(world);
        }
        if (p->stand_deployed) {
            return grind_action_craft_lemonade(world, 2);
        }
    }

    if (vec3_dist_xz(p->pos, world->buildings[ZONE_BIKE_VEHICLE_SHOP].pos) <= 16.0f) {
        if (!p->has_satchel && p->cash >= 25.0f) return grind_action_buy_satchel(world);
        if (p->vehicle == VEHICLE_NONE && p->cash >= 85.0f) return grind_action_buy_vehicle(world, VEHICLE_BICYCLE);
        if (p->vehicle == VEHICLE_BICYCLE && p->cash >= 320.0f) return grind_action_buy_vehicle(world, VEHICLE_CARGO_EBIKE);
        if (p->vehicle == VEHICLE_CARGO_EBIKE && p->cash >= 1200.0f) return grind_action_buy_vehicle(world, VEHICLE_DELIVERY_VAN);
    }

    if (vec3_dist_xz(p->pos, world->buildings[ZONE_DISPATCH_HUB].pos) <= 16.0f) {
        for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
            if (world->contracts[i].active && !world->contracts[i].claimed_by_player) {
                grind_action_claim_contract(world, i);
                if (vec3_dist_xz(p->pos, world->contracts[i].pickup_pos) <= 16.0f) {
                    grind_action_pickup_contract_parcel(world, i);
                }
                return true;
            }
        }
    }

    if (vec3_dist_xz(p->pos, world->buildings[ZONE_BANK_FINANCE].pos) <= 16.0f) {
        if (!p->owns_storefront && p->cash >= 350.0f) return grind_action_lease_storefront(world);
        if (p->cash >= 200.0f) return grind_action_invest_supplier_equity(world, 200.0f);
    }

    if (vec3_dist_xz(p->pos, world->buildings[ZONE_BOUTIQUE_LUXURY].pos) <= 16.0f) {
        if (p->apparel < APPAREL_TAILORED_SUIT && p->cash >= 400.0f) {
            return grind_action_buy_apparel(world, APPAREL_TAILORED_SUIT);
        }
        if (p->housing < HOUSING_STUDIO_APT && p->cash >= 250.0f) {
            return grind_action_upgrade_housing(world, HOUSING_STUDIO_APT);
        }
        if (p->housing < HOUSING_PENTHOUSE_VILLA && p->cash >= 2500.0f) {
            return grind_action_upgrade_housing(world, HOUSING_PENTHOUSE_VILLA);
        }
        if (p->vehicle < VEHICLE_SPORTS_CAR && p->cash >= 4500.0f) {
            return grind_action_buy_vehicle(world, VEHICLE_SPORTS_CAR);
        }
    }

    set_status(world, "Walk closer to cash, a market, stand, parcel, or NPC to interact [E].");
    return false;
}

void grind_world_tick(GrindWorld *world, float sim_minutes) {
    if (sim_minutes <= 0.0f) return;
    float dt_hours = sim_minutes / 60.0f;
    GrindPlayer *p = &world->player;

    world->time_of_day_hours += dt_hours;
    world->anim_time_sec += sim_minutes * 0.6f;
    if (world->sale_popup_timer > 0.0f) {
        world->sale_popup_timer = grind_clampf(world->sale_popup_timer - sim_minutes * 0.25f, 0.0f, 5.0f);
        world->sale_popup_pos.y += sim_minutes * 0.12f;
    }
    if (world->time_of_day_hours >= 23.5f) {
        grind_action_sleep_overnight(world);
        return;
    }

    /* Update ambient temperature curve across the day */
    float day_phase = (world->time_of_day_hours - 6.0f) / 16.0f;
    float temp_wave = sinf(grind_clampf(day_phase, 0.0f, 1.0f) * GRIND_PI);
    float base_t = (world->weather == WEATHER_HOT) ? 24.0f : (world->weather == WEATHER_RAINY ? 14.0f : 19.0f);
    world->ambient_temp_c = base_t + 9.0f * temp_wave;

    /* Player bio-energetics & spoilage */
    p->hunger = grind_clampf(p->hunger + dt_hours * 3.5f, 0.0f, 100.0f);
    if (world->time_of_day_hours > 18.5f) {
        p->stress = grind_clampf(p->stress + dt_hours * 2.2f, 0.0f, 100.0f);
    }
    if (world->weather == WEATHER_RAINY && !p->owns_storefront) {
        p->stress = grind_clampf(p->stress + dt_hours * 1.8f, 0.0f, 100.0f);
    }

    /* Envious Peer (Devon) opens a competing street stand once player gains traction */
    if (!world->rival_stand_active && (p->cups_sold_total >= 15 || world->npcs[1].envy >= 0.30f)) {
        world->rival_stand_active = true;
    }
    if (p->meal_speed_buff_hours > 0.0f) {
        p->meal_speed_buff_hours = grind_clampf(p->meal_speed_buff_hours - dt_hours, 0.0f, 10.0f);
    }
    for (int i = 0; i < ITEM_COUNT; ++i) {
        if (p->inventory[i] > 0 && k_item_defs[i].spoilage_per_hour > 0.0f) {
            p->item_freshness[i] = grind_clampf(
                p->item_freshness[i] - k_item_defs[i].spoilage_per_hour * dt_hours,
                0.15f, 1.0f
            );
        }
    }

    /* Hired Employees execute their delegated Task Graphs */
    for (int i = 0; i < world->employee_count; ++i) {
        GrindEmployee *e = &world->employees[i];
        if (!e->hired) continue;

        float wage = e->wage_per_hour * dt_hours;
        if (p->cash >= wage) {
            p->cash -= wage;
            world->wages_paid_today += wage;
        } else {
            e->morale = grind_clampf(e->morale - 0.1f * dt_hours, 0.1f, 1.0f);
        }

        float eff = e->skill * e->morale * (1.0f + 0.05f * p->skill_management);
        e->task_progress += dt_hours * eff;

        if (e->role == EMP_ROLE_STAND_OPERATOR && (p->stand_deployed || p->owns_storefront)) {
            /* Automatically restocks & crafts lemonade when stock runs low */
            if (p->stand_lemonade_stock < 5) {
                float restock_cost = effective_item_cost(world, ITEM_LEMON) +
                                     effective_item_cost(world, ITEM_SUGAR) +
                                     effective_item_cost(world, ITEM_WATER_JUG) +
                                     effective_item_cost(world, ITEM_PAPER_CUPS);
                if (p->cash >= restock_cost) {
                    p->cash -= restock_cost;
                    p->stand_lemonade_stock += 5;
                }
            }
        } else if (e->role == EMP_ROLE_COURIER) {
            /* Move courier employee visually around city and complete automated deliveries */
            Vec3 to_target = vec3_sub(e->target_pos, e->pos);
            float d = vec3_len(to_target);
            if (d < 3.0f) {
                int next_z = (int)(grind_rng_u32(&world->rng) % ZONE_COUNT);
                e->target_pos = world->buildings[next_z].pos;
            } else {
                Vec3 dir = vec3_norm(to_target);
                e->facing_yaw = atan2f(dir.x, -dir.z);
                Vec3 step = vec3_scale(dir, 12.0f * dt_hours * 4.0f);
                e->pos = vec3_add(e->pos, step);
                e->walk_phase += vec3_len(step) * 1.8f;
            }
            if (e->task_progress >= 0.45f) {
                e->task_progress -= 0.45f;
                float rev = 11.50f * eff;
                p->cash += rev;
                e->revenue_generated_today += rev;
                world->revenue_today += rev;
                p->deliveries_completed_total++;
            }
        }
    }

    /* NPC kinematics, thirst growth, and first-principles customer utility purchases */
    for (int i = 0; i < world->npc_count; ++i) {
        GrindNpc *npc = &world->npcs[i];
        npc->thirst = grind_clampf(npc->thirst + dt_hours * 0.18f * (world->ambient_temp_c / 22.0f), 0.0f, 1.0f);
        if (npc->drink_timer > 0.0f) {
            npc->drink_timer = grind_clampf(npc->drink_timer - dt_hours, 0.0f, 2.0f);
        }

        Vec3 diff = vec3_sub(npc->target_pos, npc->pos);
        float dist = vec3_len(diff);
        if (dist < 2.0f) {
            /* Pick a new destination weighted toward Central Plaza */
            if ((grind_rng_u32(&world->rng) % 100) < 65) {
                npc->target_pos = vec3_make(
                    grind_rng_range(&world->rng, -16.0f, 16.0f),
                    0.0f,
                    grind_rng_range(&world->rng, -16.0f, 16.0f)
                );
            } else {
                npc->target_pos = vec3_make(
                    grind_rng_range(&world->rng, -48.0f, 48.0f),
                    0.0f,
                    grind_rng_range(&world->rng, -48.0f, 48.0f)
                );
            }
        } else {
            Vec3 ndir = vec3_norm(diff);
            npc->facing_yaw = atan2f(ndir.x, -ndir.z);
            Vec3 step = vec3_scale(ndir, npc->walk_speed * (sim_minutes * 0.8f));
            if (vec3_len(step) > dist) step = diff;
            npc->pos = vec3_add(npc->pos, step);
            npc->walk_phase += vec3_len(step) * 2.2f;
        }

        /* Evaluate Point-of-Sale if stand is deployed and stocked */
        if ((p->stand_deployed || p->owns_storefront) && p->stand_lemonade_stock > 0 && npc->thirst >= 0.25f) {
            float detect_radius = p->owns_storefront ? 28.0f : 18.0f;
            detect_radius += p->marketing_awareness * 22.0f;
            if (vec3_dist_xz(npc->pos, p->stand_pos) <= detect_radius) {
                float u = grind_eval_customer_utility(world, npc, p->stand_price, p->stand_lemonade_quality);
                float u_rival = -10.0f;
                if (world->rival_stand_active && !world->rival_allied_franchise) {
                    /* Rival has lower brand awareness (-0.12 offset), so fair price or good quality beats rival */
                    u_rival = grind_eval_customer_utility(world, npc, world->rival_stand_price, world->rival_stand_quality) - 0.12f;
                }
                if (u > 0.0f && u >= u_rival && npc->cash >= p->stand_price) {
                    npc->cash -= p->stand_price;
                    npc->thirst = grind_clampf(npc->thirst - 0.65f, 0.0f, 1.0f);
                    npc->drink_timer = 0.8f;
                    p->cash += p->stand_price;
                    world->revenue_today += p->stand_price;
                    world->sound_events_mask |= GRIND_SFX_SALE;
                    p->stand_lemonade_stock--;
                    p->cups_sold_total++;
                    if (p->cups_sold_total >= 10) {
                        p->unlocked_insights |= GRIND_INSIGHT_DEMAND_ELASTICITY;
                    }
                    if (p->skill_persuasion >= 0.5f) {
                        p->unlocked_insights |= GRIND_INSIGHT_COMPETITOR_INTEL;
                    }
                    p->skill_persuasion = grind_clampf(p->skill_persuasion + 0.03f, 0.0f, 10.0f);
                    p->business_reputation = grind_clampf(p->business_reputation + 0.35f, 0.0f, 100.0f);
                    world->sale_popup_timer = 2.2f;
                    world->sale_popup_pos = vec3_make(p->stand_pos.x, 3.0f, p->stand_pos.z);
                    world->sale_popup_amount = p->stand_price;
                } else if (world->rival_stand_active && !world->rival_allied_franchise && u_rival > 0.0f && u_rival > u) {
                    /* Customer chose the rival stand because player's price/quality utility was worse! */
                    npc->thirst = grind_clampf(npc->thirst - 0.55f, 0.0f, 1.0f);
                    world->rival_cups_sold++;
                    p->unlocked_insights |= GRIND_INSIGHT_COMPETITOR_INTEL;
                } else if (world->rival_stand_active && world->rival_allied_franchise && u > 0.0f) {
                    /* Allied franchise partner Devon generates passive royalty ($0.75/cup) when stand is out of stock */
                    npc->thirst = grind_clampf(npc->thirst - 0.55f, 0.0f, 1.0f);
                    p->cash += 0.75f;
                    world->revenue_today += 0.75f;
                    world->rival_cups_sold++;
                }
            }
        }
    }
}
