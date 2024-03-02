let mov = [0.0, 0.0, 0.0, 0.0];
let moveOnce = [0.0, 0.0, 0.0, 0.0];
let zoom = 0.0;
let zoomTime = 0.0;
let camZoom = -2.5;
let zoomCoef = 1;
const camZoomMin = -2.5;
const camZoomMax = -0.5;

let camPan = [0.0, 0.0, 0.0];
const camPanLimitVert = 1.0;
const camPanLimitHor = 1.6;

const speed = 0.005;
const mouseCoef = 0.003;
const zoomFactor = 0.04;
let isLeftDown = false;
let isMiddleDown = false;
let isRightDown = false;
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
}
recreateModelViewMatrix();

/**
 * 
 * @param {HTMLCanvasElement} canvas 
 */
export function initWASD(canvas) {
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
        if(e.explicitOriginalTarget !== canvas) mov = [0.0, 0.0, 0.0, 0.0];
        if (e.button & 0) isLeftDown = true;
        if (e.button & 1) isMiddleDown = true;
        if (e.button & 2) isRightDown = true;
    }
    document.onmouseup = e => {
        isLeftDown *= 1 - (e.button == 0);
        isMiddleDown *= 1 - (e.button == 1);
        isRightDown *= 1 - (e.button == 2);
    }
    canvas.onmousemove = e => {
        if (isRightDown) {
            moveOnce[1] = mouseCoef * e.movementY;
            moveOnce[0] = mouseCoef * e.movementX;
        }
    }
    document.onkeydown = e => {
        if (mouseInCanvas) {
            let coef = 1;
            switch (e.key) {
                case 's': case 'S':
                    coef = -1;
                case 'w': case 'W':
                    mov[1] += coef * (1 - e.repeat) * speed;
                    break;
                case 'd': case 'D':
                    coef = -1;
                case 'a': case 'A':
                    mov[0] += coef * (1 - e.repeat) * speed;
                    break;
            }
        }
    }
    document.onkeyup = e => {
        if (mouseInCanvas) {
            let coef = 1;
            switch (e.key) {
                case 'w': case 'W':
                    coef = -1;
                case 's': case 'S':
                    mov[1] += coef * speed;
                    break;
                case 'a': case 'A':
                    coef = -1;
                case 'd': case 'D':
                    mov[0] += coef * speed;
                    break;
            }
        }
    }
    canvas.onwheel = e => {
        console.log(e);
        e.preventDefault();
        const res = -Math.sign(e.deltaY) * zoomFactor / Math.abs(camZoom);
        zoom = res;
        zoomTime = 0.06;
        // setTimeout(function () {
        //     zoom += res;
        // }, 30);
    }
}

export function getCamPan(dt) {
    const dtCoef = dt * 144;
    camPan[0] = camPan[0] + ((mov[0] + moveOnce[0]) / zoomCoef * dtCoef);
    camPan[0] = Math.min(Math.max(-camPanLimitHor, camPan[0]), camPanLimitHor);
    moveOnce[0] = 0.0;

    camPan[1] = camPan[1] - ((mov[1] + moveOnce[1]) / zoomCoef * dtCoef);
    camPan[1] = Math.min(Math.max(-camPanLimitVert, camPan[1]), camPanLimitVert);
    moveOnce[1] = 0.0;

    recreateModelViewMatrix();
    return camPan;
}

export function getZoom(dt) {
    const dtCoef = dt * 144;
    zoom *= (zoomTime > 0);
    zoomTime -= dt;
    const condA = camZoom <= camZoomMax;
    const condB = camZoom >= camZoomMin;
    const res = condA * condB * dtCoef * zoom - ((1 - condA) * condB * 0.012) + ((1 - condB) * condA * 0.006);

    recreateModelViewMatrix();
    camZoom += res;
    zoomCoef += res;

    return camZoom;
}

