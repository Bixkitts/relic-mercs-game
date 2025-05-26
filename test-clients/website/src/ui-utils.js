import * as Shaders from './rendering/shaders.js';
import * as GlBuffers from './rendering/gl-buffers.js';
import { getGLContext } from './canvas-getter';

// TODO:
// probably don't need some of this data
export class TextElement {
    constructor(vao, coords, len, size, isHidden, posBuffer, texCoordBuffer) {
        this.vao            = vao;
        this.coords         = coords;
        this.len            = len;
        this.size           = size;
        this.isHidden       = isHidden;
        this.posBuffer      = posBuffer;
        this.texCoordBuffer = texCoordBuffer;
        // Is this text element still in the
        // primary array with it's GL buffers
        // intact?
        this.deleted        = false;
    }
}

// TODO:
// probably don't need some of this data
export class Button {
    constructor(coords, width, height, label, callback, isHidden) {
        this.coords         = coords;
        this.width          = width;
        this.height         = height;
        this.isHidden       = isHidden;
        this.callback       = callback;
        const letterWidth = (width * 16) / (label.length+2);
        this.label          = buildTextElement(label,
                                               [coords[0] + (0.0625 * letterWidth),
                                               (coords[1] - (height/2)) + (height/10)],
                                               letterWidth);
    }
    hide()
    {
        this.isHidden = true;
        this.label.isHidden = true;
    }
    show()
    {
        this.isHidden = false;
        this.label.isHidden = false;
    }
}

const _textElements = [];
const _buttons      = [];

export function getButtons()
{
    return _buttons;
}

export function getTextElements()
{
    return _textElements;
}

export function buildTextElement(text, coords, size)
{
    const gl = getGLContext();
    const shaders = Shaders.getShaders();
    const buffers = GlBuffers.getVertBuffers();
    const textShader = shaders[2];
    const textBuffers = buffers[2];
    const vao = gl.createVertexArray();
    const charWidth = 0.0625;
    const uvScale = 1 / 16.0; // Assuming 16x16 atlas
    const lineHeight = charWidth * 1.52;

    const uvArray = [];
    const posArray = [];
    const colorArray = [];

    let instanceCount = 0;
    let currentLine = 0;

    generateTextInstanceData(text,
                             uvScale,
                             charWidth,
                             lineHeight,
                             uvArray,
                             posArray,
                             colorArray,
                             () => currentLine++,
                             () => currentLine);

    const texCoordBuffer = GlBuffers.createFloatBuffer(gl, uvArray);
    const posBuffer      = GlBuffers.createFloatBuffer(gl, posArray);
    const colorBuffer    = GlBuffers.createFloatBuffer(gl, colorArray);

    gl.bindVertexArray(vao);
        Shaders.setPositionAttribute2d(gl, textBuffers.vertices, textShader);
        Shaders.setTextureAttribute(gl, textBuffers.uvs, textShader);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, textBuffers.indices);

        Shaders.setTextureAttributeInstanced(gl, texCoordBuffer, textShader);
        Shaders.setPosAttributeInstanced(gl, posBuffer, textShader);
        Shaders.setColorAttributeInstanced(gl, colorBuffer, textShader);
    gl.bindVertexArray(null);

    const textElement = new TextElement(
        vao,
        coords,
        posArray.length / 2, // one (x, y) pair per instance
        size,
        false, // isHidden
        posBuffer,
        texCoordBuffer
    );

    _textElements.push(textElement);
    return textElement;
}

function generateTextInstanceData(text,
                                  uvScale,
                                  charWidth,
                                  lineHeight,
                                  uvArray,
                                  posArray,
                                  colorArray,
                                  onNewLine,
                                  getCurrentLine)
{
    let column = 0;

    for (let i = 0; i < text.length; i++) {
        const char = text[i];
        if (char === '\n') {
            onNewLine();
            column = 0;
            continue;
        }

        const code = char.charCodeAt(0);
        const col = code % 16;
        const row = Math.floor(code / 16);

        const u = col * uvScale;
        const v = -row * uvScale;

        uvArray.push(u, v);
        posArray.push(column * charWidth, getCurrentLine() * lineHeight);
        colorArray.push(0.0, 1.0, 0.0, 1.0); // Green RGBA

        column++;
    }
}



// Deletes the text to free up memory.
// Expensive operation, and you need
// to rebuild the TextElement if you need
// it again.
// Only do this if the text will not be shown again,
// otherwise set isHidden
export function deleteTextElement(textElement) {
    if (textElement.deleted) {
        return;
    }
    textElement.deleted = true;
    const index   = _textElements.indexOf(textElement);
    const gl      = getGLContext();
    
    gl.deleteVertexArray (textElement.vao);
    gl.deleteBuffer      (textElement.posBuffer);
    gl.deleteBuffer      (textElement.texCoordBuffer);

    _textElements.splice(index, 1);
}

// Defined in GL screenspace coordinates
export function buildButton(coords, width, height, label, callback)
{
    const button = new Button(coords,
                              width,
                              height,
                              label,
                              callback,
                              false);
    _buttons.push(button);
    return button;
}
