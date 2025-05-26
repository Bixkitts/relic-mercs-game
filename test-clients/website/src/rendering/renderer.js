import { getGLContext } from '../canvas-getter.js'
import * as GlBuffers from './gl-buffers.js';
import * as Drawing from './drawing.js';
import * as UserInputs from '../user-inputs.js';
import * as ResourceLoading from './resource-loading.js';
import * as Shaders from './shaders.js';

const _gl           = getGLContext();
let   _deltaTime    = 0;
const _canvas       = document.getElementById('glcanvas');

const _canvasScale  = 2; // Larger = smaller canvas
let   _screenWidth  = window.screen.availWidth;
let   _screenHeight = window.screen.availHeight;
let   _canvasWidth  = Math.floor(_screenWidth / _canvasScale);
let   _canvasHeight = Math.floor(_screenHeight / _canvasScale);
// We'll want to run this refularly in case
// the screen resolution changes e.g.
// browser window is moved to a different monitor
function screenResUpdate() {
    _screenWidth  = window.screen.availWidth;
    _screenHeight = window.screen.availHeight;
    _canvasWidth  = Math.floor(_screenWidth / _canvasScale);
    _canvasHeight = Math.floor(_screenHeight / _canvasScale);
}

let   _perspMatrix = createPerspMatrix(45);
export function getPerspMatrix() {
    return _perspMatrix;
}

let   _orthMatrix = createOrthMatrix();
export function getOrthMatrix() {
    return _orthMatrix;
}

let _modelViewMatrix = mat4.create();
export function getModelViewMatrix() {
    return _modelViewMatrix;
}

export function getCamWorldPos() {

}


document.getElementById('fullscreenButton').addEventListener('click', toggleFullScreen);
main();

function main() {
    canvasInit()
    UserInputs.initWASD();
    if (_gl === null) {
        return;
    }
    // Flip image pixels into the bottom-to-top order that WebGL expects.
    _gl.pixelStorei(_gl.UNPACK_FLIP_Y_WEBGL, true);

    const shaders       = Shaders.getShaders();
    const vaos          = GlBuffers.getVAOs();

    const fpscap        = 50;

    document.sperframe  = (1000 / fpscap);
    document.fps        = 0;

    _gl.clearColor        (0.0, 0.0, 0.0, 1.0);
    _gl.clearDepth        (1.0);
    _gl.enable            (_gl.DEPTH_TEST);
    _gl.enable            (_gl.BLEND);
    _gl.enable            (_gl.CULL_FACE);
    _gl.blendFunc         (_gl.SRC_ALPHA, _gl.ONE_MINUS_SRC_ALPHA);
    _gl.depthFunc         (_gl.LEQUAL);
    _gl.clear             (_gl.COLOR_BUFFER_BIT | _gl.DEPTH_BUFFER_BIT);

    startRenderLoop(shaders, vaos);
}


// Define an array to hold callback functions
let _renderCallbacks = [];

// Function to subscribe to the onRender event
export function subscribeToRender(callback) {
    _renderCallbacks.push(callback);
}

// Function to unsubscribe from the onRender event
export function unsubscribeFromRender(callback) {
    const index = _renderCallbacks.indexOf(callback);
    if (index !== -1) {
        _renderCallbacks.splice(index, 1);
    }
}

function startRenderLoop(shaders, vaos)
{
    const textTexture   = ResourceLoading.loadTexture(_gl, "BirdFont88.bmp", _gl.NEAREST, false);
    const mapTexture    = ResourceLoading.loadTexture(_gl, "map01.png", _gl.LINEAR_MIPMAP_LINEAR, true);
    const playerTexture = ResourceLoading.loadTexture(_gl, "playerTest.png", _gl.LINEAR_MIPMAP_LINEAR, true);
    const hudTexture    = ResourceLoading.loadTexture(_gl, "hud01.png", _gl.LINEAR_MIPMAP_LINEAR, true);
    let then = 0;
    requestAnimationFrame(render);
    function render(now) {
        _renderCallbacks.forEach(callback => {
            callback(_deltaTime);
        });

        _gl.clear(_gl.COLOR_BUFFER_BIT | _gl.DEPTH_BUFFER_BIT);
        document.fps++;
        _deltaTime = now - then;
        const wait = document.sperframe - _deltaTime;
        then = now;

        const camPan    = UserInputs.getCamPan (_deltaTime);
        const camZoom   = UserInputs.getZoom   (_deltaTime);

        _modelViewMatrix = doCameraTransforms(mat4.create(),
                                              camZoom,
                                              camPan);

        let locModelViewMatrix = mat4.create();
        mat4.copy(locModelViewMatrix, _modelViewMatrix);

        _gl.enable           (_gl.DEPTH_TEST);
        Drawing.drawMapPlane (_gl, 
                              vaos,
                              shaders, 
                              mapTexture,
                              locModelViewMatrix);
        Drawing.drawPlayers  (_gl, 
                              vaos,
                              camZoom, 
                              shaders, 
                              playerTexture,
                              locModelViewMatrix); 
        // From this point we render UI, so we
        // make it orthographic and disable the depth testing
        _gl.disable          (_gl.DEPTH_TEST);
        Drawing.drawHUD      (_gl,
                              vaos,
                              shaders,
                              hudTexture,
                              mat4.create());
        // Each text element has it's own vao,
        // special and separate from the global ones
        Drawing.drawText     (_gl,
                              shaders,
                              textTexture,
                              mat4.create());

        setTimeout(() => requestAnimationFrame(render), Math.max(0, wait))
    }
}

function doCameraTransforms(matrix, camZoom, camPan)
{
    mat4.translate (matrix,
                    matrix,
                    [0.0, 0.0, (camZoom - 1.2) * 2]);
    mat4.rotate    (matrix,
                    matrix,
                    Math.PI * ((1 - camZoom) * 0.3),
                    [-1, 0, 0]);
    mat4.translate (matrix,
                    matrix,
                    camPan);
    return matrix;
}

function createPerspMatrix(fov)
{
    const fieldOfView      = (fov * Math.PI) / 180; // in radians
    const aspect           = _gl.canvas.clientWidth / _gl.canvas.clientHeight;
    const zNear            = 0.1;
    const zFar             = 100.0;
    const projectionMatrix = mat4.create();

    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);

    return projectionMatrix;
}

function createOrthMatrix()
{
    const orthMatrix = mat4.create();
    mat4.ortho(orthMatrix, 0, 1, 0, 1, -1, 1);

    return orthMatrix;
}

/*
 * Attempts to auto-detect resolution.
 * Maybe replace this with just a resolution
 * selection, or "createElement".
 */
function canvasInit()
{
    _canvas.width  = _canvasWidth;
    _canvas.height = _canvasHeight;

    _canvas.style.width  = _canvasWidth + 'px';
    _canvas.style.height = _canvasHeight + 'px';

    _gl.viewport(0, 0, _canvas.width, _canvas.height);
}

function toggleFullScreen() {
    screenResUpdate();
    if (!document.fullscreenElement) {
        _canvas.requestFullscreen();
        _canvas.width  = _screenWidth;
        _canvas.height = _screenHeight;

        _canvas.style.width  = _screenWidth + 'px';
        _canvas.style.height = _screenHeight + 'px';
        _gl.viewport(0, 0, _screenWidth, _screenHeight);
    } else {
        if (document.exitFullscreen) {
            document.exitFullscreen();
            _canvas.width  = _canvasWidth;
            _canvas.height = _canvasHeight;

            _canvas.style.width  = _canvasWidth + 'px';
            _canvas.style.height = _canvasHeight + 'px';

            _gl.viewport(0, 0, _canvas.width, _canvas.height);
        }
    }
}
document.addEventListener('fullscreenchange', function()
{
    screenResUpdate();
    if (!document.fullscreenElement) {
        _canvas.width  = _canvasWidth;
        _canvas.height = _canvasHeight;

        _canvas.style.width  = _canvasWidth + 'px';
        _canvas.style.height = _canvasHeight + 'px';
        _gl.viewport(0, 0, _canvas.width, _canvas.height);
    }
});
