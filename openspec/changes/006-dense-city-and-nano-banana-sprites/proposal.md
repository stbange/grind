# Change Proposal 006: Dense Procedural City Grid, Web Pointer Lock & Nano Banana Textures/Billboards

## 1. Summary
Addresses the user's feedback to dramatically enrich the 3D Web & Desktop experience:
1. **Reliable Web Mouse Capture (HTML5 Pointer Lock):**
   - Left-clicking anywhere on the 3D canvas in Desktop/Web mode (when touch controls are not active and the Business Tablet is closed) immediately captures the mouse via `SDL_SetRelativeMouseMode(SDL_TRUE)` and `emscripten_request_pointerlock("#canvas", 1)`.
   - Adds a click listener in `web/shell.html` so browsers grant Pointer Lock directly inside the user gesture, plus an on-screen hint (`CLICK TO CAPTURE MOUSE / ESC TO RELEASE`).
2. **Dense Procedurally Generated City Grid (`GRIND_MAX_BUILDINGS = 96`):**
   - Expand `GrindWorld` from 16 sparse buildings to a dense, walkable **96-building procedural city grid** organized into blocks separated by North-South avenues and East-West streets, narrow alleys, courtyard plazas, and varied skyscraper/tenement/shop heights—while keeping walkable corridors clear via building AABB collision resolution in `grind_player_move`.
3. **Nano Banana Generated Texture Atlas & 3D Billboard Sprite Sheet:**
   - Generate two 4x4 atlases (`assets/textures/city_atlas.png` and `assets/textures/sprites_atlas.png`) using the **Nano Banana** image generation model.
   - Upgrade `grind_renderer.c` shaders (`#version 300 es` / `#version 330 core`) with UV texture coordinates (`a_uv`) and alpha-cutout support (`discard` for transparent sprite pixels) to render:
     - Textured 3D city facades, roofs, roads, sidewalks, tables, and pools.
     - Camera-facing 3D billboard sprites for NPCs, employees, trees, streetlamps, cash bundles, parcels, and bicycles alongside 3D geometry.
   - Embed/preload the generated textures via `--preload-file assets` in Emscripten (and single-header `stb_image` / raw asset loader in C) so both Desktop and GitHub Pages Web builds display the Nano Banana art seamlessly.
