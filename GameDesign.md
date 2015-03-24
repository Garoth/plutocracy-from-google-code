# Nations #
At the start of a game, each player chooses one **nation** (or pirate) to affilite with. There is a fixed number of nations, each one has a unique color. Nations names are configured by the game server and have no nation has special advantages over another. National affiliation can be changed at any time by any player. The player must pay the minimum fee for citizenship, scaled by the player's reputation against that nation. There is no fee for going pirate. All territory rented from and property owned on now hostile territory will be lost without compensation by the change.

When a player begins the game, he or she will receive a lightly armed ship and some gold near a friendly port. The player is then free to find a shore tile to begin renting land or engage in combat.

# Winning and Losing #
The game ends when one player obtains enough gold to purchase **victory**. The exact amount depends on the size of the map and the number of players. Servers may disable victory to allow players to play indefinitely in "sandbox" mode.

Players do not lose the game when all ships, rented tiles, and property are lost and are allowed to start over with a new initial vessel and national affiliation with a reset reputation.

# Territory #
The **globe** is a tiled icosahedron sphere. Each _face_ is a game tile. Every land tile is associated with a larger island. If a Town Hall has been built on that island, players can acquire land tiles for buildings (see Renting Land below).

Goods are pooled between buildings on the same **plot** of land. A plot of land is defined as a contiguous area of rented tiles. Gold is treated as a good, and must be carried on ships and is shared among one plot of land at a time.

Island territory is owned by the nation and player merchants may **rent** but not own tiles. At regular intervals the player must pay rent for every tile owned to the island government. Pirates pay no rent on territory they conquer but cannot bid on tiles. If the player does not have enough gold on a plot of land to pay for all of the tiles' rent, the island government will reclaim tiles on the outer edges of the player's property, paying fair price for any buildings there. This extra revenue is then used to pay for the due rent.

Land is obtained by **bidding**. If a player either already rents and adjacent tile or controls a ship in an adjacent water tile, the player may bid on that tile. By bidding, the player sets the price he or she is willing to pay regularly in rent for that tile. If a bid is not challenged by another, higher bid within a certain duration, the tile's ownership changes to the highest bidder. The bidder must then pay the previous owner fair price for any property owned on that tile. This price is determined by cost of labor (a fixed amount per building) and cost of construction materials used at the time of construction.

Players can only bid the price up and only to a limit. The player must have enough gold to pay the rent for a certain amount of turns at minimum. If a tile can no longer be bid on by any player other than the owner, if it is fully enclosed for instance, the rent price gradually decreases over time down to a minimum.

Players may relinquish their bids at any time, in which case the tile ownership will go to the next highest bidder, who must then pay the original owner for the property. If there are no other bidders, the island government reclaims the tile and pays for any property which then becomes idle. Any players wishing to bid on the tile again will have to purchase the property from the island government.

# Buildings and Ships #
Players may construct **buildings** on tiles by paying their construction labor cost in gold and using the resources gathered. Resources must be stored on ships adjacent to a dock tile or in warehouses on the plot of land. See [Buildings](Buildings.md) for a list of constructible structures and their requirements.

**Ships** may be constructed at docks. The characteristics of a ship are determined by the cargo it carries -- allowing for a wide variety of customizations. See [Ships](Ships.md) for a list of ships, construction criteria, and ship parts.

Trading is a major gameplay mechanic. [Buildings](Buildings.md) harvest or refine [Resources](Resources.md) which can be bought or sold at docks or ship-to-ship. The player has a high degree of control over prices and is able to set minimum and maximum desired quantities and buy/sell prices (see [UserInterface](UserInterface.md)).

# Warfare #
Player ships capable of combat can only attack enemies of the player's nation. When an island is conquered by a player, the island government is overthrown and a new government, affiliated to the player's nation is established. Because pirates are not affiliated with a government, pirate players can claim entire islands for themselves. Pirates are permanently at war with all other players, including other pirates.

To conquer an island, the player must destroy the existing **Town Hall** and construct a new one. The player can choose the nationality of the new Town Hall unless the player is a pirate, in which case the Town Hall belongs to that player.

In order to declare war on or make peace with another nation, the players affiliated with that nation must bribe their government by starting a **vote** and paying money either for 'yes' or 'no' votes.

# Pirate Camouflage #
When a pirate captures and claims an enemy vessel, that vessel retains the insignia of its former owner and can **camouflage** as one of its former owner's ships. The pirate can then use that vessel and appear to other players as the former owner until the are revealed or engage in combat. While hidden in this fashion, the player may interact with islands and ships under the same rules as if he or she was the former owner (war/peace state, reputation etc).

When a camouflaged pirate ship approaches a hostile ship or building, a **reveal timer** starts running. When the timer reaches zero, the ship is revealed as a pirate vessel. The rate the timer runs down depends on how many ships and buildings are within sight range of the vessel.

# Reputation #
Each player has a **reputation** in each nation. Reputation affects a player's buy/sell prices to that nation's towns by imposing tariffs on despised merchants and discounts to favored merchants. Reputations for a nation are conserved, meaning that if one player's reputation decreases, the other players' reputations increase equivalently.

Reputation can be changed by the following:
  * War actions against a nation's ships harm your reputation
  * Nations may be bribed to hate a specific player

Reputation cannot be increased directly.

# Strategy #
There are many strategies to gain land and wealth:
  * **War Declarations** can be used to deprive players who rent territory from a neutral nation of their land there.
  * **Destroying a Town Hall** will deprive all players' on that island of their property.
  * **Direct Bidding**, if backed by sufficient gold reserves, can be used to buy up all of a player's land.
  * **Separating** a single plot into two plots in which one plot does not have a warehouse or dock by buying a strip of tiles across that plot can be used to cheaply "suffocate" a plot by depriving it of gold income and waiting until the island reclaims the land to pay for rent.