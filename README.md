### 10.10.2026 Colonization 2071 v2.2.4 Patch Notes:<br>
---
☑️Removed the repeated scan through every profession formerly used to add a direct-production supply tier; direct-job recognition now uses the evaluated profession and local plot data.<br>
☑️Reused the current Improvement's already calculated yield change. Candidate Improvement checks run only when the old filter would discard a weak positive yield.<br>
☑️Domestic demand and overflow-sale calculations are skipped when projected stock cannot exceed storage capacity. No additional map searches or persistent save data were added.<br>
☑️Domestic demand is calculated once per stocked cargo yield before domestic sales, and empty stocks skip the demand scan. Resident demand metadata is read once per unit; consumption, Credits, warehouse decay and overflow sales retain their existing results.<br>
☑️The Technology Advisor groups technologies by category once per redraw, preserving technology order, current research checks, widgets and prerequisite arrows.<br>
☑️Combat metadata used only by the log is read after logging is enabled; Progenitor AI rewards, Credits and random draws retain their existing behaviour.<br>
☑️Polished some images (icons).<br>
<br>
🌀Reworked Transcendence Victory:<br>
☑️New building: 🌀Eye of Pandora.<br>
☑️Transcendence is achieved by completing the Eye of Pandora; the previous Progenitor Tech export requirement has been removed.<br>
☑️Construction requires every technology to be researched, excluding branch headings. Only one Eye of Pandora can be under construction worldwide at a time.<br>
☑️The constructing Colony becomes the Capital immediately and retains this status throughout construction. Losing this Colony eliminates the entire civilisation.<br>
☑️All owned Colonies contribute their Industry and share the required goods from their warehouses. Their ordinary construction projects remain paused, with their queues and existing progress preserved.<br>
☑️Once started, construction cannot be cancelled, reordered or hurried. The hurry button is hidden during construction.<br>
☑️The Eye of Pandora appears in its Colony from the start of construction. Its model increases in scale by 0.1 with each turn of construction. (It will cover the game world in "Warp darkness.")<br>
☑️After the constructing player leaves the Colony screen and acknowledges a Progenitor Exarch's warning, ordinary civilisations and all Progenitor Exarchs begin a crusade to capture the constructing Colony. States and the Outer Gods Pantheon are excluded.<br>
☑️Each Progenitor Exarch deploys a separate copy of its current Progenitor Expeditionary Force. These forces follow normal invasion rules with the constructing Colony as their highest-priority target; the original REF remains available for Revolution.<br>
☑️Other eligible human players acknowledge a separate Exarch announcement before joining the war. The announcement identifies the constructing civilisation and Colony.<br>
☑️The 11×11 area centred on the constructing Colony remains visible to the ordinary civilisations joining the crusade until that Colony is lost. Peace with the constructing civilisation is blocked during construction; relations among ordinary civilisations continue normally.<br>
☑️Progenitor Exarchs form an alliance for the active crusade, ending any wars among themselves and preventing new ones until the constructing Colony is lost. The alliance then ends immediately; normal diplomatic relations resume without automatically restarting previous wars.<br>
☑️Progenitor Exarchs always raze Colonies captured during the crusade, leaving no Colony ruins. After the constructing Colony is lost, the crusade's REF copies withdraw through Sail to Earth departure points and disappear with their cargo; the original REF is preserved.<br>
☑️During withdrawal, transports collect their own land troops and disappear with them at Sail to Earth departure points. Remaining warships withdraw only after all their own withdrawing land troops, including embarked troops, have departed through Sail to Earth; the original REF remains untouched.<br>
☑️A new Transcendence construction starts a fresh crusade with new REF copies. Forces still withdrawing from an earlier crusade do not rejoin its attack.<br>
☑️Computer opponents wait for military readiness before committing to construction, comparing their armed forces, fleet and target Colony's garrison with the expected crusade forces.<br>
☑️Computer opponents pay three times the required goods by default, excluding Industry.<br>
☑️AI construction discounts and AI era cost modifiers do not apply to the Eye of Pandora.<br>
☑️The Victory screen shows research eligibility, the constructing Colony, pooled Industry and required goods. Special Abilities identifies the building as the Transcendence Victory Condition.<br>
<br>
💻Computer Opponents (Both):<br>
☑️Reduced repeated calculations when evaluating technologies, selecting Colonies for specialists and choosing new settlement locations.<br>
☑️Production buildings of the same category reuse their supply and expert evaluations during a single building choice.<br>
☑️Work assignment reuses the current worker evaluation while preserving profession choices and replacement decisions.<br>
☑️Skipped the default empty Python AI hooks; custom Python overrides can be enabled through GlobalDefinesAlt.xml.<br>
☑️Preserved AI priorities, candidate order, random draws and save format; later decisions use the current Colony and unit state.<br>
☑️AI now values passive building production independently of indoor jobs, so buildings without job slots no longer lose their economic benefits in the assessment.<br>
☑️Mass Driver is valued for additional protection against bombardment, with greater priority during war or immediate danger and no added value beyond full protection.<br>
☑️Hydroelectric Plant is valued for Industry and river production from usable local jobs; mutually exclusive jobs are not added together, and the Colony plot retains its single natural cargo yield.<br>
☑️Interstellar Bank is valued for Platinum and Earth Goods production, including its sea bonus through usable local jobs.<br>
☑️Space Elevator is valued for additional Credits from actual warehouse overflow after existing AI assistance, domestic consumption, decay, Earth tax and trade modifiers are taken into account.<br>
☑️AI gives a proportional preference to useful input-free Research, Industry, Tools, Weapons, Progenitor Tech, Biotech, Narcotics, Fusion Cores and Earth Goods produced directly by a Bonus or Improvement. This changes job evaluation only; actual output and consumption are unchanged.<br>
☑️Small positive direct production remains a useful job when the Colony can feed its workers. Jobs still compete by their evaluated benefit, including consumed goods and available inputs.<br>
☑️When supplying construction and equipment, Colonists and ordinary Aliens compare positive net production benefit. Input-free work no longer automatically outranks a much more productive factory through an extra fixed priority tier.<br>
☑️Improvement evaluation retains small positive yields contributed by the Improvement itself, including the projected upgrades already considered by the worker AI.<br>
☑️Routine job evaluation counts the additional warehouse loss caused by a worker and the additional overflow-sale Credits after domestic demand, Earth tax and trade modifiers. Existing stocks' losses are not charged entirely to the new worker; refunded goods are not counted twice.<br>
☑️AI worker bonuses cannot make a land profession productive on water or a water profession productive on land. Research jobs still require available research and Colonist professions retain their existing unlock requirements.<br>
☑️Profession eligibility uses the existing list of related technologies while retaining the same profession restrictions, research checks and equipment requirements.<br>
☑️Worker exchanges reuse unchanged job scores within a single decision. The original evaluation path remains available when external rule callbacks are enabled.<br>
☑️Job evaluation skips a plot-production calculation whose result would immediately be replaced by the existing AI worker calculation; production and profession choices retain their existing results.<br>
☑️Industrial planning and market valuation reuse profession output already returned by the input calculation, reducing repeated production-building scans.<br>
☑️Space builders and excavation workers collect existing construction reservations once per decision. External building, movement, war and research callbacks retain live mission queries.<br>
☑️Empty ships skip port-unit scans when checking their own passengers or cargo. Available ships still process Explorer pickup requests.<br>
☑️These optimisations retain existing gameplay rules, building bonuses, candidate order, random draws and saved-game data, with no new persistent world-state caches.<br>
☑️Research and technology purchase decisions value the path to Freighter or Carrier production, including the required Dock and Dry Dock. Transport benefits account for cargo capacity, the current fleet, waiting Earth passengers and potential Credits from compatible Progenitor Treasures; already available production, owned ships and queued ships reduce redundant priorities.<br>
☑️Colonist AI and Alien AI prioritise the infrastructure needed to produce Freighters and Carriers, including the earlier required Dock. Ship production follows transport demand and queued capacity, adding more ships when capacity is insufficient without forcing an extra ship when the fleet already covers demand.<br>
☑️Technology purchases from the AI's own State or Progenitor Exarch use half of the current XML technology contact delay. Other technology contacts retain the full XML delay; the asking price, technology effects and payment rules remain unchanged.<br>
<br>
🧑‍🚀💻Colonist AI:<br>
☑️Intrepid Explorers check compatible ships once per pickup decision and reuse exploration targets and incoming missions for each landing area.<br>
☑️Ships reject impossible Earth trips before scanning the map for a departure route.<br>
☑️Power Plant and Oil Refinery jobs are assessed against local Industry production, allowing useful construction outside major Colonies while accounting for Hydrocarbons availability.<br>
☑️Oil Refinery's additional indoor cargo production is valued together with extra input goods and Food consumption; unsupported or unprofitable extra production adds no value.<br>
☑️Food forecasts remove the production of both the moving and displaced workers before giving priority to direct production or construction supplies, using the Colony's Food production modifier.<br>
<br>
👽💻Alien AI:<br>
☑️Ordinary Aliens compare routine jobs and useful worker swaps on the shared economic scale. Existing priorities for preventing starvation, recruitment Food, independence preparations, growing improvements and specialist roles remain in place.<br>
☑️Larger surpluses of Credits reduce the estimated opportunity cost of spending Credits on technology, while the full asking price is still paid.<br>
### 08.10.2026 Colonization 2071 v2.2.3 Patch Notes:<br>
---
☑️Optimised building lookups and Colony production calculations.<br>
☑️Reduced repeated checks of profession requirements, research availability and research materials.<br>
☑️Optimised technology bonuses for routes and yield calculations for Improvements.<br>
☑️Reduced repeated data lookups when updating Colony workers, garrisons and transport cargo.<br>
☑️Streamlined the Technology Advisor while preserving its layout and prerequisite arrows.<br>
☑️Removed unnecessary data lookups when combat logging is disabled, while preserving Progenitor AI rewards.<br>
☑️Fixed failed resident departures clearing professions. Residents remain in their Colony until their default map profession becomes available.<br>
☑️Fixed Colony yield modifier tooltips to include technology bonuses and count tax bonuses only once.<br>
☑️Corrected displayed production totals during material shortages and Immigration conversion.<br>
☑️Fixed processing jobs being selected despite producing no actual output; AI-only worker bonuses no longer hide zero production.<br>
☑️Fixed workforce replacement loops that could trigger the “AI plot assignment confusion” assertion.<br>
☑️Fixed AI building plans ignoring availability requirements, including civilisation-specific restrictions.<br>
☑️Fixed AI retreats choosing routes that could not be followed immediately. Units wait when they need movement points from the next turn.<br>
☑️Fixed first settlers waiting for an existing Colony instead of moving towards a founding site.<br>
☑️Fixed Alien attackers attempting actions after profession changes exhausted their movement. Transported attackers handle their cargo state before selecting combat missions.<br>
☑️Fixed saved AI ships retaining land combat roles. Affected ships recover an appropriate space role on their next AI turn.<br>
☑️Fixed Alien workforce changes removing Food production needed to prevent starvation, including when Convicts enter education. Food checks now include Colony production modifiers.<br>
☑️Fixed useful Alien specialists remaining unassigned when workforce reassignment assessed their jobs before assigning Food experts.<br>
☑️Worker construction plans refresh after loading a game without changing the saved-game data format.<br>
☑️Alien units now benefit from researched road movement bonuses.<br>
☑️Removed additional Alien settlement restrictions based on Colony distance and nearby ownership; standard founding restrictions still apply.<br>
<br>
💻Computer Opponents (Both):<br>
☑️Specialist job assignments account for Food needs, available workplaces and production materials.<br>
☑️Computer opponents select valid research projects, exclude technology branch headings and stop assigning Research workers when no research is available. Colony jobs are reassessed when research ends or becomes available again.<br>
☑️Technology purchases from the State or Progenitor Exarch account for practical benefits, export income and tax-linked production bonuses. AI compares payments in Credits with tax increases, using tax purchases for urgent or exceptionally useful research.<br>
☑️Technology purchases for Credits preserve an economic reserve.<br>
☑️Computer opponents attack visible feral units when the odds are favourable. Progenitor AI units receive greater target priority because of their rewards, without lowering the required chance of victory.<br>
☑️Computer opponents prioritise work on Lunar Outposts and Lunar Settlements so these Improvements can develop.<br>
☑️Each computer opponent is limited to one Janus Device, including queued construction, unfinished work and upgraded stages. It favours useful defensive locations, and the limit persists in saved games and after the device is lost.<br>
☑️Production choices compare buildings, specialists and ships according to current needs, while retaining emergency defence and valuing useful workplace upgrades.<br>
☑️Transport demand accounts for export stocks, production, journey times, passengers, Progenitor Treasures, waiting Explorers and ships already in production.<br>
☑️AI places greater value on transport ships with larger cargo capacity. Transports deliver passengers before returning to Earth, and recruitment slows when passenger queues grow.<br>
☑️When Earth has a passenger backlog, AI prioritises affordable transport ships while retaining a reserve of Credits; purchases of further specialists wait until the backlog eases.<br>
☑️Mining Vessels, Science Vessels and Colony Ships are produced, purchased and deployed for useful construction tasks. Space builders are purchased as workers rather than passenger transports, and lunar construction is planned on owned moons that a parent Colony can work.<br>
☑️Combat ships, Privateers and UFOs carrying colonists prioritise passenger delivery before resuming their space combat duties.<br>
☑️Industrial planning considers workers, input goods and factories already built or queued elsewhere, avoiding factories without usable raw materials.<br>
☑️Direct manufactured goods from Progenitor Improvements are recognised when assigning workers, evaluating technologies and choosing Colony sites.<br>
☑️Computer opponents reserve Credits for useful construction work and release surplus goods when production requirements change.<br>
☑️Production blocked by missing materials is reconsidered, with greater value placed on shortages of Food and industrial inputs.<br>
☑️Specialist purchases are ranked by useful production gains relative to their cost in Credits, and purchased specialists are delivered to the selected Colony.<br>
☑️Progenitor Treasures are sent to Earth aboard Freighters and Carriers instead of paying the State or Progenitor Exarch to transport them.<br>
☑️Goods otherwise lost to warehouse overflow can be sold without changing Earth market prices or trade-volume tax counters.<br>
☑️Difficulty settings limit ordinary AI tax increases; voluntary technology purchases retain their separate tax payment rules.<br>
☑️On Illuminatus difficulty, AI workers receive a +150% work-rate bonus.<br>
☑️Whenever a capital building is completed, AI selects its best Colony as the capital using its existing assessment. The current capital is retained on a tied score, and the selection runs once for each completed capital building.
<br>
🧑‍🚀💻Colonist AI:<br>
☑️Colonist AI can equip one Intrepid Explorer for free at an owned Colony, with a default cooldown of 35 turns. Waiting Explorers account for this allowance, and its cooldown is retained in saved games.<br>
☑️Workers favour suitable productive Improvements when replacing inherited Alien Fertilizers, Future Farms and Alien Burrows.<br>
☑️Workers can seek revealed Ancient Mounds and establish Excavations using normal construction costs and reward ownership rules.<br>
☑️Intrepid Explorers gain transport priority after a configurable wait; accepted pickup requests retain their assigned unit and destination.<br>
☑️Additional immigration scales with world size, available land and game speed, while retaining independent rolls and dock capacity limits. These immigrants do not consume Propaganda or raise the normal immigration threshold.<br>
<br>
👽💻Alien AI:<br>
☑️Civilian units join useful Colony jobs instead of remaining idle outside, while armed defenders retain their military duties.<br>
☑️Transports without a usable trade route can return to a reachable owned Colony and unload their goods.<br>
☑️Venerable Elders follow revised priorities for Research sites and Liberty, Research and Propaganda workplaces. Research jobs require an available research project.<br>
☑️Brilliant Polymaths prioritise Progenitor Tech sites and workplaces, followed by Narcotics and Fusion Cores, then Biotech.<br>
☑️Industrious Cyborgs prioritise outdoor Weapons and Vehicles production, then outdoor Tools and Industry, followed by suitable indoor jobs. Urgent equipment shortages can raise the priority of Weapons and Vehicles production.<br>
☑️Colony workers are reassigned when a worked reward site disappears or changes ownership, even if its production totals remain unchanged.<br>
☑️Venerable Elders, Brilliant Polymaths, Industrious Cyborgs and acquired human specialists retain civilian jobs rather than switching to Alien Soldier, Alien Mecha or Alien Corsair professions.<br>
☑️The Alien Soldier, Alien Mecha and Alien Corsair professions are restricted to Aliens, Elite Warriors and Alien Crossbreeds. Convicts prioritise schooling or lessons in Alien settlements when the Colony can spare their labour.<br>
☑️Elite Warriors explore their planet without changing unit type or seeking ruin rewards unavailable to Alien civilisations. They favour the Alien Corsair profession when suitable equipment is available.<br>
☑️Ordinary workers follow yield and workplace priorities, fill gaps left by unavailable experts and concentrate indoor Research production in one suitable Colony.<br>
☑️Weapons are reserved for spaceship production when an eligible vessel can be built with the materials available.<br>
☑️Acquired human specialists, including Expert Farmers and Expert Astronauts, replace less productive residents where actual output improves, including Venerable Elders, Brilliant Polymaths and Industrious Cyborgs.<br>
☑️Ordinary jobs and specialist jobs use comparable valuation scales, preserving Food requirements and specialist priorities during worker replacement.<br>
☑️Alien AI uses the same production and market valuation rules as Colonist AI.<br>
☑️Food, production inputs and construction materials are protected when goods are sold.<br>
☑️Desired trade goods prioritise missing construction materials and production inputs.<br>
☑️Specialist recruitment favours Brilliant Polymaths for development; recruitment of Venerable Elders and Industrious Cyborgs adapts to available jobs, resources and shortages.<br>
☑️Progenitor Tech, Narcotics, Fusion Cores and Biotech are reserved for recruitment, accounting for specialists already present, travelling or in production.<br>
☑️Elite Warriors are recruited for exploration and military needs; Cultivators are recruited where more improvement workers are needed.<br>
☑️Ordinary Aliens needed for defence remain available as soldiers. Military professions use existing Weapons and Vehicles, while Colony specialists retain civilian roles.<br>
☑️Food assignments support upkeep and modest growth while maintaining work on developing lunar colonies, Alien Fertilizers and Alien Burrows.<br>
☑️Venerable Elders, Brilliant Polymaths and Industrious Cyborgs are kept out of unnecessary surplus Food jobs; ordinary workers help gather Food required for recruitment.<br>
☑️Specialist recruitment responds to individual Colony needs, allowing useful economic buildings to compete with routine recruitment.<br>
☑️The Alien unit's positive production bonuses count towards job suitability, while its production penalties are ignored during assessment. Actual production still applies all bonuses and penalties.<br>
☑️Surplus Food can be redistributed for unit production while preserving the donor Colony's upkeep, growth and production needs.<br>
☑️Army planning preserves ships, builders, explorers and civilian specialists in their appropriate roles.<br>
☑️Colonies preparing for independence prioritise assigning their first worker to Liberty production while below 75% Rebel Sentiment, provided their Food needs are met.<br>
<br>
👽🟩Alien:<br>
☑️Alien (Replaces Free Colonist) Research production: -1 → 0.<br>
<br>
🤖🔳Royal Expeditionary Force (Earth):<br>
☑️Expeditionary Forces stage sufficient transport capacity and load ships according to their compatible cargo limits.<br>
☑️Expeditionary Forces use configurable landing waves and valid Deep Space entry plots. Only empty ships needed for reinforcements return to Earth; spare ships support the war.<br>
<br>
👾🔳Royal Expeditionary Force (Progenitor):<br>
☑️Expeditionary Forces stage sufficient transport capacity and load ships according to their compatible cargo limits.<br>
☑️Expeditionary Forces use configurable landing waves and valid Deep Space entry plots. Only empty ships needed for reinforcements return to Earth; spare ships support the war.<br>
### 06.10.2026 Colonization 2071 v2.2.2 Patch Notes:<br>
---
☑️Fixed further assertion failures and crashes involving professions, production yields and unit movement.<br>
☑️Fixed Civilopedia errors when displaying Constitution and technology help without an active player.<br>
☑️Fixed the Technology Advisor crashing when the Visionary Researcher progress threshold was zero or unavailable.<br>
☑️Fixed trait promotion updates for Colony residents and repeated trait bonuses, preventing negative promotion counts and errors after profession changes.<br>
☑️Fixed missing dialogue for leaders unique to 2071.<br>
☑️Excavations can be established worldwide. Reward events belong to the player who built the site, regardless of its distance from their territory or whether the land is neutral or owned by another civilisation.<br>
☑️Starting technologies apply their effects and bonuses to Alien civilisations and both Imperial Expeditionary Forces.<br>
☑️Restored rewards in Credits for being the first to research Venture Capital and Corporate Investment. Starting knowledge and acquired technologies neither claim nor block these rewards; they go to the first civilisation to complete the research.<br>
☑️Fixed AI ships being unable to return from Earth because of invalid map locations. Purchased and free ships use valid entry plots, and affected ships are relocated together with their cargo.<br>
☑️Fixed errors in AI profession assignment, worker exchanges and resident removal when no valid profession or unit was available.<br>
☑️Fixed AI diplomatic attitude calculations and Credits trade valuations when a civilisation was evaluated against itself.<br>
☑️Fixed material-transfer calculations using outdated requirements when deliveries resumed or completed production.<br>
☑️Fixed AI counterattacks being attempted after movement was exhausted or a unit became immobilised.<br>
☑️Improved textures for The Chief of Staff (NAFTA Colonies) and NAFTA President (NAFTA).<br>
☑️Improved textures for many leader backgrounds.<br>
<br>
💻Computer Opponents (Both):<br>
☑️Improved protection of civilians, Progenitor Treasures and transports. Units in these roles avoid dangerous routes even when they have combat strength.<br>
☑️Colony threat assessments account for nearby rivals, war plans and diplomatic relations.<br>
☑️Ships in planetary ports use the appropriate area for their movement rules: regular space units use connected space, while amphibious Alien ships retain their current land or space area.<br>
☑️Danger checks detect hostile amphibious units across the boundary between planetary terrain and space.<br>
☑️Pickup requests include transports docked at Colonies.<br>
☑️Suitable experts replace less effective workers in occupied jobs, and locked workers do not prevent other valid replacements.<br>
☑️Job selection considers all a unit's production bonuses, including multiple specialities such as those of Industrious Cyborgs, Brilliant Polymaths and Venerable Elders.<br>
☑️Building selection favours facilities that available experts can staff and that have the required production materials.<br>
☑️Hardy Laborers and Cultivators prioritise construction over Colony jobs.<br>
☑️Mining Vessels and Science Vessels search space for suitable tasks, including on unexplored plots. Colony Ships seek construction tasks within their own territory.<br>
☑️Workers place greater value on Food when choosing Improvements.<br>
☑️Improvement choices account for Plasteel, Progenitor Artifacts, Hydrocarbons and bonus resources, reducing replacements that would lower valuable production.<br>
☑️Each newly founded AI Colony can receive one free defender from its civilisation's eligible units.<br>
☑️The last starving resident can receive +20 Food. At Normal game speed, Colonist AI pays up to 100 Credits while Alien AI receives this aid for free; the price adjusts to game speed.<br>
☑️Surplus materials other than Food are transferred between Colonies every 10 turns to supply current production, preserving the donor Colony's own needs and a 50-unit reserve of each material.<br>
☑️Event rewards match the civilisation, including Human and Alien starting events.<br>
☑️Computer opponents receive all valid rewards from events with multiple choices; human players still choose one option.<br>
☑️AI Progenitor Treasure rewards no longer replace Excavations with Ruined Settlements. Active sites can produce further discoveries in later turns.<br>
☑️Colonies without a suitable unit or building choice receive a fallback production order.<br>
<br>
🧑‍🚀💻Colonist AI:<br>
☑️Intrepid Explorers explore planetary land and collect rewards from eligible sites.<br>
☑️After a planet is explored and its reward sites are exhausted, an available transport can take an Intrepid Explorer to another planet. The unit receives the Explorer profession before its first transfer and retains it for later expeditions.<br>
☑️After all land and reward sites are exhausted, Intrepid Explorers can be equipped as Mecha Pilots for offensive or defensive duties.<br>
☑️Useful training in Alien settlements is considered throughout the game, including settlements controlled by a human player. Units visit the chief before starting lessons.<br>
<br>
👽💻Alien AI:<br>
☑️Convicts are considered for schooling or lessons in other Alien settlements, accounting for training speed, travel time and available jobs.<br>
☑️School graduates can become any eligible teachable specialist for free, without requiring that specialist to be present beforehand.<br>
☑️Improved combat profession selection for counter units.<br>
☑️Normal new games outside Advanced Start provide a starting Saucer, matching human-controlled Alien civilisations.<br>
☑️Transported counter units and defenders unload at suitable Colonies or wait aboard, without attempting ground actions or actions that require movement they no longer have.<br>
☑️Military units recheck movement after profession changes and wait when they can no longer move.<br>
<br>
🧑‍🚀🟦Human:<br>
☑️The Intrepid Explorer now Can Explore Rival Territory.<br>
❌Added unique dialogue lines for Sayyadina and Director Black.<br>
☑️Updated the visual model of the Intrepid Explorer.<br>
<br>
🛕🟧India Colonies (Human):<br>
❌Units now speak Indian languages.<br>
<br>
🪽🟥Polish Colonies (Human):<br>
❌Units now speak Polish.<br>
<br>
👽🟩Alien:<br>
☑️Fixed movement and pathfinding for amphibious Alien ships. They can now enter planetary coast tiles with their cargo, without requiring a passenger landing.<br>
☑️Removed Lunar Outpost, Lunar Settlement and Lunar Colony from Advanced Start and re-enabled Alien unit purchases, with prices adjusted according to game speed.<br>
☑️Alien units controlled by humans or AI can now speak with chiefs in other Alien settlements and learn eligible professions there. Units cannot speak with their own chief, learn in their own settlement or trade with their own settlement.<br>
☑️Player-controlled Alien settlements now display their desired trade goods and teaching speciality to their owner as well as visitors.<br>
☑️Alien (Replaces Free Colonist) can now be produced at a cost of 100 Hammers and 100 Food.<br>
☑️Venerable Elder (Replaces Firebrand Demagogue) Research production: 0 → 3.<br>
☑️🪬Improved the quality of Cyean symbols.<br>
☑️🐯Improved the quality of Felid flag banners and symbols.<br>
☑️🐍Improved the quality of Reptilian flag banners and symbols.<br>
<br>
🤖🔳Royal Expeditionary Force (Earth):<br>
☑️The State knows all technologies from turn 1 and receives their effects and bonuses. Colonial leaders with Annoyed or better relations can purchase technologies from the State through the normal trade window, paying in Credits or accepting a higher tax rate. This also provides a technology trading partner in solo games.<br>
☑️Technology purchases require peace with the player's own State and all prerequisite technologies to be known before the deal. Buying a prerequisite in the same offer does not unlock its successor. Valid tax offers are accepted regardless of their Credit value.<br>
<br>
👾🔳Royal Expeditionary Force (Progenitor):<br>
☑️The Progenitor Exarch knows all technologies from turn 1 and receives their effects and bonuses. Alien leaders with Annoyed or better relations can purchase technologies from him through the normal trade window, paying in Credits or accepting a higher tax rate. This also provides a technology trading partner in solo games.<br>
☑️Technology purchases require peace with the player's own Progenitor Exarch and all prerequisite technologies to be known before the deal. Buying a prerequisite in the same offer does not unlock its successor. Valid tax offers are accepted regardless of their Credit value.<br>
☑️Updated the visual model of the artillery unit.<br>
<br>
🪼🌌Outer Gods Pantheon (Invasion):<br>
☑️Extended the music theme.<br>
☑️The Outer Gods Pantheon is now recognised as a barbarian civilisation, with a highly aggressive and hostile leader.<br>
☑️Improved faction setup in new games. If absent, the Outer Gods Pantheon is created when a free player slot and a free team slot are available, keeping ownership of barbarian map spawns consistent.<br>
☑️The faction now remains active even when it has no units or Colonies.<br>
☑️After the initial peace period, its leader declares war on every known civilisation whenever game rules allow it, including while other wars are already under way.<br>
☑️The leader no longer initiates negotiations. Players can still open a conversation, but he refuses all diplomatic proposals and requests, including peace, trade and gifts.<br>
☑️Defeating a Progenitor AI unit now rewards the player with a Progenitor Treasure unit of random value and 500 Credits ( 1000 for AI players).<br>
### 02.10.2026 Colonization 2071 v2.2.1 Patch Notes:<br>
---
☑️Fixed many Assert Failed errors.<br>
☑️Created a suitable environment for future language translations. Added lore snippets and updated Civilopedia text guidelines for gameplay features across all content. (Thanks, @Gammazytron) Separated mod-only text from base-game content.<br>
☑️Improved the texture quality of the following leaders: EU Prime Minister (European Union), Androrc (Felids), Dagon (Ichthyoids), Progenitor Exarch (The Progenitors), Russian Premier (Russian Federation), Sayyadina (Caliphate Colonies), Quetzalcoatl (Reptilians), Kailric (Sasquatch), and The Lady in Black (Syndicate Colonies).<br>
☑️Improved the texture quality of many leader backgrounds.<br>
<br>
🧑‍🚀🟦Human:<br>
☑️Fixed a bug where the Intrepid Explorer unit was not considered a colonist.<br>
👽🟩Alien:<br>
☑️Fixed an issue where the Intrepid Archaeologist was unavailable to Aliens, preventing them from researching Containment Fields.<br>
☑️Fixed a bug where the Alien Crossbreed could not work at a Research Lab station, preventing Aliens from ever researching Alien Autopsy.<br>
☑️Improved the texture quality of the Shamanic Lodge.<br>
☑️🐸Added new flag banner for Glusk the Moist (Amphibians).<br>
☑️🦑Added new flag banner for Dagon (Ichthyoids).<br>
### 07.09.2026 Colonization 2071 v2.2 Patch Notes:<br>
---
❌All previous saved games are no longer compatible.<br>
☑️Fixed severe game crashes related to AI profession assignment.<br>
☑️Fixed an issue where Unique Civilization units could be overwritten and replaced with default units when mousing over technologies that unlock the corresponding unit class.<br>
☑️Increased the maximum number of players in the mod: 32 → 48.<br>
☑️Increased the number of players for each map size by +5.<br>
☑️Increased the number of Alien Civilizations for each map size by +1.<br>
☑️Fixed a bug where a Colony Ship could not build the Janus Device I.<br>
☑️Increased the Credit cost of the unit action "Build the Janus Device": 100 → 5000.<br>
☑️Reduced the Tile Defense of the Janus Device Stages:<br>
I: Acts as a Colony for combat purposes.<br>
II: +200 → +100 % Tile Defense.<br>
III: +400 → +200 % Tile Defense.<br>
IV: +600 → +300 % Tile Defense.<br>
V: +1000 → +500 % Tile Defense. Acts as a Colony for combat purposes.<br>
☑️Added lore snippets and updated Civilopedia text guidelines for gameplay features. More to come! (Thanks, @Gammazytron) Please note that this is a work in progress and it will take a little longer to bring everything together.<br>
☑️Updated the visual model and avatar of the Expert Farmer.<br>
☑️Updated the visual model of the Expert Spore Grower.<br>
☑️Updated all profession icons to match the 2071 sci-fi theme.<br>
☑️Polished many images (icons).<br>
<br>
🧑‍🚀🟦Human:<br>
☑️Caliphate Colonies now start the game with a Freighter instead of a Corvette.<br>
☑️Extravagant Trait no longer gives +1 Platinum per Spaceport. It now gives +50 % Platinum in all Colonies.<br>
☑️Consortium Colonies now start the game with a Merchantman instead of a Corvette.<br>
☑️Nafta Colonies now Starts with Knowledge of: + Arms Trading.<br>
☑️Nafta Colonies now start the game with a Mecha Pilot (Veteran Soldier) instead of a Soldier (Veteran Soldier).<br>
<br>
🛕🟧New Playable Civilization: India Colonies (Human):<br>
☑️New Playable Leader: The Ascetic Cenobite. (Behaves like Samuel de Champlain.)<br>
☑️Ascetic, Agriculturalist.<br>
☑️New Trait: Ascetic (opposite of the Extravagant Trait).<br>
☑️New Trait: Agriculturalist ( +1 Food on plots with 4 Food, Increases Food production by the tax rate, +10 % Food in all Colonies, +1 Food from Farm).<br>
☑️Starts with Knowledge of: Interstellar Ecology, Soil Enrichment.<br>
☑️Starts the game with a (Colonist) Expert Farmer.<br>
<br>
日本🗾Japanese Colonies (Human):<br>
☑️Android (Replaces Master Mechanic) is now 30 % cheaper to recruit on Earth than Master Mechanic.<br>
☑️Android (Replaces Master Mechanic) production costs: 300 → 200 Hammers, 100 → 50 Tools.<br>
☑️Japanese Colonies now start the game with a Mechanized Laborer (Hardy Laborer) instead of a Laborer (Free Colonist).<br>
<br>
🪽🟥New Playable Civilization: Polish Colonies (Human):<br>
☑️New Playable Leader: Minister of Liberty. (Behaves like Simon Bolivar.)<br>
☑️Libertarian, Insurgent.<br>
☑️New Trait: Insurgent (+100% effect of Rebel Sentiment on Strength) ~this is a whole game length effect.<br>
☑️Starts with Knowledge of: Hydrocarbon Geology, Civil Rights, Literacy.<br>
<br>
🪆🟨Russian Colonies (Human):<br>
☑️New unit: Spetsnaz (Replaces Veteran Soldier). Differences are: +1 Strength, +1 Movement Points , + Can See Hidden Units, + Can Explore Rival Territory, + Ignores Terrain Movement Costs, + Starts with Survivalist III promotion.<br>
☑️Russian Colonies now start the game with a Soldier (Spetsnaz) instead of a Merchantman.<br>
☑️Russian Colonies now start the game with a Corvette.<br>
☑️Russian Colonies now Starts with Knowledge of: + Organized Militia.<br>
<br>
🉐🏴Syndicate Colonies (Human):<br>
☑️Reduced the "Ghost in the Shell" Privateer MKII (Replaces Privateer) % Attack vs. Freighter/👽Carrier 50 → 25.<br>
☑️Reduced the "Ghost in the Shell" Privateer MKII (Replaces Privateer) % Defense vs. Merchantman/👽Smuggler 50 → 0.<br>
☑️Reduced the "Ghost in the Shell" Privateer MKII (Replaces Privateer) % Defense vs. Freighter/👽Carrier 50 → 0.<br>
<br>
👽🟩Alien:<br>
☑️🪬New Playable Civilization: Cyeans.<br>
☑️New Playable Leader: Nightinggale.<br>
☑️Intellectual, Diplomatic, Alien.<br>
<br>
👾🔳Royal Expeditionary Force (Progenitor):<br>
☑️Fixed the artillery units count in REF.<br>
☑️Reduced the Warship unit % Defense vs. Cruiser/👽Battlestar/🤖Dreadnought 50 → 0.<br>
<br>
🪼🌌New NON-playable Civilization: Outer Gods Pantheon (Invasion):<br>
☑️All Neutral Feral Units on the world map now belong to the Outer Gods Pantheon.<br>
☑️Giant Spider Movement Points: 1 → 0.<br>
### 23.08.2026 Colonization 2071 v2.1 Patch Notes:<br>
---
❌All previous saved games are no longer compatible.<br>
☑️Added a debugger version of CvGameCoreDLL that displays game errors. If you press Ignore Always, similar errors will not appear again during the current session. (Thanks, Nightinggale)<br>
☑️Optimised the DLL database. (Thanks, Nightinggale)<br>
☑️Fixed a bug where attempting to build an Armory, City Center, or Narcotrafficking Complex on the city screen crashed the game.<br>
☑️Fixed a bug where the Expert Oilman unit and Oilman profession were missing from the Civilopedia and also could not be used in-game.<br>
☑️Fixed a bug where Alien Colonies would not display the icons for sought-after resources.<br>
☑️Fixed a bug where the Plasteel Mill was missing from the Civilopedia.<br>
☑️Fixed a bug where the order of required and allowed technologies in the Research Tree was incorrect.<br>
☑️Fixed a bug where the Brilliant Biochemist unit changed its UV map to that of the Intrepid Archaeologist unit.<br>
☑️The Janus Device Stage I (Improvement Wonder) now requires Military-Industrial Complex research.<br>
☑️The Janus Device Stage I can now be built anywhere in space and consumes the Colony Ship used to construct it.<br>
☑️The Janus Device Stages that previously did nothing now have the following effects:<br>
I: Acts as a Colony for combat purposes.<br>
II: +200 % Tile Defense.<br>
III: +400 % Tile Defense.<br>
IV: +600 % Tile Defense.<br>
V: +1000 % Tile Defense. Acts as a Colony for combat purposes.<br>
☑️Restored several missing events from previous mod versions and integrated them into the current patch.<br>
☑️The Ancient Mound (or Buried Ruins) can now be excavated. (Events can reveal a random Progenitor Improvement buried beneath it. In the future, a failed Excavation may also have a small chance of spawning a Mound Beast mini-boss.)<br>
☑️Restored the Science Vessel's ability to conduct research on the following Terrain Features: Dark Matter, Gravitational Anomaly, Nebula, and Space-Time Anomaly. (Previously, it could only do so on Moon Atmospheres.)<br>
☑️All units with Hidden Nationality now also Can Explore Rival Territory.<br>
☑️For simplicity, all units that are Invisible to Most Units now also Can See Hidden Units.<br>
☑️Hardy Laborers and Cultivators can both take on the Mechanized Laborer profession, which works even faster, has +2 Movement and +4 Strength, but can only defend. Requires 50 Tools and 50 Vehicles.<br>
☑️Xenotoxins and Phase Desequencers promotions can now be obtained by all unit classes.<br>
☑️The Feral Units and Cosmic Horrors categories were added. (Cosmic Horrors are Bosses or Event-only units. Feral Units include the Mound Beast, Giant Spider, Progenitor AI, and Progenitor Killbots.)<br>
☑️Feral Units and Cosmic Horrors can obtain all promotions (except the Unique ones and ones that would give them +% Strength vs. themselves) regardless of unit class rules.<br>
☑️Abomination was reworked and renamed to Greater Warp Demon. The Greater Warp Demon is now a Cosmic Horror.<br>
☑️The Mound Beast and Greater Warp Demons do not spawn yet. They are intended to appear as Event-related unit spawns in the future.<br>
☑️The Revolution Advisor screen has been changed to match the 2071 sci-fi theme.<br>
☑️Updated many profession icons to match the 2071 sci-fi theme.<br>
☑️Improved the Plasteel Mill's texture quality.<br>
☑️Polished many images (icons).<br>
<br>
🔧Known current issues. To be fixed in the near future:<br>
❗Severe game crashes related to AI profession assignment are still present. The issue is currently being investigated. See ASSERT_FAILED.txt for details.<br>
❗Unique Civilization units are overwritten and turned into default units. (Mousing over a technology that Allows the specific unit is the culprit.)<br>
❗The calculation for the number of artillery units in the Progenitor Exarch REF is incorrect. (Cause unknown.)<br>
<br>
🧑‍🚀🟦Human:<br>
☑️New (returning) unit: Colonial Garrison. A mercenary unit that is an effective defender in the early game, but whose offensive capabilities leave much to be desired. Its purchase cost on Earth starts at just 500 Credits but increases by an additional +750 Credits with each consecutive purchase.<br>
☑️Expeditions can now be conducted on an updated list of bonuses: Paleontology Site, Ruined Temple, Nesting Grounds, Spider Lair, Grand Ziggurat, Iridescent Pyramids, Limestone Spire, Antediluvian Catacomb, Sacred Grove, Ruined Biosphere, Plinth Circle, Sacrificial Pit, Obsidian Faces, and Abandoned Redoubt. This requires an Intrepid Explorer with the Laborer profession and consumes the unit. Aliens cannot conduct Expeditions because the Intrepid Explorer is unavailable to them.<br>
☑️Updated the avatars for the Brilliant Biochemist, Expert Farmer, and Expert Spore Grower.<br>
☑️Updated the visual model and avatar image of the Freighter. (Previously the same as the Colony Ship.)<br>
☑️Updated the visual model and avatar of the Laborer profession.<br>
☑️Fixed the missing avatar for the Hardy Laborer with the Laborer profession.<br>
☑️Fixed the missing avatar for Progenitor Treasure when the player selected it and moved the camera far away from the unit.<br>
☑️The Expert Ore Miner is now of red colour, and its avatar has been changed. (Previously the same as the Expert Prospector.)<br>
<br>
日本🗾Japanese Colonies (Human):<br>
☑️Fixed the displayed Japanese Colonies symbol in the Civilopedia, on the Board of Directors screen, and elsewhere.<br>
☑️Updated the avatar for Artificial Intelligence.<br>
<br>
🉐🏴Syndicate Colonies (Human):<br>
☑️Fixed a bug where the Unique Privateer unit could permanently turn into a regular Privateer during gameplay. (Renamed to Privateer MKII).<br>
☑️Updated the avatar image for The Lady in Black.<br>
☑️Improved the quality of Syndicate flag banners and symbols.<br>
<br>
👽🟩Alien:<br>
☑️Changed the Alien second-turn event reward from a Fabrication Plant to a Construction Facility + Cultivator unit. (Skipping the Construction Facility and going directly to its upgraded version could prevent the affected city from ever building basic structures that require a Construction Facility rather than a Fabrication Plant.)<br>
☑️Restored the previously non-functional special ability of the Elite Warrior (Replaces Veteran Soldier). It now has +50 % vs. Feral Units and +50 % vs. Cosmic Horrors.<br>
☑️Cultivators (Replace Hardy Laborer) now use the Laborer profession by default. (This makes them 50 Tools cheaper, as they no longer need to be equipped after production, which was already expensive enough.)<br>
☑️Updated the visual model of the Venerable Elder and added a VFX.<br>
☑️Changed the avatar image of the Alien Crossbreed.<br>
☑️Changed the avatar image of the Industrious Cyborg.<br>
☑️Alien units are no longer stretched on the city screen.<br>
☑️Fixed and adjusted missing engine-glow animations for the Smuggler, Carrier, and Cloaked Warship.<br>
☑️Added new flag banners for He-Who-Seeks (Greens), He of the Ashes (Grey), Androrc (Felids), Quetzalcoatl (Reptilians), Kailric (Sasquatch), and TC01 (Silicoids).<br>
<br>
🤖🔳Royal Expeditionary Force (Earth):<br>
☑️Added a new flag banner for the Japanese Everemperor.<br>
☑️Swapped the player colours of the PRC Chairman and the Japanese Everemperor.<br>
<br>
👾🔳Royal Expeditionary Force (Progenitor):<br>
☑️Updated the avatar image of the infantry unit.<br>
☑️The warship now has a whiter colour scheme.<br>
☑️One of the units is now a Cosmic Horror.<br>
### 15.08.2026 Colonization 2071 v2.01 Patch Notes by Kaszkaj/Rakso:<br>
---
☑️Restored the old UI, which was unique to Colonization 2071.<br>
☑️Changed the visual appearance of Roads and Bridges to a futuristic style.<br>
☑️Fixed Fortifications so they no longer require or replace a Fabrication Plant.<br>
☑️Reduced the Arsenal Hammers cost: 900 → 600.<br>
☑️Fixed missing projectiles and added new ones to the attack and defence animations of many units.<br>
☑️Changed the visual appearance of Pastures to a futuristic style.<br>
☑️Made minor improvements and fixes to animations, unit sizes, and more.<br>
☑️Implemented a space-themed visual effect for unit flags.<br>
☑️Introduced a new experimental unit category: BOSSES. So far, only one BOSS-tier unit is available in the mod, exclusively as an enemy in Alien gameplay. These units are intended to be deliberately overpowered and challenging to defeat.<br>
☑️Restored the old (new) promotions system from the mod's earliest versions.<br>
☑️Updated the models and avatars of many units, buildings, improvements, and terrain features across the mod.<br>
☑️Added 6 new units: Unique Privateer, Unique Freighter, Unique Stormtrooper, Unique Artillery, Unique Cruiser, and Unique Convoy.<br>
☑️Restored many map bonuses that had been lost during previous version updates.<br>
☑️Experimental ⭐ Reduced Research Ideas costs by 25 %: 100 → 75. 80 → 60. 40 → 30. 30 → 22. 20 → 15.<br>
☑️Changed the Plasteel Mill's visual appearance to a futuristic style.<br>
☑️The Fortifications building was renamed Domed City. It is intended to grant immunity to radiation effects in the future, once nuclear bombs are added.<br>
☑️Xenotoxins and Phase Desequencers promotions currently do nothing because the technical implementation (categorisation) of Feral Units and Cosmic Horrors (Bosses) is not yet in the game.<br>
☑️The Ancient Mound temporarily does not prevent you from building Improvements. In the future, the mound will need to be excavated to remove this restriction.<br>
☑️Civilization colours and flags are now locked. You will no longer receive a random colour when playing a civilisation whose assigned colour would otherwise conflict with another civilisation spawned in the game.<br>
<br>
🧑‍🚀🟦Human:<br>
☑️The Intrepid Explorer is now a unique unit for Human civilisations. This is not displayed in-game. (Aliens cannot loot ruins.)<br>
☑️The Intrepid Explorer is now an Armed Unit ~with both advantages and drawbacks.<br>
☑️The Intrepid Explorer is now Invisible to Most Units.<br>
☑️The Intrepid Explorer now Can See Hidden Units. (This is not yet shown in Unit abilities.)<br>
☑️Intrepid Explorer Movement Points: 1 → 2.<br>
☑️Intrepid Explorer Strength: 0 → 1.<br>
☑️Humans can now build a Megastructure (Wonder) ~indicated by the Capital icon above a colony when one is built there.<br>
<br>
日本🗾Japanese Colonies (Human):<br>
☑️Fixed the Artificial Intelligence (Replaces Artful Politician) unit's inability to move. (It could not move, so it could not be used in colonies or transferred between them.)<br>
☑️Changed the Android (Replaces Master Mechanic) unit's avatar to match its actual visual appearance.<br>
<br>
🉐🏴Syndicate Colonies (Human):<br>
☑️The Lady in Black of the Syndicate Colonies makes her return.<br>
☑️Criminal, Researcher, Resourceful.<br>
☑️Starts with Knowledge of: Soil Enrichment, Alien Neurotransmitters, Organized Militia.<br>
☑️Also starts the game with two space units for extra flavour. One of them is a Privateer. ~This allows you to adopt an annoying and aggressive playstyle with her from the very beginning of the game.<br>
☑️The Syndicate Colonies come with their own unique Privateer variant.<br>
<br>
👽🟩Alien:<br>
☑️Fixed faction leader traits that grant exclusive genetic traits (global promotions) to their units.<br>
☑️Restored an old script that allows Aliens to start the game with a Saucer (Corvette).<br>
☑️Reworked the turn 2 event reward choices for Alien Civilizations so that their start is less painfully slow.<br>
☑️Adjusted unit sizes. (Alien Mecha, Alien Corsair, Battlestar, etc.)<br>
☑️Alien (Replaces Free Colonist) units should no longer override or replace Earthling Free Colonists.<br>
☑️Updated the Alien civilisations' building tree. (Alien buildings could not be built when playing as an Alien civilization because they conflicted with future buildings and technology trees.) ~Aliens no longer replace early-game Human buildings, only later ones when you play as them.<br>
☑️Fixed the Brilliant Polymath unit for Alien Civs. (It could not be recruited when playing as Aliens and caused unintended and illogical behaviour for Human factions.)<br>
☑️All Alien Unique space units are now amphibious (they can traverse both space and planetary terrain). ~This change applies only to Alien civilisations and their space units.<br>
☑️Mutant (Replaces Artillery) is now harder to unlock.<br>
☑️New unique unit for Alien Civs: Carrion. (Replaces Freighter)<br>
☑️New unique unit for Alien Civs: Alien Palanquin. (Replaces Convoy)<br>
☑️Updated the visual appearance of the Alien Garden (now Alien Fertilizers) and Alien Plantation (now Future Farm).<br>
☑️Aliens can now build a Megastructure (Wonder) ~indicated by the Capital icon above a colony when one is built there.<br>
☑️Fixed the texture of the UFO.<br>
☑️The Elite Warrior (Replaces Veteran Soldier) now uses the Alien Soldier profession by default. (This makes it 25 Weapons cheaper, as it no longer needs to be armed after production, which was already expensive enough.)<br>
<br>
🤖🔳Royal Expeditionary Force (Earth):<br>
☑️Fixed an issue where a Mecha Pilot (Stormtrooper) engaging in combat would crash the game, making it impossible to achieve victory through Revolution.<br>
☑️Expeditionary Force units (Stormtrooper) now Can See Hidden Units.<br>
☑️Expeditionary Force units (Stormtrooper) are now Invisible to Most Units.<br>
☑️Updated the Dreadnought's visual appearance.<br>
<br>
👾🔳Royal Expeditionary Force (Progenitor):<br>
☑️The Progenitor Expeditionary Force now has its own unique units replacing Stormtroopers, Artillery, and Cruisers. (They are significantly stronger than their Earth Expeditionary Force counterparts.)<br>
<br>
Future Ideas / Roadmap:<br>
❔Rework the Education Building Tree branch for Alien civilisations.<br>
❔Make it possible for AI Colonists to live among the natives inside Human player colonies.<br>
❔Unique units for each Human Colony faction; for example, Spetsnaz for Russian Colonies, etc.<br>
❔New civilisations? Up to the game's limit.<br>
❔Possibly more new units, including Bosses.<br>
❔Unit strength and ability balancing.<br>
❔2071 City Screen rework (building visuals).<br>
❔Make AI opponents more challenging.<br>
❔Moving Barbarians.<br>
❔A scripted "Outer Galaxy" Alien Invasion event on a specific turn. (Similar to the Mongols and Timurids in Medieval 2: Total War.)<br>
❔Bug fixes.<br>
❔Performance improvements.<br>
❔Upgrade System: a more advanced profession system for military units. It would include different combinations of all of the following (Veteran Soldier example only):<br>
🔹Veteran Soldier (Soldier)<br>
→🔹(Soldier I Armored)<br>
→→🔹(Soldier II Armored)<br>
→→→🔹(Soldier III Armored)<br>
→🔹(Soldier I Force Field)<br>
→🔹(Soldier I Guns)<br>
→→🔹(Soldier II Guns)<br>
→→→🔹(Soldier III Guns)<br>
→🔹(Soldier I NVG Optics)<br>
→🔹(Soldier I Stealthsuit)<br>
→→→→🔹(Soldier IV Ghost)🔸Requires All Previous Upgrades<br>
<br>
❌There are no plans to add more resources because the mod's Technology Tree and unit costs already make all existing resources highly valuable and in demand. If player inflation ever becomes an issue, I will instead try to increase player expenses.<br>
