# Installation

This document explains how to set up the RUHMI AI compiler workflow for Renesas RZ/G3E.

The compiler stack is powered by EdgeCortix MERA, and wheel file versions in this repository follow the MERA version naming.

## Prerequisites

- Ubuntu 22.04 (native Linux or WSL)
- Python 3.10 for host-side model compilation
- Python 3.12 for board-side runtime environment
- Required shared libraries:
  - `libdnnl.so` (package: `libdnnl-dev`)
  - `libglog.so` (package: `libgoogle-glog-dev`)

## Host Environment Setup

Use the following Dockerfile as a baseline host environment:

```dockerfile
FROM ubuntu:22.04
RUN ln -sf /usr/share/zoneinfo/Asia/Tokyo /etc/localtime
RUN apt update && apt install -y ca-certificates software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && apt update && apt install -y libstdc++6
RUN apt install -y python3.10-venv libdnnl-dev libgoogle-glog-dev python3-dev
RUN apt install -y cmake build-essential file
RUN apt install -y unzip tree
ENTRYPOINT bash
```

After extracting the release package, move to the release directory and run:

```bash
python3.10 -m venv host_env
source host_env/bin/activate
python -m pip install --upgrade pip
python -m pip install mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl
python -m pip install tensorflow
python -m pip install ethos-u-vela==4.0.0
```

## Board Environment Setup

On the target board:

```bash
python3.12 -m venv board_env
source board_env/bin/activate
python -m pip install --upgrade pip
python -m pip install mera-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl
python -m pip install mera2_runtime-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl
```

## Compile a Model on Host

Use the scripts in `scripts/` with a model file placed in `source_model_files/`.

Example structure:

```text
example/
  deploy.py
  gen_ref_data.py
  source_model_files/
    mobilenet_v2_1.0_224_INT8.tflite
```

Run deployment and reference-data generation:

```bash
python deploy.py
python gen_ref_data.py
```

Expected outputs:

```text
deploy_mobilenetv2/
  input_0.bin
  ref_result_0.bin
  ...
```

## Transfer Artifacts to Board

```bash
mkdir -p ~/mobilenetv2
scp -r deploy_mobilenetv2/ <BOARD_USER>@<BOARD_IP>:~/mobilenetv2/
```

Replace `<BOARD_USER>` and `<BOARD_IP>` with your board settings.
