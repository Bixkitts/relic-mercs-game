import * as n from './networking.js';
import * as r from './renderer.js';
document.getElementById("messageInput").onkeydown = e => { if(e.keyCode === 13) sendMessage(); };