import { initBuffers } from './gl-buffers.js';
import { drawMapPlane } from './gl-draw-scene.js';
import { drawPlayers } from './gl-draw-scene.js';
import { initWASD } from '../user-inputs.js';
import { getZoom, getCamPan } from '../user-inputs.js';
import { getGLContext } from '../canvas-getter.js'
import { initShaderProgram, loadTexture } from './resource-loading.js';

const gl           = getGLContext();
let   deltaTime    = 0;
const canvas       = document.getElementById('glcanvas');

const canvasScale  = 2; // Larger = smaller canvas
let   screenWidth  = window.screen.availWidth;
let   screenHeight = window.screen.availHeight;
let   canvasWidth  = Math.floor(screenWidth / canvasScale);
let   canvasHeight = Math.floor(screenHeight / canvasScale);
// We'll want to run this refularly in case
// the screen resolution changes e.g.
// browser window is moved to a different monitor
function screenResUpdate() {
    screenWidth  = window.screen.availWidth;
    screenHeight = window.screen.availHeight;
    canvasWidth  = Math.floor(screenWidth / canvasScale);
    canvasHeight = Math.floor(screenHeight / canvasScale);
}

let   projectionMatrix = createProjMatrix(45);
export function getProjectionMatrix() {
    return projectionMatrix;
}

let modelViewMatrix = mat4.create();
export function getModelViewMatrix() {
    return modelViewMatrix;
}

export function getCamWorldPos() {

}


document.getElementById('fullscreenButton').addEventListener('click', toggleFullScreen);
main();

function main() {
    canvasInit()
    initWASD();
    const vertexShaderSource = `
            attribute vec4 aVertexPosition;
            attribute vec2 aTextureCoord; 

            uniform mat4 uModelViewMatrix;
            uniform mat4 uProjectionMatrix;

            varying highp vec2 vTextureCoord;

            void main(void) {
                gl_Position   = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
                vTextureCoord = aTextureCoord;
            }
          `;
    const fragmentShaderSource = `
            varying highp vec2 vTextureCoord;

            uniform sampler2D uSampler;

            void main(void) {
                gl_FragColor = texture2D(uSampler, vTextureCoord);
            }
          `;
    if (gl === null) {
        return;
    }
    // Flip image pixels into the bottom-to-top order that WebGL expects.
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    const shaderProgram = initShaderProgram (gl, vertexShaderSource, fragmentShaderSource);
    const buffers       = initBuffers       (gl);
    const programInfo   = createProgramInfo (gl, shaderProgram);
    const fpscap        = 50;

    document.sperframe  = (1000 / fpscap);
    document.fps        = 0;

    gl.clearColor        (0.0, 0.0, 0.0, 1.0);
    gl.clearDepth        (1.0);
    gl.enable            (gl.DEPTH_TEST);
    gl.enable            (gl.BLEND);
    gl.blendFunc         (gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    gl.depthFunc         (gl.LEQUAL);
    gl.clear             (gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    setPositionAttribute (gl, buffers, programInfo);
    setTextureAttribute  (gl, buffers, programInfo);

    gl.bindBuffer        (gl.ELEMENT_ARRAY_BUFFER, buffers.indices);
    gl.useProgram        (programInfo.program);

    gl.uniformMatrix4fv(programInfo.uniformLocations.projectionMatrix,
                        false,
                        projectionMatrix);

    startRenderLoop(programInfo);
}

function createProgramInfo(gl, shaderProgram) {
    const programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, "aVertexPosition"),
            textureCoord:   gl.getAttribLocation(shaderProgram, "aTextureCoord"),
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, "uProjectionMatrix"),
            modelViewMatrix:  gl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
            uSampler:         gl.getUniformLocation(shaderProgram, "uSampler"),
        },
    };
    return programInfo;
}

// Define an array to hold callback functions
let renderCallbacks = [];

// Function to subscribe to the onRender event
export function subscribeToRender(callback) {
    renderCallbacks.push(callback);
}

// Function to unsubscribe from the onRender event
export function unsubscribeFromRender(callback) {
    const index = renderCallbacks.indexOf(callback);
    if (index !== -1) {
        renderCallbacks.splice(index, 1);
    }
}

function startRenderLoop(programInfo) {
    let   then        = 0;
    const mapTexture  = loadTexture(gl, "map01.png");
    requestAnimationFrame(render);
    function render(now) {

        renderCallbacks.forEach(callback => {
            callback(deltaTime);
        });

        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        document.fps++;
        deltaTime = now - then;
        const wait = document.sperframe - deltaTime;
        then = now;

        const camPan          = getCamPan (deltaTime);
        const camZoom         = getZoom   (deltaTime);

        modelViewMatrix = doCameraTransforms(mat4.create(), camZoom, camPan);

        let locModelViewMatrix = mat4.create();
        mat4.copy(locModelViewMatrix, modelViewMatrix);

        drawMapPlane  (gl, 
                       programInfo, 
                       mapTexture, 
                       locModelViewMatrix);
        drawPlayers   (gl, 
                       camZoom, 
                       programInfo, 
                       locModelViewMatrix); 
        setTimeout(() => requestAnimationFrame(render), Math.max(0, wait))
    }
}

function doCameraTransforms(matrix, camZoom, camPan) {
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

function createProjMatrix(fov) {
    const fieldOfView      = (fov * Math.PI) / 180; // in radians
    const aspect           = gl.canvas.clientWidth / gl.canvas.clientHeight;
    const zNear            = 0.1;
    const zFar             = 100.0;
    const projectionMatrix = mat4.create();

    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);

    return projectionMatrix;
}

/*
 * Attempts to auto-detect resolution.
 * Maybe replace this with just a resolution
 * selection, or "createElement".
 */
function canvasInit() {
    canvas.width  = canvasWidth;
    canvas.height = canvasHeight;

    canvas.style.width  = canvasWidth + 'px';
    canvas.style.height = canvasHeight + 'px';

    gl.viewport(0, 0, canvas.width, canvas.height);
}

/*
 *  Was copy pasted from chat gpt,
 *  make it work in our setup
 */
function rayPlaneIntersection(rayDir, planeNormal) {
    let dotProduct = Vec3.dot(rayDir, planeNormal);
    // intersection
    if (Math.abs(dotProduct) < 1e-6) {
        // Return null to indicate no intersection
        return null;
    }

    // Calculate the distance from the origin to the plane along the ray direction
    // This distance is proportional to the dot product
    let distance = -1 * dotProduct;

    // Calculate the intersection point
    let intersectionPoint = {
        x: rayDir.x * distance,
        y: rayDir.y * distance,
        z: rayDir.z * distance
    };

    return intersectionPoint;
}

function setPositionAttribute(gl, buffers, programInfo) {
    const numComponents = 3;
    const type          = gl.FLOAT;
    const normalize     = false;
    const stride        = 0; 
    const offset        = 0;
    gl.bindBuffer             (gl.ARRAY_BUFFER, buffers.position);
    gl.vertexAttribPointer    (programInfo.attribLocations.vertexPosition,
                               numComponents,
                               type,
                               normalize,
                               stride,
                               offset);
    gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);
}

function setColorAttribute(gl, buffers, programInfo) {
    const numComponents = 4;
    const type          = gl.FLOAT;
    const normalize     = false;
    const stride        = 0;
    const offset        = 0;
    gl.bindBuffer             (gl.ARRAY_BUFFER, 
                               buffers.color);
    gl.vertexAttribPointer    (programInfo.attribLocations.vertexColor,
                               numComponents,
                               type,
                               normalize,
                               stride,
                               offset);
    gl.enableVertexAttribArray(programInfo.attribLocations.vertexColor);
}

function setTextureAttribute(gl, buffers, programInfo) {
    const num       = 2; // every coordinate composed of 2 values
    const type      = gl.FLOAT;
    const normalize = false;
    const stride    = 0;
    const offset    = 0;
    gl.bindBuffer         (gl.ARRAY_BUFFER, buffers.texCoord);
    gl.vertexAttribPointer(programInfo.attribLocations.textureCoord,
                           num,
                           type,
                           normalize,
                           stride,
                           offset,);
    gl.enableVertexAttribArray(programInfo.attribLocations.textureCoord);
}

function toggleFullScreen() {
    screenResUpdate();
    if (!document.fullscreenElement) {
        canvas.requestFullscreen();
        canvas.width  = screenWidth;
        canvas.height = screenHeight;

        canvas.style.width  = screenWidth + 'px';
        canvas.style.height = screenHeight + 'px';
        gl.viewport(0, 0, screenWidth, screenHeight);
    } else {
        if (document.exitFullscreen) {
            document.exitFullscreen();
            canvas.width  = canvasWidth;
            canvas.height = canvasHeight;

            canvas.style.width  = canvasWidth + 'px';
            canvas.style.height = canvasHeight + 'px';

            gl.viewport(0, 0, canvas.width, canvas.height);
        }
    }
}
document.addEventListener('fullscreenchange', function() {
    screenResUpdate();
    if (!document.fullscreenElement) {
        canvas.width  = canvasWidth;
        canvas.height = canvasHeight;

        canvas.style.width  = canvasWidth + 'px';
        canvas.style.height = canvasHeight + 'px';
        gl.viewport(0, 0, canvas.width, canvas.height);
    }
});
