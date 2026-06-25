import mqtt from 'mqtt';

export const MQTT_BROKER = import.meta.env.VITE_MQTT_URL || `ws://${window.location.hostname}:9101`;
const TOPIC_RESULTS = '/smartpong/throws/results';

let client = null;
let connected = false;

const resultListeners = new Set();
const statusListeners = new Set();

const emitStatus = () => {
  statusListeners.forEach((cb) => cb(connected));
};

export const connectMqtt = () => {
  if (client) return client;

  client = mqtt.connect(MQTT_BROKER);

  client.on('connect', () => {
    connected = true;
    emitStatus();
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
        resultListeners.forEach((cb) => cb(payload));
      } catch (e) {
        console.error('Error parsing MQTT message', e);
      }
    }
  });

  client.on('reconnect', () => {
    connected = false;
    emitStatus();
  });

  client.on('close', () => {
    connected = false;
    emitStatus();
  });

  client.on('error', (err) => {
    console.error('MQTT Client Error:', err);
  });

  return client;
};

export const onResult = (callback) => {
  resultListeners.add(callback);
  return () => resultListeners.delete(callback);
};

export const onStatus = (callback) => {
  statusListeners.add(callback);
  callback(connected);
  return () => statusListeners.delete(callback);
};

export const isConnected = () => connected;

export const disconnectMqtt = () => {
  resultListeners.clear();
  statusListeners.clear();
  if (client) {
    client.end();
    client = null;
    connected = false;
  }
};
