# Project Overview: The Grind (openspec/project.md)

## 1. Vision & Core Philosophy
**The Grind** is a 3D first-principles business and life simulation game where the player starts as a young person with $0 who spots a small amount of money on the ground and builds it into a multi-tiered business empire while experiencing the physical grind, learning business fundamentals on the way, seeing surrounding NPC relationships react to growing wealth, and enjoying personal quality-of-life rewards.

### First-Principles Design Mandate
- **No Arbitrary Business Menus:** Businesses are not abstract "unlockable tiers" in a menu. They emerge from combining **physical items**, **spatial containers/workstations**, **human bio-energetics**, **customer utility calculations**, and **contractual task delegation**.
  - *Example 1 (Street Stand):* Placing a portable table on a sidewalk, stocking it with cups, water, lemons, and sugar, crafting lemonade into a pitcher, and posting a price sign creates a point-of-sale that thirsty passersby evaluate using a micro-economic utility function.
  - *Example 2 (Courier Service):* Equipping a satchel increases carry volume/mass capacity; riding a bicycle converts player stamina into 3.5x movement speed; claiming physical parcel contracts from local merchants and delivering them before deadline yields revenue and builds business reputation.
  - *Example 3 (Empire Scaling):* Hiring NPC employees binds them to a **Task Graph** (the exact same sequence of actions the player performs manually) in exchange for an hourly wage. Marketing campaigns propagate brand awareness through the NPC population. Investing acquires real estate (stalls, storefronts, warehouses, luxury apartments) or supplier equity.

## 2. Technical Stack & Constraints
- **Language:** Standard C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`).
- **Architecture Split:**
  - `libgrind_core`: 100% deterministic, zero-dependency C11 simulation library. No OpenGL or SDL headers allowed.
  - `grind_app`: Cross-platform SDL2 + OpenGL 3.3 Core (Desktop) / OpenGL ES 3.0 & WebGL 2.0 (Web via Emscripten) 3D presentation and input shell.
- **Target Platforms:**
  - **Linux** (GCC/Clang native + CMake)
  - **Windows** (MinGW-w64 / MSVC + CMake)
  - **Web** (Emscripten -> HTML5 WebGL 2.0 + responsive touch controls)
- **Camera & Controls:**
  - **First-Person 3D Camera** at street level (with visible held items, vehicle handlebars, and NPC interactions) + **Toggleable Top-Down 3D Tactical Management Map** (`M` key or on-screen Map button) for managing employees, logistics routes, marketing zones, and properties.
  - **Adaptive Touch Controls:** Automatically switches to on-screen virtual dual-thumb controls (left movement joystick, right look/swipe + large contextual action buttons) when running on small viewports (`width < 900px`) or when touch events (`SDL_FINGERDOWN`) are detected.
- **Testing Requirements:**
  - **Unit Tests (`tests/unit/`):** Fast, deterministic C unit tests covering physics/containers, micro-economy utility, bio-energetics/skills, task delegation/empire scaling, social wealth reactions, and adaptive UI/input state.
  - **Automated Game Tests (`tests/game/`):** Headless full-simulation bot tests verifying multi-day progression from finding money on the ground -> lemonade stand -> satchel & bike courier -> hiring, marketing, investing, social shifts, and personal luxury upgrades.
