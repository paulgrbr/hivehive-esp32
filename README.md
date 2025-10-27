# HiveHive (ESP-32 Cam)

| Service         | Status                                                                                                                        |
| --------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| **Backend-API** | ![Build Status](https://img.shields.io/github/actions/workflow/status/paulgrbr/hivehive-test/build-and-push-backend-api.yaml) |

## 🧑‍💻 Project Overview

This project was developed as part of the course “Selected Topics in Data Science and AI in Business” at **DHBW Ravensburg**.
It focuses on building a low-cost vision system using an ESP32-CAM, which captures images and sends them to a server running a segmentation model.
The model detects and classifies simple geometric shapes (black circles) for use in quality assurance and component sorting applications.

## 🧩 Deployment Setup

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
