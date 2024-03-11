import { initBuffers } from './gl-buffers.js';
import { drawMapPlane } from './gl-draw-scene.js';
import { drawCharacter } from './gl-draw-scene.js';
import { initWASD } from './user-inputs.js';
import { getZoom, getCamPan } from './user-inputs.js';
import { getContext } from './canvas-getter.js'
import { initShaderProgram, loadTexture } from './resource-loading.js';

let deltaTime = 0;

main();
function main() {
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

    const gl = getContext();
    initWASD();

    if (gl === null) {
        return;
    }

    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    const shaderProgram = initShaderProgram(gl, vertexShaderSource, fragmentShaderSource);
    const texture = loadTexture(gl, "map01.png");
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
    let then = 0;

    let pos = [0.0, 0.0, 0.0];
    // Draw the scene repeatedly

    const fpscap = 50;
    document.sperframe = 1 / fpscap;
    const msperframe = 1000 / fpscap;
    // setInterval(() => {
    //     requestAnimationFrame(render);
    // }, msperframe);
    document.fps = 0;
    setInterval(() => {
        document.lastFps = document.fps
        //console.log(document.fps);
        document.fps = 0;
    }, 1000)

    function render(now) {
        document.fps++;
        now *= 0.001; // convert to seconds
        deltaTime = now - then;
        const wait = document.sperframe - deltaTime;
        then = now;

        const camPan = getCamPan(deltaTime);
        const camZoom = getZoom(deltaTime);

        let s = Math.sin(Math.PI / (camZoom + 4) * 0.5);
        let c = Math.cos(Math.PI / (camZoom + 4) * 0.5);
        let crotvec = [0, -c, s];
        const eye = [camPan[0], camPan[1], -2];
        const target = [camPan[0], camPan[1], 0];
        const up = [0, 1, 0];  
        const camera = mat4.create();
        mat4.lookAt(camera, eye, target, up);
        mat4.invert(camera, camera);
        //0.8660254037844386 0.5000000000000001
        //0.43596792373595283 0.8999622044693668

        // camera[5] *= c;
        // camera[6] -= s;
        // camera[9] -= s;
        // camera[10] *= c;

        /* 0: -1 0  0   0
               0 1  0   0
               0 0 -1   0
​               0 0 -2.5 1*/

        /* 1: -1 0                    0                  0
​               0 0.5                 -0.8660253882408142 0
​               0 -0.8660253882408142 -0.5                0
​               0 0                   -2.5                1 */

        /* 2: -1 0                   0                   0
        ​       0 0.9003360271453857  -0.4351954162120819 0
        ​       0 -0.4351954162120819 -0.9003360271453857 0
        ​       0 0                   -0.511320948600769  1 */
        const modelViewMatrix = mat4.clone(camera);
        // mat4.translate(modelViewMatrix,
        //     modelViewMatrix,
        //     [0.0, 0.0, camZoom]);

        // mat4.rotate(modelViewMatrix,
        //     modelViewMatrix,
        //     Math.PI / (camZoom + 4) * 0.5,
        //     [1, 0, 0]);

        // mat4.translate(modelViewMatrix,
        //     modelViewMatrix,
        //     camPan);

        drawMapPlane(gl, programInfo, buffers, texture, modelViewMatrix);
        //pos[0] += 1 * deltaTime;
        //drawCharacter(gl, programInfo, modelViewMatrix, pos);
        setTimeout(() => requestAnimationFrame(render), Math.max(0, wait * 1000))
        //requestAnimationFrame(render);
    }
    requestAnimationFrame(render);


}
