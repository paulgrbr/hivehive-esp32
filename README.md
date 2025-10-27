# HiveHive (ESP-32 Cam)

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
