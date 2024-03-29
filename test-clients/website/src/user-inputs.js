import { getContext, getCanvas } from './canvas-getter.js'
import { printMat4 } from './helpers.js';
import { getSocket } from './networking.js';

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
const gl               = getContext();
const fieldOfView      = (45 * Math.PI) / 180;
const aspect           = gl.canvas.clientWidth / gl.canvas.clientHeight;
const zNear            = 0.1; const zFar = 100.0;
const projectionMatrix = mat4.create();
mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);

let mouseInCanvas   = false;
let modelViewMatrix = mat4.create();

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
        console.log(mouseInCanvas);
        if (mouseInCanvas && pressed[mouse.left]) {

            const x = e.pageX - canvas.offsetLeft;
            const y = e.pageY - canvas.offsetTop;

            const ndcX = (x / gl.canvas.width) * 2 - 1;
            const ndcY = 1 - (y / gl.canvas.height) * 2;
            const vec = [
                ndcX,
                ndcY,
                -1, 1
            ];
            let matt = mat4.clone(modelViewMatrix);
            mat4.multiply(matt, matt, projectionMatrix);

            mat4.invert(matt, matt);
            let ov = [0, 0, 0, 1];
            mat4.transformMat4(ov, vec, matt);
            console.log("#######")
            printMat4(modelViewMatrix);

            const ab = new ArrayBuffer(18);
            const dataView = new DataView(ab);

            dataView.setInt16(0, 1, true);
            dataView.setFloat64(2, ov[0], true);
            dataView.setFloat64(10, ov[1], true);

            getSocket().send(ab);
            console.log(vec + " # " + ov);
            console.log(e);
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
