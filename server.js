const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');
const app = express();

app.use(cors());
app.use(express.json());

let lightLogs = [];

const mqttClient = mqtt.connect('mqtt://broker.hivemq.com');

mqttClient.on('connect', () => {

    console.log("MQTT Connected");
    mqttClient.subscribe('home/light/+/status'); 
});

mqttClient.on('message', (topic, message) => {
    
    const msg = message.toString(); 
    
    const newLog = {
        status: msg,
        timestamp: new Date().toLocaleString('th-TH') 
    };

    console.log("ไฟเปลี่ยนสถานะ:", newLog);
    lightLogs.unshift(newLog);
});


app.get('/api/light-logs', (req, res) => {
    res.json(lightLogs);
});

app.listen(3000, () => console.log('Server running on port 3000'));