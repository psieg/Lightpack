ARG OS=ubuntu
ARG RELEASE=24.04
FROM $OS:$RELEASE
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt dist-upgrade -y && apt install -y \
    bash \
    build-essential \
    fakeroot \
    gcc \
    git \
    hashdeep \
    libfftw3-dev \
    libgl-dev \
    libpulse-dev \
    libqt6serialport6-dev \
    libudev-dev \
    libusb-1.0-0-dev \
    libx11-dev \
    libxext-dev \
    lsb-release \
    pkg-config \
    qmake6 \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-l10n-tools \
    qt6-tools-dev-tools \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /etc/localtime

VOLUME ["/Lightpack", "/build.sh", "/etc/localtime"]

ENTRYPOINT /build.sh
