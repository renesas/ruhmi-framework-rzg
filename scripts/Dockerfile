FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN \
	apt update && \
	apt install -y \
		build-essential \
		ca-certificates \
		cmake \
		file \
		software-properties-common \
		sudo \
		tree \
		unzip \
		wget

# Install Python and MERA dependencies
RUN \
	add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
	apt update && \
	apt install -y \
		libdnnl-dev \
		libgoogle-glog-dev \
		libstdc++6 \
		python3.10 \
		python3.10-dev \
		python3.10-venv \
		python3-pip

# Download and install MERA x86 package
RUN \
	wget https://github.com/renesas/ruhmi-framework-rzg/raw/62295b271f4b0b90ddd676f799f00d7ad4a7adf6/install/mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl && \
	python3 -m pip install mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl && \
	rm mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl

# Upgrade pip to the latest version
RUN python3 -m pip install --upgrade pip

# Install TensorFlow and Vela
RUN python3 -m pip install tensorflow==2.17.0 ethos-u-vela==4.0.0

# Install LiteRT
RUN python3 -m pip install ai-edge-litert==2.1.2

# COPY --chmod=755 ./generate-model-data.py /generate-model-data.py
COPY ./generate-model-data.py /generate-model-data.py
RUN chmod 755 /generate-model-data.py

# Create a user with the UID/GID passed by the host
# so that the host can access to files created by the
# user in the container.
ARG UID=1000
ARG GID=1000
ARG USERNAME=user
ARG GROUPNAME=user

RUN addgroup --gid ${GID} ${GROUPNAME}
RUN adduser --uid ${UID} --gid ${UID} --disabled-password --gecos "" ${USERNAME}
RUN echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/${USERNAME}

USER ${USERNAME}

SHELL ["/bin/bash", "-c"]
