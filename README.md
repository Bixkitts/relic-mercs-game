# relic-mercs-game

## Intro
A fun turn based game you can host on the web, featuring a simple websocket interface
to write a client against in any language.
I aim to provide a simple client as a test/example within this project however.

## Building and Running
### Dependencies
- https://github.com/warmcat/libwebsockets version 4.3.x

(Should be fine to just have these installed on the system)

### Building The Project:
- clone the repo: 
  git clone https://github.com/Bixkitts/relic-mercs-game.git
- cd relic-mercs-game
- mkdir build
- cd build
- cmake -DCMAKE_BUILD_TYPE=Release ..
- cmake --build .
### Running The Server:
- Make sure the example "server.conf" exists in the same directory as the "relicServer" binary
- Configure the settings in "server.conf"
- Run the server with: ./relicServer
- Connect with client

## Code Organisation
The game server is written in C, and resides in the "source" directory and
depends on the libraries in the "dependencies" directory.

The client is written in Javascript and will reside in the "test-client" directory.
