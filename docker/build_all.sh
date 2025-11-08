#!/usr/bin/env bash

docker buildx build --load --platform=armhf -t plutosdr-trixie-armhf -f trixie.Dockerfile .
docker buildx build --load --platform=arm64 -t plutosdr-trixie-arm64 -f trixie.Dockerfile .
