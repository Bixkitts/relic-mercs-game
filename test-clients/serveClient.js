//thin server to test clients
//run with "node serveClient.js"

const http = require('http');
const fs = require('fs');

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
    for (const path of paths) {
        let cpath = (root + "/" + path).replace(/\/\//, "/");
        console.log("  - " + path);
        fs.stat(cpath, (err, stats) => {
            if(err) throw err;
            if(stats.isDirectory()) {
                console.log("dir")
                loadScripts(scrobj, cpath + "/", enc, orir)
            } else {
                console.log("reading: " + cpath);
                let key = cpath.split(orir)[1].replace(/^\//, "");
                scrobj[key] = fs[handler](cpath, enc);
            }
        })
    }
}

const resources = {
    "index.js" : fs.readFileSync("./website/index.js"),
    "index.html" : fs.readFileSync("./website/index.html")
    //"map01.png": fs.readFileSync("./website/images/map01.png")
};

loadScripts(resources, "./website/src");
loadScripts(resources, "./website/images", { autoClose: false }, "createReadStream");

http.createServer((req, res) => {
    apply(res);
    let url = req.url;
    console.log(url);
    res.statusCode = 200;
    let ext = url.split(".").at(-1);
    if(url === "/") {
        res.mime("html");
        res.write(resources["index.html"]);
        res.end(); 
        return;
    }
    const cMime = mimes[ext];
    let content = resources[url.substring(1)];
    if(cMime === undefined || content === undefined) {
        res.statusCode = 404;
        res.write("fucking uhhh");
        res.end();
        return;
    }
    res.mime(ext);
    if(content instanceof fs.ReadStream) {
        console.log("readstream");
        content.pipe(res);
    } else {
        if(!res.write(content, (err) => {
            if(err) console.log(err);
            res.end();
        })) {
            console.error("buffer write failure");
        }
    }
}).listen(80);