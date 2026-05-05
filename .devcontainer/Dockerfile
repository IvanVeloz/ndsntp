
ARG VARIANT=dev # may be `dev` or `slim`
ARG VERSION=latest

ARG TARGETARCH # may be `amd64` or `arm64`

FROM docker.io/skylyrac/blocksds:${VARIANT}-${VERSION} AS devcontainer-base

ENV DEBIAN_FRONTEND=noninteractive
LABEL maintainer="IvanVeloz"
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        unzip \
        curl \
        git \
    && rm -rf /var/lib/apt/lists/*
 

FROM devcontainer-base AS devcontainer-melonds
RUN ARCH=$(uname -m) \
    && MELONDS_URL=$(curl -s https://api.github.com/repos/melonDS-emu/melonDS/releases/263129025 \
    | grep "browser_download_url.*appimage-${ARCH}.zip" \
    | cut -d '"' -f 4) \
    && curl -L -o melonDS.zip "$MELONDS_URL" \
    && unzip melonDS.zip \
    && chmod +x melonDS*.AppImage \
    && rm melonDS.zip \
    && mkdir -p /opt/melonDS \
    && mv melonDS*.AppImage /opt/melonDS/melonDS.AppImage \
    && cd /opt/melonDS/ && ./melonDS.AppImage --appimage-extract \
    && ln -s /opt/melonDS/squashfs-root/AppRun /usr/local/bin/melonDS

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libegl1 \
        libasound2t64 \
        libopengl0 \
    && rm -rf /var/lib/apt/lists/*

EXPOSE 3333-3334
EXPOSE 8000

#COPY openbox-rc.xml /etc/xdg/openbox/
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]



