import { getContext } from "./canvas-getter.js"

const ctx = getContext()

export class ProgramInfo {
    constructor() {
        this.program = ctx.createProgram()
        this.attribLocations = {
            vertexPosition: 0,
            textureCoord: 0
        }
        this.uniformLocations = {
            projectionMatrix: ctx.getUniformLocation(),
            modelViewMatrix: ctx.getUniformLocation(),
            uSampler: ctx.getUniformLocation(),
        }
    }
}