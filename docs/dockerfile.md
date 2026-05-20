# `Dockerfile` Guide

## Overview
`scripts/Dockerfile` builds an Ubuntu 22.04-based environment for compiling TFLite models with RUHMI (MERA) and running `generate-model-data.py`.

The image includes:
- Build tools (`build-essential`, `cmake`, etc.)
- Python 3.10 + pip
- MERA x86 wheel (`mera-2.5.0+pkg.3782`)
- TensorFlow 2.17.0
- Ethos-U Vela 4.0.0
- LiteRT (`ai-edge-litert==2.1.2`)
- `/generate-model-data.py`

## Main Build Steps
1. Start from `ubuntu:22.04`
1. Install core development and utility packages via `apt`
1. Add PPA and install MERA dependencies plus Python 3.10 packages
1. Download and install MERA x86 wheel from GitHub
1. Upgrade `pip`, then install TensorFlow, Vela, and LiteRT
1. Copy `generate-model-data.py` into the container root (`/generate-model-data.py`)
1. Create a non-root user matching host UID/GID and switch to that user

## Build Arguments
You can pass the following args to `docker build`:
- `UID` (default: `1000`)
- `GID` (default: `1000`)
- `USERNAME` (default: `user`)
- `GROUPNAME` (default: `user`)

Purpose:
This helps avoid file permission issues when mounting host directories into the container.

## Build Example
```bash
docker build \
  --build-arg UID=$(id -u) \
  --build-arg GID=$(id -g) \
  -t ruhmi-env \
  -f scripts/Dockerfile \
  scripts
```

## Run Example
```bash
docker run --rm -it \
  -v /path/to/work:/shared \
  -w /shared \
  ruhmi-env
```

Run model data generation after container startup:
```bash
python3 /generate-model-data.py -d models_to_deploy -m model.tflite
```

## Notes
- The MERA wheel URL is pinned to a specific GitHub commit. If the URL becomes invalid, update it.
- The Dockerfile uses `adduser --gid ${UID}`. In many setups, `--gid ${GID}` is more typical (if `UID == GID`, this often still works).
- `generate-model-data.py` must exist in the build context (for this Dockerfile, the `scripts` directory).

