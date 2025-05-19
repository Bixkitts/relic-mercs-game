import { getGLContext, getCanvas } from './canvas-getter.js'
import { getAllPlayers } from './game-logic.js';
import { getMyPlayerId } from './game-logic.js';
import { getPlayer } from './game-logic.js';
import { printMat4 } from './helpers.js';
import { getSocket } from './networking.js';
import { getPerspMatrix } from './rendering/renderer.js';
import { getModelViewMatrix } from './rendering/renderer.js';
import { rayPlaneIntersection } from './helpers';
import { sendMovePacket } from './networking.js';
import { getButtons } from './ui-utils.js';

let   mov         = [0.0, 0.0];
let   moveOnce    = [0.0, 0.0];
let   scrollDelta = 0.0;
let   camZoom     = 0.5;
let   zoomCoef    = 1;
const speed       = 0.005;
const mouseCoef   = 0.003;
const zoomFactor  = 0.05;
const errInputThreshold = 0.005;

const camZoomMin  = 0.0;
const camZoomMax  = 1.0;

let   camPan              = [0.0, 0.0, 0.0];
const camPanLimitVertDown = -0.85;
const camPanLimitVertUp   = 0.85;
const camPanLimitHor      = 1.5;

const mouse = {
    left: 0,
    middle: 1,
    right: 2
};
globalThis.peekZoom = () => camZoom;
globalThis.peekPan  = () => camPan;
globalThis.unfuck   = () => {
    mov     = [0, 0];
    camPan  = [0, 0, 0];
    pressed = [0, 0, 0];
}
let   pressed          = [0, 0, 0];
const gl               = getGLContext();

let mouseInCanvas   = false;

function keyPressed(e, coef) {
    if (mouseInCanvas) {
        switch (e.key) {
            case 'w': case 'W':
                coef *= -1;
            case 's': case 'S':
                mov[1] += (1 - e.repeat) * coef * speed;
                break;
            case 'a': case 'A':
                coef *= -1;
            case 'd': case 'D':
                mov[0] += (1 - e.repeat) * coef * speed;
                break;
        }
    }
}

/**
 * @param {HTMLCanvasElement} canvas 
 */
export function initWASD() {
    const canvas = getCanvas();
    canvas.onmousedown = e => {
        e.preventDefault();
    }
    canvas.onmouseup = e => {
        e.preventDefault();
    }
    canvas.onclick = () => {
        document.activeElement.blur();
        mouseInCanvas = true;
    }
    canvas.oncontextmenu = () => false;

    /**
     * @param {MouseEvent} e 
     */
    function mouseDown(e) {
        mouseInCanvas = e.explicitOriginalTarget === canvas;
        if (e.explicitOriginalTarget !== canvas) mov = [0.0, 0.0];
        if (e.button === 0) pressed[mouse.left] = true;
        if (e.button === 1) pressed[mouse.middle] = true;
        if (e.button === 2) pressed[mouse.right] = true;
        if (mouseInCanvas && pressed[mouse.left]) {
            const rect    = canvas.getBoundingClientRect();
            const mouseX  = e.clientX - rect.left;
            const mouseY  = e.clientY - rect.top;
            const screenX = mouseX / gl.canvas.width;
            const screenY = 1.0 - (mouseY / gl.canvas.height);
            if (doUiClick(screenX, screenY)) {
                return;
            }
            const worldCoords = clickToWorldCoord(mouseX, mouseY);
            getPlayer(getMyPlayerId()).move(worldCoords[0], worldCoords[1]);
            sendMovePacket(worldCoords[0], worldCoords[1]);
        }
    }
    document.onmousedown = e => mouseDown(e)
    document.onmouseup = e => {
        pressed[e.button] = false;
    }
    canvas.onmousemove = e => {
        moveOnce = [pressed[mouse.right] * mouseCoef * e.movementX,
                    pressed[mouse.right] * mouseCoef * e.movementY];
    }
    document.onkeydown = e => keyPressed(e, -1);
    document.onkeyup = e => keyPressed(e, 1);
    canvas.onwheel = e => {
        e.preventDefault();
        const res = -Math.sign(e.deltaY) * zoomFactor;
        scrollDelta += res;
    }
}

function doUiClick(screenX, screenY)
{
    const buttons = getButtons();
    let wasUiClicked = false;
    buttons.forEach( button => {
        if (button.isHidden)
            return;
        // detect collision and execute callback here
        if (screenX >= button.coords[0]
            && screenX <= button.coords[0] + button.width
            && screenY <= button.coords[1]
            && screenY >= button.coords[1] - button.height)
        {
            button.callback(button);
            wasUiClicked = true;
        }
    });
    return wasUiClicked;
}

function clickToWorldCoord(mouseX, mouseY)
{
    const projectionMatrix = getPerspMatrix();
    const modelViewMatrix  = getModelViewMatrix();
    const ndcX = (2.0 * mouseX) / gl.canvas.width - 1;
    const ndcY = 1 - (mouseY * 2.0) / gl.canvas.height;
    const rayClip = vec4.fromValues(ndcX,
                                    ndcY,
                                    -0.001, 1);
    let ndcToView = mat4.create();
    mat4.invert   (ndcToView, projectionMatrix);

    let rayEye = vec4.create();
    vec4.transformMat4(rayEye, rayClip, ndcToView);

    let viewToWorld = mat4.create();
    mat4.invert   (viewToWorld, modelViewMatrix);

    let rayWorld = vec3.create();
    vec3.transformMat4 (rayWorld, rayEye, viewToWorld);

    let camPos = vec3.create();
    vec3.set(camPos, viewToWorld[12], viewToWorld[13], viewToWorld[14]);

    let rayRelative = vec3.subtract(vec3.create(), rayWorld, camPos);
    const iPoint = rayPlaneIntersection(camPos, 
                                        rayRelative, 
                                        vec3.fromValues(0,0,0), 
                                        vec3.fromValues(0,0,1));

    return iPoint
}

export function getCamPan(deltaTime) {
    // First I delete delta time and get it
    // working that way
    //
    const dt      = deltaTime * 0.001;
    const dtCoef  = (((dt * 144) - 1) / 1.5) + 1;
    const dtCoef2 = dt * 144;

    camPan[0]   = camPan[0] + ((mov[0] * dtCoef + moveOnce[0] * dtCoef2) / zoomCoef);
    camPan[0]   = Math.min(Math.max(-camPanLimitHor, camPan[0]), camPanLimitHor);
    moveOnce[0] = 0.0;

    camPan[1] = camPan[1] - ((mov[1] * dtCoef + moveOnce[1] * dtCoef2) / zoomCoef);
    camPan[1] = Math.min(Math.max(camPanLimitVertDown, camPan[1]), camPanLimitVertUp);
    moveOnce[1] = 0.0;

    return camPan;
}

/*
 * The returned value is in the range
 * 0.0 - 1.0
 * where 1.0 is max zoom I think?
 */
export function getZoom(deltaTime) {
    // dtCoef makes the animation smooth and
    // scales with framerate
    const dtCoef = deltaTime / 100;
    const scrollDeltaInFrame = scrollDelta * dtCoef;
    const condA = camZoom <= camZoomMax;
    const condB = camZoom >= camZoomMin;

    const res = (condA * condB * scrollDeltaInFrame) - 
                // Snap the zoom back into place when it exceeds limits
                ((1 - condA) * condB * dtCoef * 0.05) + ((1 - condB) * condA * dtCoef * 0.04);
    scrollDelta -= (!( condA & condB ) * scrollDelta) 
                   + (( condA & condB ) * scrollDeltaInFrame);
    camZoom  += res;
    // This is for panning calculation
    zoomCoef += res;

    return camZoom;
}
