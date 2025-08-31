# syntax=docker/dockerfile:1.4

FROM ubuntu:25.04 AS base

SHELL ["/bin/bash", "-lc"]
ARG DEBIAN_FRONTEND=noninteractive

# System dependencies required for building RDKit and Qt offscreen rendering
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      g++ \
      wget \
      curl \
      make \
      git \
      libgl1-mesa-dev \
      mesa-common-dev \
      ca-certificates \
      bzip2 \
    && rm -rf /var/lib/apt/lists/*

# Install miniconda
ARG MINICONDA_VERSION=latest
ENV CONDA=/opt/miniconda3
RUN curl -fsSL https://repo.anaconda.com/miniconda/Miniconda3-${MINICONDA_VERSION}-Linux-x86_64.sh -o miniconda.sh && \
    bash miniconda.sh -b -p ${CONDA} && \
    rm miniconda.sh

# Add conda to PATH
ENV PATH=${CONDA}/bin:${PATH}

RUN conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/main
RUN conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/r

# Configure conda with ToS acceptance and libmamba resolver
ARG BOOST_VERSION=1.82
ARG PYTHON_VERSION=3.11
RUN conda config --set always_yes yes --set changeps1 no && \
    conda config --set auto_activate_base false && \
    conda update -q conda && \
    conda config --set channel_priority strict && \
    conda config --add channels conda-forge && \
    conda install -n base conda-libmamba-solver && \
    conda config --set solver libmamba && \
    conda info -a && \
    conda create --name rdkit_build -c conda-forge --override-channels \
        "python=${PYTHON_VERSION}" cmake \
        "boost-cpp=${BOOST_VERSION}" \
        "boost=${BOOST_VERSION}" \
        numpy pillow eigen pandas matplotlib-base \
        cairo && \
    conda run -n rdkit_build conda config --env --add channels conda-forge && \
    conda run -n rdkit_build conda config --env --set channel_priority strict && \
    conda run -n rdkit_build conda install -c conda-forge --override-channels \
        pytest nbval "ipykernel>=6.0"

FROM base AS builder
SHELL ["/bin/bash", "-lc"]
ENV CONDA=/opt/miniconda3
ENV QT_QPA_PLATFORM=offscreen

# Tunables
ARG CORES=4
ARG PYTHON_NAME=python311
ARG WERROR=1
ARG RDKIT_GIT_URL=https://github.com/rdkit/rdkit.git
ARG RDKIT_GIT_REF=Release_2024_03_1
ARG MAKE_VERBOSE=0
ARG CMAKE_VERBOSE=0

WORKDIR /src
RUN git clone --depth 1 --branch "${RDKIT_GIT_REF}" "${RDKIT_GIT_URL}" rdkit

# Configure CMake
WORKDIR /src/rdkit
RUN source ${CONDA}/etc/profile.d/conda.sh && conda activate rdkit_build && \
    export CXXFLAGS="${CXXFLAGS} -Wall -D_GLIBCXX_USE_CXX11_ABI=1" && \
    if [ "${WERROR}" = "1" ]; then export CXXFLAGS="${CXXFLAGS} -Werror"; fi && \
    mkdir -p build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_STANDARD=17 \
      -DCMAKE_CXX_STANDARD_REQUIRED=ON \
      -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE} \
      -DCMAKE_INSTALL_PREFIX=/rdkit_install \
      -DRDK_INSTALL_INTREE=OFF \
      -DRDK_INSTALL_STATIC_LIBS=OFF \
      -DRDK_BUILD_CPP_TESTS=ON \
      -DRDK_BUILD_PYTHON_WRAPPERS=OFF \
      -DRDK_BUILD_COORDGEN_SUPPORT=ON \
      -DRDK_BUILD_MAEPARSER_SUPPORT=ON \
      -DRDK_OPTIMIZE_POPCNT=ON \
      -DRDK_BUILD_TEST_GZIP=ON \
      -DRDK_BUILD_FREESASA_SUPPORT=ON \
      -DRDK_BUILD_AVALON_SUPPORT=ON \
      -DRDK_BUILD_INCHI_SUPPORT=ON \
      -DRDK_BUILD_YAEHMOP_SUPPORT=ON \
      -DRDK_BUILD_XYZ2MOL_SUPPORT=ON \
      -DRDK_BUILD_CAIRO_SUPPORT=ON \
      -DRDK_BUILD_QT_SUPPORT=OFF \
      -DRDK_BUILD_SWIG_WRAPPERS=OFF \
      -DRDK_SWIG_STATIC=OFF \
      -DRDK_BUILD_THREADSAFE_SSS=ON \
      -DRDK_TEST_MULTITHREADED=ON \
      -DRDK_BUILD_CFFI_LIB=ON \
      -DBOOST_ROOT="${CONDA_PREFIX}" \
      -DBoost_NO_SYSTEM_PATHS=ON \
      -DRDK_BOOST_PYTHON3_NAME="${PYTHON_NAME}" \
      -DPYTHON_EXECUTABLE="${CONDA_PREFIX}/bin/python3" \
      -DCMAKE_INCLUDE_PATH="${CONDA_PREFIX}/include" \
      -DCMAKE_LIBRARY_PATH="${CONDA_PREFIX}/lib"

# Build and install
RUN source ${CONDA}/etc/profile.d/conda.sh && conda activate rdkit_build && \
    cd /src/rdkit/build && \
    make -j "${CORES}" install VERBOSE=${MAKE_VERBOSE}

# Optionally run unit tests
ARG RUN_TESTS=0
RUN if [ "${RUN_TESTS}" = "1" ]; then \
      source ${CONDA}/etc/profile.d/conda.sh && conda activate rdkit_build && \
      export RDBASE=/src/rdkit && \
      export PYTHONPATH=${RDBASE}:${PYTHONPATH} && \
      export LD_LIBRARY_PATH=${RDBASE}/lib:${CONDA_PREFIX}/lib:${LD_LIBRARY_PATH} && \
      cd /src/rdkit/build && \
      ctest -j "${CORES}" --output-on-failure -T Test ; \
    fi

# Optionally run docs doctest
ARG RUN_DOCS=0
RUN if [ "${RUN_DOCS}" = "1" ]; then \
      source ${CONDA}/etc/profile.d/conda.sh && conda activate rdkit_build && \
      conda install -c conda-forge --override-channels ipython=8.12 sphinx myst-parser && \
      export RDBASE=/src/rdkit && \
      export PYTHONPATH=${RDBASE}:${PYTHONPATH} && \
      export LD_LIBRARY_PATH=${RDBASE}/lib:${LD_LIBRARY_PATH} && \
      cd /src/rdkit/Docs/Book && \
      make doctest ; \
    fi

FROM alpine:latest AS exporter
# Export the install directory content with headers and libraries
COPY --from=builder /rdkit_install/ /rdkit_build/
# Add a dummy command for docker create to work
CMD ["/bin/sh"]


