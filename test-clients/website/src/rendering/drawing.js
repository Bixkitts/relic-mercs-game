import { ProgramInfo } from "./type-hints.js";
import * as Renderer from "./renderer.js";
import * as GameLogic from "../game-logic.js";
import * as Ui from "../ui-utils.js";

/**
 * @param {WebGLRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {*} buffers 
 * @param {*} texture 
 * @param {mat4} modelViewMatrix 
 * @returns 
 */
export function drawMapPlane(gl,
                             vaos,
                             shaders,
                             mapTexture,
                             modelViewMatrix) {
    gl.bindVertexArray(null);
    const programInfo = shaders[1];
    gl.useProgram       (programInfo.program);
    setPersp            (gl, programInfo);
    gl.bindVertexArray  (vaos[0]);
    gl.activeTexture    (gl.TEXTURE0);
    gl.bindTexture      (gl.TEXTURE_2D, mapTexture);

    gl.uniform1i(programInfo.uniformLocations["uSampler"], 0);
    gl.uniform2f(programInfo.uniformLocations["uUVOffset"],
                 0.0, 0.0);
    gl.uniform4f(programInfo.uniformLocations["uTintColor"],
                 1.0, 1.0, 1.0, 1.0);
    gl.uniformMatrix4fv (programInfo.uniformLocations["uModelViewMatrix"],
                         false,
                         modelViewMatrix);
    {
        const offset      = 0;
        const type        = gl.UNSIGNED_SHORT;
        const vertexCount = 4;
        gl.drawElements(gl.TRIANGLE_STRIP, vertexCount, type, offset);
    }
    gl.bindVertexArray(null);
}

/**
 * 
 * @param {WebGlRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {mat4} modelViewMatrix 
 * @param {Array<number>} pos 
 */
export function drawPlayers(gl, vaos, camZoom, shaders, playerTexture, modelViewMatrix) 
{
    const programInfo = shaders[1];
    gl.useProgram       (programInfo.program);
    setPersp            (gl, programInfo);
    gl.bindVertexArray  (vaos[1]);
    gl.activeTexture    (gl.TEXTURE0);
    gl.bindTexture      (gl.TEXTURE_2D, playerTexture);
    const players = GameLogic.getAllPlayers();
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

        gl.uniform4f(programInfo.uniformLocations["uTintColor"],
                     1.0, 1.0, 1.0, 1.0);
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
    gl.bindVertexArray  (null);
}

function drawButtons(gl, shader, modelViewMatrix)
{
    const buttons = Ui.getButtons();
    buttons.forEach( button => {
        if (button.isHidden)
            return;
        let mv = mat4.clone(modelViewMatrix);
        mat4.translate      (mv,
                             mv,
                             [button.transform.x, button.transform.y, 0.0]);
        mat4.scale          (mv,
                             mv,
                             [button.transform.width, button.transform.height, 0.5]);
        gl.uniformMatrix4fv (shader.uniformLocations["uModelViewMatrix"],
                             false,
                             mv);
        gl.uniform4f        (shader.uniformLocations["uColor"],
                             button.color[0],
                             button.color[1],
                             button.color[2],
                             button.color[3]);
        const offset      = 8;
        const type        = gl.UNSIGNED_SHORT;
        const vertexCount = 4;
        gl.drawElements     (gl.TRIANGLE_STRIP, vertexCount, type, offset);
    });
}

function drawLabels(gl, shader, modelViewMatrix)
{
    const buttons = Ui.getLabels();
    buttons.forEach( label => {
        if (label.isHidden)
            return;
        let mv = mat4.clone(modelViewMatrix);
        mat4.translate      (mv,
                             mv,
                             [label.transform.x, label.transform.y, 0.0]);
        mat4.scale          (mv,
                             mv,
                             [label.transform.width, label.transform.height, 0.5]);
        gl.uniformMatrix4fv (shader.uniformLocations["uModelViewMatrix"],
                             false,
                             mv);
        gl.uniform4f        (shader.uniformLocations["uColor"],
                             label.color[0],
                             label.color[1],
                             label.color[2],
                             label.color[3]);
        const offset      = 8;
        const type        = gl.UNSIGNED_SHORT;
        const vertexCount = 4;
        gl.drawElements     (gl.TRIANGLE_STRIP, vertexCount, type, offset);
    });
}

export function drawHUD(gl, vaos, shaders, hudTexture, modelViewMatrix)
{
    const shader = shaders[0]; // solid color shader
    gl.useProgram       (shader.program);
    setOrtho            (gl, shader);
    gl.bindVertexArray  (vaos[2]);
    //gl.activeTexture    (gl.TEXTURE0);
    //gl.bindTexture      (gl.TEXTURE_2D, hudTexture);
    // Hud Bar
    {
        let mv = mat4.clone(modelViewMatrix);
        gl.uniformMatrix4fv(shader.uniformLocations["uModelViewMatrix"],
                            false,
                            mv);
        gl.uniform4f(shader.uniformLocations["uColor"],
                     1.0, 1.0, 1.0, 1.0);
        const offset      = 0;
        const type        = gl.UNSIGNED_SHORT;
        const vertexCount = 4;
        gl.drawElements(gl.TRIANGLE_STRIP, vertexCount, type, offset);
    }
    drawButtons(gl, shader, modelViewMatrix);
    drawLabels(gl, shader, modelViewMatrix);

    gl.bindVertexArray(null);
}

// This function is to be a consumer that grabs
// text, it's coordinates, and size from a buffer
// and draws that in a loop, similar to how the players
// are drawn.
export function drawText(gl, shaders, textTexture, modelViewMatrix)
{
    const programInfo = shaders[2]; // text shader
    gl.useProgram       (programInfo.program);
    setOrtho            (gl, programInfo);
    gl.activeTexture    (gl.TEXTURE0);
    gl.bindTexture      (gl.TEXTURE_2D, textTexture);
    const textElements = Ui.getTextElements();
    const offset       = 0;
    const type         = gl.UNSIGNED_SHORT;
    for (const textElement of textElements) {
        if (textElement.isHidden) {
            continue;
        }
        const { vao, coords, len, size } = textElement;
        let matrix = mat4.clone(modelViewMatrix);
        gl.bindVertexArray(vao);
        mat4.translate (matrix,
                        matrix,
                        [coords[0], coords[1], 0.0]);
        mat4.scale     (matrix,
                        matrix,
                        [size, size, 0.0]);
        gl.uniformMatrix4fv(programInfo.uniformLocations["uModelViewMatrix"],
                            false,
                            matrix);
        gl.drawElementsInstanced(gl.TRIANGLE_STRIP, 4, type, offset, len);
        gl.bindVertexArray(null);
    }
}

function setPersp(gl, programInfo)
{
    gl.uniformMatrix4fv(programInfo.uniformLocations["uProjectionMatrix"],
                        false,
                        Renderer.getPerspMatrix());
}

function setOrtho(gl, programInfo)
{
    gl.uniformMatrix4fv(programInfo.uniformLocations["uProjectionMatrix"],
                        false,
                        Renderer.getOrthMatrix());
}
