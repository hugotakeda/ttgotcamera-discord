const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let logs = [];

// Middleware para parsing de JSON
app.use(express.json());

// Servir o arquivo HTML
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html')); // Certifique-se de que index.html está no mesmo diretório
});

// Endpoint para receber logs do ESP32
app.post('/log', (req, res) => {
  console.log('Recebido POST /log:', req.body);
  const { timestamp, message } = req.body;
  if (timestamp && message) {
    const log = { timestamp, message };
    logs.push(log);
    console.log('Log armazenado:', log);
    wss.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(JSON.stringify(log));
        console.log('Log enviado via WebSocket:', log);
      }
    });
    res.status(200).send('Log recebido');
  } else {
    console.log('Dados inválidos recebidos:', req.body);
    res.status(400).send('Dados inválidos');
  }
});

// Configurar WebSocket
wss.on('connection', ws => {
  console.log('Novo cliente conectado');
  // Enviar logs existentes para o novo cliente
  logs.forEach(log => ws.send(JSON.stringify(log)));
});

server.listen(3000, () => {
  console.log('Servidor rodando na porta 3000');
});