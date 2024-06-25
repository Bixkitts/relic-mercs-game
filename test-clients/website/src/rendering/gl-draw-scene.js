import { ProgramInfo } from "./type-hints.js";
import { getAllPlayers } from "../game-logic.js";
import { getTextElements } from "../ui-utils.js";

/**
 * @param {WebGLRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {*} buffers 
 * @param {*} texture 
 * @param {mat4} modelViewMatrix 
 * @returns 
 */
export function drawMapPlane(gl, programInfo, modelViewMatrix) {
    gl.uniformMatrix4fv(programInfo.uniformLocations["uModelViewMatrix"],
                        false,
                        modelViewMatrix);

    const offset      = 0;
    const type        = gl.UNSIGNED_SHORT;
    const vertexCount = 4;
    gl.drawElements(gl.TRIANGLE_STRIP, vertexCount, type, offset);
}

/**
 * 
 * @param {WebGlRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {mat4} modelViewMatrix 
 * @param {Array<number>} pos 
 */
export function drawPlayers(gl, camZoom, programInfo, modelViewMatrix) 
{
    const players = getAllPlayers();
    players.forEach(player => {
        let mv = mat4.clone(modelViewMatrix);
        mat4.translate (mv,
                        mv,
                        player.position,);
        mat4.rotate    (mv,
                        mv,
                        (Math.PI * ((1 - camZoom) * 0.3) + 0.4),
                        [1, 0, 0]);
        mat4.scale     (mv,
                        mv,
                        [0.05, 0.05, 0.05],);

        gl.uniformMatrix4fv(programInfo.uniformLocations["uModelViewMatrix"],
                            false,
                            mv);
        {
            const offset      = 16;
            const type        = gl.UNSIGNED_SHORT;
            const vertexCount = 4;
            gl.drawElements(gl.TRIANGLE_STRIP, vertexCount, type, offset);
        }
    });
}

export function drawHUD(gl, programInfo, modelViewMatrix)
{
    // TODO:
    // Render from a list of active HUD elements
    // from another higher level source file
    gl.uniformMatrix4fv(programInfo.uniformLocations["uModelViewMatrix"],
                        false,
                        modelViewMatrix);
    const offset      = 0;
    const type        = gl.UNSIGNED_SHORT;
    const vertexCount = 4;
    gl.drawElements(gl.TRIANGLE_STRIP, vertexCount, type, offset);
}

// This function is to be a consumer that grabs
// text, it's coordinates, and size from a buffer
// and draws that in a loop, similar to how the players
// are drawn.
export function drawText(gl, programInfo, modelViewMatrix)
{
    const textElements = getTextElements();
    const offset       = 0;
    const type         = gl.UNSIGNED_SHORT;
    for (const textElement of textElements) {
        const { vao, coords, len, size } = textElement;
        gl.bindVertexArray(vao);
        mat4.translate (modelViewMatrix,
                        modelViewMatrix,
                        [coords[0], coords[1], 0.0]);
        mat4.scale     (modelViewMatrix,
                        modelViewMatrix,
                        [size, size, 0.0]);
        gl.uniformMatrix4fv(programInfo.uniformLocations["uModelViewMatrix"],
                            false,
                            modelViewMatrix);
        gl.drawElementsInstanced(gl.TRIANGLE_STRIP, 4, type, offset, len);
        gl.bindVertexArray(null);
    }
}
