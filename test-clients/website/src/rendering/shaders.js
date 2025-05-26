import { getGLContext } from '../canvas-getter.js'
import * as ResourceLoading from './resource-loading';

const _solidVertShaderSource = 
`#version 300 es

in vec4 aVertexPosition;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

void main(void) {
    gl_Position   = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
}
`;

const _solidFragShaderSource =
`#version 300 es

precision highp float;

uniform vec4 uColor;

out vec4 fragColor;

void main(void) {
    fragColor = uColor;
}
`;

const _textureVertShaderSource = 
`#version 300 es

in vec4 aVertexPosition;
in vec2 aTextureCoord; 

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform vec2 uUVOffset;

out vec2 vTextureCoord;

void main(void) {
    gl_Position   = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
    vTextureCoord = aTextureCoord + uUVOffset;
}
`;
const _textureFragShaderSource =
`#version 300 es

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D uSampler;
uniform vec4 uTintColor;

out vec4 fragColor;

void main(void) {
    vec4 texColor = texture(uSampler, vTextureCoord);
    fragColor = texColor * uTintColor;
}
`;
const _textVertShaderSource = 
`#version 300 es

in vec4 aVertexPosition;
in vec2 aTextureCoord; 
in vec2 aTextureOffsets;
in vec2 aLineDisplace;
in vec4 aLetterColor;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform vec2 uUVOffset;

out vec2 vTextureCoord;
out vec4 vLetterColor;

void main(void)
{
    vec4 instanceOffset = vec4(aLineDisplace.x, -aLineDisplace.y, 0.0, 0.0);
    vec4 worldPosition = aVertexPosition + instanceOffset;
    gl_Position = uProjectionMatrix * uModelViewMatrix * worldPosition;
    vTextureCoord = aTextureCoord + uUVOffset + aTextureOffsets;
    vLetterColor = aLetterColor;
}
`;
const _textFragShaderSource = 
`#version 300 es

precision highp float;

in vec2 vTextureCoord;
in vec4 vLetterColor;

uniform sampler2D uSampler;

out vec4 fragColor;

void main(void)
{
    vec4 textureColor = texture(uSampler, vTextureCoord);
    
    if (textureColor.rgb == vec3(1.0)) {
        discard;
    }

    fragColor = vLetterColor;
}
`;

// This correlates to the shader
// indices returned from "GetShaders"
const _shaderConfigs = [
    {
        vertexSource:   _solidVertShaderSource,
        fragmentSource: _solidFragShaderSource,
        attributes:     ["aVertexPosition"],
        uniforms:       ["uProjectionMatrix",
                         "uModelViewMatrix",
                         "uColor"]
    },
    {
        vertexSource:   _textureVertShaderSource,
        fragmentSource: _textureFragShaderSource,
        attributes:     ["aVertexPosition",
                         "aTextureCoord"],
        uniforms:       ["uProjectionMatrix",
                         "uModelViewMatrix",
                         "uSampler",
                         "uUVOffset",
                         "uTintColor"]
    },
    {
        vertexSource:   _textVertShaderSource,
        fragmentSource: _textFragShaderSource,
        attributes:     ["aVertexPosition",
                         "aTextureCoord",
                         "aTextureOffsets",
                         "aLineDisplace",
                         "aLetterColor"],
        uniforms:       ["uProjectionMatrix",
                         "uModelViewMatrix",
                         "uUVOffset",
                         "uInstanceOffset"]
    },
    // Add more shader configurations here
];

function createProgramInfo(gl, shaderProgram, attributes, uniforms) {
    const programInfo = {
        program:          shaderProgram,
        attribLocations:  {},
        uniformLocations: {},
    };

    for (const attr of attributes) {
        programInfo.attribLocations[attr] = gl.getAttribLocation(shaderProgram, attr);
    }

    for (const uniform of uniforms) {
        programInfo.uniformLocations[uniform] = gl.getUniformLocation(shaderProgram, uniform);
    }

    return programInfo;
}

// Array of shaders where the program itself
// is the first member, then the attributes and
// uniforms are accessed as the subsequent members.
const _programInfos = [];
let   _programsInitialized = false;

function initShaderPrograms(gl) {

    for (const config of _shaderConfigs) {
        const shaderProgram = ResourceLoading.initShaderProgram(gl,
                                                                config.vertexSource,
                                                                config.fragmentSource);
        const programInfo   = createProgramInfo(gl,
                                                shaderProgram,
                                                config.attributes,
                                                config.uniforms);
        _programInfos.push(programInfo);
    }
}

export function getShaders()
{
    if (!_programsInitialized) {
        initShaderPrograms(getGLContext());
        _programsInitialized = true;
    }
    return _programInfos;
}

export function setPositionAttribute(gl, posBuffer, programInfo, offset) {
    const numComponents = 3;
    const type          = gl.FLOAT;
    const normalize     = false;
    const stride        = 0; 
    gl.bindBuffer             (gl.ARRAY_BUFFER, posBuffer);
    gl.vertexAttribPointer    (programInfo.attribLocations["aVertexPosition"],
                               numComponents,
                               type,
                               normalize,
                               stride,
                               offset);
    gl.enableVertexAttribArray(programInfo.attribLocations["aVertexPosition"]);
}

export function setPositionAttribute2d(gl, posBuffer, programInfo, offset) {
    const numComponents = 2;
    const type          = gl.FLOAT;
    const normalize     = false;
    const stride        = 0; 
    gl.bindBuffer             (gl.ARRAY_BUFFER, posBuffer);
    gl.vertexAttribPointer    (programInfo.attribLocations["aVertexPosition"],
                               numComponents,
                               type,
                               normalize,
                               stride,
                               offset);
    gl.enableVertexAttribArray(programInfo.attribLocations["aVertexPosition"]);
}

export function setTextureAttribute(gl, texCoordBuffer, programInfo, offset) {
    const num       = 2; // every coordinate composed of 2 values
    const type      = gl.FLOAT;
    const normalize = false;
    const stride    = 0;
    gl.bindBuffer         (gl.ARRAY_BUFFER, texCoordBuffer);
    gl.vertexAttribPointer(programInfo.attribLocations["aTextureCoord"],
                           num,
                           type,
                           normalize,
                           stride,
                           offset,);
    gl.enableVertexAttribArray(programInfo.attribLocations["aTextureCoord"]);
}

export function setTextureAttributeInstanced(gl, texCoordBuffer, programInfo) {
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

export function setPosAttributeInstanced(gl, posBuffer, programInfo) {
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

export function setColorAttributeInstanced(gl, colorBuffer, programInfo) {
    const colorSize = 4;
    const type      = gl.FLOAT;
    const normalize = false;
    const stride    = 0;
    const offset    = 0;
    gl.bindBuffer              (gl.ARRAY_BUFFER, colorBuffer);
    gl.enableVertexAttribArray (programInfo.attribLocations["aLetterColor"]);
    gl.vertexAttribPointer     (programInfo.attribLocations["aLetterColor"],
                                colorSize,
                                type,
                                normalize,
                                stride,
                                offset,);
    gl.vertexAttribDivisor     (programInfo.attribLocations["aLetterColor"], 1);
}
