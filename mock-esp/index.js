const http = require('http');
const fs = require('fs');
const path = require('path');

const hostname = '0.0.0.0';
const port = 8080;
let temp = 25;
let setTemp = 40;
let state = "Idle";

const server = http.createServer((req, res) => {
    console.log('Request for ' + req.url + ' by method ' + req.method);

    if (req.method === 'GET' && req.url === '/api/temp') {
        // Handle GET requests to /api_get_temp
        res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
        console.log('Sending data:', setTemp);
        let json = JSON.stringify({
            "temp": temp,
            "setTemp": setTemp,
            "state": state
        })
        temp = temp + 1;
        res.end(json);
    } else if (req.method == 'GET') {
        const urlObj = new URL(req.url, `http://${req.headers.host}`);
        fileUrl = path.basename(urlObj.pathname);
        search = urlObj.search;

        console.log('Filename:', fileUrl);
        console.log('Arguments:', search);

        if (fileUrl == '/' || fileUrl == '')
            fileUrl = '/index.html';

        var filePath = path.resolve('./public/' + fileUrl);
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
    } else if (req.method === 'POST' && req.url === '/api/temp') {
        // Handle POST requests to /api_set_temp
        let requestBody = '';

        // Accumulate the request body data
        req.on('data', (chunk) => {
            requestBody += chunk.toString();
        });

        // Process the request body when complete
        req.on('end', () => {
            try {
                // Parse the request body as JSON
                const requestData = JSON.parse(requestBody);
                // Assuming requestData contains { "temperature": 20 }
                const temperature = requestData.temperature;

                // Here you can process the temperature value as needed
                // For now, let's log it
                console.log('Received temperature:', temperature);
                setTemp = temperature;
                if (temp < setTemp) {
                    state = "Heating";
                } else if (temp > setTemp) {
                    state = "Cooling";
                } else {
                    state = "Idle";
                }
                // Send a response indicating successful processing
                res.statusCode = 200;
                res.setHeader('Content-Type', 'application/json');
                res.end(JSON.stringify({
                    message: 'Temperature set successfully'
                }));
            } catch (error) {
                // If JSON parsing fails or any other error occurs, handle it
                console.error('Error processing request:', error);
                res.statusCode = 400; // Bad Request
                res.setHeader('Content-Type', 'application/json');
                res.end(JSON.stringify({
                    error: 'Invalid JSON or request format'
                }));
            }
        });
    } else {
        filePath = path.resolve('./public/404.html');
        res.statusCode = 404;
        res.setHeader('Content-Type', 'text/html');
    }
});

server.on('error', (error) => {
    console.error('Server error:', error);
});


server.listen(port, hostname, () => {
    console.log(`Server running at http://${hostname}:${port}/`);
});