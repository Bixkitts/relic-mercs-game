import * as n from './networking.js';
import * as gl from './gl-matrix-min.js'
import * as r from './renderer.js';
document.getElementById("messageInput").onkeydown = e => { if(e.key === "Enter") sendMessage(); };