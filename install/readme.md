# Installation

RUHMI Framework[^1] AI MCU compiler includes MERA IPs supported by EdgeCortix, so you will see files and descriptions with the name MERA included.
The version number in the file name (e.g., 2.5.0) corresponds to the MERA IP version.

# Host Environment Setup

The following Dockerfile works as base host environment. Specifically, ensure that dependency
libraries `libdnnl.so`(from `apt install libdnnl-dev`) and `libglog.so` (from `apt install libgoogle-glog-dev`)
are available on the system.

```docker
FROM ubuntu:22.04
RUN ln -sf /usr/share/zoneinfo/Asia/Tokyo /etc/localtime
RUN apt update && apt install -y ca-certificates software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && apt update && apt install -y libstdc++6
RUN apt install -y python3.10-venv libdnnl-dev libgoogle-glog-dev python3-dev
RUN apt install -y cmake build-essential file
RUN apt install -y unzip tree
ENTRYPOINT bash
```

With this, unzip the release package, then `cd` into release directory, and perform the followings:

- Create virtual environment with `python3.10 -m venv host_env`, then activate with `source host_env/bin/activate`
- Install mera package for host architecture (x86): `python -m pip install mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl`
- Install TensorFlow: `python -m pip install tensorflow`
- Install Vela compiler: `python -m pip install ethos-u-vela==4.0.0`

### Board Environment Setup
- Create virtual environment with `python3.12 -m venv board_env`, then activate with `source board_env/bin/activate`
- Install mera package for board architecture (aarch64): `python -m pip install mera-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl`
- Install mera2 runtime package for board architecture (aarch64): `python -m pip install mera2_runtime-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl`

### Compiling Model on Host

- Using the example directory provided in the release, ensure the model file is under `source_model_files` directory and 
  run `python deploy.py`. This generates `deploy_mobilenetv2` directory.
```
example
├── gen_ref_data.py
├── deploy.py
└── source_model_files
    └── mobilenet_v2_1.0_224_INT8.tflite
```
- Run `python gen_ref_data.py` to generate random input and reference output data on host. This produces `input_0.bin` and
  `ref_result_0.bin` under `deploy_mobilenetv2` directory
```
example
├── gen_ref_data.py
├── ...
└── deploy_mobilenet_v2
    ├── ...
    ├── input_0.bin
    └── ref_result_0.bin
```
- Create `mobilenetv2` directory on board with `mkdir mobilenetv2`
- Copy the deploy directory onto the board, for example with scp: `scp -r deploy_mobilenetv2/ 10.0.0.200:~/mobilenetv2`
  - Replace the IP address of board and destination directory as necessary
  \# Placing the installation files for host machine and for the target Linux system.



