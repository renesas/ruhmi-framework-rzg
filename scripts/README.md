# Directory Guide

## Overview
This directory contains tooling and documentation for preparing RUHMI (MERA)
model deployment data for RZ/G3E Ethos-U workflows.

## Directory Structure
```text
scripts/
  Dockerfile
  generate-model-data.py
  README.md
```

## File Descriptions
- `Dockerfile`
  - Builds a Docker image with Ubuntu 22.04, Python 3.10, MERA, TensorFlow,
    Ethos-U Vela, and LiteRT.
  - Intended for model compilation and test-data generation in a reproducible environment.

- `../docs/dockerfile.md`
  - Detailed guide for [`Dockerfile`](./Dockerfile).
  - Includes build arguments, build/run examples, and operational notes.

- `generate-model-data.py`
  - Compiles one or more TFLite models with MERA.
  - Generates model artifacts, randomized input binaries, expected output binaries,
    and `config.yaml` metadata for runtime execution.

- `../docs/generate-model-data.md`
  - Detailed guide for [`generate-model-data.py`](./generate-model-data.py).
  - Includes arguments, processing flow, output layout, and limitations.

- `README.md`
  - This document.
  - Provides a quick map of the `scripts` folder and where to find details.

## Typical Workflow
1. Read `../docs/dockerfile.md` and build the container from `Dockerfile`.
1. Read `../docs/generate-model-data.md` and prepare model compilation inputs.
1. Run `generate-model-data.py` to generate deployable model data.
