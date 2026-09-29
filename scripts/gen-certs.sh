#!/usr/bin/env bash
# Usage: scripts/gen-certs.sh <server-ip>
set -euo pipefail

SERVER_IP="${1:?Usage: $0 <server-ip>}"
OUT_DIR="certs"
CA_DAYS=3650
SERVER_DAYS=825

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"

if [ -e ca.key ]; then
    echo "$OUT_DIR/ already contains a CA. Remove it first to create a new one." >&2
    exit 1
fi

openssl req -x509 -newkey rsa:2048 -nodes -days "$CA_DAYS" \
    -keyout ca.key -out ca.crt -subj "/CN=iot25-ca"

openssl req -newkey rsa:2048 -nodes \
    -keyout server.key -out server.csr -subj "/CN=$SERVER_IP"

printf "subjectAltName=IP:%s\n" "$SERVER_IP" > server.ext
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$SERVER_DAYS" -extfile server.ext -out server.crt

rm server.csr server.ext
chmod 600 ca.key server.key

openssl x509 -in server.crt -noout -subject -issuer -dates -ext subjectAltName
