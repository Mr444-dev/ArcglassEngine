#!/usr/bin/env bash
set -euo pipefail

NAMESPACE="game-servers"

echo "=========================================================="
echo "  Deploying Engine Server Fleet to Kubernetes Cluster     "
echo "=========================================================="

kubectl create namespace "${NAMESPACE}" --dry-run=client -o yaml | kubectl apply -f -
kubectl apply -f deploy/k8s/configmap.yaml
kubectl apply -f deploy/k8s/game-server-deployment.yaml
kubectl apply -f deploy/k8s/game-server-service.yaml
kubectl apply -f deploy/k8s/gameserver-fleet.yaml

echo "Waiting for Game Server Pods to be ready..."
kubectl rollout status deployment/engine-dedicated-server -n "${NAMESPACE}" --timeout=60s

echo "Game Server Fleet successfully deployed!"
kubectl get pods -n "${NAMESPACE}" -l app=engine-dedicated-server
kubectl get svc -n "${NAMESPACE}"
