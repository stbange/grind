# Specification 02: Spatial World, Kinematics & Container Physics

## 1. Purpose
Specifies the 3D coordinate system, city block topology, entity movement kinematics, encumbrance physics, and container volume/mass constraints.

## 2. Coordinate System & City Layout
- Right-handed 3D coordinate system:
  - `+X`: East, `-X`: West
  - `+Y`: Up (vertical elevation / building height)
  - `+Z`: South, `-Z`: North
- The city covers `[-120m, +120m]` on X and Z, containing functional zones with distinct foot-traffic and economic profiles:
  1. **Alleyway & Tenement (`(-60, 0, -60)`)**: Player's starting dilapidated room (`Rent: $0` initially, noisy, poor sleep recovery) and the street corner where the initial lost cash (`$12.00`) lies on the ground.
  2. **Wholesale Grocery & Hardware Market (`(-35, 0, -10)`)**: Sells raw commodities (lemons, sugar, cups, water jugs, flyers, portable folding table, courier satchel, energy bars, fresh meals).
  3. **Central Plaza & Park (`(0, 0, 0)`)**: High pedestrian foot traffic; prime spot for beverage/snack stands.
  4. **Bike & Vehicle Shop (`(40, 0, -20)`)**: Sells second-hand bicycle, cargo e-bike, delivery van, and personal sports coupe.
  5. **Logistics & Courier Dispatch Hub (`(30, 0, 35)`)**: Generates physical parcel delivery contracts between city buildings.
  6. **Financial District & Bank (`(-10, 0, 50)`)**: Offers savings accounts, B2B credit lines, commercial storefront leases, supplier equity investments, and business courses.
  7. **Boutique & Lifestyle District (`(55, 0, 55)`)**: Sells apparel upgrades (streetwear, durable courier gear, tailored suit, luxury watch) and real-estate upgrades (clean studio apartment, luxury penthouse villa).

## 3. Items, Mass, Volume & Spoilage
### REQ-PHY-001: Physical Item Properties
Every `GrindItemType` defines:
- `unit_mass_kg`: Physical weight in kilograms.
- `unit_volume_l`: Physical volume in liters.
- `spoilage_rate_per_hour`: Rate at which perishable goods (lemons, prepared lemonade, fresh meals) lose quality.
- `base_wholesale_price`: Baseline market cost before supply/demand modifiers.

### REQ-PHY-002: Container Capacity & Encumbrance
- Bare hands (`CONTAINER_HANDS`) hold up to `5.0 L` volume and `6.0 kg` mass.
- Equipping a **Satchel (`ITEM_SATCHEL`)** increases player carrying capacity by `+30.0 L` volume and `+18.0 kg` mass.
- Equipping a **Bicycle (`ITEM_BICYCLE`)** adds a rear basket (`+20.0 L`, `+15.0 kg`) and increases base movement speed from `4.2 m/s` (walking) to `11.5 m/s` (cycling).
- Equipping a **Delivery Van (`ITEM_DELIVERY_VAN`)** adds `+400.0 L` / `+250.0 kg` capacity and `18.0 m/s` movement speed with minimal stamina drain.
- **Encumbrance Formula:**
  - If `carried_mass_kg > max_comfortable_mass_kg`, movement speed is scaled by `max(0.25, 1.0 - (overflow / max_mass))` and stamina drain per meter increases proportionally.
  - Attempting to pick up an item that exceeds available `max_volume_l` or absolute `max_mass_kg` fails with a physical capacity error.
