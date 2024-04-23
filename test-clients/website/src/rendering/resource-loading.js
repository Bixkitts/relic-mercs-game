

/**
 * @param {WebGLRenderingContext} gl 
 */
export function loadShader(gl, type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        alert(`An error occurred compiling the shaders: ${gl.getShaderInfoLog(shader)}`);
        gl.deleteShader(shader);
        return null;
    }
    return shader;
}

/**
 * @param {WebGLRenderingContext} gl 
 */
export function loadTexture(gl, url) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);

    const level          = 0;
    const internalFormat = gl.RGBA;
    const width          = 1;
    const height         = 1;
    const border         = 0;
    const srcFormat      = gl.RGBA;
    const srcType        = gl.UNSIGNED_BYTE;
    const pixel          = new Uint8Array([0, 0, 255, 255]);
    gl.texImage2D (gl.TEXTURE_2D,
                   level,
                   internalFormat,
                   width,
                   height,
                   border,
                   srcFormat,
                   srcType,
                   pixel,);

    // Texture will be solid blue until the image actually loads
    const image = new Image();
    image.onload = () => {
        gl.bindTexture (gl.TEXTURE_2D, texture);
        gl.texImage2D  (gl.TEXTURE_2D, level, internalFormat, 
                        srcFormat, srcType, image);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        // WebGL1 has different requirements for power of 2 images
        // vs. non power of 2 images so check if the image is a
        // power of 2 in both dimensions.
        if (isPowerOf2(image.width) && isPowerOf2(image.height)) {
            gl.generateMipmap(gl.TEXTURE_2D);
        }
    };
    image.src = url;

    return texture;
}

function isPowerOf2(value) {
    return (value & (value - 1)) === 0;
}
/**
 * 
 * @param {WebGLRenderingContext} gl 
 * @param {string} vsSource 
 * @param {string} fsSource 
 * @returns 
 */
export function initShaderProgram(gl, vsSource, fsSource) {
    const vertexShader   = loadShader(gl, gl.VERTEX_SHADER, vsSource);
    const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);
    const shaderProgram  = gl.createProgram();

    gl.attachShader (shaderProgram, vertexShader);
    gl.attachShader (shaderProgram, fragmentShader);
    gl.linkProgram  (shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        alert(`Unable to initialize the shader program: ${gl.getProgramInfoLog(shaderProgram)}`);
        return null;
    }

    return shaderProgram;
}
