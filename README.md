# relic-mercs-game

## Intro
This is a co-petetive turn-based adventure game where players travel a fantasy world to be the
first ones to find a lost relic

## How It Works
To play the game, you would run the backend server on a machine to
which every participant has network access
(Note, the server is currently only written for Linux x86_64).
The clients all then connect to
it through the browser.

It should be fairly self explanatory how it goes on from there,
but I may write a gameplay guide if it is necessary.

## Building and Running
### Dependencies
- glibc                                  (You should already have it)
- libpthread                             (You should already have it)
- https://github.com/Bixkitts/bb-net-lib (This is a git submodule, no need to install)
- OpenSSL v3.2.x+                        (Have it installed when building)

### Building The Project:
- clone the repo: 
  git clone https://github.com/Bixkitts/relic-mercs-game.git
- cd relic-mercs-game
- git submodule update --init --recursive
- mkdir build
- cd build
- cmake -DCMAKE_BUILD_TYPE=Release .. <br/>OR cmake -DCMAKE_BUILD_TYPE=Debug ..
- cmake --build .
### Running The Server:
- Run the server with: ./relicServer
- Connect with client browser to https://SERVER_IP:7676
### Frontend Test Server
There's also a node server for frontend testing in the test-clients folder, if you're so inclined.
## Code Organisation
The game server is written in C, and resides in the "source" directory and
depends on the libraries in the "dependencies" directory.

The client is written in Javascript+WebGL and will reside in the "test-client/website" directory.

## Project Plan
I try to keep an up to date project plan in the "project-plan"
folder.
This should serve as a reference for people looking to contribute,
and as information for people simply interested in the game and how
it will work from a technical and gameplay perspective.
You can (hopefully) also view the project plan directly at https://bixkitts.de/relic-mercs/Project_Plan.html
