
export function printMat4(mat4) {
    let t = "";
    for(let i = 0; i < 4; i++) {
        for(let j = 0; j < 4; j++) {
            const res = mat4[4*i + j]
            t += " ".repeat(5 * j) + res + "\n";
        }
        t += "\n";
    }
    console.log(t);

}

export function rayPlaneIntersection(rayOrigin, rayDirection, planePoint, planeNormal) {
    // Calculate the denominator of the intersection formula
    console.log("plane:", planePoint);
    let denominator = vec3.dot(planeNormal, rayDirection);

    // If denominator is 0, ray and plane are parallel, no intersection
    if (Math.abs(denominator) < 1e-6) {
        return null;
    }

    // Calculate the parameter t of the intersection formula
    let t = vec3.dot(vec3.subtract(vec3.create(), planePoint, rayOrigin), planeNormal) / denominator;

    // If t is negative, intersection is behind the ray's origin
    if (t < 0) {
        return null;
    }

    // Calculate the intersection point using the parameter t
    let intersectionPoint = vec3.scaleAndAdd(vec3.create(), rayOrigin, rayDirection, t);

    return intersectionPoint;
}

export function longestLineLength(text) {
    let maxLength = 0;
    let currentLength = 0;
    let i = 0;

    while (i < text.length) {
        // Handle opening color tag: <color="#RRGGBB">
        if (text.startsWith('<color="#', i)) {
            const hexStart = i + 8;
            const hexEnd = text.indexOf('"', hexStart);
            const tagClose = text.indexOf('>', hexEnd);

            if (hexEnd !== -1 && tagClose !== -1) {
                const hex = text.slice(hexStart, hexEnd);
                i = tagClose + 1;
                continue;
            }
        }
        // Handle closing tag: </color>
        if (text.startsWith('</color>', i)) {
            i += 8;
            continue;
        }
        const char = text[i];
        if (char === '\n') {
            if (currentLength > maxLength) {
              maxLength = currentLength;
            }
            currentLength = 0;
        }
        else {
            currentLength++;
        }
        i++;
    }
    // Check last line
    if (currentLength > maxLength) {
      maxLength = currentLength;
    }
    return maxLength;
}

export function countLines(text) {
  if (text.length === 0) return 0;

  let count = 1; // Start with 1 line
  for (let i = 0; i < text.length; i++) {
    if (text[i] === '\n') {
      count++;
    }
  }

  return count;
}

