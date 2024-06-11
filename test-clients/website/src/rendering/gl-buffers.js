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
    const charWidth   = 0.1;
    const charHeight  = 0.1;
    const charCount   = 80;
    let   indexOffset = 0;
    for (let i = 0; i < charCount; i++) {
        const xOffset = i * charWidth;
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
        const letterIndex = 1;
        uvs.push(0.0625, 1.0-0.0625,
                 0.0625, 1.0,
                 0.0625*2.0, 1.0,
                 0.0625*2.0, 1.0-0.0625);
    }

    const vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

    const texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uvs), gl.DYNAMIC_DRAW);

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

