import { getShaders,
         setPositionAttribute2d,
         setTextureAttribute,
         setTextureAttributeInstanced,
         setPosAttributeInstanced } from './rendering/shaders.js';
import { getVertBuffers } from './rendering/gl-buffers.js';
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

export function buildTextElement(string, coords, size) {
    const shaders      = getShaders();
    const buffers      = getVertBuffers();
    const textBuffers  = buffers[2];
    const textShader   = shaders[2];
    const charWidth    = 0.0625;
    const gl           = getGLContext();
    const uvs          = [];
    const pos          = [];
    const charWidthUV  = 1.0 / 16.0;  // Assuming 16x16 grid
    const charHeightUV = 1.0 / 16.0;  // Assuming 16x16 grid
    let   len          = 0;
    let   lineCount    = 0;
    const vao          = gl.createVertexArray();
    let   isHidden     = false;

    // Loop through each character in the string
    for (let i = 0, counter = 0; i < string.length; i++) {
        const char      = string[i];
        if (char == '\n') {
            lineCount ++;
            counter   = 0;
            continue;
        }
        const charIndex = char.charCodeAt(0); // ASCII index

        // Calculate the row and column in the texture atlas
        const col = charIndex % 16;
        const row = Math.floor(charIndex / 16);

        // Calculate the UV offsets
        const uMin = col * charWidthUV;
        const vMin = -(row * charHeightUV);

        // Push the UV coordinates for the character's quad
        uvs.push(uMin, vMin);
        pos.push(counter * charWidth, lineCount * charWidth * 1.52);
        counter ++;
        len ++;
    }
    console.log("text pos array:" + pos);

    // Create and bind the texture coordinate buffer
    const texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uvs), gl.STATIC_DRAW);

    const posBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, posBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(pos), gl.STATIC_DRAW);

    gl.bindVertexArray(vao);
        setPositionAttribute2d       (gl, textBuffers.vertices, textShader);
        setTextureAttribute          (gl, textBuffers.uvs, textShader);
        gl.bindBuffer                (gl.ELEMENT_ARRAY_BUFFER, textBuffers.indices);
        setTextureAttributeInstanced (gl, texCoordBuffer, textShader);
        setPosAttributeInstanced     (gl, posBuffer, textShader);
    gl.bindVertexArray(null);

    const textElement = new TextElement(vao,
                                        coords,
                                        len,
                                        size,
                                        isHidden,
                                        posBuffer,
                                        texCoordBuffer);
    _textElements.push(textElement);

    return textElement;
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
