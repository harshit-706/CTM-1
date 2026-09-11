const http = require('http');
const WebSocket = require('ws');

const server = http.createServer((req, res) => {
    // Basic CORS
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, PUT, DELETE');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.writeHead(200);
        res.end();
        return;
    }

    if (req.url === '/api/v1/telemetry' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        req.on('end', () => {
            try {
                const payload = JSON.parse(body);
                const msg = JSON.stringify({
                    type: 'telemetry.update',
                    timestamp: payload.timestamp_ns ? payload.timestamp_ns / 1000000 : Date.now(),
                    payload: payload
                });
                wss.clients.forEach(client => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(msg);
                    }
                });
                res.writeHead(200, {'Content-Type': 'application/json'});
                res.end(JSON.stringify({status: 'ok'}));
            } catch(e) {
                res.writeHead(400);
                res.end('Bad Request');
            }
        });
    } else {
        // Proxy all other requests to Simulator at port 8082
        const options = {
            hostname: '127.0.0.1',
            port: 8082,
            path: req.url,
            method: req.method,
            headers: { ...req.headers }
        };
        
        // Remove host header so http.request sets it correctly
        delete options.headers.host;

        const proxyReq = http.request(options, proxyRes => {
            // Forward headers and status
            Object.keys(proxyRes.headers).forEach(key => {
                res.setHeader(key, proxyRes.headers[key]);
            });
            res.writeHead(proxyRes.statusCode);
            proxyRes.pipe(res, { end: true });
        });

        proxyReq.on('error', e => {
            res.writeHead(502);
            res.end('Simulator proxy error: ' + e.message);
        });

        req.pipe(proxyReq, { end: true });
    }
});

const wss = new WebSocket.Server({ server, path: '/live' });

wss.on('connection', ws => {
    console.log('Client connected to Edge Core Mock WS');
    ws.on('close', () => console.log('Client disconnected from WS'));
});

server.listen(8080, () => {
    console.log('Mock Edge Core listening on port 8080 (HTTP Proxy to 8082 + WS)');
});
