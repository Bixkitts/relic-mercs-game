import { initShaderProgram } from './resource-loading';

const _basicVertShaderSource = `
attribute vec4 aVertexPosition;
attribute vec2 aTextureCoord; 

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform vec2 uUVOffset;

varying highp vec2 vTextureCoord;

void main(void) {
    gl_Position   = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
    vTextureCoord = aTextureCoord + uUVOffset;
}
`;
const _basicFragShaderSource = `
precision mediump float;
varying highp vec2 vTextureCoord;
uniform sampler2D uSampler;

void main(void) {
    gl_FragColor = texture2D(uSampler, vTextureCoord);
}
`;
const _textFragShaderSource = `
precision mediump float;
varying highp vec2 vTextureCoord;
uniform sampler2D uSampler;

void main(void) {
    vec4 textureColor = texture2D(uSampler, vTextureCoord);
    
    // Hardcoded magenta color
    vec3 transparentColor = vec3(1.0, 1.0, 1.0);
    float threshold = 0.01; // Tolerance for color matching

    // Calculate the difference between the texture color and the transparent color
    vec3 diff = abs(textureColor.rgb - transparentColor);

    // If the difference is less than the threshold, set alpha to zero
    float alpha =  step(threshold, max(diff.r, max(diff.g, diff.b)));

    gl_FragColor = vec4(textureColor.rgb, textureColor.a * alpha);
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
                         "uUVOffset",
                         "uSampler"]
    },
    {
        vertexSource:   _basicVertShaderSource,
        fragmentSource: _textFragShaderSource,
        attributes:     ["aVertexPosition",
                         "aTextureCoord"],
        uniforms:       ["uProjectionMatrix",
                         "uModelViewMatrix",
                         "uUVOffset",
                         "uSampler"]
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

