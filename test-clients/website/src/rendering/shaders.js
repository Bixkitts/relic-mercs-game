import { initShaderProgram } from './resource-loading';

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
    
    // Hardcoded magenta color
    vec3 transparentColor = vec3(1.0, 1.0, 1.0);
    float threshold = 0.01; // Tolerance for color matching

    // Calculate the difference between the texture color and the transparent color
    vec3 diff = abs(textureColor.rgb - transparentColor);

    // If the difference is less than the threshold, set alpha to zero
    float alpha =  step(threshold, max(diff.r, max(diff.g, diff.b)));

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

export function initShaderPrograms(gl) {
    const programs = [];

    for (const config of _shaderConfigs) {
        const shaderProgram = initShaderProgram(gl,
                                                config.vertexSource,
                                                config.fragmentSource);
        const programInfo   = createProgramInfo(gl,
                                                shaderProgram,
                                                config.attributes,
                                                config.uniforms);
        programs.push(programInfo);
    }

    return programs;
}

export function setPositionAttribute(gl, posBuffer, programInfo) {
    const numComponents = 3;
    const type          = gl.FLOAT;
    const normalize     = false;
    const stride        = 0; 
    const offset        = 0;
    gl.bindBuffer             (gl.ARRAY_BUFFER, posBuffer);
    gl.vertexAttribPointer    (programInfo.attribLocations["aVertexPosition"],
                               numComponents,
                               type,
                               normalize,
                               stride,
                               offset);
    gl.enableVertexAttribArray(programInfo.attribLocations["aVertexPosition"]);
}

export function setTextureAttribute(gl, texCoordBuffer, programInfo) {
    const num       = 2; // every coordinate composed of 2 values
    const type      = gl.FLOAT;
    const normalize = false;
    const stride    = 0;
    const offset    = 0;
    gl.bindBuffer         (gl.ARRAY_BUFFER, texCoordBuffer);
    gl.vertexAttribPointer(programInfo.attribLocations["aTextureCoord"],
                           num,
                           type,
                           normalize,
                           stride,
                           offset,);
    gl.enableVertexAttribArray(programInfo.attribLocations["aTextureCoord"]);
}
