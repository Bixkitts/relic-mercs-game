import { getShaders,
         setPositionAttribute2d,
         setTextureAttribute,
         setTextureAttributeInstanced,
         setPosAttributeInstanced } from './rendering/shaders.js';
import { getVertBuffers } from './rendering/gl-buffers.js';
import { getGLContext } from './canvas-getter';

const _textElements = [];

export function getTextElements()
{
    return _textElements;
}

export function hideTextElement(index)
{
    _textElements[index].isHidden = true;
}
export function unhideTextElement(index)
{
    _textElements[index].isHidden = false;
}

export function buildTextElement(string, coords, size) {
    const shaders      = getShaders();
    const buffers      = getVertBuffers();
    const textBuffers  = buffers[2];
    const textShader   = shaders[2];
    const charWidth    = 0.0625;
    const gl           = getGLContext();
    const uvs          = [];
    const pos          = [];
    const charWidthUV  = 1.0 / 16.0;  // Assuming 16x16 grid
    const charHeightUV = 1.0 / 16.0;  // Assuming 16x16 grid
    let   len          = 0;
    let   lineCount    = 0;
    const vao          = gl.createVertexArray();
    let   counter      = 0;
    let   isHidden     = true;

    // Loop through each character in the string
    for (let i = 0; i < string.length; i++) {
        counter ++;
        const char      = string[i];
        if (char == '\n') {
            lineCount ++;
            counter   = 0;
            continue;
        }
        const charIndex = char.charCodeAt(0); // ASCII index

        // Calculate the row and column in the texture atlas
        const col = charIndex % 16;
        const row = Math.floor(charIndex / 16);

        // Calculate the UV offsets
        const uMin = col * charWidthUV;
        const vMin = -(row * charHeightUV);

        // Push the UV coordinates for the character's quad
        uvs.push(uMin, vMin);
        pos.push(counter * charWidth, lineCount * charWidth * 1.52);
        len ++;
    }

    // Create and bind the texture coordinate buffer
    const texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uvs), gl.STATIC_DRAW);

    const posBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, posBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(pos), gl.STATIC_DRAW);

    gl.bindVertexArray(vao);
    setPositionAttribute2d       (gl, textBuffers.vertices, textShader);
    setTextureAttribute          (gl, textBuffers.uvs, textShader);
    gl.bindBuffer                (gl.ELEMENT_ARRAY_BUFFER, textBuffers.indices);
    setTextureAttributeInstanced (gl, texCoordBuffer, textShader);
    setPosAttributeInstanced     (gl, posBuffer, textShader);
    gl.bindVertexArray(null);
    // Store the buffer and coordinates
    _textElements.push({ vao, coords, len, size, isHidden, posBuffer, texCoordBuffer});
    return _textElements.length - 1;
}

// Assumes a valid index.
// Deletes the text to free up memory.
// Expensive operation.
export function removeTextElement(index) {
    const gl      = getGLContext();
    const element = _textElements[index];
    
    // Clean up WebGL resources
    gl.deleteVertexArray (element.vao);
    gl.deleteBuffer      (element.posBuffer);
    gl.deleteBuffer      (element.texCoordBuffer);

    // You might also need to delete other buffers associated with the element if created separately
    // Example: gl.deleteBuffer(element.someBuffer);

    // Remove the element from the array
    _textElements.splice(index, 1);
}
