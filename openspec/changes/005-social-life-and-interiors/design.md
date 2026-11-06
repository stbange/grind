# Technical Design 005: Social Choice Engine, 3D Luxury Showcase & Procedural Audio

## 1. Core Simulation Additions (`grind_core.h` / `grind_core.c`)
- `GrindWorld` additions:
  - `bool rival_allied_franchise`: True when the player reconciles with `Devon` by offering a partnership (`$80`), converting the rival stand into a royalty-generating franchise (`+$0.75` to player per rival cup sold).
  - `uint32_t sound_events_mask`: Bitmask of procedural audio cues triggered during the tick (`GRIND_SFX_COIN`, `GRIND_SFX_SALE`, `GRIND_SFX_CRAFT`, `GRIND_SFX_UPGRADE`), consumed by the SDL2 audio callback.
- `bool grind_action_social_choice(GrindWorld *world, int npc_idx, int choice_id)`:
  - Executes archetype-specific social/financial choices for `Marcus` (`npc_idx=0`), `Devon` (`npc_idx=1`), `Chloe` (`npc_idx=2`), and `Director Vance` (`npc_idx=3`).

## 2. 3D Luxury Courtyard & Sports Coupe Renderer (`grind_renderer.c`)
- **`draw_sports_car_3d(...)`**:
  - Renders low-slung chassis, cabin greenhouse, front splitter, rear wing spoiler, and 4 wheels.
- **`draw_lifestyle_courtyard(...)`**:
  - Renders the player's current residence exterior/terrace at `(-52, 0, -48)` and `(52, 0, 42)`—including a glowing turquoise penthouse pool and lounge deck when `HOUSING_PENTHOUSE_VILLA` is owned.
