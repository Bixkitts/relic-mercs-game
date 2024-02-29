import {initBuffers} from './gl-buffers.js';
import {drawScene} from './gl-draw-scene.js'

main();
function main() {
      // Vertex shader program
      const vsSource = `
          attribute vec4 aVertexPosition;
          uniform mat4 uModelViewMatrix;
          uniform mat4 uProjectionMatrix;
          void main() {
            gl_Position = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
          }
        `;
      const fsSource = `
          void main() {
            gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
          }
        `;

    const canvas = document.querySelector("#glcanvas");
    const gl     = canvas.getContext("webgl");

    // Only continue if WebGL is available and working
    if (gl === null) {
      alert(
        "Unable to initialize WebGL. Your browser or machine may not support it.",
      );
      return;
    }
    // Set clear color to black, fully opaque
    gl.clearColor (0.0, 0.0, 0.0, 1.0);
    // Clear the color buffer with specified clear color
    gl.clear      (gl.COLOR_BUFFER_BIT);

    const shaderProgram = initShaderProgram(gl, vsSource, fsSource); 
    // Collect all the info needed to use the shader program.
    // Look up which attribute our shader program is using
    // for aVertexPosition and look up uniform locations.
    const programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition:   gl.getAttribLocation(shaderProgram, "aVertexPosition"),
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, "uProjectionMatrix"),
            modelViewMatrix:  gl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
        },
    };
    // Here's where we call the routine that builds all the
    // objects we'll be drawing.
    const buffers = initBuffers(gl);
    // Draw the scene
    drawScene(gl, programInfo, buffers);

}
function initShaderProgram(gl, vsSource, fsSource) {
    const vertexShader   = loadShader(gl, gl.VERTEX_SHADER, vsSource);
    const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

    const shaderProgram  = gl.createProgram();

    gl.attachShader (shaderProgram, vertexShader);
    gl.attachShader (shaderProgram, fragmentShader);
    gl.linkProgram  (shaderProgram);

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
    gl.shaderSource  (shader, source);
    gl.compileShader (shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      alert(
        `An error occurred compiling the shaders: ${gl.getShaderInfoLog(shader)}`,
      );
      gl.deleteShader(shader);
      return null;
    }
    return shader;
}
