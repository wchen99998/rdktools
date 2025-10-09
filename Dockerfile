# syntax=docker/dockerfile:1.7

FROM python:3.12-slim AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        git \
        curl \
        ca-certificates \
        pkg-config \
        bzip2 \
        tar \
        unzip \
    && rm -rf /var/lib/apt/lists/*

ENV PIP_NO_CACHE_DIR=1
WORKDIR /workspace

COPY pyproject.toml README.md CMakeLists.txt ./
COPY src ./src
COPY tests ./tests
COPY docs ./docs
COPY External ./External

RUN python -m pip install --upgrade pip
RUN python -m pip install build pytest

# Build the wheel once (this vendors Boost and RDKit)
RUN python -m build --wheel --outdir /tmp/wheels

# Install the freshly built wheel and run the test suite
RUN python -m pip install /tmp/wheels/*.whl
RUN python -m pytest

FROM python:3.12-slim

WORKDIR /opt/rdktools
ENV PIP_NO_CACHE_DIR=1

COPY --from=builder /tmp/wheels /tmp/wheels
RUN python -m pip install --upgrade pip && \
    python -m pip install /tmp/wheels/*.whl && \
    rm -rf /tmp/wheels

CMD ["python"]
