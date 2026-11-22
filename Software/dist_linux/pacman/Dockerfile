ARG OS=archlinux
ARG RELEASE=latest
FROM $OS:$RELEASE
RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    base-devel \
    fftw \
    gcc \
    git \
    hicolor-icon-theme \
    libpulse \
    libusb \
    libx11 \
    libxext \
    lsb-release \
    namcap \
    qt6-base \
    qt6-serialport \
    qt6-tools \
    && (yes | pacman -Scc) \
    && rm -rf /etc/localtime

VOLUME ["/Lightpack", "/build.sh", "/etc/localtime"]

ENTRYPOINT /build.sh
