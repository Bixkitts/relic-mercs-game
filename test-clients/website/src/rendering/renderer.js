import { initBuffers } from './gl-buffers.js';
import { drawMapPlane } from './gl-draw-scene.js';
import { drawCharacter } from './gl-draw-scene.js';
import { initWASD } from '../user-inputs.js';
import { getZoom, getCamPan } from '../user-inputs.js';
import { getContext } from '../canvas-getter.js'
import { initShaderProgram, loadTexture } from './resource-loading.js';

const gl        = getContext();
let   deltaTime = 0;

main();

/*
 * Attempts to auto-detect resolution.
 * Maybe replace this with just a resolution
 * selection, or "createElement".
 */
function canvasInit() {
    var canvas = document.getElementById('glcanvas');

    var screenWidth  = window.screen.availWidth;
    var screenHeight = window.screen.availHeight;

    var canvasWidth  = Math.floor(screenWidth / 2);
    var canvasHeight = Math.floor(screenHeight / 2);

    canvas.width  = canvasWidth;
    canvas.height = canvasHeight;

    canvas.style.width  = canvasWidth + 'px';
    canvas.style.height = canvasHeight + 'px';

    gl.viewport(0, 0, canvas.width, canvas.height);
}

/*
 *
 *  Was copy pasted from chat gpt,
 *  make it work in our setup
 */
function rayPlaneIntersection(rayDir, planeNormal) {
    // Assuming the ray starts from the origin (0, 0, 0)
    // and extends infinitely in the direction of rayDir

    // Assuming the plane passes through the origin (0, 0, 0)
    // If not, you'll need to adjust the plane's distance from the origin

    // Calculate the dot product of the ray direction and the plane normal
    let dotProduct = rayDir.x * planeNormal.x +
                     rayDir.y * planeNormal.y +
                     rayDir.z * planeNormal.z;

    // If the dot product is close to zero, the ray is parallel to the plane
    // and does not intersect
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

    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    const shaderProgram = initShaderProgram(gl, vertexShaderSource, fragmentShaderSource);
    const mapTexture       = loadTexture(gl, "map01.png");
    const charTexture       = loadTexture(gl, "player-test.png");

    // Flip image pixels into the bottom-to-top order that WebGL expects.
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    // Collect all the info needed to use the shader program.
    // Look up which attribute our shader program is using
    // for aVertexPosition and look up uniform locations.
    const programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, "aVertexPosition"),
            textureCoord: gl.getAttribLocation(shaderProgram, "aTextureCoord"),
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, "uProjectionMatrix"),
            modelViewMatrix: gl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
            uSampler: gl.getUniformLocation(shaderProgram, "uSampler"),
        },
    };
    // Call the routine that builds all the
    // objects we'll be drawing.
    const buffers = initBuffers(gl);
    let   then    = 0;
    const fpscap  = 50;
    document.sperframe = (1000 / fpscap);
    // setInterval(() => {
    //     requestAnimationFrame(render);
    // }, msperframe);
    document.fps = 0;
    setInterval(() => {
        document.lastFps = document.fps
        //console.log(document.fps)
        document.fps = 0;
    }, 1000)

    function render(now) {
        document.fps++;
        deltaTime = now - then;
        const wait = document.sperframe - deltaTime;
        then = now;

        const camPan  = getCamPan (deltaTime);
        const camZoom = getZoom   (deltaTime);

        // the camera world position is decided here
        const modelViewMatrix = mat4.create();
        mat4.translate (modelViewMatrix,
                        modelViewMatrix,
                        [0.0, 0.0, (camZoom - 1.2) * 2]);
        mat4.rotate    (modelViewMatrix,
                        modelViewMatrix,
                        Math.PI * ((1 - camZoom) * 0.3),
                        [-1, 0, 0]);
        mat4.translate (modelViewMatrix,
                        modelViewMatrix,
                        camPan);
        drawMapPlane  (gl, 
                       programInfo, 
                       buffers, 
                       mapTexture, 
                       modelViewMatrix);
        // We'll use this to move the character around
        const pos = [0.0, 0.0, 0.0125];
        drawCharacter (gl, 
                       camZoom, 
                       programInfo, 
                       charTexture, 
                       modelViewMatrix, 
                       pos);
        setTimeout(() => requestAnimationFrame(render), Math.max(0, wait))
    }
    requestAnimationFrame(render);


}
