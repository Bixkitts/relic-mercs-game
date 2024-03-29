import { ProgramInfo } from "./type-hints.js";

/**
 * @param {WebGLRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {*} buffers 
 * @param {*} texture 
 * @param {mat4} modelViewMatrix 
 * @returns 
 */
export function drawMapPlane(gl, programInfo, buffers, texture, modelViewMatrix) {
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clearDepth(1.0);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LEQUAL);

    // Clear the canvas before we start drawing on it.
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    // TODO: maybe move the projection matrix out of here
    const fieldOfView      = (45 * Math.PI) / 180; // in radians
    const aspect           = gl.canvas.clientWidth / gl.canvas.clientHeight;
    const zNear            = 0.1;
    const zFar             = 100.0;
    const projectionMatrix = mat4.create();

    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);

    setPositionAttribute (gl, buffers, programInfo);
    setTextureAttribute  (gl, buffers, programInfo);

    gl.bindBuffer        (gl.ELEMENT_ARRAY_BUFFER, buffers.indices);
    gl.useProgram        (programInfo.program);

    // Set the shader uniforms
    gl.uniformMatrix4fv(programInfo.uniformLocations.projectionMatrix,
                        false,
                        projectionMatrix);
    gl.uniformMatrix4fv(programInfo.uniformLocations.modelViewMatrix,
                        false,
                        modelViewMatrix);

    // Texture shit
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture  (gl.TEXTURE_2D, texture);
    gl.uniform1i    (programInfo.uniformLocations.uSampler, 0);


    const offset      = 0;
    const type        = gl.UNSIGNED_SHORT;
    const vertexCount = 6;
    gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);

    return modelViewMatrix;
}

/**
 * 
 * @param {WebGlRenderingContext} gl 
 * @param {ProgramInfo} programInfo 
 * @param {mat4} modelViewMatrix 
 * @param {Array<number>} pos 
 */
export function drawCharacter(gl, zoomLevel, programInfo, texture, modelViewMatrix, pos) 
{
    mat4.translate (modelViewMatrix,
                    modelViewMatrix,
                    pos);
    mat4.rotate    (modelViewMatrix,
                    modelViewMatrix,
                    (Math.PI * ((1 - zoomLevel) * 0.3) + 0.4),
                    [1, 0, 0]);
    mat4.scale     (modelViewMatrix,
                    modelViewMatrix,
                    [0.05, 0.05, 0.05],);

    gl.uniformMatrix4fv(programInfo.uniformLocations.modelViewMatrix,
                        false,
                        modelViewMatrix);
    // texture shit
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture  (gl.TEXTURE_2D, texture);
    gl.uniform1i    (programInfo.uniformLocations.uSampler, 0);

    {
        const offset      = 24;
        const type        = gl.UNSIGNED_SHORT;
        const vertexCount = 6;
        gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
    }
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
