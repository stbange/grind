# The Grind — 3D First-Principles Business & Life Simulation

A 3D simulation game written in **C11 / SDL2 / OpenGL (3.3 Core & WebGL 2.0 / ES 3.0)** where a young person finds a small amount of money (`$12`) on the street and builds it into a business empire by learning business from first principles along the way:

1. **The Street Grind (Lemonade Stand):** Buy a folding table, lemons, sugar, water, and cups at the Wholesale Market, set up on Central Plaza, craft batches, and price them for thirsty passersby evaluated via a micro-economic utility function.
2. **Logistics & Courier Hustle:** Invest profits into a **Courier Satchel** (expanding volume/mass carry capacity) and a **Second-Hand Bicycle** (converting stamina into 3.5x speed), claiming and delivering physical parcel contracts across the city.
3. **Scaling to an Empire:** Delegate Task Graphs by hiring NPC Stand Operators and Couriers, run Marketing campaigns to increase brand awareness, lease a Flagship Storefront, and invest in Wholesale Supplier Equity & Index Funds.
4. **Social Reactions to Growing Wealth:** Surrounding NPCs (`Marcus` the loyal friend, `Devon` the envious peer, `Chloe` the opportunist, and `Director Vance` the banker) dynamically react to your **Perceived Wealth** (clothing, watch, housing, vehicles, and businesses).
5. **The Good Things in Life:** Upgrade from a noisy tenement to a Sunny Studio or Luxury Penthouse Villa, eat artisan meals, wear a tailored suit and Swiss watch, and drive a personal sports coupe.

## Controls
* **Desktop:**
  * `W A S D` / Arrow Keys: Move / Ride / Drive
  * `Right-Click` (or hold Right Mouse Button): Mouse-look in First-Person 3D
  * `E` / `Space`: Contextual Interact (Pick up cash, buy supplies, deploy stand, craft, claim/deliver parcels, talk to NPCs)
  * `M` / `Tab`: Toggle between **First-Person 3D Street Camera** and **Top-Down 3D Tactical Management Map**
  * `B` / `1-4`: Quick Business Actions (Hire Stand Operator, Hire Courier, Launch Marketing, Consume Meal)
  * `R`: Sleep overnight & collect passive dividends
* **Web / Mobile Small Devices:**
  * Automatically switches to **On-Screen Touch Controls** (Virtual left movement thumbstick, right-side swipe look, and thumb action buttons `ACT`, `BIZ`, `MAP`, `REST`) when viewport `< 900px` or when touch input is detected.

## OpenSpec Architecture
All specifications and architectural decisions live in [`openspec/`](openspec/):
* [`openspec/project.md`](openspec/project.md)
* [`openspec/specs/`](openspec/specs/) (`01` Core Simulation through `07` Testing Architecture)
* [`openspec/changes/001-foundation-and-mvp/`](openspec/changes/001-foundation-and-mvp/)

## Building & Running Tests

### Linux / Windows (CMake)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure -V
./build/grind_app
```

### Web (Emscripten)
```bash
emcmake cmake -B build_web -DCMAKE_BUILD_TYPE=Release
cmake --build build_web --parallel
```
