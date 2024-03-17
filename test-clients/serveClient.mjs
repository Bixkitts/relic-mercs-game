//thin server to test clients
//run with "node serveClient.js"

import * as http from 'http';
import * as fs from 'fs';
import { Stream } from 'stream';

const mimes = {
    js: "text/javascript",
    html: "text/html",
    css: "text/css",
    gif: "image/gif",
    ico: "image/x-icon",
    jpg: "image/jpeg",
    png: "image/png",
    json: "application/json",
    pdf: "application/pdf",
    txt: "text/plain",
    xlsx: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    docx: "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    bin: "application/octet-stream",
}

/**
 * @param {string} ext 
 */
function mime(ext) {
    this.setHeader("Content-Type", mimes[ext] ?? ext);
}

/**
 * @param {http.ServerResponse} res 
 */
const apply = (res) => {
    res.mime = mime
    res.mime.bind(res);
}

/**
 * @param {object} scrobj 
 * @param {string} root path
 * @param {never} orir 
 * @param {keyof typeof fs} handler
 * @param {BufferEncoding} enc
 */
const loadScripts = (scrobj, root, enc = "utf-8", handler = "readFileSync", opt = undefined, orir = root) => {
    console.log("root: " + root);
    let paths = fs.readdirSync(root, { recursive: true, encoding: "utf-8" });
    function handlePath(path) {
        let cpath = (root + "/" + path).replace(/\/\//, "/");
        console.log("  - " + path);
        fs.stat(cpath, (err, stats) => {
            if (err) throw err;
            if (stats.isDirectory()) {
                console.log("dir")
                loadScripts(scrobj, cpath + "/", enc, handler, opt, orir);
            } else {
                console.log("reading: " + cpath);
                let key = cpath.split(orir)[1].replace(/^\//, "");
                let value = null;
                switch (handler) {
                    case "readFileSync":
                        value = fs.readFileSync(cpath, { encoding: enc, ...opt });
                        break;
                    case "createReadStream":
                        value = fs.createReadStream(cpath, { encoding: enc, ...opt });
                        break;
                    case "readFile":
                        fs.readFile(cpath, enc, (err, data) => {
                            if (err) {
                                console.error(err);
                                return;
                            }
                            const buf = Buffer.from(data, 'binary');
                            value = buf.buffer;
                        })
                }
                scrobj[key] = value;
            }
        })
    }
    paths.forEach(path => handlePath(path));
}

const resources = {
    "index.js": fs.readFileSync("./website/index.js"),
    "index.html": fs.readFileSync("./website/index.html")
};

loadScripts(resources, "./website/src");
loadScripts(resources, "./website/images", "base64", "readFileSync", { autoClose: false });

/**
 * 
 * @param {Stream} stream 
 * @param {*} data 
 * @returns 
 */
const write = (stream, data, enc) => new Promise((resolve) => {
    stream.write(data, enc) && resolve();
    stream.once('drain', resolve);
});

const server = http.createServer();
server.on('request', async (req, res) => {
    apply(res);
    let url = req.url;
    console.log(url);
    res.statusCode = 200;
    let ext = url.split(".").at(-1);
    if (url === "/") {
        res.mime("html");
        res.write(resources["index.html"]);
        res.end();
        return;
    }
    const cMime = mimes[ext];
    let content = resources[url.substring(1)];
    res.mime(ext);
    if (cMime === undefined || content === undefined) {
        res.statusCode = 404;
        res.write("fucking uhhh");
        res.end();
        return;
    }
    if (content instanceof fs.ReadStream) {
        console.log("readstream");
        content.pipe(res);
    } else {
        await write(res, content, ext == "png" ? "base64" : "utf-8");
        res.end();
    }
})
server.listen(80);