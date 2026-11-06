# Specification 06: 3D OpenGL Renderer, First-Person Camera & Adaptive Touch Controls

## 1. Purpose
Defines the cross-platform SDL2 + OpenGL 3D rendering pipeline, the First-Person + Top-Down Management Map camera system, and automatic switching to on-screen touch controls for small web/mobile devices.

## 2. Camera Modes
### REQ-RND-001: First-Person 3D Street Camera (Default)
- Eye height positioned at `player.pos.y + 1.65f` (with subtle head-bob scaled by movement speed and stamina fatigue).
- Yaw (`camera_yaw`) and Pitch (`camera_pitch`, clamped to `[-75°, +75°]`) controlled via mouse look (or right-side touch drag on mobile).
- Renders first-person viewmodel cues in the lower corners of the viewport:
  - Currently held item/parcel or beverage pitcher.
  - Bicycle handlebars / vehicle dashboard when riding/driving.
  - Apparel sleeve/watch indicator reflecting equipped clothing tier.

### REQ-RND-002: Toggleable Top-Down 3D Management Map (`M` Key / Map Button)
- Pressing `M` (or tapping the `[MAP]` on-screen button) smoothly transitions the camera to an elevated 3D orthographic/angled tactical view (`Y = 85.0m`, looking down at the city).
- Highlights:
  - Player position, active stands/storefronts, hired couriers moving along routes in real time, available parcel contracts, and property lots available for lease/investment.
  - Allows quick commands for hiring, setting lemonade prices, launching marketing campaigns, and investing.

## 3. Procedural Low-Poly 3D Geometry & Shaders
### REQ-RND-003: Cross-Platform Shader Pipeline
- Uses a single unified shader pair compatible with **OpenGL 3.3 Core** (Linux/Windows) and **OpenGL ES 3.0 / WebGL 2.0** (Emscripten):
  - Directional sunlight + hemisphere sky/ground ambient lighting that shifts color temperature across morning, noon, sunset, and night.
  - Procedural 3D meshes generated in C at startup: city roads, sidewalks, distinct building blocks by district, trees, folding tables with pitchers/umbrellas, parcels, bicycles, vans, sports cars, and articulated NPCs whose headband/aura and body posture reflect their relationship state (`Affection`, `Envy`, `Respect`) toward the player.

## 4. Adaptive On-Screen Controls for Web & Small Devices
### REQ-UI-001: Automatic Small-Device & Touch Detection
- The input subsystem (`GrindInputState`) evaluates two triggers every frame:
  1. `viewport_width < 900` or `viewport_height < 600` (small screen / mobile browser window).
  2. Receipt of any `SDL_FINGERDOWN` / `SDL_FINGERMOTION` event, or `--touch-ui` CLI flag.
- When active (`touch_controls_active == true`), the renderer overlays translucent, ergonomic on-screen controls:
  - **Bottom-Left Virtual Joystick:** 2D analog movement vector (`move_forward`, `move_strafe`).
  - **Right-Half Touch Look Zone:** Dragging rotates `camera_yaw` and `camera_pitch` (or pans the Management Map).
  - **Bottom-Right Action Cluster:** Contextual thumb buttons:
    - `[ACT / E]` (Pick up money, buy ingredients, craft, claim parcel, talk to NPC)
    - `[BIZ / B]` (Open quick business actions: set price, hire employee, market, invest)
    - `[MAP / M]` (Toggle First-Person <-> Top-Down 3D Management Map)
    - `[SLEEP / R]` (Rest / advance night when tired)
