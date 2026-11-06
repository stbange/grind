# Technical Design 004: Experiential Insights, Rival Utility Competition & Grind Feedback

## 1. Core Simulation Additions (`grind_core.h` / `grind_core.c`)
- **Insight Bitflags (`GrindInsightFlags`)**:
  - `GRIND_INSIGHT_UNIT_ECONOMICS   = (1u << 0)`
  - `GRIND_INSIGHT_DEMAND_ELASTICITY= (1u << 1)`
  - `GRIND_INSIGHT_ROUTE_OPTIMIZER  = (1u << 2)`
  - `GRIND_INSIGHT_COMPETITOR_INTEL = (1u << 3)`
- `grind_player_unit_cost_per_cup(const GrindWorld *world)` and `grind_player_profit_margin_pct(const GrindWorld *world)`:
  - Computes exact first-principles COGS per cup from current recipe (`recipe_lemons_per_batch`, `recipe_sugar_per_batch`, water, cups) and supplier equity discounts.
- **Rival Stand State in `GrindWorld`**:
  - `bool rival_stand_active;`
  - `Vec3 rival_stand_pos;`
  - `float rival_stand_price;` (`$1.45`)
  - `float rival_stand_quality;` (`0.68f`)
  - During `grind_world_tick`, if `rival_stand_active` is true, thirsty NPCs compare `u_player` against `u_rival = grind_eval_customer_utility(world, npc, rival_stand_price, rival_stand_quality) - 0.08f`. The player wins the sale only if `u_player >= u_rival` (rewarding recipe quality, brand marketing, or fair pricing).

## 2. Visual Grind & Rain Rendering (`grind_renderer.c` / `grind_input.c`)
- When `stamina < 25.0f` or `stress > 70.0f`, `grind_input_compute_view_matrix` adds a low-frequency fatigue sway to eye height/target, and `grind_renderer.c` draws a pulsing stamina/stress warning border.
- When `world->weather == WEATHER_RAINY`, `grind_renderer.c` renders animated 3D diagonal rain streaks around the camera and a wet-pavement specular sheen.
