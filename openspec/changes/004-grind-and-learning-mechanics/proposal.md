# Change Proposal 004: Deepening "The Grind", Experiential Business Learning & Rival Competition

## 1. Summary
Implement **Step 3 (`004-grind-and-learning-mechanics`)** of the post-MVP roadmap:
1. **Experiential Business Learning & Insights Discovery:**
   - Rather than having full market transparency from Day 1, the player discovers 4 business insights organically by practicing business, talking to customers, or reading the Business Handbook:
     - `INSIGHT_UNIT_ECONOMICS`: Unlocked after crafting `>= 2` batches or reading a handbook -> reveals exact unit cost ($/cup), gross profit margin %, and break-even metrics on the HUD/Tablet.
     - `INSIGHT_DEMAND_ELASTICITY`: Unlocked after selling `>= 10` cups -> reveals live customer conversion propensity and weather-adjusted optimal pricing.
     - `INSIGHT_ROUTE_OPTIMIZATION`: Unlocked after `>= 3` courier deliveries -> highlights high-yield/kg contracts and reduces stamina drain on routes by an extra `15%`.
     - `INSIGHT_COMPETITOR_INTEL`: Unlocked when `skill_persuasion >= 1.0` or talking to NPCs -> reveals rival vendor pricing and how to out-compete them on quality or brand awareness.
2. **Emergent Rival Street Vendor Competition:**
   - As the player's `perceived_wealth` or `cups_sold_total` grows (`>= 15` cups sold), `Devon (Street Peer)` opens a competing street stall (`rival_stand_active`) near Central Plaza when his `envy` rises.
   - NPCs walking through the plaza compare the **Customer Utility ($U_{\text{player}}$ vs. $U_{\text{rival}}$)** of both stands from first principles! If the player price-gouges with low quality, customers flock to the rival stand; if the player improves recipe quality (`2L/2S`), runs marketing, or prices competitively, the player wins market share.
3. **Visceral Physical & Weather Grind Feedback:**
   - **Exhaustion & Stress Camera Sway:** When `stamina < 25` or `stress > 70`, the First-Person camera exhibits subtle fatigue sway/roll and a fatigued vignette warning on the HUD.
   - **3D Rain & Weather Exposure:** During `WEATHER_RAINY`, 3D rain streaks fall across the city, and working outdoors without a leased storefront increases `stress` and reduces foot traffic.

## 2. Motivation
Makes both the physical/mental grind and the intellectual journey of learning business feel earned, systemic, and reactive.
