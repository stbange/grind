# Tasks: 001-foundation-and-mvp

- [x] 1. Initialize OpenSpec repository structure (`openspec/project.md`, `openspec/specs/01..07`, `openspec/changes/001-foundation-and-mvp/`).
- [x] 2. Implement `grind_math.h` (3D vector/matrix math, camera projections, deterministic PRNG).
- [x] 3. Implement `libgrind_core` (`grind_core.h` & `grind_core.c`) with:
  - [x] Physical item properties (mass, volume, spoilage) and container capacity (hands, satchel, bicycle, delivery van).
  - [x] Player bio-energetics (stamina, hunger, stress, sleep) and experiential skill learning.
  - [x] First-principles retail crafting (lemonade stand) & NPC micro-economic utility purchasing.
  - [x] Physical courier parcel contracts & route logistics.
  - [x] Empire scaling: employee task-graph delegation (stand operators & couriers), marketing campaigns, and storefront/equity investments.
  - [x] Social relationship matrix (`affection`, `respect`, `trust`, `envy`, `perceived_wealth`) across NPC archetypes (`Loyal Friend`, `Envious Peer`, `Opportunist`, `Banker`).
  - [x] Personal lifestyle upgrades (food, studio/penthouse housing, clothing, luxury watch, sports car).
- [x] 4. Implement `grind_input` with First-Person / Top-Down Map camera controls and automatic small-screen touch virtual joystick & buttons.
- [x] 5. Implement `grind_renderer` and `main.c` with SDL2 + OpenGL 3.3 / ES 3.0 procedural 3D meshes, dynamic lighting, vector HUD, and Emscripten Web shell.
- [x] 6. Implement Unit Tests (`tests/unit/test_main.c`) and Automated Headless Game Progression Tests (`tests/game/test_game_progression.c`).
- [ ] 7. Configure CMake (`CMakeLists.txt`) for Linux, Windows (MinGW), and Web (Emscripten), compile, and pass all unit and automated game tests (pending `apt-get` sudo password authentication).
