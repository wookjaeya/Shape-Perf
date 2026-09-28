# Reproducible toolchain + harness image for the measurement VM (spec §5.1, §5.4).
# A container reproduces dependencies; it does not remove host noise (spec §5.3).
#
#   docker build -f env/Dockerfile -t shapeperf:v0.5.1.1 --build-arg NPROC=<n> .
#   docker run --rm -it --cpuset-cpus=<cores> -v $PWD/results:/opt/shapeperf/results shapeperf:v0.5.1.1
#
# Record the resulting image ID in SHAPEPERF_IMAGE_ID for scripts/collect_env.py.
# Do not build this image on the VM while timing runs (spec §5.3).
FROM ubuntu:24.04@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3

ARG DEBIAN_FRONTEND=noninteractive
ARG NPROC=4
ARG LINK_JOBS=2
RUN apt-get update -qq && apt-get install -qq -y --no-install-recommends \
        autoconf automake binutils ca-certificates clang cmake curl git libncurses-dev \
        libtool lld make ninja-build python3 python3-dev python3-venv python3-pip \
        util-linux zlib1g-dev jq \
    && rm -rf /var/lib/apt/lists/* \
    && dpkg -l > /opt/dpkg-list.txt

ENV SHAPEPERF_WORK=/opt/shapeperf-work
WORKDIR /opt/shapeperf
# Toolchain first so harness edits do not invalidate the multi-hour LLVM layer.
COPY env/ env/
RUN NPROC=${NPROC} LINK_JOBS=${LINK_JOBS} env/build_toolchain.sh python protobuf llvm onnx-mlir

COPY . .
ENV PATH=${SHAPEPERF_WORK}/venv/bin:${PATH}
CMD ["bash"]
