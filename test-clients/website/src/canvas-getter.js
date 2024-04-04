

const canvasQuery = "#glcanvas";
const canvas = document.querySelector(canvasQuery);
const context = canvas.getContext("webgl");
if (context === null) {
    alert("Unable to initialize WebGL. Your browser or machine may not support it.");
}

/**
 * @returns {HTMLCanvasElement}
 */
export function getCanvas() {
    return canvas;
}

/**
 * @returns {WebGLRenderingContext}
 */
export function getGLContext() {
    return context;
}
