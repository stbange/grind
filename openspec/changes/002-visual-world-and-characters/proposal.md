# Change Proposal 002: Richer 3D World, Articulated Characters & Architectural Detail

## 1. Summary
Upgrade the 3D presentation and spatial simulation feedback of **The Grind** (`Step 1` of the post-MVP roadmap):
1. **Articulated 3D NPCs & Employees:** Replace single-box NPCs with hierarchical multi-part 3D characters (torso, head, hair/hat, swinging left/right arms and legs synchronized with movement stride, held lemonade cups or courier parcels, and posture/aura indicators reflecting `affection`, `envy`, and `respect`).
2. **Detailed 3D City Architecture & Environment:**
   - Multi-story buildings with recessed windows (which glow warmly at dusk/night), entrance doors, striped shop awnings, rooftop AC units, and 3D landmark signs.
   - Asphalt roads with dashed lane markings, raised concrete sidewalks, park trees, benches, and streetlamps that automatically illuminate in the evening (`time_of_day_hours >= 18.0` or `< 6.5`).
3. **Tactile First-Person Viewmodel & Props:**
   - Articulated left and right arms/hands in First-Person view showing sleeve fabric tier, Swiss watch, held items (folded table, lemon/ingredients crate, parcel box), or gripping a detailed bicycle handlebar stem with a rotating front wheel fork / sports car steering wheel & dashboard.
4. **3D World-Space Projected Labels, Price Signs & Speech Bubbles:**
   - True 3D-to-2D screen-space projection (`grind_math_project_to_screen`) rendering floating landmark names above buildings, live price/stock boards over the player's lemonade stand, floating `+$1.60` sale popups, and NPC speech/reaction bubbles when nearby.

## 2. Motivation
Transforms the visual experience from abstract geometric blocks into a living, readable 3D street-level world where business activity, physical items, and NPC social reactions are immediately visible at a glance.
