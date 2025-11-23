# HiveHive (ESP-32 Cam)

| Service         | Status                                                                                                                        |
| --------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| **Backend-API** | ![Build Status](https://img.shields.io/github/actions/workflow/status/paulgrbr/hivehive-test/build-and-push-backend-api.yaml) |
| **ESP32-Cam Firmware** | ![GitHub Release](https://img.shields.io/github/v/release/paulgrbr/hivehive-esp32?include_prereleases&display_name=tag&label=Firmware)
 


## 🧑‍💻 Project Overview

This project was developed as part of the course “Selected Topics in Data Science and AI in Business” at **DHBW Ravensburg**.
It focuses on building a low-cost vision system using an ESP32-CAM, which captures images and sends them to a server running a segmentation model.
The model detects and classifies simple geometric shapes (black circles) for use in quality assurance and component sorting applications.

## 🧩 ESP32-Cam Deployment


Flashing the Firmware to a ESP32-CAM board using a web-flasher. Compatible boards: 
- ESP32-CAM (AI Thinker)

### 📷 Steps:

1.	Connect the ESP32-CAM via USB to a PC 
2. Open in browser: https://esptool.spacehuhn.com/
3.	Click **Connect**, select the USB/Serial device.

4.	Click Erase Flash (optional but recommended).

5. Download latest Firmware release here:
[Download Latest ESP32-Cam Release](https://github.com/paulgrbr/hivehive-esp32/releases/latest)

6.	Click **Program** and select the firmware file from latest release

7.	After flashing, disconnect and **reset/restart** the ESP.

## 🧩 Server Deployment

### 🚀 Production Setup (use published image)

Start containers using the published image:

```bash
docker compose up -d
```

➡️ Uses the prebuilt image paulgrbr/hivehive-be:latest from Docker Hub.

### 🔧 Alternative: Local Development (build locally)

Build image locally and start the dev environment:

```bash
docker compose -f docker-compose-dev.yml up --build
```

➡️ Builds the backend image from backend-api/Dockerfile and runs the container.

### 🎛️ Environment Variables (Optional)

The backend uses an AWS-compatible S3 storage endpoint to store all raw incoming images.
These images can be used for training, validation, and dataset management.

➡️ If left out, the backend will skip this step.
Configure these values in a .env file or via Docker Compose.
A .env.template file is provided. 

#### 1. AWS S3 / S3-Compatible Storage

```bash
AWS_ENDPOINT="https://[your-endpoint-here]"

# Access credentials
AWS_ACCESS_KEY_ID="[your-access-key-id-here]"
AWS_SECRET_ACCESS_KEY="[your-secret-access-key-here]"
```
