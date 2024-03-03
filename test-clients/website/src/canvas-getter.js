

const canvasQuery = "#glcanvas";
const canvas = document.querySelector(canvasQuery);
const context = canvas.getContext("webgl");

/**
 * @returns {HTMLCanvasElement}
 */
export function getCanvas() {
    return canvas;
}

/**
 * @returns {WebGLRenderingContext}
 */
export function getContext() {
    return context;
}