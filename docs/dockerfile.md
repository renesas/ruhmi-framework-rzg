# Dockerfile Guide

## Overview
`scripts/Dockerfile` builds an Ubuntu 22.04-based environment for compiling TFLite models with RUHMI AI model compiler and running `generate-model-data.py` which is the script to compile the target model.

## Main Build Steps
1. Start from `ubuntu:22.04`
2. Install core development and utility packages via `apt`
3. Add PPA and install MERA dependencies plus Python 3.10 packages
4. Download and install MERA x86 wheel from GitHub
5. Upgrade `pip`, then install TensorFlow, Vela, and LiteRT
6. Copy `generate-model-data.py` into the container root (`/generate-model-data.py`)
7. Create a non-root user matching host UID/GID and switch to that user

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
cd ~/ruhmi-framework-rzg/scripts/
sudo docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t ruhmi-env
sudo docker run --rm -it -v /path/to/shared:/shared -w /shared ruhmi-env
```

> [NOTE]  
>Update */path/to/shared* path to the directory used for storing models.

The console should appear with user user and a different hostname as shown below.
```
user@5c0832859d04:/shared$
```

> [NOTE]  
> - The MERA wheel URL is pinned to a specific GitHub commit. If the URL becomes invalid, update it.
> - The Dockerfile uses `adduser --gid ${UID}`. In many setups, `--gid ${GID}` is more typical (if `UID == GID`, this often still works).
- `generate-model-data.py` must exist in the build context (for this Dockerfile, the `scripts` directory).


