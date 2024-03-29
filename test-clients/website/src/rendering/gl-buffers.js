function initBuffers(gl) 
{
    const positionBuffer = initPositionBuffer(gl);
    //const colorBuffer    = initColorBuffer   (gl);
    const indexBuffer    = initIndexBuffer   (gl);
    const texBuffer      = initTextureBuffer (gl);
      
    return {
        position: positionBuffer,
        texCoord: texBuffer,
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

    const indices = [0, 1, 2,  1, 2,  3, 
                     4, 5, 6,  5, 6,  7,
                     8, 9, 10, 9, 10, 11];

    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,
                  new Uint16Array(indices),
                  gl.STATIC_DRAW,);

    return indexBuffer;
}

function initTextureBuffer(gl) 
{
  const textureCoordBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);

  const textureCoordinates = [
    0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
    0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
    0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0
  ];

  gl.bufferData(
    gl.ARRAY_BUFFER,
    new Float32Array(textureCoordinates),
    gl.STATIC_DRAW,
  );

  return textureCoordBuffer;
}


export { initBuffers };
