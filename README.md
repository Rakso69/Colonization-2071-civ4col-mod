### 06.10.2026 Colonization 2071 v2.2.2 Patch Notes:<br>
---
☑️Fixed further assertion failures and crashes involving professions, production yields and unit movement.<br>
☑️Fixed Civilopedia errors when viewing Constitution and technology help without an active player.<br>
☑️Fixed the Technology Advisor crashing when the Inventor progress threshold was zero or unavailable.<br>
☑️Fixed trait promotion updates for Colony residents and repeated trait bonuses, preventing negative promotion counts after profession changes.<br>
☑️Fixed missing dialogue lines for leaders unique to 2071.<br>
☑️Excavations can now be established anywhere in the world. The associated reward events will occur for the player who built them, regardless of the distance from their territory and regardless of whether the territory is neutral or owned by another Civilization.<br>
☑️Starting technologies now apply their effects and bonuses to Alien civilisations and both Royal Expeditionary Force factions.<br>
☑️Restored the first-research Credit bonuses for Venture Capital and Corporate Investment. Starting knowledge and acquired technologies no longer claim or block these rewards; the bonus goes to the first civilisation to complete the research.<br>
☑️Improved the texture quality of the following leaders: The Chief of Staff (NAFTA Colonies), NAFTA President (NAFTA).<br>
☑️Improved the texture quality of many leader backgrounds.<br>
<br>
💻Computer Opponents:<br>
☑️Improved AI protection of civilian units, Treasure and transports. Units assigned to these roles now avoid dangerous routes even when they have combat strength.<br>
☑️Improved AI assessment of threats to Colonies, taking nearby rivals, war plans and diplomatic relations into account.<br>
☑️Improved AI handling of ships in planetary ports. Regular space units use the connected space area, while amphibious Alien ships retain their current land or space area.<br>
☑️AI danger checks now detect hostile amphibious units across the boundaries between planetary terrain and space.<br>
☑️AI pickup requests now include transport ships docked at Colonies.<br>
☑️Improved specialist assignments. Suitable experts can replace less effective workers in occupied jobs, and locked workers no longer prevent the AI from finding other available replacements.<br>
☑️AI job selection now considers every production bonus a unit provides, including multiple specialities. This covers Industrious Cyborgs, Brilliant Polymaths, Venerable Elders and other units with several productive roles.<br>
☑️AI treats the basic Alien unit as a generalist when assigning Colony jobs.<br>
☑️Improved building selection to favour production facilities that can be staffed by available experts and supplied with the required input materials, including experts with multiple specialities.<br>
☑️Hardy Laborers and Cultivators now prioritise construction work over jobs inside Colonies.<br>
☑️Mining Vessels and Science Vessels now search space for suitable construction and research tasks, including on unexplored plots. Colony Ships seek their construction tasks within their own territory.<br>
☑️AI Workers now place greater value on Food when choosing Improvements.<br>
☑️Improved AI evaluation of Improvements on plots producing Plasteel, Progenitor Artifacts, Hydrocarbons or bonus resources, reducing the risk of replacing valuable production with less useful Improvements.<br>
☑️Colonial AI now has a 10 % chance per turn to receive an additional immigrant on Earth at Normal game speed. This does not consume Propaganda or increase the normal immigration threshold, and the chance adjusts to game speed.<br>
☑️Each newly founded AI Colony can now receive one free defender chosen from its civilisation's eligible units. This applies to both colonial and Alien AI.<br>
☑️Added Food relief for the last starving resident of an AI Colony: +20 Food. Colonial AI pays up to 100 Credits at Normal game speed, while Alien AI receives this aid for free. The price adjusts to game speed.<br>
☑️AI civilisations now automatically transfer surplus materials between their Colonies every 10 turns to supply current production. Transfers respect the supplying Colony's own production needs and a 50-unit reserve of each material. Food is excluded.<br>
☑️Fixed material-transfer calculations when deliveries resume or complete production, preventing later deliveries from using outdated material requirements.<br>
☑️Computer opponents now receive faction-appropriate event rewards, including Human and Alien starting events.<br>
☑️AI civilisations now receive all valid rewards from events with multiple choices. Human-controlled civilisations still choose a single reward option.<br>
☑️AI Treasure rewards from Excavations no longer replace the site with City Ruins. Excavations that remain active can trigger further discoveries in later turns.<br>
☑️Colonial AI now uses Intrepid Explorers to explore planetary land and collect rewards from explorable sites.<br>
☑️Once a planet has been fully explored and its reward sites exhausted, an available transport can take an Intrepid Explorer to another planet. The unit receives the Explorer profession before its first transfer and keeps it for later expeditions.<br>
☑️After all land has been explored and its reward sites exhausted, colonial AI seeks to equip its Intrepid Explorers as Mecha Pilots and use them in offensive or defensive roles.<br>
☑️Alien AI now considers Convicts for schooling or lessons in other Alien settlements, taking actual training speed, travel time and available jobs into account.<br>
☑️After completing school training, Alien AI can choose any eligible teachable specialist for free, without requiring that specialist to be present beforehand.<br>
☑️Colonial AI now considers useful training in Alien settlements throughout the game, including settlements controlled by a human player. Units visit the chief before starting their lessons.<br>
☑️Fixed AI ships being unable to return from Earth because they were attached to an invalid map location. Purchased and free ships now use valid entry plots, and affected AI ships are relocated together with their cargo.<br>
☑️Fixed errors in AI profession assignment, worker exchanges and resident removal when no valid profession or unit was available.<br>
☑️Improved combat profession selection for Alien AI counter units.<br>
☑️Added a fallback production order for AI Colonies that cannot select a suitable unit or building.<br>
☑️Fixed AI diplomatic attitude and gold-trade calculations that could trigger errors when a civilisation was evaluated against itself.<br>
☑️Fixed trait-promotion updates for AI Colony residents, preventing errors during subsequent profession changes.<br>
☑️Alien AI now receives a starting Saucer in normal new games outside Advanced Start, matching player-controlled Alien civilisations.<br>
☑️Improved Alien AI handling of transported counter units and defenders. Passengers unload at suitable Colonies or wait for transport without attempting ground actions while aboard, including when they have no movement remaining.<br>
☑️Alien AI military units now recheck movement after profession changes and wait when they can no longer move.<br>
☑️AI counterattacks are now considered only when the unit can move, preventing errors after movement is exhausted or the unit becomes immobilised.<br>
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
☑️The State knows all technologies from turn 1 and receives their effects and bonuses. Colonial leaders with Cautious or better relations can purchase technologies from the State through the normal trade window, paying in Credits or accepting a higher tax rate. This also provides a technology trading partner in solo games.<br>
☑️Technology purchases require peace with the player's own State and all prerequisite technologies to be known before the deal. Buying a prerequisite in the same offer does not unlock its successor. Valid tax offers are accepted regardless of their Credit value.<br>
<br>
👾🔳Royal Expeditionary Force (Progenitor):<br>
☑️The Progenitor Exarch knows all technologies from turn 1 and receives their effects and bonuses. Alien leaders with Cautious or better relations can purchase technologies from him through the normal trade window, paying in Credits or accepting a higher tax rate. This also provides a technology trading partner in solo games.<br>
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
