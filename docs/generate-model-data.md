# `generate-model-data.py` Guide

## Overview
`generate-model-data.py` compiles one or more TFLite models with RUHMI (MERA) and generates inference test data.
It automates the following steps:

1. Deploy each model with MERA (`target=IP`, `mera_platform=ALT3`)
1. Check that the generated `model_vela.tflite` is within the 32 MB limit
1. Generate randomized sample inputs (`input-*.bin`) based on model input tensor metadata
1. Run inference and save expected outputs (`expected-output-*.bin`)
1. Generate a `config.yaml` file for runtime execution

## Usage
```bash
python3 scripts/generate-model-data.py \
  -d <output_dir> \
  -m <model1.tflite> [model2.tflite ...] \
  [-c "<build_config_json_like_string>"]
```

Example:
```bash
python3 scripts/generate-model-data.py \
  -d models_to_deploy \
  -m mobilenet_v1.tflite yolo_fastest.tflite
```

## Arguments
- `-d`, `--output_dir` (required)
  - Output directory where generated files are stored.
- `-m`, `--model_paths` (required)
  - One or more input TFLite model paths (space separated).
- `-c`, `--build_config` (optional)
  - MERA build configuration.  
    Default: `{'scheduler_config': {'mode': 'Fast'}}`
  - Internally parsed as JSON (`'` is converted to `"` before parsing).

## Output Directory Structure
The script creates one directory per model plus a top-level `config.yaml`.

```text
<output_dir>/
  config.yaml
  <model_name>/
    input-0.bin
    expected-output-0.bin
    project.mdp
    build/
      IP/
        compilation/
        ir_dumps/
        ...
```

`config.yaml` is generated in a format similar to:

```yaml
models:
  - name: mobilenet_v1
    data_directory: mobilenet_v1/
    inputs:
      - name: serving_default_input:0
        data_type: int8
        file_name: input-0.bin
        shape: [1, 224, 224, 3]
```

## Processing Flow
1. Parse arguments and verify that all model files exist.
1. If `output_dir` already exists, move it to `<output_dir>_backup` (and remove an old backup if needed).
1. For each model:
   - Derive the model name from filename (remove `.tflite`)
   - Compile/deploy with `mera.Deployer`
   - Validate `model_vela.tflite` size (fail if over 32 MB)
   - Use LiteRT `Interpreter` to inspect input tensors
   - Generate randomized inputs by data type:
     - `int8`: `[-128, 127]`
     - `uint8`: `[0, 255]`
     - `float32`: uniform random values in `[0.0, 1.0)`
   - Run inference and save each output tensor to `expected-output-*.bin`
1. Save consolidated model metadata into `config.yaml`.

## Notes
- Supported input dtypes are only `int8`, `uint8`, and `float32`.
- The script exits with error if `model_vela.tflite` is missing or empty.
- Models larger than 32 MB after Vela compilation are treated as unsupported on RZ/G3E.
- Input data is randomized on each run. Add an RNG seed if reproducibility is required.

