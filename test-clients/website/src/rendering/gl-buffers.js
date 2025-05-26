import { getGLContext } from '../canvas-getter.js';
import * as Shaders from './shaders.js';

// All the other buffers are
// collected into _vertBuffers
let   _vertBuffersInitialized = false;
const _vertBuffers = [];

let _geoBuffers  = [];
let _hudBuffers  = [];
let _textBuffers = [];

function initVertBuffers(gl)
{
    _geoBuffers  = createVertexBuffers(gl, getGeoModel());
    _hudBuffers  = createVertexBuffers(gl, getHudModel());
    _textBuffers = createVertexBuffers(gl, getTextModel());
    _vertBuffers.push(_geoBuffers);
    _vertBuffers.push(_hudBuffers);
    _vertBuffers.push(_textBuffers);
}

export function getVertBuffers()
{
    if (!_vertBuffersInitialized) {
        initVertBuffers(getGLContext());
        _vertBuffersInitialized = true;
    }
    return _vertBuffers;
}

function createVertexBuffers(gl, modelData)
{
    const positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(modelData.positions), gl.STATIC_DRAW);

    const texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(modelData.uvs), gl.STATIC_DRAW);

    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(modelData.indices), gl.STATIC_DRAW);

    return {vertices: positionBuffer,
            uvs:      texCoordBuffer,
            indices:  indexBuffer};
}

function getGeoModel() 
{
    const positions = [
        -1.618, -1.0, 0.0, // Mesh for map 
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
        1.0, 2.0, 0.0
    ];
    const uvs = [
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0
    ];
    const indices = [
        0, 1, 2, 3, 
        4, 5, 6, 7,
        8, 9, 10, 11
    ];
    return {positions: positions, uvs: uvs, indices:indices};
}

function getHudModel()
{
    const positions = [
        0.01, 0.01, 0.0,       // Bottom Bar
        1.0 - 0.01, 0.01, 0.0,
        0.01, 0.24, 0.0,
        1.0 - 0.01, 0.24, 0.0,
        0.0, -1.0, 0.0,        // Mesh for square
        1.0, -1.0, 0.0,
        0.0, 0.0, 0.0,
        1.0, 0.0, 0.0 
    ];
    const uvs = [
        0.0029, 0.9,    // Bottom Bar
        0.49, 0.9,
        0.0029, 0.9985,
        0.49, 0.9985,
        0.0803, 0.8317, // Button texture
        0.1948, 0.8317,
        0.0803, 0.8946,
        0.1948, 0.8946
    ];
    const indices = [
        0, 1, 2, 3,
        4, 5, 6, 7
    ];
    return {positions: positions, uvs: uvs, indices:indices};
}

function getTextModel() {
    const charWidth   = 0.0625;
    const charHeight  = 0.1;

    // Each character is a quad (2 triangles)
    const positions = [
        0.0, 0.0,
        0.0, -charHeight,
        charWidth,0.0,
        charWidth, -charHeight,
    ];

    const indices = [
        0, 1, 2, 3
    ];
    const uvs = [
        0.0, 1.0,
        0.0, 1.0-0.0625,
        0.0625, 1.0,
        0.0625, 1.0-0.0625,
    ];
    return {positions: positions, uvs: uvs, indices:indices};
}


let   _vaosInitialized = false;
const _vaos = [];


// We need to pass in the results from
// getVertBuffers() and getShaders()
function initVAOs(programs, buffers)
{
    const gl         = getGLContext();
    const mapVao     = gl.createVertexArray();
    const playerVao  = gl.createVertexArray();
    const hudVao     = gl.createVertexArray();

    gl.bindVertexArray(null);
    gl.bindVertexArray(mapVao);
    Shaders.setPositionAttribute (gl, buffers[0].vertices, programs[1], 0);
    Shaders.setTextureAttribute  (gl, buffers[0].uvs, programs[1], 0);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers[0].indices);

    gl.bindVertexArray(null);
    gl.bindVertexArray(playerVao);
    Shaders.setPositionAttribute (gl, buffers[0].vertices, programs[1], 0);
    Shaders.setTextureAttribute  (gl, buffers[0].uvs, programs[1], 0);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers[0].indices);

    gl.bindVertexArray(null);
    gl.bindVertexArray(hudVao);
    Shaders.setPositionAttribute (gl, buffers[1].vertices, programs[0], 0);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers[1].indices);
    gl.bindVertexArray(null);

    _vaos.push(mapVao);
    _vaos.push(playerVao);
    _vaos.push(hudVao);
}

export function getVAOs()
{
    if (!_vaosInitialized) {
        initVAOs(Shaders.getShaders(), getVertBuffers());
    }
    return _vaos;
}

export function createFloatBuffer(gl, dataArray) {
    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(dataArray), gl.STATIC_DRAW);
    return buffer;
}
