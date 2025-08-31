#!/usr/bin/env bash
set -euo pipefail

_usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Build RDKit in Docker and copy artifacts into ./build

Options:
  --cores N               Number of make/ctest cores (default: host nproc)
  --python-version V      Python version for conda env (default: 3.11)
  --boost-version V       Boost version (default: 1.82)
  --python-name NAME      RDK_BOOST_PYTHON3_NAME (default: python311)
  --git-ref REF           RDKit git tag/branch (default: Release_2024_03_1)
  --git-url URL           RDKit git repo URL (default: https://github.com/rdkit/rdkit.git)
  --run-tests             Run ctest inside build container
  --run-docs              Run docs doctest inside build container
  --werror                Treat warnings as errors during compile
  --no-werror             Do not treat warnings as errors
  --make-verbose          Set make VERBOSE=1
  --cmake-verbose         Set -DCMAKE_VERBOSE_MAKEFILE=ON
  --no-cache              Build Docker image without cache
  --image-tag TAG         Tag for the build image (default: rdkit-builder:local)
  -h, --help              Show this help
EOF
}

CORES=$(nproc)
PYTHON_VERSION=3.12
BOOST_VERSION=1.84
PYTHON_NAME=python312
RDKIT_GIT_REF=Release_2025_03_5
RDKIT_GIT_URL=https://github.com/rdkit/rdkit.git
RUN_TESTS=1
RUN_DOCS=0
WERROR=0
NO_CACHE=
IMAGE_TAG=rdkit-builder:local
MAKE_VERBOSE=0
CMAKE_VERBOSE=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --cores) CORES=$2; shift 2;;
    --python-version) PYTHON_VERSION=$2; shift 2;;
    --boost-version) BOOST_VERSION=$2; shift 2;;
    --python-name) PYTHON_NAME=$2; shift 2;;
    --git-ref) RDKIT_GIT_REF=$2; shift 2;;
    --git-url) RDKIT_GIT_URL=$2; shift 2;;
    --run-tests) RUN_TESTS=1; shift;;
    --run-docs) RUN_DOCS=1; shift;;
    --werror) WERROR=1; shift;;
    --no-werror) WERROR=0; shift;;
    --make-verbose) MAKE_VERBOSE=1; shift;;
    --cmake-verbose) CMAKE_VERBOSE=1; shift;;
    --no-cache) NO_CACHE=--no-cache; shift;;
    --image-tag) IMAGE_TAG=$2; shift 2;;
    -h|--help) _usage; exit 0;;
    *) echo "Unknown option: $1"; _usage; exit 1;;
  esac
done

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_OUT_DIR="$ROOT_DIR/build/rdkit"
mkdir -p "$BUILD_OUT_DIR"

echo "Building Docker image: $IMAGE_TAG"
DOCKER_BUILDKIT=1 docker build $NO_CACHE \
  --build-arg PYTHON_VERSION="$PYTHON_VERSION" \
  --build-arg BOOST_VERSION="$BOOST_VERSION" \
  --build-arg CORES="$CORES" \
  --build-arg PYTHON_NAME="$PYTHON_NAME" \
  --build-arg RDKIT_GIT_REF="$RDKIT_GIT_REF" \
  --build-arg RDKIT_GIT_URL="$RDKIT_GIT_URL" \
  --build-arg RUN_TESTS="$RUN_TESTS" \
  --build-arg RUN_DOCS="$RUN_DOCS" \
  --build-arg WERROR="$WERROR" \
  --build-arg MAKE_VERBOSE="$MAKE_VERBOSE" \
  --build-arg CMAKE_VERBOSE="$CMAKE_VERBOSE" \
  -t "$IMAGE_TAG" \
  -f "$ROOT_DIR/Dockerfile" \
  "$ROOT_DIR"

echo "Exporting artifacts to $BUILD_OUT_DIR"
rm -rf "$BUILD_OUT_DIR"/*
mkdir -p "$BUILD_OUT_DIR"

# Use docker create + cp to extract from the builder stage through exporter image
CID=$(docker create "$IMAGE_TAG")
trap 'docker rm -f "$CID" >/dev/null 2>&1 || true' EXIT
docker cp "$CID":/rdkit_build/. "$BUILD_OUT_DIR"
echo "Artifacts copied to $BUILD_OUT_DIR"
