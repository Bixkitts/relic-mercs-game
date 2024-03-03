
/**
 * 
 * @param {WebGLRenderingContext} gl 
 * @param {*} programInfo 
 * @param {*} buffers 
 * @param {*} texture 
 * @param {mat4} modelViewMatrix 
 * @returns 
 */
export function drawMapPlane(gl, programInfo, buffers, texture, modelViewMatrix) {
    gl.clearColor(0.0, 0.0, 0.0, 1.0); // Clear to black, fully opaque
    gl.clearDepth(1.0); // Clear everything
    gl.enable(gl.DEPTH_TEST); // Enable depth testing
    gl.depthFunc(gl.LEQUAL); // Near things obscure far things

    // Clear the canvas before we start drawing on it.
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    const fieldOfView      = (45 * Math.PI) / 180; // in radians
    const aspect           = gl.canvas.clientWidth / gl.canvas.clientHeight;
    const zNear            = 0.1;
    const zFar             = 100.0;
    const projectionMatrix = mat4.create();

    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);
    const scaleMatrix     = mat4.create();
    
    mat4.scale (scaleMatrix,
                    modelViewMatrix,
                    [1.0, 1.0, 1.0]);

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
                        scaleMatrix);

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

export function drawCharacter(gl, programInfo, modelViewMatrix, pos) 
{

    mat4.rotate(modelViewMatrix,
                modelViewMatrix,
                (Math.PI / 2) - ((camZoom + 3) * 0.5),
                [1, 0, 0]);
    mat4.scale(modelViewMatrix,
                modelViewMatrix,
                [0.06, 0.1, 0.1],);
    mat4.translate(modelViewMatrix,
                   modelViewMatrix,
                   pos);
    gl.uniformMatrix4fv(programInfo.uniformLocations.modelViewMatrix,
                        false,
                        modelViewMatrix);
    {
        const offset      = 0;
        const type        = gl.UNSIGNED_SHORT;
        const vertexCount = 6;
        gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
    }

}

// Tell WebGL how to pull out the positions from the position
// buffer into the vertexPosition attribute.
function setPositionAttribute(gl, buffers, programInfo) {
    const numComponents = 3;        // pull out 2 values per iteration
    const type          = gl.FLOAT; // the data in the buffer is 32bit floats
    const normalize     = false;    // don't normalize
    const stride        = 0;        // how many bytes to get from one set of values to the next
    // 0 = use type and numComponents above
    const offset        = 0;        // how many bytes inside the buffer to start from
    gl.bindBuffer             (gl.ARRAY_BUFFER, buffers.position);
    gl.vertexAttribPointer    (programInfo.attribLocations.vertexPosition,
                               numComponents,
                               type,
                               normalize,
                               stride,
                               offset);
    gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);
}

// Tell WebGL how to pull out the colors from the color buffer
// into the vertexColor attribute.
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
// tell webgl how to pull out the texture coordinates from buffer
function setTextureAttribute(gl, buffers, programInfo) {
    const num       = 2; // every coordinate composed of 2 values
    const type      = gl.FLOAT; // the data in the buffer is 32-bit float
    const normalize = false; // don't normalize
    const stride    = 0; // how many bytes to get from one set to the next
    const offset    = 0; // how many bytes inside the buffer to start from
    gl.bindBuffer         (gl.ARRAY_BUFFER, buffers.texCoord);
    gl.vertexAttribPointer(programInfo.attribLocations.textureCoord,
                           num,
                           type,
                           normalize,
                           stride,
                           offset,);
    gl.enableVertexAttribArray(programInfo.attribLocations.textureCoord);
}
