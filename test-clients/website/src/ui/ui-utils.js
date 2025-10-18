import * as Shaders from '../rendering/shaders.js';
import * as GlBuffers from '../rendering/gl-buffers.js';
import { getGLContext } from '../canvas-getter';
import * as Helpers from '../helpers.js';

// Global variables
const _textElements = [];
const _buttons      = [];
const _labels       = [];

// How wide and tall a letter is in the font texture
const FONT_ATLAS_DIVISOR = 16;
const LETTER_TEXTURE_WIDTH = 1.0 / FONT_ATLAS_DIVISOR;

export const TextAlignment = Object.freeze({
    Top: 0,
    Center: 1
});

export function makeUiTransform(x, y, width, height)
{
    return {
        x,
        y,
        width,
        height
    }
}

export function makeButton(uiTransform, text, color, callback, alignEnum, isHidden)
{
    const textLength   = Helpers.longestLineLength(text) + 2;
    const letterWidth  = (uiTransform.width * FONT_ATLAS_DIVISOR) / textLength;
    const letterHeight = LETTER_TEXTURE_WIDTH * letterWidth * 1.52;
    const lineCount    = Helpers.countLines(text);

    // Compute total text block height
    const totalTextHeight = letterHeight * lineCount;

    // Compute vertical starting position to center all lines in button
    let startY = 0.0;
    if (alignEnum === TextAlignment.Center) {
        startY = (uiTransform.y - (uiTransform.height / 2)) + (totalTextHeight / 2);
    }
    else if (alignEnum === TextAlignment.Top) {
        startY = (uiTransform.y - (0)) + (totalTextHeight / 2);
    }

    const textElement = new makeTextElement(
        text,
        [uiTransform.x + (LETTER_TEXTURE_WIDTH * letterWidth), startY],
        letterWidth
    );
    const button = {
        uiTransform,
        isHidden,
        color,
        callback,
        textElement
    }
    _buttons.push(button);
    return button;
}

export function hideUiElement(uiElement)
{
    uiElement.isHidden = true;
    uiElement.textElement.isHidden = true;
}

export function showUiElement(uiElement)
{
    uiElement.isHidden = false;
    uiElement.textElement.isHidden = false;
}

// Like a button, but not clickable.
// Just allows a background square for text
// TODO: Labels are 99.9% the same as buttons, but copy-pasted.
//       Perhaps I should address that?
export function makeLabel(uiTransform, text, color, alignEnum, isHidden)
{
    const textLength      = Helpers.longestLineLength(text) + 2;
    const letterWidth     = (uiTransform.width * FONT_ATLAS_DIVISOR) / textLength;
    const letterHeight    = LETTER_TEXTURE_WIDTH * letterWidth * 1.52;
    const lineCount       = Helpers.countLines(text);
    const totalTextHeight = letterHeight * lineCount;

    let startY = 0.0;
    if (alignEnum === TextAlignment.Center) {
        startY = (uiTransform.y - (uiTransform.height / 2)) + (totalTextHeight / 2);
    }
    else if (alignEnum === TextAlignment.Top) {
        startY = (uiTransform.y);
    }
    const textElement = makeTextElement(
        text,
        [uiTransform.x + (LETTER_TEXTURE_WIDTH * letterWidth), startY],
        letterWidth
    );

    const label = {
        uiTransform,
        isHidden,
        color,
        textElement
    }
    _labels.push(label);
    return label;
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

export function doNotification(text)
{
    makeTextElement()
}

export function makeTextElement(text, coords, size)
{
    const gl          = getGLContext();
    const shaders     = Shaders.getShaders();
    const buffers     = GlBuffers.getVertBuffers();
    const textShader  = shaders[2];
    const textBuffers = buffers[2];
    const vao         = gl.createVertexArray();
    const lineHeight  = LETTER_TEXTURE_WIDTH * 1.52;

    const uvArray     = [];
    const posArray    = [];
    const colorArray  = [];

    let currentLine   = 0;

    generateTextInstanceData(text,
                             LETTER_TEXTURE_WIDTH,
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

    const textElement = {
        vao,
        coords,
        length: posArray.length / 2, // one (x, y) pair per instance
        size,
        isHidden: false, // isHidden
        posBuffer,
        texCoordBuffer,
        colorBuffer,
        deleted: false
    };

    _textElements.push(textElement);
    return textElement;
}

// TODO: Refactor this function
function generateTextInstanceData(text,
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

        const u = col * charWidth;
        const v = -row * charWidth;

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
