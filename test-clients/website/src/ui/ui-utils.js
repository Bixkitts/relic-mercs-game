import * as Shaders from '../rendering/shaders.js';
import * as GlBuffers from '../rendering/gl-buffers.js';
import { getGLContext } from '../canvas-getter';
import * as Helpers from '../helpers.js';
import * as Text from './text.js';

// Global variables
const _textElements = [];
const _buttons      = [];
const _labels       = [];

// How wide and tall a letter is in the font texture

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

// TODO: Buttons are 99% the same as labels
export function makeButton(uiTransform, text, color, callback, alignEnum, isHidden)
{
    const textLength   = Helpers.longestLineLength(text) + 2;
    const letterWidth  = (uiTransform.width * Text.FONT_ATLAS_DIVISOR) / textLength;
    const letterHeight = Text.LETTER_TEXTURE_WIDTH * letterWidth * 1.52;
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
        [uiTransform.x + (Text.LETTER_TEXTURE_WIDTH * letterWidth), startY],
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
    const letterWidth     = (uiTransform.width * Text.FONT_ATLAS_DIVISOR) / textLength;
    const letterHeight    = Text.LETTER_TEXTURE_WIDTH * letterWidth * 1.52;
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
        [uiTransform.x + (Text.LETTER_TEXTURE_WIDTH * letterWidth), startY],
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

export function tryDeleteLabel(label)
{
    deleteTextElement(label.textElement);
    // The renderer loops through these
    // UI element dicts to draw the UI,
    // hence we remove it.
    Helpers.tryDeleteFromDict(_labels, label);
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
    makeLabel();
    makeTextElement("NOTIFICATION", [0.4, 0.6], 0.5);
}

// The coords argument should be a
// two-element array with opengl coordinate
// x and y components.
export function makeTextElement(text, coords, size)
{
    const gl          = getGLContext();
    const shaders     = Shaders.getShaders();
    const buffers     = GlBuffers.getVertBuffers();
    const textShader  = shaders[2];
    const textBuffers = buffers[2];
    const vao         = gl.createVertexArray();
    const lineHeight  = Text.LETTER_TEXTURE_WIDTH * 1.52;

    const uvArray     = [];
    const posArray    = [];
    const colorArray  = [];

    let currentLine   = 0;

    Text.generateTextInstanceData(text,
                                  Text.LETTER_TEXTURE_WIDTH,
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

// This function creates all sorts of ui elements
// to demonstrate how to create them, and to be
// able to visually check if they look correct.
export function doUiTest()
{
    const labelTransform = makeUiTransform(0.25, 0.9, 0.5, 0.54);
    const labelColor     = [0.8, 0.8, 1.0, 0.7];
    let lab = makeLabel(labelTransform,
                        '<color="#FF0000">Colored Text!</color>\nNot colored text!',
                        labelColor,
                        TextAlignment.Top);

    const buttonTransform = makeUiTransform(0.3, 0.35, 0.4, 0.04);
    const buttonColor     = [0.9, 0.9, 1.0, 1.0];
    let but1 = makeButton(buttonTransform,
                          '<color="#FF0000">Option2:</color> Say nothing',
                          buttonColor,
                          buttonCallbackTest,
                          TextAlignment.Center);
}

function buttonCallbackTest(button)
{
    console.log("Button was clicked!");
    Ui.hideUiElement(button);
}

function truncateAtNull(string)
{
    const nullIndex = string.indexOf('\0');
    return nullIndex !== -1 ? string.substring(0, nullIndex) : string;
}

export function greetPlayer(playerName)
{
    const truncatedName = truncateAtNull(playerName);
    const welcomeMsg    = `Welcome ${truncatedName}.`;
    const textCoords    = [0.3, 0.2];
    let welcomMsgTransform = makeUiTransform(0.5, 0.5, 0.4, 0.2);
    let te   = makeLabel(welcomMsgTransform,
                         welcomeMsg,
                         textCoords,
                         0.125,
                         TextAlignment.Center);
    // TODO: Implement
    //setTimeout(() => {
    //}, 3000);
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
