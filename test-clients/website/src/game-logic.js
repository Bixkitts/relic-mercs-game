import { getGLContext } from '../canvas-getter.js'
import { loadTexture } from './resource-loading.js';
import { subscribeToRender } from './rendering/renderer.js';
import { unsubscribeFromRender } from './rendering/renderer.js';

// Where we should position the player
// models so they align with the map plane
// on Z axis
const playerZHeight = 0.0125;

// Define a class for player
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
    //    let currentX = this.position[0];
    //    let currentY = this.position[1];

    //    // Calculate distance and direction to move
    //    let deltaX = targetX - currentX;
    //    let deltaY = targetY - currentY;
    //    let distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);

    //    // Calculate speed (units per millisecond)
    //    let speed = 0.001; // Adjust as needed
    //    let totalTime = distance / speed;

    //    let startTime = null;

    //    const moveCallback = (deltaTime) => {
    //        if (!startTime) startTime = deltaTime;

    //        let elapsedTime = deltaTime - startTime;

    //        if (elapsedTime < totalTime) {
    //            let fraction = elapsedTime / totalTime;
    //            this.position[0] = currentX + deltaX * fraction;
    //            this.position[1] = currentY + deltaY * fraction;
    //        } else {
    //            // Player reached target position, unsubscribe from render
    //            unsubscribeFromRender(moveCallback);
    //        }
    //    };

    //    subscribeToRender(moveCallback);
    }
}

// Test values
let players = [
    new Player(0, 0.0, 0.0, 1, 2, 3, "player-test.png"),
];
export function getPlayers() {
    return players;
}
