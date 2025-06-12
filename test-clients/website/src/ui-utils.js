import * as Shaders from './rendering/shaders.js';
import * as GlBuffers from './rendering/gl-buffers.js';
import { getGLContext } from './canvas-getter';
import * as Helpers from './helpers.js';

// Global variables
const _textElements = [];
const _buttons      = [];
const _labels       = [];

export const UiAlignmentEnum = Object.freeze({
    Top: 0,
    Center: 1
});

export class TextElement {
    constructor(vao, coords, len, size, isHidden, posBuffer, texCoordBuffer, colorBuffer) {
        this.vao            = vao;
        this.coords         = coords;
        this.len            = len;
        this.size           = size;
        this.isHidden       = isHidden;
        this.posBuffer      = posBuffer;
        this.texCoordBuffer = texCoordBuffer;
        this.colorBuffer    = colorBuffer;
        // Is this text element still in the
        // primary array with it's GL buffers
        // intact?
        this.deleted        = false;
    }
}

export class UiTransformStruct {
    constructor(x, y, width, height) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }
}

export class Button {
    constructor(uiTransform, text, color, callback, alignEnum, isHidden) {
        this.transform = uiTransform;
        this.isHidden  = isHidden;
        this.color     = color;
        this.callback  = callback;
    
        const textLength = Helpers.longestLineLength(text) + 2;
        const letterWidth = (this.transform.width * 16) / textLength;
        const letterHeight = 0.0625 * letterWidth * 1.52;
        const lineCount = Helpers.countLines(text);
    
        // Compute total text block height
        const totalTextHeight = letterHeight * lineCount;
    
        // Compute vertical starting position to center all lines in button
        let startY = 0.0;
        if (alignEnum === UiAlignmentEnum.Center) {
            startY = (this.transform.y - (this.transform.height / 2)) + (totalTextHeight / 2);
        }
        else if (alignEnum === UiAlignmentEnum.Top) {
            startY = (this.transform.y - (0)) + (totalTextHeight / 2);
        }
    
        this.text = buildTextElement(
            text,
            [this.transform.x + (0.0625 * letterWidth), startY],
            letterWidth
        );
    }

    hide()
    {
        this.isHidden = true;
        this.text.isHidden = true;
    }
    show()
    {
        this.isHidden = false;
        this.text.isHidden = false;
    }
}

// Like a button, but not clickable.
// Just allows a background square for text
// TODO: Labels are 99.9% the same as buttons, but copy-pasted.
//       Perhaps I should address that?
export class Label {
    constructor(uiTransform, text, color, alignEnum, isHidden) {
        this.transform = uiTransform;
        this.isHidden  = isHidden;
        this.color     = color;
    
        const textLength = Helpers.longestLineLength(text) + 2;
        const letterWidth = (this.transform.width * 16) / textLength;
        const letterHeight = 0.0625 * letterWidth * 1.52;
        const lineCount = Helpers.countLines(text);
    
        // Compute total text block height
        const totalTextHeight = letterHeight * lineCount;
    
        // Compute vertical starting position to center all lines in button
        let startY = 0.0;
        if (alignEnum === UiAlignmentEnum.Center) {
            startY = (this.transform.y - (this.transform.height / 2)) + (totalTextHeight / 2);
        }
        else if (alignEnum === UiAlignmentEnum.Top) {
            startY = (this.transform.y);
        }
    
        this.text = buildTextElement(
            text,
            [this.transform.x + (0.0625 * letterWidth), startY],
            letterWidth
        );
    }

    hide()
    {
        this.isHidden = true;
        this.text.isHidden = true;
    }
    show()
    {
        this.isHidden = false;
        this.text.isHidden = false;
    }
}

export function getButtons()
{
    return _buttons;
}

export function getLabels()
{
    return _labels;
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
        texCoordBuffer,
        colorBuffer
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
    let i = 0;

    let currentColor = [0.0, 0.0, 0.0, 1.0]; // Default: Green RGBA
    const colorStack = [];

    while (i < text.length) {
        // Check for opening <color="#RRGGBB">
        if (text.startsWith('<color="#', i)) {
            console.log("Found text color!");
            const hexStart = i + 8;
            const hexEnd = text.indexOf('"', hexStart);
            const tagClose = text.indexOf('>', hexEnd);

            if (hexEnd !== -1 && tagClose !== -1) {
                const hex = text.slice(hexStart, hexEnd);
                colorStack.push(currentColor);
                const r = parseInt(hex.slice(1, 3), 16) / 255;
                const g = parseInt(hex.slice(3, 5), 16) / 255;
                const b = parseInt(hex.slice(5, 7), 16) / 255;
                currentColor = [r, g, b, 1.0];
                console.log(`CurrentColor: ${currentColor}`);
                i = tagClose + 1;
                continue;
            }
        }

        // Check for closing </color> tag
        if (text.startsWith('</color>', i)) {
            if (colorStack.length > 0) {
                currentColor = colorStack.pop();
            }
            i += 8; // skip "</color>"
            continue;
        }

        // Handle newlines
        const char = text[i];
        if (char === '\n') {
            onNewLine();
            column = 0;
            i++;
            continue;
        }

        // Normal character rendering
        const code = char.charCodeAt(0);
        const col = code % 16;
        const row = Math.floor(code / 16);

        const u = col * uvScale;
        const v = -row * uvScale;

        uvArray.push(u, v);
        posArray.push(column * charWidth, getCurrentLine() * lineHeight);
        colorArray.push(...currentColor);

        column++;
        i++;
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
    gl.deleteBuffer      (textElement.colorBuffer);

    _textElements.splice(index, 1);
}

// Defined in GL screenspace coordinates
export function buildButton(transformStruct, text, color, callback, alignEnum)
{
    const button = new Button(transformStruct,
                              text,
                              color,
                              callback,
                              alignEnum,
                              false);
    _buttons.push(button);
    return button;
}

// Defined in GL screenspace coordinates
export function buildLabel(transformStruct, text, color, alignEnum)
{
    const label = new Label( transformStruct,
                             text,
                             color,
                             alignEnum,
                             false);
    _labels.push(label);
    return label;
}
