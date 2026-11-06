# Technical Design 001: First-Principles Core & 3D OpenGL Presentation

## 1. Module Breakdown
- `include/grind_core/grind_math.h`: 3D vectors (`Vec3`), 4x4 matrices (`Mat4`) for perspective/lookAt/orthographic projections, and deterministic `GrindRng` (`xoshiro128++`).
- `include/grind_core/grind_core.h`: Complete data-oriented world state (`GrindWorld`), items, containers, workstations, courier contracts, NPCs, social relations, skills, and core simulation API.
- `include/grind_app/grind_input.h`: Cross-platform input translator supporting keyboard/mouse and automatic small-viewport/touch virtual joystick & action buttons.
- `include/grind_app/grind_renderer.h`: OpenGL 3.3 / WebGL 2.0 procedural low-poly 3D mesh builder, shader pipeline, First-Person viewmodel + Top-Down 3D Management Map renderer, and immediate-mode vector HUD/text renderer.
- `src/core/grind_core.c`: Implementation of all first-principles simulation loops.
- `src/app/grind_input.c`: Implementation of desktop + mobile touch UI state machine and hit-testing.
- `src/app/grind_renderer.c`: Implementation of 3D world rendering, NPC status indicators, held items/vehicles, HUD, and touch controls overlay.
- `src/main.c`: Main SDL2 + OpenGL / Emscripten loop (`emscripten_set_main_loop_arg`), plus `--headless-bot` and `--smoke-test` CLI modes.
