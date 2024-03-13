import { getContext, getCanvas } from './canvas-getter.js'
import { printMat4 } from './helpers.js';
import { getSocket } from './networking.js';

let mov = [0.0, 0.0];
let moveOnce = [0.0, 0.0];
let zoom = 0.0;
let camZoom = -2.5;
let zoomCoef = 1;
const speed = 0.005;
const mouseCoef = 0.003;
const zoomFactor = 0.06;

const camZoomMin = -2.5;
const camZoomMax = -1.5;

let camPan = [0.0, 0.0, 2];
const camPanLimitVertDown = -0.5;
const camPanLimitVertUp = 0.5;
const camPanLimitHor = 1;

const mouse = {
    left: 0,
    middle: 1,
    right: 2
};
globalThis.peekZoom = () => camZoom;
globalThis.peekPan = () => camPan;
globalThis.unfuck = () => {
    mov = [0, 0];
    camPan = [0, 0, 2];
    pressed = [0, 0, 0];
}
let pressed = [0, 0, 0];
const gl = getContext();
const fieldOfView = (45 * Math.PI) / 180;
const aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
const zNear = 0.1; const zFar = 100.0;
const projectionMatrix = mat4.create();
mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);
//mat4.invert(projectionMatrix, projectionMatrix);

let mouseInCanvas = false;
let modelViewMatrix = mat4.create();
function recreateModelViewMatrix() {
    let s = Math.sin(Math.PI / (camZoom + 4) * 0.5);
    let c = Math.cos(Math.PI / (camZoom + 4) * 0.5);
    let crotvec = [0, -c, s];
    const eye = [camPan[0], camPan[1], -2];
    const target = [camPan[0], camPan[1], 0];
    const up = [0, 1, 0];
    const camera = mat4.create();
    mat4.lookAt(camera, eye, target, up);
    mat4.invert(camera, camera);

    modelViewMatrix = mat4.clone(camera);

    // mat4.translate(modelViewMatrix,
    //     modelViewMatrix,
    //     [-0.0, 0.0, camZoom]);

    // mat4.rotate(modelViewMatrix,
    //     modelViewMatrix,
    //     Math.PI / (camZoom + 4) * 0.5,
    //     [-1, 0, 0]);

    //mat4.invert(modelViewMatrix, modelViewMatrix);
}
recreateModelViewMatrix();

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
            vec4.transformMat4(ov, vec, matt);
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
        const res = -Math.sign(e.deltaY) * zoomFactor / Math.abs(camZoom);
        zoom = res;
    }
}

export function getCamPan(dt) {
    const dtCoef = (((dt * 144) - 1) / 1.5) + 1;
    const dtCoef2 = dt * 144;

    camPan[0] = camPan[0] + ((mov[0] * dtCoef + moveOnce[0] * dtCoef2) / zoomCoef);
    camPan[0] = Math.min(Math.max(-camPanLimitHor, camPan[0]), camPanLimitHor);
    moveOnce[0] = 0.0;

    camPan[1] = camPan[1] - ((mov[1] * dtCoef + moveOnce[1] * dtCoef2) / zoomCoef);
    camPan[1] = Math.min(Math.max(camPanLimitVertDown, camPan[1]), camPanLimitVertUp);
    moveOnce[1] = 0.0;

    recreateModelViewMatrix();
    return camPan;
}

export function getZoom(dt) {
    const dtCoef = (((dt * 144) - 1) / 1.5) + 1;
    zoom *= (Math.abs(zoom) > 0.005);
    zoom -= Math.sign(zoom) * dt / 4;
    const condA = camZoom <= camZoomMax;
    const condB = camZoom >= camZoomMin;
    const res = condA * condB * dtCoef * zoom - ((1 - condA) * condB * dtCoef * 0.008) + ((1 - condB) * condA * dtCoef * 0.004);

    recreateModelViewMatrix();
    camZoom += res;
    zoomCoef += res;

    return camZoom;
}
