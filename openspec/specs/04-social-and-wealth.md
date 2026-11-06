# Specification 04: Social Relationship Matrix & Wealth Reactions

## 1. Purpose
Models how the people surrounding the player react dynamically to the player's growing wealth, visible lifestyle signals, and work ethic.

## 2. Visible vs. Hidden Wealth
### REQ-SOC-001: Perceived Wealth Calculation
- NPCs do not read `player.cash` directly when forming street-level impressions; they compute `perceived_wealth` (`0.0` to `100.0`) from **observable signals**:
  - Equipped Apparel (`worn_hoodie = 0`, `clean_casual = 10`, `courier_uniform = 15`, `tailored_suit = 45`, `luxury_watch = +25`)
  - Active Vehicle (`on_foot = 0`, `bicycle = 5`, `cargo_ebike = 12`, `delivery_van = 20`, `sports_car = 50`)
  - Residence Tier (`tenement = 0`, `studio_apt = 15`, `luxury_penthouse = 45`)
  - Known Business Ownership (`stands_count * 5 + employees_count * 8 + storefronts_count * 20`)

## 3. NPC Archetypes & Relationship Dynamics
### REQ-SOC-002: Multidimensional Relationship State
Each named NPC in the player's social circle maintains:
- `affection` (`-1.0` to `+1.0`): Warmth and friendliness in dialogue and interactions.
- `respect` (`0.0` to `1.0`): Admiration for the player's competence, reliability, and business acumen.
- `trust` (`0.0` to `1.0`): Reliability in financial and personal commitments.
- `envy` (`0.0` to `1.0`): Resentment triggered by rapid increases in `perceived_wealth` relative to the NPC's own status.

### REQ-SOC-003: Archetype Behavioral Responses to Growing Wealth
1. **Childhood Friend / Mentor (`ARCHETYPE_LOYAL_FRIEND`, e.g., "Marcus"):**
   - High baseline `affection`. Happy when the player succeeds (`respect` rises with wealth), but loses `affection` if the player completely neglects social conversation or treats people arrogantly while flaunting luxury. Offers helpful market tips early on.
2. **Envious Peer / Rival (`ARCHETYPE_ENVIOUS_PEER`, e.g., "Devon"):**
   - Friendly when the player is broke (`perceived_wealth < 10`). As `perceived_wealth` climbs past `25`, `envy` rises sharply (`+0.02` per wealth point), causing snide remarks, attempts to undercut prices, or spreading rumors unless the player either keeps a modest profile or hires/collaborates with them.
3. **Opportunist / Hanger-On (`ARCHETYPE_OPPORTUNIST`, e.g., "Chloe"):**
   - Cold and dismissive when `perceived_wealth < 15` (`affection = -0.2`). Once the player wears a tailored suit, buys a sports car, or runs a storefront (`perceived_wealth > 40`), displayed `affection` spikes to `+0.85` while `trust` remains low—approaching the player for "can't-miss" cash handouts or favors.
4. **Institutional Gatekeeper (`ARCHETYPE_BANKER`, e.g., "Director Vance"):**
   - Evaluates `respect`, `trust` (repayment/delivery history), and `perceived_wealth`. Refuses B2B credit lines and commercial property leases when `perceived_wealth + respect * 50 < 25`. Lowers loan interest rates and unlocks prime investments when the player dresses professionally and demonstrates cash flow.
