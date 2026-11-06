# Tasks: 006-dense-city-and-nano-banana-sprites

- [x] 1. Generate `assets/textures/city_atlas.jpg` and `assets/textures/sprites_atlas.jpg` using the Nano Banana image generation subagent.
- [x] 2. Fix Web mouse capture (HTML5 Pointer Lock on left/right click in `web/shell.html` and `src/main.c`).
- [x] 3. Expand procedural city generator in `grind_core.c` to spawn a dense 80+ building skyline + 48 street trees with AABB street collision.
- [x] 4. Upgrade `grind_renderer.c` with textured 3D geometry, 4x4 UV atlas indexing, and camera-facing 3D sprite billboards with magenta chroma-key transparency.
- [x] 5. Copy `assets/textures/*.jpg` into `dist_web/assets/textures/` in `.github/workflows/deploy-web.yml` and add unit tests for dense city generation.
