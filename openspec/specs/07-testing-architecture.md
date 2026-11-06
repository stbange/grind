# Specification 07: Unit Testing & Automated Game Testing Architecture

## 1. Purpose
Ensures every first-principles mechanic, economic progression loop, social wealth reaction, and adaptive input/UI controller is automatically verified via CTest.

## 2. Unit Test Suite (`tests/unit/`)
- Built into `grind_unit_tests` binary (zero GUI required, runs in `< 0.5s`).
- Required test modules:
  1. `test_physics_and_containers`: Validates mass/volume limits, satchel & bicycle capacity upgrades, and encumbrance speed penalties.
  2. `test_economy_and_crafting`: Validates lemonade crafting from raw ingredients, customer utility formula under varying temperature/thirst/price/brand awareness, and spoilage.
  3. `test_courier_and_delegation`: Validates parcel contract pickup/delivery, bicycle speed multiplier, and hired NPC employee task-graph execution (autonomous crafting/selling and courier runs).
  4. `test_social_and_lifestyle`: Validates `perceived_wealth` calculation from clothes/vehicles/housing, divergent NPC archetype reactions (`Loyal Friend`, `Envious Peer`, `Opportunist`, `Banker`), and stamina/stress recovery from lifestyle upgrades.
  5. `test_adaptive_input_and_camera`: Validates First-Person vs. Top-Down Map camera matrices and automatic activation + virtual joystick/button mapping for small-screen touch controls.

## 3. Automated Game Test Suite (`tests/game/`)
- Built into `grind_game_tests` binary (and callable via `./grind_app --automated-test`).
- Runs an autonomous **First-Principles Bot Agent** through a full multi-day "Rags to Empire" simulation campaign:
  - **Phase A (The Found Money & Lemonade Grind):** Player starts with `$0.00`, locates and picks up the `$12.00` bill on the street, walks to the Wholesale Market, buys a table + lemons + sugar + water + cups, sets up at Central Plaza, crafts and sells lemonade to thirsty NPCs, and reaches `>= $90.00`.
  - **Phase B (Satchel, Bicycle & Courier Hustle):** Bot buys the Courier Satchel (`$25`) and Bicycle (`$85`), claims and completes multiple parcel delivery contracts across the city, demonstrating higher hourly earnings and growing `skill_logistics`.
  - **Phase C (Empire Scaling, Social Shift & The Good Life):** Bot hires stand operators and courier employees, launches marketing campaigns, leases a Commercial Storefront, invests in Supplier Equity, and buys a Tailored Suit, Luxury Penthouse, and Sports Car.
  - **Assertions:** Verifies net worth exceeds `$5,000`, passive daily business revenue is positive, `Envious Peer` envy increased while `Banker` respect/credit unlocked, and player bio-stats reflect upgraded housing/lifestyle.
