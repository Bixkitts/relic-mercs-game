import { getGLContext } from '../canvas-getter.js';
import { loadTexture } from './resource-loading.js';
import { subscribeToRender } from './rendering/renderer.js';
import { unsubscribeFromRender } from './rendering/renderer.js';

// Where we should position the player
// models so they align with the map plane
// on Z axis
const playerZHeight = 0.0125;

export class Player {
    constructor(x, y, vigour, violence, cunning, image) {
        this.position = vec3.fromValues(x, y, playerZHeight);
        this.vigour   = vigour;
        this.violence = violence;
        this.cunning  = cunning;
        this.image    = loadTexture(getGLContext(), image);
    }
    move(targetX, targetY) {
        this.position[0] = targetX;
        this.position[1] = targetY;
    }
}

// Test values
let players = [
    new Player(0.0, 0.0, 1, 2, 3, "playerTest.png"),
];
export function getPlayers() {
    return players;
}


