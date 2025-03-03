FROM debian:bookworm

ARG DEBIAN_FRONTEND=noninteractive
ARG GCC_ARM_VERSION="12.3.rel1"
ARG ZAP_TOOL_RELEASE="v2024.12.13"
ARG GECKO_SDK_VERSION="v4.4.5"

ARG USERNAME=builder
ARG USER_UID=1000
ARG USER_GID=$USER_UID


RUN \
    apt-get update \
    && apt-get install -y --no-install-recommends \
       bzip2 \
       curl \
       git \
       git-lfs \
       jq \
       libgl1 \
       make \
       default-jre-headless \
       patch \
       python3 \
       unzip \
       xz-utils \
       less \
    && apt-get clean

WORKDIR /tmp
# Install Simplicity Commander (unfortunately no stable URL available, this
# is known to be working with Commander_linux_x86_64_1v15p0b1306.tar.bz).
RUN \
    curl -O https://www.silabs.com/documents/login/software/SimplicityCommander-Linux.zip \
    && unzip -q SimplicityCommander-Linux.zip \
    && tar -C /opt -xjf SimplicityCommander-Linux/Commander_linux_x86_64_*.tar.bz \
    && rm -r SimplicityCommander-Linux \
    && rm SimplicityCommander-Linux.zip

# Install Silicon Labs Configurator (slc)
RUN \
    curl -O https://www.silabs.com/documents/login/software/slc_cli_linux.zip \
    && unzip -d /opt -q slc_cli_linux.zip \
    && chmod go-w /opt/slc_cli \
    && rm slc_cli_linux.zip

# Install ARM GCC embedded toolchain
RUN \
    curl -L -O https://developer.arm.com/-/media/Files/downloads/gnu/${GCC_ARM_VERSION}/binrel/arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-arm-none-eabi.tar.xz \
    && xzcat arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-arm-none-eabi.tar.xz | tar -C /opt -xf -\
    && rm arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-arm-none-eabi.tar.xz

ENV PATH="$PATH:/opt/arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-arm-none-eabi/bin:/opt/slc_cli:/opt/commander"
ENV ARM_GCC_DIR="/opt/arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-arm-none-eabi"

# Install ZAP adapter
RUN \
    curl -OL https://github.com/project-chip/zap/releases/download/${ZAP_TOOL_RELEASE}/zap-linux-x64.zip \
    && umask 022 \
    && unzip -d /opt/zap -q zap-linux-x64.zip \
    && rm zap-linux-x64.zip

ENV STUDIO_ADAPTER_PACK_PATH="/opt/zap:/opt/commander"
ENV POST_BUILD_EXE="/opt/commander/commander"

RUN \
    git clone --depth 1 -b ${GECKO_SDK_VERSION} https://github.com/SiliconLabs/gecko_sdk.git /gecko_sdk \
    && rm -rf /gecko_sdk/.git \
    && chmod 755 /gecko_sdk/protocol/zigbee/tool/image-builder/image-builder-linux

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

USER $USERNAME
WORKDIR /build

RUN \
    slc configuration --sdk="/gecko_sdk/" \
    && slc signature trust --sdk "/gecko_sdk/" \
    && slc configuration \
           --gcc-toolchain="/opt/arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-arm-none-eabi/"
