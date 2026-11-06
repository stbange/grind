# Tasks: 002-visual-world-and-characters

- [x] 1. Create OpenSpec change package `002-visual-world-and-characters` (`proposal.md`, `design.md`, `tasks.md`) and update `openspec/specs/06-renderer-and-ui.md`.
- [x] 2. Add 3D-to-2D frustum projection (`grind_project_world_to_screen`) in `grind_math.h` and animation/popup state (`walk_phase`, `drink_timer`, `sale_popup`) in `grind_core.h` / `grind_core.c`.
- [x] 3. Upgrade `grind_renderer.c` with:
  - [x] Detailed multi-story buildings (windows with night glow, awnings, doors, roofs).
  - [x] Streetscape details (sidewalks, road lane markings, park trees, benches, streetlamps, detailed lemonade stand & vehicles).
  - [x] Articulated 3D NPCs & employees (legs, torso, arms, head, walking swing animation, held lemonade cups & parcels).
  - [x] Detailed First-Person viewmodel (both arms/hands, held items, bicycle fork/wheel/handlebars, car steering wheel & dash).
  - [x] 3D world-space floating labels (building signs, stand price/stock tag, `+$` sale popups, and nearby NPC speech bubbles).
- [x] 4. Add unit tests for 3D world-to-screen projection and animation states in `tests/unit/test_main.c`.
- [x] 5. Commit, push to `main`, verify CI test suite, and deploy updated WebGL 2.0 build to GitHub Pages.
