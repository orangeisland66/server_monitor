const express = require('express');
const { createProxyMiddleware } = require('http-proxy-middleware');
const path = require('path');

const app = express();
const PORT = 23334;
const API_URL = 'http://localhost:8080';

// Proxy API requests to the backend
app.use('/api', createProxyMiddleware({
    target: API_URL,
    changeOrigin: true,
}));

// Serve static files from the 'dist' directory
app.use(express.static(path.join(__dirname, 'dist')));

// Handle SPA routing: return index.html for any unknown route
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'dist', 'index.html'));
});

app.listen(PORT, '0.0.0.0', () => {
    console.log(`Frontend server running on http://0.0.0.0:${PORT}`);
    console.log(`Proxying API requests to ${API_URL}`);
});
