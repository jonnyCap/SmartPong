# SmartPong
An IoT wearable and data analytics platform dedicated to objective, in-depth throw analysis. Features on-device and serverside processing to capture, reconstruct, and evaluate throwing biomechanics.

## Project Structure

- `firmware/`: ESP32 source code. Deployed to the M5 watch. It handles data capture and on-device machine learning for rapid throw detection.
- `classification/`: Jupyter notebooks detailing the data science pipeline (feature extraction, decision tree generation, and metric exploration).
- `backend/`: FastAPI application that acts as the server-side processor. It receives detected throws via MQTT, computes detailed metrics (acceleration, drift, trajectory), and stores personalized user profiles. See `backend/README.md` for running instructions.
