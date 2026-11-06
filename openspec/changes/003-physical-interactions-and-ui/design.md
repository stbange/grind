# Technical Design 003: Recipe Physics, Diegetic Counters & Interactive Tablet

## 1. Core Simulation Additions (`grind_core.h` / `grind_core.c`)
- `GrindPlayer` fields:
  - `int recipe_lemons_per_batch` (1 to 4, default 1)
  - `int recipe_sugar_per_batch` (1 to 4, default 1)
- `grind_action_set_recipe(GrindWorld *world, int lemons_per_batch, int sugar_per_batch)`:
  - Clamps inputs to `[1, 4]`.
  - Computes recipe quality multiplier: optimal balance shifts with `ambient_temp_c` (e.g., 2 lemons + 2 sugar yields premium quality `>0.90`, whereas 1 lemon + 1 sugar yields baseline `0.70`, and unbalanced ratios reduce quality).
- `grind_action_adjust_price(GrindWorld *world, float delta)`:
  - Adjusts `player.stand_price` by `delta` within `[$0.50, $10.00]`.

## 2. Interactive Business Tablet & Touch Hit-Testing (`grind_input.h` / `grind_input.c`)
- When `in->biz_drawer_open == true`:
  - Touching or left-clicking inside one of the 6 tablet action rows (`norm_x` in `[0.18, 0.82]`, `norm_y` in `[0.22, 0.72]`) triggers `in->tablet_action_selected = row_index + 1`.
  - Pressing keys `1` through `6` (or using Up/Down + Enter) also triggers `tablet_action_selected`.
  - `grind_input_apply_to_world` executes the selected business action on `GrindWorld`.
