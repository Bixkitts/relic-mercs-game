# relic-mercs-game

## Intro
A fun turn based game you can host on the web, featuring a simple websocket interface
to write a client against in any language.
I aim to provide a simple client as a test/example within this project however.

## Building and Running
### Dependencies
- https://github.com/Bixkitts/bb-net-lib
- OpenSSL v3.2.x+ (just have it installed)

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
- Run the server with: ./relicServer. You need permissions for https/port 443 (I do sudo).
- Connect with client browser to https://SERVER_IP/game
### Frontend Test Server
There's also a node server for frontend testing in the test-clients folder, if you're so inclined.
## Code Organisation
The game server is written in C, and resides in the "source" directory and
depends on the libraries in the "dependencies" directory.

The client is written in Javascript+WebGL and will reside in the "test-client/website" directory.
