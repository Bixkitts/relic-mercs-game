import { initShaderProgram } from './resource-loading';
import { getGLContext } from '../canvas-getter.js'

const _basicVertShaderSource = 
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
const _basicFragShaderSource =
`#version 300 es

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D uSampler;

out vec4 fragColor;

void main(void) {
    fragColor = texture(uSampler, vTextureCoord);
}
`;
const _hudVertShaderSource = 
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
const _hudFragShaderSource =
`#version 300 es

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D uSampler;

out vec4 fragColor;

void main(void) {
    fragColor = texture(uSampler, vTextureCoord);
}
`;
const _textVertShaderSource = 
`#version 300 es

in vec4 aVertexPosition;
in vec2 aTextureCoord; 
in vec2 aTextureOffsets;
in vec2 aLineDisplace;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform vec2 uUVOffset;

out vec2 vTextureCoord;

void main(void) {

    vec4 instanceOffset = vec4(aLineDisplace.x, -aLineDisplace.y, 0.0, 0.0);
    vec4 worldPosition = aVertexPosition + instanceOffset;
    gl_Position = uProjectionMatrix * uModelViewMatrix * worldPosition;
    vTextureCoord = aTextureCoord + uUVOffset + aTextureOffsets;
}
`;
const _textFragShaderSource = 
`#version 300 es

precision highp float;

in vec2 vTextureCoord;

uniform sampler2D uSampler;

out vec4 fragColor;

void main(void) {
    vec4 textureColor = texture(uSampler, vTextureCoord);

    // Define white color
    vec3 whiteColor = vec3(1.0, 1.0, 1.0);

    // Calculate alpha to discard pure white pixels
    float alpha = 1.0 - step(1.0, dot(textureColor.rgb, whiteColor) / 3.0);

    fragColor = vec4(textureColor.rgb, textureColor.a * alpha);
}
`;

const _shaderConfigs = [
    {
        vertexSource:   _basicVertShaderSource,
        fragmentSource: _basicFragShaderSource,
        attributes:     ["aVertexPosition",
                         "aTextureCoord"],
        uniforms:       ["uProjectionMatrix",
                         "uModelViewMatrix",
                         "uUVOffset"]
    },
    {
        vertexSource:   _hudVertShaderSource,
        fragmentSource: _hudFragShaderSource,
        attributes:     ["aVertexPosition",
                         "aTextureCoord"],
        uniforms:       ["uProjectionMatrix",
                         "uModelViewMatrix",
                         "uUVOffset"]
    },
    {
        vertexSource:   _textVertShaderSource,
        fragmentSource: _textFragShaderSource,
        attributes:     ["aVertexPosition",
                         "aTextureCoord",
                         "aTextureOffsets",
                         "aLineDisplace"],
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
        const shaderProgram = initShaderProgram(gl,
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
