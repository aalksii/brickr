FROM --platform=linux/amd64 ubuntu:22.04 AS build

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        libboost-graph-dev \
        libglu1-mesa-dev \
        libqt5opengl5-dev \
        libqt5svg5-dev \
        mesa-common-dev \
        qt5-qmake \
        qtbase5-dev \
        qtbase5-dev-tools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

RUN qmake brickr.pro && make -j"$(nproc)"


FROM --platform=linux/amd64 ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libboost-graph1.74.0 \
        libgl1-mesa-glx \
        libgl1-mesa-dri \
        libglu1-mesa \
        libqt5core5a \
        libqt5gui5 \
        libqt5opengl5 \
        libqt5svg5 \
        libqt5widgets5 \
        novnc \
        python3-pip \
        websockify \
        xauth \
        x11vnc \
        xvfb \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir trimesh

WORKDIR /opt/brickr

COPY --from=build /src/brickr /opt/brickr/brickr
COPY --from=build /src/resources /opt/brickr/resources
COPY --from=build /src/models /opt/brickr/models
COPY tools/obj_to_binvox.py /opt/brickr/tools/obj_to_binvox.py
COPY docker/entrypoint-vnc.sh /opt/brickr/entrypoint-vnc.sh

ENV BINVOX_PATH=/opt/brickr/tools/obj_to_binvox.py
RUN chmod +x /opt/brickr/entrypoint-vnc.sh

EXPOSE 5900 6080

CMD ["/opt/brickr/entrypoint-vnc.sh"]
