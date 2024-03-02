let mov = [0.0, 0.0, 0.0, 0.0];
let moveOnce = [0.0, 0.0, 0.0, 0.0];
let zoom = 0.0;
let camZoom = -2.5;
let zoomCoef = 1 / (camZoom + 3.5);
const camZoomMin = -2.5;
const camZoomMax = -0.5;

let camPan = [0.0, 0.0, 0.0];
const camPanLimitVert = 1.0;
const camPanLimitHor = 1.6;

const speed = 0.03;
const mouseCoef = 0.003;
const zoomFactor = 0.05;
let isLeftDown = false;
let isMiddleDown = false;
let isRightDown = false;
let mouseInCanvas = false;

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
    canvas.onclick = e => {
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
            switch (e.key) {
                case 'w': case 'W':
                    mov[1] += (1 - e.repeat) * speed;
                    break;
                case 's': case 'S':
                    mov[1] -= (1 - e.repeat) * speed;
                    break;
                case 'd': case 'D':
                    mov[0] -= (1 - e.repeat) * speed;
                    break;
                case 'a': case 'A':
                    mov[0] += (1 - e.repeat) * speed;
                    break;
            }
        }
    }
    document.onkeyup = e => {
        let coef = 1;
        if (mouseInCanvas) {
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
        e.preventDefault();
        const res = Math.sign(e.deltaY) * zoomFactor / Math.abs(camZoom);
        zoom -= res;
        setTimeout(function () {
            zoom += res;
        }, 30);
    }
}

export function getCamPan() {
    camPan[0] += (mov[0] + moveOnce[0]) * zoomCoef;
    moveOnce[0] = 0.0;

    camPan[1] -= (mov[1] + moveOnce[1]) * zoomCoef;
    moveOnce[1] = 0.0;

    return camPan;
}

export function getZoom() {
    const res = zoom;
    if (camZoom <= camZoomMax && camZoom >= camZoomMin) {
        camZoom += res;
    }
    else if (camZoom > camZoomMax) {
        camZoom -= 0.004;
    }
    else {
        camZoom += 0.002;
    }
    zoomCoef = 1 / (camZoom + 3.5);
    return camZoom;
}

