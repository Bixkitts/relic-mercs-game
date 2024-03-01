let mov = [0.0, 0.0, 0.0, 0.0];
let moveOnce = [0.0, 0.0, 0.0, 0.0];
let zoom = 0.0;

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
function initWASD(canvas) {
    canvas.onmousedown = e => {
        e.preventDefault();
    }
    canvas.onmouseup = e => {
        e.preventDefault();
    }
    document.onclick = e => {
        console.log(e)
        if(e.explicitOriginalTarget !== canvas) mouseInCanvas = false;
    }
    canvas.onclick = e => {
        document.activeElement.blur();
        mouseInCanvas = true;
    }
    canvas.oncontextmenu = () => false;
    document.onmousedown = e => {
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
            moveOnce[0] = Math.max(mouseCoef * e.movementY, 0);
            moveOnce[2] = Math.max(-mouseCoef * e.movementY, 0);
            moveOnce[1] = Math.max(mouseCoef * e.movementX, 0);
            moveOnce[3] = Math.max(-mouseCoef * e.movementX, 0);
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
        if (e.deltaY > 0) {
            // scrolled down
            zoom -= zoomFactor;
            setTimeout(function () {
                zoom += zoomFactor;
            }, 30);
        }
        if (e.deltaY < 0) {
            // scrolled up
            zoom += zoomFactor;
            setTimeout(function () {
                zoom -= zoomFactor;
            }, 30);
        }
    }
}

function getKeyPan(index) {
    const res = mov[index] + moveOnce[index];
    moveOnce[index] = 0.0;
    return res;
}
function getZoom() {
    return zoom;
}

export { initWASD };
export { getKeyPan };
export { getZoom };
