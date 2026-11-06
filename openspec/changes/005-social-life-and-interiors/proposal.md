# Change Proposal 005: Deep Social Relationship Choices, Personal Lifestyle Courtyard & Procedural Audio

## 1. Summary
Implement **Step 4 (`005-social-life-and-interiors`)** of the post-MVP roadmap:
1. **Interactive Social Relationship Choices (`grind_action_social_choice`):**
   - Talking to the 4 key NPCs (`Marcus`, `Devon`, `Chloe`, `Director Vance`) now supports concrete relationship decisions with lasting economic and social consequences:
     - **Partner / Reconcile with Envious Peer (`Devon`):** Offer Devon a profit-sharing partnership (`$80`) to convert his competing rival stand into an allied franchise (`envy` drops by `-0.60`, `affection` rises to `+0.70`, and his stand's sales now pay franchise royalties to the player!).
     - **Support Loyal Friend (`Marcus`):** Treat Marcus to an artisan meal or mentor bonus (`$15`), boosting his `affection` and `trust`, reducing player `stress` by `-20`, and receiving a permanent `+0.25` boost to `skill_persuasion`.
     - **Set Boundaries with Opportunist (`Chloe`):** Politely decline Chloe's cash handout request (`$0` lost, preserves self-respect and reduces `stress`) OR fund her flashy social PR event (`$120` -> `+30%` `marketing_awareness`).
     - **Negotiate B2B Credit & Equity with Banker (`Director Vance`):** Request a `$250` B2B growth loan when status/respect qualifies, or repay active debt to boost credit trust.
2. **Visible 3D Personal Lifestyle Showcase ("The Good Things in Life"):**
   - Render a dedicated **3D Personal Lifestyle & Residence Courtyard** outside the player's home / Grand Luxe district that visually upgrades as the player purchases personal rewards:
     - **Residence Furnishings:** Upgrades from rusty alley crates (`HOUSING_TENEMENT`) $\to$ a clean patio lounge with café table and plants (`HOUSING_STUDIO_APT`) $\to$ a luxury penthouse terrace with a private swimming pool, lounge chairs, and gold trim (`HOUSING_PENTHOUSE_VILLA`).
     - **Personal Vehicle Showcase:** Renders the player's parked personal vehicle (Bicycle $\to$ Cargo E-Bike $\to$ Commercial Delivery Van $\to$ sleek red **3D Personal Sports Coupe** with low-profile wheels, spoiler, and windshield) in their driveway when not actively riding/driving.
3. **Zero-Asset Procedural Audio Synthesizer (`SDL_Audio` / WebAudio):**
   - Real-time C11 procedural sound synthesizer generating footsteps, cash register sale chimes (`+$`), craft/pour sounds, and level-up/luxury fanfare chords—working identically on Linux, Windows, and WebAssembly without external `.wav` files.

## 2. Motivation
Completes the player's journey from finding `$12` in a dingy alley to navigating complex human reactions to wealth and visibly enjoying the tangible rewards of their hard work in 3D.
