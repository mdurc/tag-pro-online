#show link: set text(fill: blue)
#show link: underline
#set page(
  paper: "us-letter",
  margin: (left: 0.75in, top: 0.75in, bottom: 1cm, right: 0.751in),
)

#align(center, [= User Manual])
#align(center, [Matthew Durcan, Jaden Tian])
#align(center, datetime.today().display())

#outline()

#v(80pt)

= Introduction

TagPro is a real-time multiplayer capture-the-flag game built with C++ and Qt. It features client-server architecture, allowing multiple players to compete accross different Windows/Linux machines.

The application consists of:
- An option to run a dedicated server instance, or for a player to locally host a server and participate in the match.
- A graphical client with lobby system and game screen using Qt6 development packages.
- Cross-platform support (Windows and Linux)

== Installation and Setup
See `doc/readme.txt` for installation/development notes and build instructions.

#pagebreak()

= Usage

== Dedicated Server
When running the program as a server-dedicated instance, there is no GUI interface.

#image("imgs/server-only.png", width: 40%)

== GUI Application
=== Main Menu
Upon launch, you'll see the main menu with three options:
- *Host Game*: Create a new game server
- *Join Game*: Connect to an existing server
- *Exit*: Close the application

#image("imgs/main-menu.png", width: 40%)

=== Hosting a Game
1. Click *Host Game*
2. Configure settings
3. Click *Start Game*
4. The server starts and you automatically join as the host

#image("imgs/host-screen.png", width: 40%)

#pagebreak()

== Joining a Game
1. Click *Join Game*
2. Enter server IP address (127.0.0.1 for local games)
3. Enter port number (match the host port)
4. Click *Connect*

#image("imgs/join-screen.png", width: 40%)

== Lobby Screen
After connecting, you'll see the lobby with:
- List of connected players
- *Start Game* button (visible only to host)
- *Leave Lobby* button
  - If the host leaves the lobby, the server is closed and all connected players are kicked from the game.
  - Clients/other players are free to leave the lobby without closing the server.

#image("imgs/lobby-screen.png", width: 90%)

#pagebreak()

== Gameplay
The game features:
- Arena with center dividing line
- Red and blue team zones
- Player avatars (circles) with name tags
  - The host is player 1 (blue)
- Team-colored flags (triangles)
- Score display

#image("imgs/game-screen.png", width: 90%)

=== Controls
- *W/A/S/D*: Move your player
- Movement is physics-based with acceleration and friction
- Collision with walls and other players is simulated

=== Game Mechanics
1. _*Objective*_: Capture the enemy flag and return it to your base
2. _*Flag Carrying*_: Pick up the enemy flag by touching it
3. _*Scoring*_: Return captured flag to your own flag location
4. _*Tagging*_: Tag enemy flag carriers to make them drop the flag
5. _*Respawn*_: Tagged players respawn at their team's base

== Exiting the Game
- Close the window to exit
- Players can leave mid-game (returns to main menu)
- Server continues until all players disconnect or host closes

#pagebreak()

== Troubleshooting and Common Issues
