#summary Detailing of the User Interface and User Interaction

**NOTE:** At the moment, I am just putting up quick sketches of the UI.

# Philosophy #
A user interface for a real time strategy game should, in the philosophy of
David Sev, be broken up into parts:

  * Level 1: Always-visible information that you refer to very frequently.
  * Level 2: Tools that you use relatively frequently.
  * Level 3: Configuration options that you want to see once in a while.

Secondly, a well-known rule of user-interface design is, "The bigger and closer
an object is to the mouse, the easier it is to use."

Thirdly, the interface for a game like Plutocracy needs to be customizable
for two reasons:

  1. A user's preferences
  1. Plutocracy is meant to be able to support multiple game modes, as well as plug-in game modes. The UI needs to flexibly bend to show different information for different game styles.

Fourthly, as a user of a tiling window manager, I subscribe to the
philosophy that no one really wants overlapping windows. If you watch the
behaviour of a lot of Windows users, they either take a lot of time carefully
arranging their windows to be side-by-side, or maximize a lot and use the
window list (or alt-tab) to pick which windows they want to be using.

Finally, I believe that a user interface is not something that you should have
to spend a tremendously long time learning for a game. It should be relatively
intuitive and easy. There should certainly not be a 200 page book to explain
how easy it really is.

As such, the Plutocracy UI tries to make use of some of the more modern
UI ideas such as circular menus. The UI tries to separate things that you
want to see often from stuff you only occasionally want to see. (It however
tries to keep those things you want to occasionally see within easy reach.)
It tries to keep things as contextual as possible, rather than forcing
the user to dig around to find the controls that they want. And finally,
it tries to have floating windows that don't overlap wherever possible.

Following this style of interface design, a new player would only have to
learn a couple simple rules for how to use it, and then be able to use it
quickly and efficiently, learning new parts of it as they pop up onto the
screen.

# Overall Layout #

![http://plutocracy.googlecode.com/svn/wiki/ui/ui-overall.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-overall.png)
**IMAGE OF THE OVERALL UI LAYOUT OF THE GAME**

# Window Types #

## General Information About Windows ##

Before I explain the different types of UI that exist in Plutocracy,
I will set down some ground rules for most windows. It should be obvious
when these apply.

  * The programming and API style should be similar to that of GTK. GTK (GIMP Toolkit) is very logical and easy to use.
  * Windows are fundamentally resizable, in the same way that GTK is. This is a necessity to be able to work with multiple resolution sizes.

## Ring Menus ##

![http://plutocracy.googlecode.com/svn/wiki/ui/ui-ring-menu.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-ring-menu.png)
**IMAGE OF RING MENU**

So the first thing I want to define are the different kinds of UI elements
that are used in Plutocracy.

First and probably most important is what we define as the **ring menu**.
A ring menu is commonly referred to as a circular menu. Basically, it is
a menu that pops up when you click on something, but it has icons arranged
in a ring rather than a drop down.

In Plutocracy, ring menus are used extremely frequently as object-to-object,
objects-to-object, or object-to-self interaction tools.
Most **control windows** are opened as a result of a ring menu
operation. For example, if you want to have one
ship trade with another ship, you select your ship by left-clicking on it,
and then select the target by right-clicking on it. After you right-click
on the target ship, it opens a ring menu around that ship, offering some
options such as "attack", "follow", and "trade". Clicking trade then has
your ship sail over to the other ship and opens up the trade controls.

Some rules about ring menus -- right clicking an object will:

  * If a different object is selected than the one you're right-clicking, the game will try to open a contextual ring menu between the two of them.
  * If no context exists, the game will select the right-clicked object and open its "self" ring menu.
  * If the object is selected and it is right clicked, the **self ring menu** will open up for sure.
  * If no self menu exists, nothing happens.
  * The **self ring menu** has an extra decorative ring around it to separate it visually from the **context ring menu**.
  * Ring menus are always the same size, and always have the icons in the same order and locations. This is done so that once you get comfortable with the locations, you can blaze through ring menus kind of like doing mouse gestures.

TODO: Explain all of the contexts that can exist between all objects. It's
mostly common sense, but this document attempts to formalize as much as
possible.

## Control Windows ##

![http://plutocracy.googlecode.com/svn/wiki/ui/ui-control-window-placement.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-control-window-placement.png)
**IMAGE OF WHERE THE CONTROL WINDOWS GO IN THE UI, WITH EXAMPLE WINDOW**

I had briefly mentioned **control windows** above. Control windows are what
I classify as the Level 2 type windows in the Philosophy section. They have
very useful information and tools, but you only want to see them once in
a while.

These windows could also be called contextual windows, as they are used
contextually. When you need to trade, you get the **manual trade control
window** (described below). When you're done trading, it goes away. To relate
to the regular desktop, this could be similar to a program that decompresses
zipped files. Something that you want to see very frequently should be a
**widget window** (see **Widget Windows** section). An example of one of those might
be the **game scoreboard widget**.

**Control windows** spawn in the central area of the screen, right on top of the
globe. That kind of puts them in the way, but you should only need to interact
with one for a little while, and have the option of filing it away in the
**minimized windows widget**. However, whenever possible, the control windows
should not overlap the widgets, which have space reserved for them.

Now I will describe the **control windows** that exist in Plutocracy.

### Manual Trade Control Window ###

![http://plutocracy.googlecode.com/svn/wiki/ui/ui-manual-trade.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-manual-trade.png)
**IMAGE OF MANUAL TRADE CONTROL WINDOW**

**Opens:** through **contextual ring menu**, as a result of a click on the trade
option.

**Minimized Information:** When this window is minimized, the information it shows
is:
![http://plutocracy.googlecode.com/svn/wiki/ui/ui-minimized-manual-trade.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-minimized-manual-trade.png)
**IMAGE OF A MINIMIZED MANUAL TRADE CONTROL WINDOW**

  * Two lines, each with "player object accepted?". The first line is the person who initiated the trade. One of the players is, obviously, you.

**Points About This Window:**

  * If a ship's movement has to be stopped to start trade, the ship will stop until the trade is resolved. Of course, the other person has the ability to quickly cancel. If the ship was already going somewhere, it continues there when the trade is resolved.
  * There is an icon for each resource type the object has. Hovering over one such icon _instantly_ opens a **tooltip pop-up control**, which simply says what the resource is.
  * Trade is done by dragging one of your resources onto your offering area. When you drag a resource in that area, an **amount selector pop-up control** spawns, to make it possible to select the amount.  The default is 1.
  * Once you have offered something, the "Notify" button un-greys-out. Pressing this button notifies the player you're trading with of your intent and adds a trade window to their **minimized windows widget**
  * Once you're content with what the other person is offering, press the "Accept" button. After the person you're trading with has pressed Accept, you may no longer remove anything offered from your offering area. Of course, you may still add things, but chances are that this is not what you want to do (ie. they've already accepted less).
  * If you want to remove something before the other person has pressed accept, you just drag it from the offering area back onto your inventory area. Another **amount selector pop-up control** will spawn asking you how much to remove. The default is all.
  * If one of the objects trading has automatic rules set up that could be fulfilled, the object automatically does so. However, since this is a manual trade setup, Accept still needs to be pressed by both parties. For example, if you try to buy wood, and the other object has an automatic rule to trade wood for x gold, it'll offer you the gold. Then you can notify the other player and sort out what to do with them.
  * Cancel button cancels the trade altogether.

### Automatic Trade Control Window ###

![http://plutocracy.googlecode.com/svn/wiki/ui/ui-automatic-trade.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-automatic-trade.png)
**IMAGE OF AUTOMATIC TRADE CONTROL WINDOW**

**Opens:** through **self ring menu**, as a result of the trade option being selected

**Minimized Information:** When this window is minimized, its label is "Automatic Trade Controls for -object-"

**Points About This Window:**

  * The trade option doesn't exist for a **self ring menu** of an object you don't own. (Ie. this window is not accesible)
  * If a ship is moving when you open the **automatic trade control window** on it, it will halt moving. Once you close this window, it'll continue on its way.
  * The purpose of this window is to configure the automatic trade options that a ship (or other trade-capable object) has. Again, this configures options _per object_.
  * The top section is a collection of icons, one for each resource type. Hovering over one of the icons immediately spawns a **tooltip pop-up control** that says what the icon represents.
  * Selecting an icon un-grays-out the bottom controls.
  * The bottom section has Risujin-UI (old style) inspired controls for configuring automatic trade:
    * The buy and sell checkboxes toggle automatically buying/selling the selected resource.
    * Clicking on the for field will spawn an **amount selector pop-up-control** which lets you select how much you want the resource to be bought/sold for, at maximum or minimum respectively.
    * The Max/Min Carried field designates how much of the resource the ship is willing to carry, space permitting. Clicking on these will, again, spawn an **amount selector pop-up control**

### Configuration Window ###

![http://plutocracy.googlecode.com/svn/wiki/ui/ui-config.png](http://plutocracy.googlecode.com/svn/wiki/ui/ui-config.png)
**IMAGE OF CONFIGURATION WINDOW**

**Opens:** manually, through the **manual access widget**

**Minimized Information:** When minimized, the label is "Configuration"

**Points About This Window:**

  * This has all the general game options that you may want to configure.
  * This is really just a collection of tabs which have pages of configuration options with descriptions. This is really just a frontend to the console.
  * The special one at the bottom is "Console". This page lets you run the textual commands rather than configuring options graphically.
  * Note that options changed in console and on the graphical UI should by synchronized
  * The pages are: Video, Sound, Server, Game, Windows, Console. Below is a listing of what should _at least_ be in each page.
    * **Video:** Resolution, Color bits, Windowed, Multisampling, Gamma correction, Draw Distance
    * **Sound:** Ambient volume, Effects volume, Sound quality
  * **Server:** Port, Victory gold, Time limit
  * **Game:** Player Name
  * **Windows:** Scale 2D, Which Widgets are Active

**Implementation Priorities:**

  * Really, as long as we have the console, that's good enough for version 0.1.
  * This layout would be nice to have sooner rather than later though.

## Widget Windows ##

**IMAGE OF A GENERIC WIDGET**

So, Widget Windows. These are called different things by different people. On Linux, they've been called Desklets, Screenlets, and Widgets.
On Mac, I think they call them just Widgets. In Windows Vista, they call them Gadgets. Plutocracy's Widgets are very much similar to these
desktop ideas.

Basically, Widget Windows are simple, easy to create, informative windows that exist on the screen on a relatively permanent basis. A Plutocracy
Widget has a titlebar which has an area to grab to be able to move the widget around, a button to "scroll" the widget up into being just a
titlebar, an area you can drag to resize the widget, and of course a close button.

If you recall the philosophy section, Widgets are designed to fill the role of, "always visible information that you want to refer to very
frequently". For example, a clock widget would show how far into the game you are, or how close to the time limit you are.

A couple extra fine points about Plutocracy's Widget Windows:

  * Plutocracy's Widgets actually have little windows. They're not quite as free-form as most implementations.
  * Plutocracy's Widgets are free-form in terms of placement. This means that if a mod is made, it can safely just throw its widget at the screen, and the user would be able to position it where he or she feels is right.
  * Plutocracy's Widgets may not be minimized into the Minimized Windows Widget, but may be "scrolled up" as described above.
  * Which widgets are visible can be configured in the **Configuration Window**
  * Plutocracy's Widgets interact: If you bring two similarly sized widgets close together, they join into a single widget with a divider. This is entirely cosmetic and is a low priority feature.

**IMAGE OF HOW WIDGETS CAN JOIN TOGETHER**

### Object Information Widget ###

**IMAGE OF OBJECT INFORMATION WIDGET**

**Points About This Widget:**

  * The information given is flexible.
  * For ships: Name, Owner, Cargo out of Max Cargo, Health, Crew out of Optimum Crew, Time Till Food Runs Out
  * For buildings: Name, Owner, Cargo out of Max Cargo (if applicable), Health
  * TODO explain other contexts

### Object Cargo Information Widget ###

  * Information about what cargo is being carried by object.
  * Should list which items are available for trade (buy/sell)

### Manual Access Widget ###

  * Widget that must always be on and lets you manually start control windows

### Minimized Windows Widget ###

  * Has windows that have spawned not by your request, and windows that you've hidden away.

### Game Scoreboard Widget ###

  * Shows who's winning. Usually this is measured in gold.

### Clock Widget ###

  * Current local time + time ticking up if no timelimit, or time ticking down if there is a timelimit

### Chat Widget ###

  * Text input widget with some history of what's been said recently, and a way to scroll back further.

## Pop-up Controls ##

### Amount Selector Pop-up Control ###

  * A slider + text input to let you quickly or precisely enter how much of something you want to select

### Tooltip Pop-up Control ###

  * A simple floating text label

# Mouse System #

  * Basically: left click to select something, right click to activate a ring on something, middle scroll/click to move the globe around.