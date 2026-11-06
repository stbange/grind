# Specification 01: Core Simulation Architecture & Determinism

## 1. Purpose
Defines the foundational simulation loop, deterministic pseudo-random number generator (PRNG), time-keeping, and data-oriented state structures in `libgrind_core`.

## 2. Requirements & Invariants

### REQ-CORE-001: Fixed-Timestep Determinism
- The simulation advances in discrete ticks via `grind_world_tick(GrindWorld *world, float dt_sec)`.
- Standard simulation tick rate is `10 Hz` (`0.1s` real time = `1.0` simulated minute), allowing a 16-hour waking day (`06:00` to `22:00`, `960` simulated minutes) to play out in `96` real seconds at `1x` speed, or thousands of ticks per second in headless automated game tests.
- All random events MUST use `grind_rng_next(GrindRng *rng)` (`xoshiro128++` or `splitmix64`), seeded explicitly in `grind_world_init(GrindWorld *world, uint64_t seed)`.

### REQ-CORE-002: Zero Platform Dependencies in Core
- Files under `include/grind_core/` and `src/core/` MUST only include standard C11 headers (`<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, `<math.h>`, `<string.h>`, `<stdio.h>`).
- Zero dynamic heap allocations (`malloc`/`free`) during `grind_world_tick()`. All entities, NPCs, items, contracts, and workstations reside in fixed-capacity contiguous arrays inside `GrindWorld`.

### REQ-CORE-003: Time & Weather Cycle
- `GrindWorld` tracks:
  - `day_index` (starting at Day 1)
  - `time_of_day_hours` (`0.0f` to `24.0f`, initialized at `07:30f`)
  - `ambient_temp_c` (varies sinusoidally with time of day and daily weather seed, e.g., `14°C` morning to `31°C` afternoon)
  - `weather_condition` (`WEATHER_SUNNY`, `WEATHER_CLOUDY`, `WEATHER_HOT`, `WEATHER_RAINY`)
- Weather directly affects first-principles mechanics:
  - Higher `ambient_temp_c` increases NPC thirst decay rate and utility for cold beverages.
  - `WEATHER_RAINY` reduces pedestrian foot traffic and increases demand for courier deliveries.
