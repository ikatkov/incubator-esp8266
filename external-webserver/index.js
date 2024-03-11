const http = require('http');
const fs = require('fs');
const path = require('path');

const hostname = 'localhost';
const port = 3000;

const server = http.createServer((req, res) => {
    console.log('Request for ' + req.url + ' by method ' + req.method);

    if (req.method == 'GET') {
        const urlObj = new URL(req.url, `http://${req.headers.host}`);
        fileUrl = path.basename(urlObj.pathname);
        search = urlObj.search;

        console.log('Filename:', fileUrl);
        console.log('Arguments:', search);

        if (fileUrl == '/' || fileUrl == '')
            fileUrl = 'index.html';

        var filePath = path.resolve('./public/' + fileUrl);
        if(!fs.existsSync(filePath)){
            filePath = path.resolve('external-webserver/public/' + fileUrl);
        }
        const fileExt = path.extname(filePath);
        if (fileExt == '.html') {
            fs.exists(filePath, (exists) => {
                if (!exists) {
                    filePath = path.resolve('./public/404.html');
                    res.statusCode = 404;
                    res.setHeader('Content-Type', 'text/html');
                    return;
                }
                res.statusCode = 200;
                res.setHeader('Content-Type', 'text/html');
                fs.createReadStream(filePath).pipe(res);
            });
        } else if (fileExt == '.css') {
            res.statusCode = 200;
            res.setHeader('Content-Type', 'text/css');
            fs.createReadStream(filePath).pipe(res);
        } else if (fileExt == '.js') {
            res.statusCode = 200;
            res.setHeader('Content-Type', 'text/javascript');
            fs.createReadStream(filePath).pipe(res);
        } else {
            filePath = path.resolve('./public/404.html');
            res.statusCode = 404;
            res.setHeader('Content-Type', 'text/html');
        }
    } else {
        filePath = path.resolve('./public/404.html');
        res.statusCode = 404;
        res.setHeader('Content-Type', 'text/html');
    }
});


server.listen(port, hostname, () => {
    console.log(`Server running at http://${hostname}:${port}/`);
});