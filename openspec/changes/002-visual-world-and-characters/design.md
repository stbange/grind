# Technical Design 002: Articulated 3D Characters, Architecture & World-Space Projections

## 1. Mathematical & Core Additions
- **`grind_math_project_to_screen(Vec3 world_pos, Mat4 view_proj, int vp_w, int vp_h, Vec2 *out_screen)`**:
  - Transforms a 3D world coordinate by `view_proj`, checks clip-space `w > 0.1f` (in front of the camera) and normalized device coordinates within frustum, and outputs pixel coordinates `(x, y)` for crisp 3D world-space billboards, building signs, stand price boards, and NPC speech bubbles.
- **Animation Phase & Visual Events in `GrindWorld`**:
  - `world->anim_time_sec`: Continuous animation clock driving NPC limb swing (`sinf(walk_phase)`), bicycle wheel rotation, and floating cash/parcel bobbing.
  - `npc->walk_phase`: Per-NPC stride accumulator incremented by distance walked.
  - `npc->drink_timer`: Timer after purchasing lemonade during which the NPC holds a yellow lemonade cup in their raised right hand.
  - `world->sale_popup_timer`, `world->sale_popup_pos`, `world->sale_popup_amount`: Triggers a rising 3D `+$X.XX` popup in the world whenever a sale or delivery occurs.

## 2. Procedural 3D Hierarchy (`src/app/grind_renderer.c`)
- **`draw_articulated_human(...)`**:
  - Renders Left Leg, Right Leg (pivoting at hip by `±swing_angle`), Torso, Head, Hair/Cap, Left Arm, Right Arm (pivoting at shoulder opposite to legs, or raised when holding a cup/parcel), and optional courier backpack.
- **`draw_detailed_building(...)`**:
  - Renders raised sidewalk plinth, main facade, striped shopfront awning, entrance double-doors, procedural grid of windows (dark blue-grey by day, warm amber glow at night), and roof parapet.
- **`draw_street_environment(...)`**:
  - Asphalt avenues along X and Z axes with yellow/white center dashes, park trees (trunk + layered foliage crowns), plaza fountain/benches, and streetlamps with glowing lanterns at dusk.
