# Specification 05: The Grind, Bio-Energetics, Learning & Personal Rewards

## 1. Purpose
Models the physical and mental toll of the grind, experiential learning of business skills, and the tangible benefits of buying good things for oneself.

## 2. The Grind: Stamina, Nutrition, Stress & Sleep
### REQ-BIO-001: Physiological State
The player tracks:
- `stamina` (`0.0` to `100.0`): Drained by walking, pedaling a bike, carrying heavy loads, and crafting items. When `stamina < 20.0`, movement speed drops by `40%` and crafting speed drops by `50%`.
- `hunger` (`0.0` to `100.0`): Rises over time and with physical exertion.
- `stress` (`0.0` to `100.0`): Rises from long working hours (`>10 hours/day`), rain exposure, envious social hostility, and sleeping in a noisy tenement. High stress (`>75`) penalizes negotiation and employee management.

### REQ-BIO-002: Learning by Doing (First-Principles Skill Growth & Insights)
The player has four continuous skills (`0.0` to `10.0`):
1. `skill_production`: Grows every time the player crafts goods (increases product quality and reduces waste).
2. `skill_logistics`: Grows per kilometer traveled under load and per on-time courier delivery (reduces stamina drain and unlocks complex multi-stop routes).
3. `skill_persuasion`: Grows per customer sale, NPC conversation, and marketing campaign (improves customer purchase utility and B2B terms).
4. `skill_management`: Grows by supervising employees, auditing books, and studying business books/courses (boosts employee efficiency and unlocks larger team capacities).

As the player practices business or studies handbooks, they unlock **Experiential Business Insights** (`GRIND_INSIGHT_UNIT_ECONOMICS`, `GRIND_INSIGHT_DEMAND_ELASTICITY`, `GRIND_INSIGHT_ROUTE_OPTIMIZER`, `GRIND_INSIGHT_COMPETITOR_INTEL`), revealing unit COGS, profit margins, and rival vendor intelligence.

## 3. Personal Rewards ("The Good Things in Life")
### REQ-BIO-003: Lifestyle & Personal Purchases
The player can spend earned wealth on personal items that visibly appear in the 3D world and improve life quality:
- **Food & Drink:**
  - *Street Snack / Instant Noodles ($2.50):* Restores `25` stamina, `+5` stress.
  - *Artisan Espresso & Fresh Meal ($12.00):* Restores `65` stamina, `-15` stress, `+10%` walk speed for 3 hours.
- **Housing Upgrades:**
  - *Dilapidated Tenement (Start):* Overnight sleep restores `55` stamina, reduces stress by `10`.
  - *Sunny Studio Apartment ($250 lease):* Overnight sleep restores `85` stamina, reduces stress by `35`, adds personal storage.
  - *Luxury Penthouse Villa ($2,500 purchase):* Overnight sleep restores `100` stamina, reduces stress by `80`, grants `+45` `perceived_wealth`, includes private garage.
- **Apparel, Gear & Vehicles:**
  - *Courier Satchel ($25)* -> *Second-Hand Bicycle ($85)* -> *Cargo E-Bike ($320)* -> *Tailored Suit ($400)* -> *Luxury Watch ($1,200)* -> *Personal Sports Coupe ($4,500)*.
