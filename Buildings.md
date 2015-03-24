# Buildings Chart #
Arrows show what buildings can be upgraded into from other buildings.

![http://plutocracy.googlecode.com/svn/wiki/resources/Buildings.png](http://plutocracy.googlecode.com/svn/wiki/resources/Buildings.png)

# Terrain #
Buildings can only be constructed on certain terrain:
  * **Desert** climate tiles are too hot to support agriculture
  * **Hot** climate tiles can support Sugarcane and Cotton
  * **Moderate** climate tiles can support Wheat
  * **Cold** climate tiles are too cold to support agriculture
  * **Shallow Water** tiles can support Barricades and Towers
  * **Shore** tiles are Shallow Water tiles that touch land and can support Docks
Some terrain cannot be built on:
  * **Mountain** tiles can provide Iron Ore, Sulfur, and/or Gemstone
  * **Deep Water** tiles can provide Fish but cannot support structures
  * **Reef** tiles are Deep Water tiles that can damage passing ships

# List of Ground Buildings #
These buildings must be constructed on a rented ground tile. The owner of the tile may demolish the building and partially recoup construction materials.

## Town Hall ##
  * The only building that can be constructed on unclaimed islands
  * Always placed adjacent to a water tile
  * Always owned by a nation or a pirate player
  * If attacked and destroyed by a hostile player, may be rebuilt to change the island government changes to the attacker's affiliation
  * Buildings on islands without a Town Hall cannot be used

## Housing ##
  * Provides living room for town residents
  * Generates income depending on the number of tenants

## Warehouse ##
  * Provides large capacity storage for goods

## Fields ##
Field efficiency can be boosted by providing Fertilizer.

### Field ###
  * If built on a moderate tile, generates small amount of wheat
  * If built on a hot tile, generates small amount of sugar and cotton
  * Generic structure that boosts the efficiency of all Fields

### Sugarcane ###
  * Can only be built on Hot climate tiles
  * Generates Sugar

### Plantation ###
  * Can only be built on Hot climate tiles
  * Generates Cotton

### Wheat Field ###
  * Can only be built on Moderate climate tiles
  * Generates Grain

## Mills ##

### Mill ###
  * Generates a small amount of lumber and fruit
  * Generic structure that boosts the efficiency of all Mills

### Sawmill ###
  * Generates Lumber
  * Will slowly consume mature trees on owner's land
  * Amount of Lumber generated depends on tree type (see [Resources](Resources.md))

### Cannery ###
  * Generates Fruit
  * Amount of Fruit generated depends on number of mature trees on owner's land
  * Amount of Fruit generated also depends on tree type (see [Resources](Resources.md))

### Distillery ###
  * Generates Rum

## Mines ##

### Mine ###
  * Generates a small amount of ore, sulfur, and/or gemstone depending on availability
  * Generic structure that boosts the efficiency of all Mines

### Ore Mine ###
  * Generates Iron Ore
  * Depletes Iron reserves of Mountain tiles on owner's land

### Gold Mine ###
  * Generates Gold
  * Depletes Gold reserves of Mountain tiles on owner's land

## Guildhalls ##

### Guildhall ###
  * Generates a small amount of bread, rum, textiles, and rope
  * Generic structure that boosts the efficiency of all Guildhalls

### Bakery ###
  * Generates Bread
  * Will generate Rations instead of Bread if ingredients are available

### Weaver ###
  * Generates Cloth

### Furnisher ###
  * Generates Furniture

## Workshops ##

### Workshop ###
  * Generic structure that boosts the efficiency of all Workshops

### Arsenal ###
  * Generates Cannon, Roundshot, Chainshot, and Grapeshot
  * Quotas for each product can be set per-region

### Armorer ###
  * Generates Plating

### Fishery ###
  * Generates Gillnets

# List of Shallow Water Buildings #
These buildings may be constructed on a rented shallow water tile up to 3 tiles away from shore. The owner of the tile may demolish the building and partially recoup construction materials.

## Dock ##
  * Allows ships to trade with and transfer goods to land tiles
  * Provides a small amount of storage capacity for goods
  * Can be raided by hostile ships for a portion of goods on the island
  * Can be upgraded to a Shipyard

## Shipyard ##
  * Can be used to construct and repair ships
  * Provides a small amount of storage capacity for goods

## Tower ##
  * Stores Roundshot
  * Attacks hostile ships up to 2 tiles away
  * Uses one Roundshot for every shot

## Barricade ##
  * Blocks ships
  * Connects to another raft to extend
  * If a section of raft is "cut" and has no connection to shore, it sinks