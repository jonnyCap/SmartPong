import mqtt from 'mqtt';

const MQTT_BROKER = 'wss://test.mosquitto.org:8081'; // Using WebSocket Secure
const TOPIC_RESULTS = '/smartpong/throws/results';

let client = null;

export const connectMqtt = (onMessageCallback) => {
  client = mqtt.connect(MQTT_BROKER);

  client.on('connect', () => {
    console.log('Connected to MQTT Broker via WebSocket');
    client.subscribe(TOPIC_RESULTS, (err) => {
      if (err) {
        console.error('Failed to subscribe to topic', err);
      } else {
        console.log(`Subscribed to ${TOPIC_RESULTS}`);
      }
    });
  });

  client.on('message', (topic, message) => {
    if (topic === TOPIC_RESULTS) {
      try {
        const payload = JSON.parse(message.toString());
        onMessageCallback(payload);
      } catch (e) {
        console.error('Error parsing MQTT message', e);
      }
    }
  });

  client.on('error', (err) => {
    console.error('MQTT Client Error:', err);
  });
};

export const disconnectMqtt = () => {
  if (client) {
    client.end();
  }
};
