import { ProgramInfo } from "./type-hints.js";
import { getAllPlayers } from "../game-logic.js";
import { Player } from "../game-logic.js";
import { loadTexture } from "./resource-loading.js";

/**
 * @param {WebGLRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {*} buffers 
 * @param {*} texture 
 * @param {mat4} modelViewMatrix 
 * @returns 
 */
export function drawMapPlane(gl, programInfo, texture, modelViewMatrix) {
    gl.uniformMatrix4fv(programInfo.uniformLocations.modelViewMatrix,
                        false,
                        modelViewMatrix);

    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture  (gl.TEXTURE_2D, texture);
    gl.uniform1i    (programInfo.uniformLocations.uSampler, 0);

    const offset      = 0;
    const type        = gl.UNSIGNED_SHORT;
    const vertexCount = 6;
    gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
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

        gl.uniformMatrix4fv(programInfo.uniformLocations.modelViewMatrix,
                            false,
                            mv);

        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture  (gl.TEXTURE_2D, player.image);
        gl.uniform1i    (programInfo.uniformLocations.uSampler, 0);

        {
            const offset      = 24;
            const type        = gl.UNSIGNED_SHORT;
            const vertexCount = 6;
            gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
        }
    });
}

export function drawHUD(gl, programInfo, modelViewMatrix, texture)
{
    gl.disable     (gl.DEPTH_TEST);
    mat4.translate (modelViewMatrix,
                    modelViewMatrix,
                    [0.5, 0.0, 0.0]);
    mat4.scale     (modelViewMatrix,
                    modelViewMatrix,
                    [0.1, 0.1, 1]);
    gl.uniformMatrix4fv(programInfo.uniformLocations.modelViewMatrix,
                        false,
                        modelViewMatrix);

    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture  (gl.TEXTURE_2D, texture);
    gl.uniform1i    (programInfo.uniformLocations.uSampler, 0);

    const offset      = 24;
    const type        = gl.UNSIGNED_SHORT;
    const vertexCount = 6;
    gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
    gl.enable     (gl.DEPTH_TEST);
}
