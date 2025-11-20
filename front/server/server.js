const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const mqtt = require("mqtt");
const path = require("path");

const MQTT_HOST = "mqtt://44.223.43.74:1883"; 
const MQTT_TOPIC = "/TEF/posture001/attrs/jsonObject";

const app = express();
const server = http.createServer(app);

app.use(express.static(path.join(__dirname, "../public")));

const wss = new WebSocket.Server({ server });

wss.on("connection", ws => {
    console.log("Cliente conectado ao WebSocket!");
    ws.send(JSON.stringify({ backDistance: 0, sideDistance: 0, seatDistance: 0, postureStatus: "", goodTime: 0, badTime: 0, sessionTime: 0, postureScore: 0 }));
});

const client = mqtt.connect(MQTT_HOST);

client.on("connect", () => {
    console.log("Conectado ao MQTT!");
    client.subscribe(MQTT_TOPIC, err => {
        if (!err) console.log(`Inscrito no tópico: ${MQTT_TOPIC}`);
    });
});

client.on("message", (topic, message) => {
    const msg = message.toString();
    console.log("Mensagem MQTT:", msg);

    wss.clients.forEach(ws => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(msg);
        }
    });
});

const PORT = 3000;
server.listen(PORT, () => {
    console.log(`Servidor rodando em http://localhost:${PORT}`);
});