

import * as n from './networking.js';
import * as r from './rendering/renderer.js';

document.getElementById('fullscreenButton').addEventListener('click', toggleFullScreen);


function toggleFullScreen() {
    var canvas = document.getElementById('glcanvas');

    if (!document.fullscreenElement) {
        canvas.requestFullscreen();
    } else {
        if (document.exitFullscreen) {
            document.exitFullscreen();
        }
    }
}

