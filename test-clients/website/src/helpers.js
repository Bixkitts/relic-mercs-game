
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