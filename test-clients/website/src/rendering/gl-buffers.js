import { getGLContext } from '../canvas-getter.js';
import { initShaderPrograms,
         setPositionAttribute,
         setTextureAttribute } from './shaders.js';

export function initBuffers(gl) 
{
    const positionBuffer = initPositionBuffer(gl);
    //const colorBuffer    = initColorBuffer   (gl);
    const indexBuffer    = initIndexBuffer   (gl);
    const texBuffer      = initTextureBuffer (gl);
      
    return {
        vertices: positionBuffer,
        uvs:      texBuffer,
        indices:  indexBuffer,
    };
}

function initPositionBuffer(gl) 
{
    const positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    const positions = [-1.618, -1.0, 0.0,   // Mesh for map 
                      1.618, -1.0, 0.0, 
                      -1.618, 1.0, 0.0, 
                      1.618, 1.0, 0.0,
                      -1.0, -1.0, 0.0,  // Mesh for square
                      1.0, -1.0, 0.0,
                      -1.0, 1.0, 0.0,
                      1.0, 1.0, 0.0,
                      -1.0, -0.0, 0.0,  // Mesh for characters
                      1.0, -0.0, 0.0,   // needs different origin
                      -1.0, 2.0, 0.0,   // for rotation
                      1.0, 2.0, 0.0];

    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
  
    return positionBuffer;
}

function initColorBuffer(gl) 
{
    const colors = [
      1.0,1.0,1.0,1.0, // white
      1.0,0.0,0.0,1.0, // red
      0.0,1.0,0.0,1.0, // green
      0.0,0.0,1.0,1.0, // blue
      1.0,1.0,1.0,1.0, // white
      1.0,0.0,0.0,1.0, // red
      0.0,1.0,0.0,1.0, // green
      0.0,0.0,1.0,1.0  // blue
    ];
  
    const colorBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, colorBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colors), gl.STATIC_DRAW);
  
    return colorBuffer;
}

function initIndexBuffer(gl)
{
    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);

    const indices = [0, 1, 2, 3, 
                     4, 5, 6, 7,
                     8, 9, 10, 11];

    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,
                  new Uint16Array(indices),
                  gl.STATIC_DRAW,);

    return indexBuffer;
}

/*
 * A buffer containing UV coordinates
 * for a single quad.
 * Will be heavily used throughout the
 * entire game.
 */
function initTextureBuffer(gl) 
{
    const textureCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);
  
    const textureCoordinates = [
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0
    ];
  
    gl.bufferData(gl.ARRAY_BUFFER,
                  new Float32Array(textureCoordinates),
                  gl.STATIC_DRAW);
  
    return textureCoordBuffer;
}

/*
 * We create a vertex and index
 * buffer with as many quads
 * as we could possibly want for a
 * line of text and send it to the
 * GPU
 */
export function initTextBuffers(gl) {
    const vertices    = [];
    const indices     = [];
    const uvs         = [];

    const charWidth   = 0.0625;
    const charHeight  = 0.1;
    const charCount   = 1;
    let   indexOffset = 0;
    
    const xOffset     = 0 * charWidth;

    // Each character is a quad (2 triangles)
    vertices.push(xOffset, 0.0, 0.0,                   // bottom left
                  xOffset, charHeight, 0.0,            // top left
                  xOffset + charWidth, charHeight, 0.0,// top right
                  xOffset + charWidth, 0.0, 0.0,       // bottom right
                  );

    indices.push(indexOffset,
                 indexOffset + 2,
                 indexOffset + 1,
                 indexOffset,
                 indexOffset + 3,
                 indexOffset + 2);
    indexOffset += 4;
    // placeholders, this will get
    // rewritten when the text is changed
    uvs.push(0.0, 1.0-0.0625,
             0.0, 1.0,
             0.0625, 1.0,
             0.0625, 1.0-0.0625);
    uvs.push(0.0, 1.0-0.0625,
             0.0, 1.0,
             0.0625, 1.0,
             0.0625, 1.0-0.0625);

    const vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

    const texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uvs), gl.STATIC_DRAW);

    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);

    return {
        vertices: vertexBuffer,
        uvs:      texCoordBuffer,
        indices:  indexBuffer,
        count:    indices.length
    };
}

let textElements = [];

export function getTextElements()
{
    return textElements;
}

export function buildTextElement(string, coords, size, programInfo, buffers) {
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
        const vMin = 0.0 - row * charHeightUV;

        // Push the UV coordinates for the character's quad
        uvs.push(uMin, vMin);
        pos.push(counter * charWidth, lineCount * charWidth * 1.45);
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
    setPositionAttribute         (gl, buffers.vertices, programInfo);
    setTextureAttribute          (gl, buffers.uvs, programInfo);
    gl.bindBuffer                (gl.ELEMENT_ARRAY_BUFFER, buffers.indices);
    setTextureAttributeInstanced (gl, texCoordBuffer, programInfo);
    setPosAttributeInstanced     (gl, posBuffer, programInfo);
    gl.bindVertexArray(null);
    // Store the buffer and coordinates
    textElements.push({ vao, coords, len, size});
}
function setTextureAttributeInstanced(gl, texCoordBuffer, programInfo) {
    const num       = 2; // every coordinate composed of 2 values
    const type      = gl.FLOAT;
    const normalize = false;
    const stride    = 0;
    const offset    = 0;
    gl.bindBuffer         (gl.ARRAY_BUFFER, texCoordBuffer);
    gl.enableVertexAttribArray(programInfo.attribLocations["aTextureOffsets"]);
    gl.vertexAttribPointer(programInfo.attribLocations["aTextureOffsets"],
                           num,
                           type,
                           normalize,
                           stride,
                           offset,);
    gl.vertexAttribDivisor(programInfo.attribLocations["aTextureOffsets"], 1);
}
function setPosAttributeInstanced(gl, posBuffer, programInfo) {
    const num       = 2; // every coordinate composed of 2 values
    const type      = gl.FLOAT;
    const normalize = false;
    const stride    = 0;
    const offset    = 0;
    gl.bindBuffer         (gl.ARRAY_BUFFER, posBuffer);
    gl.enableVertexAttribArray(programInfo.attribLocations["aLineDisplace"]);
    gl.vertexAttribPointer(programInfo.attribLocations["aLineDisplace"],
                           num,
                           type,
                           normalize,
                           stride,
                           offset,);
    gl.vertexAttribDivisor(programInfo.attribLocations["aLineDisplace"], 1);
}
