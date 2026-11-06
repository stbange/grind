#ifndef GRIND_CORE_H
#define GRIND_CORE_H

#include "grind_core/grind_math.h"
#include <stddef.h>

#define GRIND_MAX_NPCS 36
#define GRIND_MAX_CONTRACTS 12
#define GRIND_MAX_EMPLOYEES 8
#define GRIND_MAX_BUILDINGS 96
#define GRIND_MAX_TREES 64
#define GRIND_MAX_LOG_LEN 128

/* Weather states affecting foot traffic, thirst, and courier demand */
typedef enum {
    WEATHER_SUNNY = 0,
    WEATHER_HOT = 1,
    WEATHER_CLOUDY = 2,
    WEATHER_RAINY = 3
} GrindWeather;

/* Physical items in the first-principles world */
typedef enum {
    ITEM_LEMON = 0,
    ITEM_SUGAR = 1,
    ITEM_WATER_JUG = 2,
    ITEM_PAPER_CUPS = 3,
    ITEM_LEMONADE_CUP = 4,
    ITEM_FOLDING_TABLE = 5,
    ITEM_FLYER_BUNDLE = 6,
    ITEM_STREET_SNACK = 7,
    ITEM_FRESH_MEAL = 8,
    ITEM_BUSINESS_BOOK = 9,
    ITEM_COUNT
} GrindItemType;

typedef struct {
    const char *name;
    float mass_kg;
    float volume_l;
    float spoilage_per_hour;
    float base_price;
} GrindItemDef;

/* Personal gear, vehicles, clothing, and housing ("The Good Things in Life") */
typedef enum {
    VEHICLE_NONE = 0,
    VEHICLE_BICYCLE = 1,
    VEHICLE_CARGO_EBIKE = 2,
    VEHICLE_DELIVERY_VAN = 3,
    VEHICLE_SPORTS_CAR = 4
} GrindVehicleTier;

typedef enum {
    APPAREL_WORN_HOODIE = 0,
    APPAREL_CLEAN_CASUAL = 1,
    APPAREL_COURIER_GEAR = 2,
    APPAREL_TAILORED_SUIT = 3
} GrindApparelTier;

typedef enum {
    HOUSING_TENEMENT = 0,
    HOUSING_STUDIO_APT = 1,
    HOUSING_PENTHOUSE_VILLA = 2
} GrindHousingTier;

/* City Districts / Landmarks */
typedef enum {
    ZONE_TENEMENT_ALLEY = 0,
    ZONE_WHOLESALE_MARKET = 1,
    ZONE_CENTRAL_PLAZA = 2,
    ZONE_BIKE_VEHICLE_SHOP = 3,
    ZONE_DISPATCH_HUB = 4,
    ZONE_BANK_FINANCE = 5,
    ZONE_BOUTIQUE_LUXURY = 6,
    ZONE_COUNT
} GrindZoneId;

typedef struct {
    GrindZoneId zone;
    const char *name;
    Vec3 pos;
    Vec3 size;
    Color4 color;
    int tex_tile; /* Index [0..15] in the 4x4 city_atlas texture sheet */
} GrindBuilding;

/* Physical Courier Parcel Contract */
typedef struct {
    bool active;
    bool claimed_by_player;
    int id;
    GrindZoneId pickup_zone;
    GrindZoneId dropoff_zone;
    Vec3 pickup_pos;
    Vec3 dropoff_pos;
    float mass_kg;
    float volume_l;
    float reward_cash;
    float deadline_hours;
    bool picked_up;
} GrindCourierContract;

/* NPC Social Archetypes reacting differently to growing player wealth */
typedef enum {
    ARCHETYPE_CITIZEN = 0,
    ARCHETYPE_LOYAL_FRIEND = 1,   /* Marcus: Supportive, values authenticity */
    ARCHETYPE_ENVIOUS_PEER = 2,   /* Devon: Friendly when broke, envious as wealth grows */
    ARCHETYPE_OPPORTUNIST = 3,    /* Chloe: Dismissive when broke, flatters when rich */
    ARCHETYPE_BANKER = 4          /* Director Vance: Gatekeeper, respects status & credit */
} GrindNpcArchetype;

typedef struct {
    int id;
    char name[32];
    GrindNpcArchetype archetype;
    Vec3 pos;
    Vec3 target_pos;
    float facing_yaw;
    float walk_phase;
    float drink_timer;       /* > 0 when holding/drinking a recently purchased lemonade cup */
    float walk_speed;
    float cash;
    float thirst;            /* 0.0 to 1.0 */
    float price_sensitivity; /* 0.3 to 1.2 */

    /* Multidimensional relationship toward the player */
    float affection;        /* -1.0 to +1.0 */
    float respect;          /*  0.0 to +1.0 */
    float trust;            /*  0.0 to +1.0 */
    float envy;             /*  0.0 to +1.0 */
    float brand_awareness;  /*  0.0 to +1.0 */
    char last_reaction[96];
} GrindNpc;

/* Hired Employee executing a delegated Task Graph */
typedef enum {
    EMP_ROLE_NONE = 0,
    EMP_ROLE_STAND_OPERATOR = 1,
    EMP_ROLE_COURIER = 2
} GrindEmployeeRole;

typedef struct {
    bool hired;
    char name[32];
    GrindEmployeeRole role;
    float wage_per_hour;
    float skill;        /* 0.3 to 1.5 */
    float morale;       /* 0.0 to 1.0 */
    Vec3 pos;
    Vec3 target_pos;
    float facing_yaw;
    float walk_phase;
    float task_progress;
    float revenue_generated_today;
} GrindEmployee;

/* Procedural Audio Event Flags */
typedef enum {
    GRIND_SFX_NONE    = 0,
    GRIND_SFX_COIN    = (1u << 0),
    GRIND_SFX_SALE    = (1u << 1),
    GRIND_SFX_CRAFT   = (1u << 2),
    GRIND_SFX_UPGRADE = (1u << 3)
} GrindSfxFlags;

/* Experiential Business Insights unlocked by doing business or studying */
typedef enum {
    GRIND_INSIGHT_NONE             = 0,
    GRIND_INSIGHT_UNIT_ECONOMICS   = (1u << 0), /* Reveals COGS $/cup & profit margin % */
    GRIND_INSIGHT_DEMAND_ELASTICITY= (1u << 1), /* Reveals weather & price elasticity */
    GRIND_INSIGHT_ROUTE_OPTIMIZER  = (1u << 2), /* Reveals courier $/kg efficiency & cuts stamina drain */
    GRIND_INSIGHT_COMPETITOR_INTEL = (1u << 3)  /* Reveals rival vendor pricing & quality */
} GrindInsightFlags;

/* Player Bio-Energetics, Skills, Inventory & Assets */
typedef struct {
    Vec3 pos;
    float yaw_rad;
    float pitch_rad;

    /* Finances */
    float cash;
    float bank_balance;
    float loan_debt;
    float credit_limit;
    float supplier_equity_share; /* 0.0 to 0.50 (reduces wholesale costs & pays dividends) */
    float index_fund_invested;

    /* Physiology ("The Grind") */
    float stamina; /* 0.0 to 100.0 */
    float hunger;  /* 0.0 to 100.0 */
    float stress;  /* 0.0 to 100.0 */

    /* First-Principles Experiential Skills (0.0 to 10.0) & Unlocked Insights */
    float skill_production;
    float skill_logistics;
    float skill_persuasion;
    float skill_management;
    uint32_t unlocked_insights;

    /* Physical Inventory & Containers */
    int inventory[ITEM_COUNT];
    float item_freshness[ITEM_COUNT]; /* 0.0 to 1.0 */
    bool has_satchel;
    bool has_luxury_watch;
    GrindVehicleTier vehicle;
    GrindApparelTier apparel;
    GrindHousingTier housing;
    float meal_speed_buff_hours;

    /* Active Business Point-of-Sale (Stand / Storefront) */
    bool stand_deployed;
    bool owns_storefront;
    Vec3 stand_pos;
    int stand_lemons;
    int stand_sugar;
    int stand_water;
    int stand_cups;
    int stand_lemonade_stock;
    float stand_lemonade_quality;
    float stand_price;
    int recipe_lemons_per_batch;
    int recipe_sugar_per_batch;
    int cups_sold_total;
    int deliveries_completed_total;

    /* Perceived Wealth & Reputation */
    float perceived_wealth;    /* 0.0 to 100.0 calculated from visible status signals */
    float business_reputation; /* 0.0 to 100.0 */
    float marketing_awareness; /* 0.0 to 1.0 */
} GrindPlayer;

typedef struct {
    GrindRng rng;
    int day_index;
    float time_of_day_hours; /* 0.0 to 24.0 */
    float ambient_temp_c;
    GrindWeather weather;

    /* Found money on the ground at game start */
    bool ground_money_available;
    Vec3 ground_money_pos;
    float ground_money_amount;

    GrindPlayer player;
    GrindBuilding buildings[GRIND_MAX_BUILDINGS];
    int building_count;
    Vec3 tree_positions[GRIND_MAX_TREES];
    int tree_count;

    GrindNpc npcs[GRIND_MAX_NPCS];
    int npc_count;

    GrindCourierContract contracts[GRIND_MAX_CONTRACTS];
    GrindEmployee employees[GRIND_MAX_EMPLOYEES];
    int employee_count;

    /* Daily financial tracking */
    float revenue_today;
    float wages_paid_today;
    float passive_income_today;

    /* Emergent Rival Street Vendor (Devon) when envy/competition triggers */
    bool rival_stand_active;
    bool rival_allied_franchise;
    Vec3 rival_stand_pos;
    float rival_stand_price;
    float rival_stand_quality;
    int rival_cups_sold;

    /* Procedural sound events triggered since last audio poll */
    uint32_t sound_events_mask;

    /* 3D visual animation clock & floating +$ sale popup state */
    float anim_time_sec;
    float sale_popup_timer;
    Vec3 sale_popup_pos;
    float sale_popup_amount;

    char status_message[GRIND_MAX_LOG_LEN];
} GrindWorld;

/* Core Lifecycle & Simulation Step */
void grind_world_init(GrindWorld *world, uint64_t seed);
void grind_world_tick(GrindWorld *world, float sim_minutes);

/* Physical Container & Kinematics Queries */
const GrindItemDef *grind_item_def(GrindItemType type);
float grind_player_carried_mass_kg(const GrindWorld *world);
float grind_player_carried_volume_l(const GrindWorld *world);
float grind_player_max_mass_kg(const GrindWorld *world);
float grind_player_max_volume_l(const GrindWorld *world);
float grind_player_effective_move_speed(const GrindWorld *world);
void grind_player_move(GrindWorld *world, float forward_axis, float strafe_axis, float dt_sec);

/* First-Principles Interactions & Business Actions */
bool grind_action_pickup_ground_money(GrindWorld *world);
bool grind_action_buy_item(GrindWorld *world, GrindItemType type, int quantity);
bool grind_action_deploy_stand(GrindWorld *world);
bool grind_action_craft_lemonade(GrindWorld *world, int batches);
void grind_action_set_lemonade_price(GrindWorld *world, float new_price);
void grind_action_adjust_price(GrindWorld *world, float delta);
void grind_action_set_recipe(GrindWorld *world, int lemons_per_batch, int sugar_per_batch);
float grind_player_unit_cost_per_cup(const GrindWorld *world);
float grind_player_profit_margin_pct(const GrindWorld *world);
float grind_eval_customer_utility(const GrindWorld *world, const GrindNpc *npc, float price, float quality);

/* Logistics & Courier Actions */
bool grind_action_buy_satchel(GrindWorld *world);
bool grind_action_buy_vehicle(GrindWorld *world, GrindVehicleTier tier);
bool grind_action_claim_contract(GrindWorld *world, int contract_idx);
bool grind_action_pickup_contract_parcel(GrindWorld *world, int contract_idx);
bool grind_action_deliver_contract_parcel(GrindWorld *world, int contract_idx);

/* Empire Scaling: Hiring, Marketing & Investing */
bool grind_action_hire_employee(GrindWorld *world, GrindEmployeeRole role);
bool grind_action_distribute_flyers(GrindWorld *world);
bool grind_action_launch_marketing_campaign(GrindWorld *world, int tier);
bool grind_action_lease_storefront(GrindWorld *world);
bool grind_action_invest_supplier_equity(GrindWorld *world, float cash_amount);
bool grind_action_invest_index_fund(GrindWorld *world, float cash_amount);
bool grind_action_request_bank_loan(GrindWorld *world, float amount);
bool grind_action_repay_bank_loan(GrindWorld *world, float amount);

/* Personal Rewards ("The Good Things in Life"), Bio & Social Actions */
bool grind_action_consume_item(GrindWorld *world, GrindItemType type);
bool grind_action_buy_apparel(GrindWorld *world, GrindApparelTier tier);
bool grind_action_buy_luxury_watch(GrindWorld *world);
bool grind_action_upgrade_housing(GrindWorld *world, GrindHousingTier tier);
void grind_action_sleep_overnight(GrindWorld *world);
bool grind_action_talk_to_npc(GrindWorld *world, int npc_idx);
bool grind_action_social_choice(GrindWorld *world, int npc_idx, int choice_id);
bool grind_action_context_interact(GrindWorld *world);

/* Social & Wealth Metrics */
float grind_calc_perceived_wealth(const GrindWorld *world);
float grind_calc_net_worth(const GrindWorld *world);
void grind_update_social_reactions(GrindWorld *world);

#endif /* GRIND_CORE_H */
