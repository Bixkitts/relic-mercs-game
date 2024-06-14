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
function createProgramInfo(gl, shaderProgram) {
    const programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, "aVertexPosition"),
            textureCoord:   gl.getAttribLocation(shaderProgram, "aTextureCoord"),
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, "uProjectionMatrix"),
            modelViewMatrix:  gl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
            uvOffset:         gl.getUniformLocation(shaderProgram, "uUVOffset"),
            uSampler:         gl.getUniformLocation(shaderProgram, "uSampler"),
        },
    };
    return programInfo;
}

export function initShaderPrograms(gl)
{
    const programs      = [];
    const shaderProgram = initShaderProgram(gl,
                                            _basicVertShaderSource,
        _basicFragShaderSource);
    programs.push(createProgramInfo(gl, shaderProgram));
    return programs;
}
