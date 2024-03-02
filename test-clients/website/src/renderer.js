import { initBuffers } from './gl-buffers.js';
import { drawMapPlane } from './gl-draw-scene.js';
import { drawCharacter } from './gl-draw-scene.js';
import { initWASD } from './user-inputs.js';
import { getZoom, getCamPan } from './user-inputs.js';

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

    const canvas = document.querySelector("#glcanvas");
    const gl = canvas.getContext("webgl");
    initWASD(canvas);

    if (gl === null) {
        alert("Unable to initialize WebGL. Your browser or machine may not support it.");
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

    const fpscap = 30;
    const sperframe = 1 / fpscap;
    const msperframe = 1000 / fpscap;
    // setInterval(() => {
    //     requestAnimationFrame(render);
    // }, msperframe);
    document.fps = 0;
    setInterval(() => {
        console.log(document.fps);
        document.fps = 0;
    }, 1000)

    function render(now) {
        document.fps++;
        now *= 0.001; // convert to seconds
        deltaTime = now - then;
        const wait = sperframe - deltaTime;
        then = now;

        const camPan = getCamPan(deltaTime);
        const camZoom = getZoom(deltaTime);

        const modelViewMatrix = mat4.create();

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

        drawMapPlane(gl, programInfo, buffers, texture, modelViewMatrix);
        //pos[0] += 1 * deltaTime;
        //drawCharacter(gl, programInfo, modelViewMatrix, pos);
        setTimeout(() => requestAnimationFrame(render), Math.max(0, wait * 1000))
        //requestAnimationFrame(render);
    }
    requestAnimationFrame(render);


}
function initShaderProgram(gl, vsSource, fsSource) {
    const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
    const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

    const shaderProgram = gl.createProgram();

    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        alert(
            `Unable to initialize the shader program: ${gl.getProgramInfoLog(
                shaderProgram,
            )}`,
        );
        return null;
    }

    return shaderProgram;
}
function loadShader(gl, type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        alert(`An error occurred compiling the shaders: ${gl.getShaderInfoLog(shader)}`);
        gl.deleteShader(shader);
        return null;
    }
    return shader;
}
function loadTexture(gl, url) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);

    // Because images have to be downloaded over the internet
    // they might take a moment until they are ready.
    // Until then put a single pixel in the texture so we can
    // use it immediately. When the image has finished downloading
    // we'll update the texture with the contents of the image.
    const level = 0;
    const internalFormat = gl.RGBA;
    const width = 1;
    const height = 1;
    const border = 0;
    const srcFormat = gl.RGBA;
    const srcType = gl.UNSIGNED_BYTE;
    const pixel = new Uint8Array([0, 0, 255, 255]); // opaque blue
    gl.texImage2D(gl.TEXTURE_2D,
        level,
        internalFormat,
        width,
        height,
        border,
        srcFormat,
        srcType,
        pixel,);

    const image = new Image();
    image.onload = () => {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texImage2D(
            gl.TEXTURE_2D,
            level,
            internalFormat,
            srcFormat,
            srcType,
            image,
        );

        // WebGL1 has different requirements for power of 2 images
        // vs. non power of 2 images so check if the image is a
        // power of 2 in both dimensions.
        if (isPowerOf2(image.width) && isPowerOf2(image.height)) {
            gl.generateMipmap(gl.TEXTURE_2D);
        } else {
            // It's not a power of 2. Turn off mips and set
            // wrapping to clamp to edge
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        }
    };
    image.src = url;

    return texture;
}

function isPowerOf2(value) {
    return (value & (value - 1)) === 0;
}
