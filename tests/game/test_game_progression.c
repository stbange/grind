#include "grind_core/grind_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Helper: Walk/Ride player in 3D space toward target destination while ticking world time */
static void bot_travel_to(GrindWorld *w, Vec3 target, float max_seconds) {
    float elapsed = 0.0f;
    const float dt = 0.25f;
    while (vec3_dist_xz(w->player.pos, target) > 3.5f && elapsed < max_seconds) {
        if (w->player.stamina < 25.0f) {
            if (w->player.inventory[ITEM_FRESH_MEAL] > 0) {
                grind_action_consume_item(w, ITEM_FRESH_MEAL);
            } else if (w->player.inventory[ITEM_STREET_SNACK] > 0) {
                grind_action_consume_item(w, ITEM_STREET_SNACK);
            } else {
                grind_action_sleep_overnight(w);
            }
        }
        Vec3 diff = vec3_sub(target, w->player.pos);
        w->player.yaw_rad = atan2f(diff.x, -diff.z);
        grind_player_move(w, 1.0f, 0.0f, dt);
        grind_world_tick(w, dt * 1.5f);
        elapsed += dt;
    }
}

int main(void) {
    printf("=== Automated Full-Game Progression Test: Rags to Business Empire ===\n");
    GrindWorld w;
    grind_world_init(&w, 0xBEEF2026ULL);

    /* -------------------------------------------------------------------------
     * STAGE 1: Broke Start -> Find Money on Ground -> Open Lemonade Stand
     * ------------------------------------------------------------------------- */
    printf("[Stage 1] Starting cash: $%.2f. Walking to found money...\n", w.player.cash);
    bot_travel_to(&w, w.ground_money_pos, 30.0f);
    if (!grind_action_pickup_ground_money(&w) || w.player.cash < 10.0f) {
        fprintf(stderr, "FAIL: Could not pick up ground money.\n");
        return 1;
    }
    printf("  -> Picked up $%.2f! Walking to Wholesale Market...\n", w.player.cash);

    bot_travel_to(&w, w.buildings[ZONE_WHOLESALE_MARKET].pos, 60.0f);
    /* First trip: buy the Folding Table (2.0kg, 1.8L) and 1 starter batch of ingredients */
    grind_action_buy_item(&w, ITEM_FOLDING_TABLE, 1);
    grind_action_buy_item(&w, ITEM_LEMON, 1);
    grind_action_buy_item(&w, ITEM_SUGAR, 1);
    grind_action_buy_item(&w, ITEM_WATER_JUG, 1);
    grind_action_buy_item(&w, ITEM_PAPER_CUPS, 1);

    bot_travel_to(&w, w.buildings[ZONE_CENTRAL_PLAZA].pos, 60.0f);
    if (!grind_action_deploy_stand(&w)) {
        fprintf(stderr, "FAIL: Could not deploy lemonade stand.\n");
        return 1;
    }
    grind_action_craft_lemonade(&w, 1);

    /* Grind the lemonade stand until player has >= $115 for Satchel ($25) + Bicycle ($85) */
    int guard = 0;
    while (w.player.cash < 120.0f && guard++ < 40) {
        if (w.player.stand_lemonade_stock < 5) {
            bot_travel_to(&w, w.buildings[ZONE_WHOLESALE_MARKET].pos, 60.0f);
            /* Bare hands can carry 2 batches (2 lemons, 2 sugar, 2 water jugs = 4.0kg/4.3L < 6kg/5L) */
            grind_action_buy_item(&w, ITEM_LEMON, 2);
            grind_action_buy_item(&w, ITEM_SUGAR, 2);
            grind_action_buy_item(&w, ITEM_WATER_JUG, 2);
            grind_action_buy_item(&w, ITEM_PAPER_CUPS, 2);
            if (w.player.stamina < 40.0f && w.player.cash >= 5.0f) {
                grind_action_buy_item(&w, ITEM_STREET_SNACK, 1);
                grind_action_consume_item(&w, ITEM_STREET_SNACK);
            }
            bot_travel_to(&w, w.player.stand_pos, 60.0f);
            grind_action_craft_lemonade(&w, 2);
        }
        /* Wait at stand serving customers */
        for (int t = 0; t < 12; ++t) {
            grind_world_tick(&w, 15.0f);
        }
    }
    printf("  -> Stage 1 Complete! Cups sold: %d, Cash: $%.2f, Prod Skill: %.2f\n",
           w.player.cups_sold_total, w.player.cash, w.player.skill_production);
    if (w.player.cups_sold_total < 15 || w.player.cash < 110.0f) {
        fprintf(stderr, "FAIL: Lemonade stand stage did not reach target profit.\n");
        return 1;
    }

    /* -------------------------------------------------------------------------
     * STAGE 2: Buy Satchel & Bicycle -> Start Courier Service
     * ------------------------------------------------------------------------- */
    printf("[Stage 2] Buying Courier Satchel & Bicycle at Metro Bike Shop...\n");
    bot_travel_to(&w, w.buildings[ZONE_BIKE_VEHICLE_SHOP].pos, 60.0f);
    if (!grind_action_buy_satchel(&w) || !grind_action_buy_vehicle(&w, VEHICLE_BICYCLE)) {
        fprintf(stderr, "FAIL: Could not purchase Satchel and Bicycle.\n");
        return 1;
    }

    /* Complete courier contracts by physically riding bike between pickup & dropoff buildings */
    for (int run = 0; run < 18; ++run) {
        int c_idx = run % GRIND_MAX_CONTRACTS;
        grind_action_claim_contract(&w, c_idx);
        bot_travel_to(&w, w.contracts[c_idx].pickup_pos, 60.0f);
        if (!grind_action_pickup_contract_parcel(&w, c_idx)) {
            fprintf(stderr, "FAIL: Could not pick up parcel on run %d.\n", run);
            return 1;
        }
        bot_travel_to(&w, w.contracts[c_idx].dropoff_pos, 60.0f);
        if (!grind_action_deliver_contract_parcel(&w, c_idx)) {
            fprintf(stderr, "FAIL: Could not deliver parcel on run %d.\n", run);
            return 1;
        }
    }
    printf("  -> Stage 2 Complete! Deliveries: %d, Cash: $%.2f, Logistics Skill: %.2f\n",
           w.player.deliveries_completed_total, w.player.cash, w.player.skill_logistics);

    /* -------------------------------------------------------------------------
     * STAGE 3: Expand Empire (Hiring, Marketing, Storefront, Investing) & Good Life
     * ------------------------------------------------------------------------- */
    printf("[Stage 3] Scaling into a Business Empire (Hiring, Marketing, Investing)...\n");
    if (!grind_action_hire_employee(&w, EMP_ROLE_STAND_OPERATOR) ||
        !grind_action_hire_employee(&w, EMP_ROLE_COURIER) ||
        !grind_action_hire_employee(&w, EMP_ROLE_COURIER)) {
        fprintf(stderr, "FAIL: Failed to hire stand operator and courier staff.\n");
        return 1;
    }
    grind_action_launch_marketing_campaign(&w, 1);

    /* Run multi-day empire operations + reinvest in Storefront & Supplier Equity */
    for (int day_loop = 0; day_loop < 16; ++day_loop) {
        if (!w.player.owns_storefront && w.player.cash >= 360.0f) {
            grind_action_lease_storefront(&w);
        }
        if (w.player.supplier_equity_share < 0.50f && w.player.cash >= 250.0f) {
            grind_action_invest_supplier_equity(&w, 200.0f);
        }
        if (w.employee_count < 6 && w.player.cash >= 150.0f) {
            grind_action_hire_employee(&w, EMP_ROLE_COURIER);
        }
        if (w.player.cash >= 400.0f) {
            grind_action_invest_index_fund(&w, 250.0f);
        }
        for (int h = 0; h < 12; ++h) {
            grind_world_tick(&w, 60.0f);
        }
        grind_action_sleep_overnight(&w);
    }

    /* Buy "The Good Things in Life" (Tailored Suit, Luxury Watch, Penthouse, Sports Car) */
    if (w.player.index_fund_invested > 0.0f) {
        /* Liquidate part of index fund if needed to buy personal luxury assets */
        w.player.cash += w.player.index_fund_invested;
        w.player.index_fund_invested = 0.0f;
    }
    grind_action_buy_apparel(&w, APPAREL_TAILORED_SUIT);
    grind_action_buy_luxury_watch(&w);
    grind_action_upgrade_housing(&w, HOUSING_PENTHOUSE_VILLA);
    if (w.player.cash >= 4500.0f) {
        grind_action_buy_vehicle(&w, VEHICLE_SPORTS_CAR);
    }
    grind_update_social_reactions(&w);

    float final_worth = grind_calc_net_worth(&w);
    printf("  -> Final Day: %d | Net Worth: $%.2f | Perceived Wealth: %.1f/100 | Staff: %d\n",
           w.day_index, final_worth, w.player.perceived_wealth, w.employee_count);
    printf("  -> Social Reactions:\n");
    for (int i = 0; i < 4; ++i) {
        printf("     * %s -> Affection: %.2f, Respect: %.2f, Envy: %.2f | %s\n",
               w.npcs[i].name, w.npcs[i].affection, w.npcs[i].respect, w.npcs[i].envy, w.npcs[i].last_reaction);
    }

    if (final_worth < 3000.0f || w.player.perceived_wealth < 50.0f || w.npcs[1].envy < 0.40f) {
        fprintf(stderr, "FAIL: Final empire net worth or social reaction metrics below threshold.\n");
        return 1;
    }

    printf("=== Automated Game Progression Test PASSED! ===\n");
    return 0;
}
