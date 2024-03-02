let mov = [0.0, 0.0, 0.0, 0.0];
let moveOnce = [0.0, 0.0, 0.0, 0.0];
let zoom = 0.0;
let camZoom = -2.5;
let zoomCoef = 1 / (camZoom + 3.5);
const camZoomMin  = -2.5;
const camZoomMax  = -0.5;

let camPan  = [0.0, 0.0, 0.0];
const camPanLimitVert = 1.0;
const camPanLimitHor  = 1.6;

// camPan[1] -= getKeyPan(0) * (camPan[1] > -camPanLimitVert); 
// camPan[1] += getKeyPan(2) * (camPan[1] < camPanLimitVert); 
// camPan[0] -= getKeyPan(3) * (camPan[0] > -camPanLimitHor); 
// camPan[0] += getKeyPan(1) * (camPan[0] < camPanLimitHor); 

const speed = 0.05;
const mouseCoef = 0.003;
const zoomFactor = 0.05;
let isLeftDown = false;
let isMiddleDown = false;
let isRightDown = false;
let mouseInCanvas = false;
let prevMouseMove = null;

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
        if (e.button & 0) isLeftDown = true;
        if (e.button & 1) isMiddleDown = true;
        if (e.button & 2) isRightDown = true;
    }
    document.onmouseup = e => {
        mov = [0.0, 0.0, 0.0, 0.0];
        isLeftDown *= 1 - (e.button == 0);
        isMiddleDown *= 1 - (e.button == 1);
        isRightDown *= 1 - (e.button == 2);
    }
    canvas.onmousemove = e => {
        if (isRightDown) {
            moveOnce[0] = zoomCoef * mouseCoef * e.movementY;
            moveOnce[1] = zoomCoef * mouseCoef * e.movementX;
        }
        prevMouseMove = e;
    }
    document.onkeydown = e => {
        if(mouseInCanvas) {
            switch (e.key) {
                case 'w': case 'W':
                    mov[0] = speed;
                    break;
                case 's': case 'S':
                    mov[2] = speed;
                    break;
                case 'd': case 'D':
                    mov[3] = speed;
                    break;
                case 'a': case 'A':
                    mov[1] = speed;
                    break;
            }
        }
    }
    document.onkeyup = e => {
        switch (e.key) {
            case 'w': case 'W':
                mov[0] = 0;
                break;
            case 's': case 'S':
                mov[2] = 0;
                break;
            case 'd': case 'D':
                mov[3] = 0;
                break;
            case 'a': case 'A':
                mov[1] = 0;
                break;
        }
    }
    canvas.onwheel = e => {
        e.preventDefault();
        const sign = Math.sign(e.deltaY);
        const res =  sign * zoomFactor / Math.abs(camZoom)
        zoom -= res;
        setTimeout(function () {
            zoom += res;
        }, 30);
    }
}

export function getKeyPan(index) {
    const res = mov[index] + moveOnce[index];
    moveOnce[index] = 0.0;

    return res;
}

export function getCamPan() {
    camPan[1] -= getKeyPan(0) * (camPan[1] > -camPanLimitVert); 
    camPan[1] += getKeyPan(2) * (camPan[1] <  camPanLimitVert); 
    camPan[0] -= getKeyPan(3) * (camPan[0] > -camPanLimitHor); 
    camPan[0] += getKeyPan(1) * (camPan[0] <  camPanLimitHor); 

    return camPan;
}

export function getZoom() {
    const res = zoom;
    if (camZoom <= camZoomMax && camZoom >= camZoomMin) {
        camZoom += res;
    }
    else if (camZoom > camZoomMax){
        camZoom -= 0.004;
    }
    else {
        camZoom += 0.002;
    }
    zoomCoef = 1 / (camZoom + 3.5);
    return camZoom;
}

