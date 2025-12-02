// This file deals with the complicated text layout
// process: generating the UVs, vertex positions etc.
// that we need to render text.

export const FONT_ATLAS_DIVISOR = 16;
export const LETTER_TEXTURE_WIDTH = 1.0 / FONT_ATLAS_DIVISOR;

// TODO: Refactor this function
export function generateTextInstanceData(text,
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
