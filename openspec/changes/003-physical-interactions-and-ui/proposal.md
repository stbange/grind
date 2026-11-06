# Change Proposal 003: Interactive Diegetic UI, Shop Counters, Recipe Mixing & Business Tablet

## 1. Summary
Implement **Step 2 (`003-physical-interactions-and-ui`)** of the post-MVP roadmap:
1. **First-Principles Recipe Mixing & Pricing:**
   - Replace fixed lemonade crafting with a tunable recipe (`recipe_lemons`, `recipe_sugar` per batch) that interacts with ambient weather (`ambient_temp_c` and `weather`) from first principles:
     - Hot days increase customer preference for tart/refreshing balance; cooler/cloudy days increase preference for sweeter/richer balance.
     - skimping on lemons/sugar lowers ingredient cost per batch but reduces `stand_lemonade_quality` and repeat brand trust.
   - Allow real-time adjustment of `stand_price` (`+$/-$`), `recipe_lemons`, and `recipe_sugar`.
2. **Physical 3D Shop Counters & Shelf Displays:**
   - Render 3D market stalls/shelves outside the Wholesale Market with visible crates of Lemons, Sugar, Water Jugs, Cups, and Folding Tables, plus 3D world-space contextual prompt signs showing exact item costs and inventory counts.
3. **Interactive Business & Courier Tablet/Notebook (`[BIZ / B]` Drawer):**
   - Opening the Business Tablet (`B` key or tapping `[BIZ / B]` on touch devices) displays an interactive multi-row management overlay with clickable/touchable rows (and `1-6` keyboard shortcuts):
     - **Row 1:** Craft Lemonade Batch & Tune Recipe (`Lemons/Sugar` balance)
     - **Row 2:** Adjust Stand Price (`-$0.20` / `+$0.20`)
     - **Row 3:** Claim/Inspect Active Courier Contract (showing weight, distance, and reward)
     - **Row 4:** Hire Stand Operator (`$45`) or Courier Employee (`$65`)
     - **Row 5:** Launch Marketing Campaign (`$50`)
     - **Row 6:** Lease Storefront (`$350`) / Invest in Supplier Equity (`$200`)

## 2. Motivation
Empowers the player to experiment with price elasticity, ingredient quality vs. unit cost margins, courier contract selection, and empire investments through both tactile 3D counters and an intuitive desktop/touch Business Tablet.
