//thin server to test clients
//run with "node serveClient.js"

const http = require('http');
const fs = require('fs');

const mimes = {
    js: "text/javascript",
    html: "text/html",
    bin: "application/octet-stream",
    css: "text/css",
    docx: "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    gif: "image/gif",
    ico: "image/x-icon",
    jpg: "image/jpeg",
    json: "application/json",
    pdf: "application/pdf",
    txt: "text/plain",
    xlsx: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    png: "image/png",
}

const apply = (res) => {
    res.mime = (mime) => {
        res.setHeader("Content-Type", mimes[mime] ?? mime);
    }
    res.mime.bind(res);
}

loadScripts = (scrobj, root, orir = root) => {
    console.log("root: " + root);
    let paths = fs.readdirSync(root, { recursive: true, encoding: "utf-8" });
    for (const path of paths) {
        let cpath = root;
        if(cpath[cpath.length - 1] !== "/") cpath += "/";
        cpath += path;
        console.log("  - " + path);
        fs.stat(cpath, (err, stats) => {
            if(err) throw err;
            if(stats.isDirectory()) {
                console.log("dir")
                loadScripts(scrobj, cpath + "/", orir)
            } else {
                console.log("reading: " + cpath);
                let key = cpath.split(orir)[1]
                if(key[0] === "/") key = key.substring(1);
                scrobj[key] = fs.readFileSync(cpath, "utf-8");
            }
        })
    }
}

const resources = {
    scripts: {},
    "index.html" : fs.readFileSync("./website/index.html"),
    "map01.png": fs.readFileSync("./website/images/map01.png")
};

loadScripts(resources.scripts, "./website/src");

http.createServer((req, res) => {
    apply(res);
    let url = req.url;
    console.log(req.socket.remoteAddress);
    console.log(url);
    res.statusCode = 200;
    let ext = url.split(".").at(-1);
    switch(ext) {
        case "js":
            res.mime("js");
            let content = scripts[url.substring(1)] ?? ""
            res.write(content);
            break;
        case "css":
            res.mime("css");
            res.write(fs.readFileSync("index.css"));
            break;
        case ".png":
            res.mime("png");
            res.write(resources["map01.png"]);
        default:
            if(url === "/") {
                res.mime("html");
                res.write(resources["index.html"]);
            }
            else {
                res.statusCode = 404;
                res.write("fucking uhhh");
            }
    }
    res.end();
}).listen(80);