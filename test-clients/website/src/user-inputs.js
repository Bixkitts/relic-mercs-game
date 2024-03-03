import { getContext, getCanvas } from './canvas-getter.js'

let mov = [0.0, 0.0];
let moveOnce = [0.0, 0.0];
let zoom = 0.0;
let zoomTime = 0.0;
let camZoom = -2.5;
let zoomCoef = 1;
const speed = 0.005;
const mouseCoef = 0.003;
const zoomFactor = 0.06;

const camZoomMin = -2.5;
const camZoomMax = -0.5;

let camPan = [0.0, 0.0, 0.0];
const camPanLimitVert = 1.0;
const camPanLimitHor = 1.6;

const mouse = {
    left: 0,
    middle: 1,
    right: 2
};

let pressed = [0, 0, 0];
const gl = getContext();
const fieldOfView = (45 * Math.PI) / 180; // in radians
const aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
const zNear = 0.1;
const zFar = 100.0;
const projectionMatrix = mat4.create();
mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);
mat4.invert(projectionMatrix, projectionMatrix);

let mouseInCanvas = false;
let modelViewMatrix = mat4.create();
function recreateModelViewMatrix() {
    mat4.translate(modelViewMatrix,
        modelViewMatrix,
        [-0.0, 0.0, camZoom]);

    mat4.rotate(modelViewMatrix,
        modelViewMatrix,
        Math.PI / (camZoom + 4) * 0.5,
        [-1, 0, 0]);

    mat4.translate(modelViewMatrix,
        modelViewMatrix,
        camPan);

    mat4.invert(modelViewMatrix, modelViewMatrix);
}
recreateModelViewMatrix();

function keyPressed(e, coef) {
    if (mouseInCanvas) {
        switch (e.key) {
            case 'w': case 'W':
                coef *= -1;
            case 's': case 'S':
                mov[1] += coef * speed;
                break;
            case 'a': case 'A':
                coef *= -1;
            case 'd': case 'D':
                mov[0] += coef * speed;
                break;
        }
    }
}

/**
 * 
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
    document.onmousedown = e => {
        mouseInCanvas = e.explicitOriginalTarget === canvas;
        if (e.explicitOriginalTarget !== canvas) mov = [0.0, 0.0];
        if (e.button & 0) pressed[mouse.left] = true;
        if (e.button & 1) pressed[mouse.middle] = true;
        if (e.button & 2) pressed[mouse.right] = true;

        if (mouseInCanvas && pressed[mouse.left]) {
            console.log(e.clientX + " " + e.clientY);
            console.log(e);
        }
    }
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
        const res = -Math.sign(e.deltaY) * zoomFactor / Math.abs(camZoom);
        zoom = res;
        zoomTime = 0.05;
    }
}

export function getCamPan(dt) {
    const dtCoef = (((dt * 144) - 1) / 1.5) + 1;
    const dtCoef2 = dt * 144
    camPan[0] = camPan[0] + ((mov[0] * dtCoef + moveOnce[0] * dtCoef2) / zoomCoef);
    camPan[0] = Math.min(Math.max(-camPanLimitHor, camPan[0]), camPanLimitHor);
    moveOnce[0] = 0.0;

    camPan[1] = camPan[1] - ((mov[1] * dtCoef + moveOnce[1] * dtCoef2) / zoomCoef);
    camPan[1] = Math.min(Math.max(-camPanLimitVert, camPan[1]), camPanLimitVert);
    moveOnce[1] = 0.0;

    recreateModelViewMatrix();
    return camPan;
}

export function getZoom(dt) {
    const dtCoef = (((dt * 144) - 1) / 1.5) + 1;
    zoom *= (zoomTime > 0);
    zoomTime -= dt;
    const condA = camZoom <= camZoomMax;
    const condB = camZoom >= camZoomMin;
    const res = condA * condB * dtCoef * zoom - ((1 - condA) * condB * dtCoef * 0.008) + ((1 - condB) * condA * dtCoef * 0.004);

    recreateModelViewMatrix();
    camZoom += res;
    zoomCoef += res;

    return camZoom;
}

