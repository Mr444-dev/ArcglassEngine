#!/usr/bin/env bash
set -euo pipefail

IMAGE_TAG="${1:-latest}"
IMAGE_NAME="arcglass-engine/dedicated-server:${IMAGE_TAG}"

echo "=========================================================="
echo "  Building Experimental Arcglass server image scaffold   "
echo "  Target: ${IMAGE_NAME}                                   "
echo "=========================================================="

docker build -f deploy/docker/Dockerfile.server -t "${IMAGE_NAME}" .

echo "Docker container build completed successfully."
