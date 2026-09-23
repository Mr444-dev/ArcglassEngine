#!/usr/bin/env bash
set -euo pipefail

IMAGE_TAG="${1:-latest}"
IMAGE_NAME="mygameengine/dedicated-server:${IMAGE_TAG}"

echo "=========================================================="
echo "  Building Production Dedicated Server Docker Container   "
echo "  Target: ${IMAGE_NAME}                                   "
echo "=========================================================="

docker build -f deploy/docker/Dockerfile.server -t "${IMAGE_NAME}" .

echo "Docker container build completed successfully."
