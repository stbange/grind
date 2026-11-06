#include "grind_core/grind_core.h"
#include "grind_app/grind_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_NEAR(a, b, eps, msg) do { \
    if (fabsf((a) - (b)) > (eps)) { \
        fprintf(stderr, "  [FAIL] %s:%d: %s (got %.4f, expected %.4f)\n", __FILE__, __LINE__, msg, (double)(a), (double)(b)); \
        g_tests_failed++; \
        return; \
    } \
} while (0)

#define RUN_TEST(fn) do { \
    int fails_before = g_tests_failed; \
    g_tests_run++; \
    fn(); \
    if (g_tests_failed == fails_before) { \
        printf("  [PASS] %s\n", #fn); \
    } \
} while (0)

/* 1. Test physical volume/mass container limits, satchel, bicycle, and encumbrance */
static void test_physics_and_containers(void) {
    GrindWorld w;
    grind_world_init(&w, 42ULL);

    /* Bare hands start with 6kg mass / 5L volume capacity */
    ASSERT_NEAR(grind_player_max_mass_kg(&w), 6.0f, 0.01f, "Initial bare-hand mass limit");
    ASSERT_NEAR(grind_player_max_volume_l(&w), 5.0f, 0.01f, "Initial bare-hand volume limit");

    w.player.cash = 500.0f;
    /* Try buying 5 Water Jugs (7.5kg, 7.5L) with bare hands -> must fail physical capacity */
    bool bought_over_cap = grind_action_buy_item(&w, ITEM_WATER_JUG, 5);
    ASSERT_TRUE(!bought_over_cap, "Cannot carry 5 water jugs in bare hands");

    /* Buying Satchel (+18kg, +30L) enables carrying bulk goods */
    bool satchel_ok = grind_action_buy_satchel(&w);
    ASSERT_TRUE(satchel_ok && w.player.has_satchel, "Satchel purchase succeeds");
    ASSERT_NEAR(grind_player_max_mass_kg(&w), 24.0f, 0.01f, "Satchel expands mass capacity");
    ASSERT_NEAR(grind_player_max_volume_l(&w), 35.0f, 0.01f, "Satchel expands volume capacity");

    bool bought_with_satchel = grind_action_buy_item(&w, ITEM_WATER_JUG, 5);
    ASSERT_TRUE(bought_with_satchel, "Can carry 5 water jugs once Satchel is equipped");

    /* Verify encumbrance reduces walking speed when heavily loaded */
    float speed_loaded = grind_player_effective_move_speed(&w);
    bool bike_ok = grind_action_buy_vehicle(&w, VEHICLE_BICYCLE);
    ASSERT_TRUE(bike_ok && w.player.vehicle == VEHICLE_BICYCLE, "Bicycle purchase succeeds");
    float speed_bike = grind_player_effective_move_speed(&w);
    ASSERT_TRUE(speed_bike > speed_loaded * 2.0f, "Bicycle multiplies movement speed");
}

/* 2. Test first-principles crafting, customer utility pricing, and weather impact */
static void test_economy_and_crafting(void) {
    GrindWorld w;
    grind_world_init(&w, 1001ULL);

    /* Pick up the found money near start */
    w.player.pos = w.ground_money_pos;
    ASSERT_TRUE(grind_action_pickup_ground_money(&w), "Picked up found money on street");
    ASSERT_NEAR(w.player.cash, 12.0f, 0.01f, "Found $12.00 cash");

    /* Buy Folding Table and raw lemonade ingredients */
    ASSERT_TRUE(grind_action_buy_item(&w, ITEM_FOLDING_TABLE, 1), "Bought Folding Table");
    ASSERT_TRUE(grind_action_deploy_stand(&w), "Deployed Lemonade Stand");
    ASSERT_TRUE(grind_action_buy_item(&w, ITEM_LEMON, 2), "Bought Lemons");
    ASSERT_TRUE(grind_action_buy_item(&w, ITEM_SUGAR, 2), "Bought Sugar");
    ASSERT_TRUE(grind_action_buy_item(&w, ITEM_WATER_JUG, 2), "Bought Water");
    ASSERT_TRUE(grind_action_buy_item(&w, ITEM_PAPER_CUPS, 2), "Bought Cups");

    ASSERT_TRUE(grind_action_craft_lemonade(&w, 2), "Crafted 2 batches (10 cups) of lemonade");
    ASSERT_TRUE(w.player.stand_lemonade_stock == 10, "Stand holds 10 cups of lemonade");
    ASSERT_TRUE(w.player.skill_production > 0.2f, "Crafting increases production skill");

    /* Verify customer utility responds to price, temperature, and recipe mixing */
    float q_basic = w.player.stand_lemonade_quality;
    grind_action_set_recipe(&w, 2, 2);
    float q_rich = w.player.stand_lemonade_quality;
    ASSERT_TRUE(q_rich > q_basic, "Richer 2-lemon/2-sugar recipe increases lemonade quality");

    /* Verify experiential business insights (Unit Economics COGS & Gross Margin) */
    ASSERT_TRUE((w.player.unlocked_insights & GRIND_INSIGHT_UNIT_ECONOMICS) != 0,
                "Crafting batches unlocks Unit Economics insight");
    float cogs = grind_player_unit_cost_per_cup(&w);
    float margin = grind_player_profit_margin_pct(&w);
    ASSERT_TRUE(cogs > 0.15f && cogs < 1.20f, "First-principles COGS per cup is positive and bounded");
    ASSERT_TRUE(margin > 20.0f, "Gross profit margin percentage is positive at default price");

    /* Verify Rival Stand competition: if rival is active and player price-gouges, rival wins customer */
    w.rival_stand_active = true;
    w.player.stand_pos = vec3_make(0.0f, 0.0f, 0.0f);
    w.npcs[6].pos = vec3_make(1.0f, 0.0f, 1.0f);
    w.npcs[6].thirst = 0.90f;
    grind_action_set_lemonade_price(&w, 9.50f); /* Gouge price so customer prefers rival ($1.45) */
    int rival_before = w.rival_cups_sold;
    grind_world_tick(&w, 5.0f);
    ASSERT_TRUE(w.rival_cups_sold > rival_before, "Customers defect to Rival Stand when player price-gouges");
    ASSERT_TRUE((w.player.unlocked_insights & GRIND_INSIGHT_COMPETITOR_INTEL) != 0,
                "Losing a sale to a competitor unlocks Competitor Intel insight");
    grind_action_set_lemonade_price(&w, 1.60f);

    GrindNpc *customer = &w.npcs[5];
    customer->thirst = 0.85f;
    w.ambient_temp_c = 30.0f;
    w.weather = WEATHER_HOT;

    float u_fair = grind_eval_customer_utility(&w, customer, 1.50f, 0.80f);
    float u_gouge = grind_eval_customer_utility(&w, customer, 14.00f, 0.80f);
    ASSERT_TRUE(u_fair > 0.0f, "Thirsty NPC on hot day has positive utility at $1.50");
    ASSERT_TRUE(u_gouge < 0.0f, "NPC rejects overpriced $14.00 lemonade");
}

/* 3. Test courier contracts and employee task-graph delegation */
static void test_courier_and_delegation(void) {
    GrindWorld w;
    grind_world_init(&w, 777ULL);
    w.player.cash = 300.0f;
    grind_action_buy_satchel(&w);
    grind_action_buy_vehicle(&w, VEHICLE_BICYCLE);

    /* Claim, pick up, and deliver Contract #0 */
    ASSERT_TRUE(grind_action_claim_contract(&w, 0), "Claimed parcel contract #0");
    ASSERT_TRUE(grind_action_pickup_contract_parcel(&w, 0), "Picked up parcel #0 into satchel/bike");
    float cash_before = w.player.cash;
    ASSERT_TRUE(grind_action_deliver_contract_parcel(&w, 0), "Delivered parcel #0");
    ASSERT_TRUE(w.player.cash > cash_before, "Courier delivery pays reward");
    ASSERT_TRUE(w.player.deliveries_completed_total == 1, "Delivery counter incremented");

    /* Hire a Courier Employee and verify autonomous revenue generation */
    ASSERT_TRUE(grind_action_hire_employee(&w, EMP_ROLE_COURIER), "Hired Courier Employee");
    int deliv_before = w.player.deliveries_completed_total;
    for (int t = 0; t < 20; ++t) {
        grind_world_tick(&w, 15.0f); /* 5 hours simulated */
    }
    ASSERT_TRUE(w.player.deliveries_completed_total > deliv_before, "Hired courier autonomously completes deliveries");
}

/* 4. Test social relationship reactions to growing wealth and personal rewards */
static void test_social_and_lifestyle(void) {
    GrindWorld w;
    grind_world_init(&w, 999ULL);

    /* When broke: Perceived wealth is 0, Devon (Envious Peer) has low envy, Chloe (Opportunist) is cold */
    grind_update_social_reactions(&w);
    float devon_envy_broke = w.npcs[1].envy;
    float chloe_aff_broke = w.npcs[2].affection;
    float banker_resp_broke = w.npcs[3].respect;
    ASSERT_NEAR(w.player.perceived_wealth, 0.0f, 0.01f, "Broke player has 0 perceived wealth");
    ASSERT_TRUE(!grind_action_request_bank_loan(&w, 200.0f), "Banker rejects loan when player has no status/collateral");

    /* Player gets rich and buys Tailored Suit, Luxury Watch, Penthouse, and Sports Car */
    w.player.cash = 15000.0f;
    ASSERT_TRUE(grind_action_buy_apparel(&w, APPAREL_TAILORED_SUIT), "Bought Tailored Suit");
    ASSERT_TRUE(grind_action_buy_luxury_watch(&w), "Bought Swiss Watch");
    ASSERT_TRUE(grind_action_upgrade_housing(&w, HOUSING_PENTHOUSE_VILLA), "Bought Penthouse Villa");
    ASSERT_TRUE(grind_action_buy_vehicle(&w, VEHICLE_SPORTS_CAR), "Bought Sports Car");

    grind_update_social_reactions(&w);
    ASSERT_TRUE(w.player.perceived_wealth >= 80.0f, "Perceived wealth reflects luxury attire, housing & sports car");
    ASSERT_TRUE(w.npcs[1].envy > devon_envy_broke + 0.50f, "Envious peer's envy spikes as player displays wealth");
    ASSERT_TRUE(w.npcs[2].affection > chloe_aff_broke + 0.80f, "Opportunist NPC's affection spikes when player is rich");
    ASSERT_TRUE(w.npcs[3].respect > banker_resp_broke + 0.35f, "Banker respect increases with visible status");
    ASSERT_TRUE(grind_action_request_bank_loan(&w, 500.0f), "Banker approves B2B loan for high-status player");
}

/* 5. Test First-Person + Top-Down Map camera & automatic small-device on-screen touch controls */
static void test_adaptive_input_and_camera(void) {
    GrindWorld w;
    grind_world_init(&w, 123ULL);
    GrindInputState in;

    /* Desktop viewport (1280x720) starts with touch controls OFF and First-Person camera */
    grind_input_init(&in, 1280, 720, false);
    ASSERT_TRUE(!in.touch_controls_active, "Desktop 1280x720 hides touch controls initially");
    ASSERT_TRUE(in.camera_mode == CAMERA_MODE_FIRST_PERSON, "Starts in First-Person 3D camera mode");

    /* Resizing to mobile viewport (390x844) automatically activates on-screen controls */
    grind_input_resize(&in, 390, 844);
    ASSERT_TRUE(in.touch_controls_active, "Small mobile viewport (<900px) automatically enables on-screen controls");

    /* Simulate dragging the virtual left joystick forward */
    Vec3 start_pos = w.player.pos;
    grind_input_begin_frame(&in);
    grind_input_on_touch_down(&in, 1, in.joy_origin_x, in.joy_origin_y - in.joy_radius * 0.8f);
    ASSERT_TRUE(in.joy_active && in.move_forward > 0.5f, "Virtual joystick produces forward movement vector");
    grind_input_apply_to_world(&in, &w, 0.5f);
    ASSERT_TRUE(vec3_dist_xz(start_pos, w.player.pos) > 0.5f, "Virtual joystick moves player in 3D world");
    grind_input_on_touch_up(&in, 1, in.joy_origin_x, in.joy_origin_y);

    /* Simulate tapping the on-screen [MAP / M] button to toggle Top-Down 3D Management Map */
    grind_input_begin_frame(&in);
    grind_input_on_touch_down(&in, 2, in.buttons[TOUCH_BTN_MAP_VIEW].cx, in.buttons[TOUCH_BTN_MAP_VIEW].cy);
    grind_input_apply_to_world(&in, &w, 0.5f);
    ASSERT_TRUE(in.camera_mode == CAMERA_MODE_TOP_DOWN_MAP, "Tapping MAP touch button switches to Top-Down Map view");
    ASSERT_TRUE(in.map_transition_t > 0.0f, "Camera transition blends toward overhead map matrix");

    /* Test 3D world-to-screen projection for world-space labels & speech bubbles */
    grind_input_init(&in, 1280, 720, false);
    w.player.pos = vec3_make(0.0f, 0.0f, 10.0f);
    w.player.yaw_rad = 0.0f; /* Looking toward -Z (0, 0, 0) */
    w.player.pitch_rad = 0.0f;
    Mat4 vp = mat4_mul(grind_input_compute_proj_matrix(&in), grind_input_compute_view_matrix(&in, &w));
    Vec2 scr_front = {0};
    bool front_ok = grind_project_world_to_screen(vec3_make(0.0f, 1.68f, 0.0f), vp, 1280, 720, &scr_front);
    ASSERT_TRUE(front_ok, "Point directly in front of First-Person camera projects onto screen");
    ASSERT_NEAR(scr_front.x, 640.0f, 5.0f, "Centered 3D point projects to horizontal screen center");

    bool behind_ok = grind_project_world_to_screen(vec3_make(0.0f, 1.68f, 25.0f), vp, 1280, 720, NULL);
    ASSERT_TRUE(!behind_ok, "Point behind First-Person camera is culled by projection");

    /* Verify NPC walk_phase advances during simulation tick */
    float phase_before = w.npcs[0].walk_phase;
    grind_world_tick(&w, 5.0f);
    ASSERT_TRUE(w.npcs[0].walk_phase > phase_before, "NPC articulated walk_phase advances as NPC walks");

    /* Verify interactive Business Tablet row click/touch selection */
    in.biz_drawer_open = true;
    grind_input_begin_frame(&in);
    grind_input_on_touch_down(&in, 10, 0.50f, 0.32f); /* Row 2: Retail Pricing */
    ASSERT_TRUE(in.tablet_action_selected == 2, "Touching Row 2 on Business Tablet selects action 2");
    float old_price = w.player.stand_price;
    grind_input_apply_to_world(&in, &w, 0.1f);
    ASSERT_TRUE(fabsf(w.player.stand_price - old_price) > 0.1f, "Tablet Row 2 updates lemonade retail price");
}

int main(void) {
    printf("=== Running The Grind Unit Test Suite ===\n");
    RUN_TEST(test_physics_and_containers);
    RUN_TEST(test_economy_and_crafting);
    RUN_TEST(test_courier_and_delegation);
    RUN_TEST(test_social_and_lifestyle);
    RUN_TEST(test_adaptive_input_and_camera);

    printf("=== Unit Tests Complete: %d/%d passed ===\n", g_tests_run - g_tests_failed, g_tests_run);
    return g_tests_failed == 0 ? 0 : 1;
}
