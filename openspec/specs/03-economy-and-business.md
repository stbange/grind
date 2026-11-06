# Specification 03: First-Principles Economy & Business Progression

## 1. Purpose
Defines how business operations emerge from physical transformation, logistics, utility-based NPC purchasing decisions, employee task delegation, marketing propagation, and capital investment.

## 2. Crafting & Retail Point-of-Sale (e.g., Lemonade Stand)
### REQ-ECO-001: Physical Production & First-Principles Recipe Mixing
- Combining `recipe_lemons_per_batch` Lemons + `recipe_sugar_per_batch` Sugar Packs + `1x Water Jug` + `5x Paper Cups` at a placed `Folding Table` (or `Commercial Storefront`) produces `5x Cups of Lemonade`.
- Product quality emerges from ingredient freshness, operator `skill_production`, and recipe balance relative to ambient temperature (richer 2-lemon/2-sugar recipes achieve premium quality `>0.90`, supporting higher retail prices).

### REQ-ECO-002: Customer Utility Function
- When an NPC with `thirst > 0.25` walks within `12.0m` of an active point-of-sale stocking beverages at `unit_price`, the NPC computes purchase utility:
  $$U = (\text{thirst} \times \text{temp\_factor} \times (0.7 + 0.6 \times \text{quality}) \times (1.0 + 0.5 \times \text{brand\_awareness})) - (\text{unit\_price} \times \text{price\_sensitivity})$$
- If $U > 0.0$ and `npc.cash >= unit_price`:
  - The NPC purchases 1 unit, transfers `unit_price` to the stand's cash box (or owner's account), consumes the beverage (`thirst` reduced by `0.6`), and updates local `brand_trust` based on `quality - price_ratio`.

## 3. Logistics & Courier Contracts
### REQ-ECO-003: Physical Parcel Delivery
- City businesses continuously generate `GrindCourierContract` offers at the Dispatch Hub based on simulated inventory needs.
- Each contract has:
  - `pickup_pos`, `dropoff_pos`
  - `parcel_mass_kg` (`1.5 kg` to `12.0 kg`), `parcel_volume_l` (`3.0 L` to `20.0 L`)
  - `reward_cash` (scaled by distance, mass, and urgency)
  - `deadline_hours`
- Claiming a contract places the physical parcel into the carrier's inventory (requiring sufficient volume/mass capacity, e.g., a Satchel or Bicycle crate).
- Delivering to `dropoff_pos` before `deadline_hours` pays `reward_cash` and increments `skill_logistics` and `business_reputation`.

## 4. Scaling: Hiring, Marketing & Investing
### REQ-ECO-004: Task Graph Delegation (Hiring)
- The player can hire unemployed or job-seeking NPCs as employees (`ROLE_STAND_OPERATOR` or `ROLE_COURIER`) at an agreed hourly wage (`wage_per_hour`).
- Hired employees automatically execute the corresponding first-principles loop:
  - **Stand Operator:** Restocks ingredients from company storage, crafts lemonade batches, and serves passing NPCs at the assigned table/storefront.
  - **Courier Employee:** Equips company-provided satchel/bike/van, claims available contracts at the Dispatch Hub, physically moves along the route to pick up and deliver parcels, and deposits revenue minus wage into the player's company ledger.
- Employee productivity scales with their `skill`, `morale` (affected by wage fairness and player `Respect`), and player `skill_management`.

### REQ-ECO-005: Marketing & Brand Propagation
- The player can distribute flyers manually or fund marketing campaigns (Street Flyers, Local Radio Ad, Citywide Billboard Campaign).
- Marketing increases `brand_awareness` (`0.0` to `1.0`) across NPCs in the targeted district, widening the detection radius of player businesses and increasing the utility value $U$ in `REQ-ECO-002`.

### REQ-ECO-006: Investing & Equity
- The player can invest retained earnings into:
  1. **Commercial Storefront Lease/Purchase:** Replaces a weather-exposed folding table with a permanent high-capacity shop (`+60%` customer attraction, protected from rain, holds 200L stock).
  2. **Wholesale Supplier Equity:** Purchasing shares (`10%` to `50%`) in the Wholesale Market reduces raw ingredient purchase costs by up to `35%` and pays daily dividend yield.
  3. **Index / Local Business Venture Fund:** Generates compound daily yield (`~2.5%` per simulated day) with minor market variance.
