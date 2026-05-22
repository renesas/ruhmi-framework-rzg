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
- [Dockerfile](Dockerfile)
  - Builds a Docker image with Ubuntu 22.04, Python 3.10, MERA, TensorFlow,
    Ethos-U Vela, and LiteRT.
  - Intended for model compilation and test-data generation in a reproducible environment.

- [Guide for dockerfile](../docs/dockerfile.md)
  - Includes build arguments, build/run examples, and operational notes.

- [Model compilation script](generate-model-data.py)
  - Compiles one or more TFLite models with MERA.
  - Generates model artifacts, randomized input binaries, expected output binaries,
    and `config.yaml` metadata for runtime execution.

- [Guide of the model compilation script](../docs/generate-model-data.md)
  - Includes arguments, processing flow, output layout, and limitations.

## Typical Workflow
1. Read [Guide for dockerfile](../docs/dockerfile.md) and build the container from [Dockerfile](Dockerfile).  
2. Read [Guide of the model compilation script](../docs/generate-model-data.md) and prepare model compilation inputs.
1. Run [Model compilation script](generate-model-data.py) to generate deployable model data.
