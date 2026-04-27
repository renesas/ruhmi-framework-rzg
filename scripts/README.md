# Scripts

This directory contains helper scripts for RUHMI model deployment.

## Purpose

These scripts provide a minimal, repeatable workflow for preparing RUHMI deployment artifacts from a TFLite model and generating host-side reference data for validation.

They are intended to reduce manual setup effort and make it easier to verify model behavior before running on the RZ/G3E target board.

## Functional Overview

- `deploy.py`
  - Loads a TFLite model and runs RUHMI/MERA deployment.
  - Generates deployment artifacts in the specified output directory.
  - Accepts build configuration through `--build_cfg`.
- `gen_ref_data.py`
  - Runs TensorFlow Lite inference on host with generated random int8 input.
  - Writes test input (`input_0.bin`) and reference output (`ref_result_0.bin`).
  - Helps compare board runtime results with host reference outputs.

## Files

- `deploy.py`: compiles and packages a source model into deploy artifacts
- `gen_ref_data.py`: generates host-side reference input and output binaries

## Prerequisites

- Python environment with required packages installed (`mera`, `tensorflow`, `numpy`)
- A TFLite model file (default path: `./source_model_files/mobilenet_v2_1.0_224_INT8.tflite`)

## How to Use

Run from a project/example directory where `source_model_files/` exists.

1. Deploy model artifacts:

```bash
python deploy.py
```

2. Generate reference input/output binaries:

```bash
python gen_ref_data.py
```

## Script Options

### `deploy.py`

```bash
python deploy.py [--model_path <path>] [--out_dir <dir>] [--build_cfg "<dict-like string>"]
```

Options:

- `--model_path`  
  Path to input TFLite model.  
  Default: `./source_model_files/mobilenet_v2_1.0_224_INT8.tflite`
- `--out_dir`  
  Output directory for deployment artifacts.  
  Default: `./deploy_mobilenetv2`
- `--build_cfg`  
  Build configuration passed to MERA deploy API as a dictionary-like string.  
  Default: `{'scheduler_config': {'mode': 'Fast'}}`

Example:

```bash
python deploy.py \
  --model_path ./source_model_files/my_model.tflite \
  --out_dir ./deploy_my_model \
  --build_cfg "{'scheduler_config': {'mode': 'Fast'}}"
```

### `gen_ref_data.py`

```bash
python gen_ref_data.py [--model_path <path>] [--out_dir <dir>]
```

Options:

- `--model_path`  
  Path to input TFLite model used to run reference inference.  
  Default: `./source_model_files/mobilenet_v2_1.0_224_INT8.tflite`
- `--out_dir`  
  Directory where `input_0.bin` and `ref_result_0.bin` are written.  
  Default: `./deploy_mobilenetv2`

Example:

```bash
python gen_ref_data.py \
  --model_path ./source_model_files/my_model.tflite \
  --out_dir ./deploy_my_model
```

## Outputs

- Deployment directory (for example, `./deploy_mobilenetv2/`)
- `input_0.bin` (random int8 input tensor)
- `ref_result_0.bin` (reference output tensor from TensorFlow Lite)
