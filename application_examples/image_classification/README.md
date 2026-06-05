# RZ/G3E Image Classification Application

## Overview

This application performs image classification using an AI model trained on ImageNet.

Input is either an image file (.jpg/.png) or a USB camera input.
The user can specify the input mode (IMAGE/USB) when running the AI ​​application to perform image classification in the desired mode.

The output is displayed on a 1920x1080 HDMI display in a Linux (Wayland) environment on RZ/G3E.

- IMAGE mode  
FPS: Calculated by averaging 10 consecutive runs of pre-processing, inference, and post-processing.
Image classification result: Displays the top 5 classification results.

- USB mode  
FPS: Calculated by averaging the effective frame rate (FPS) including image input, pre-processing, inference, post-processing, and image display over 10 frames.
Image classification result: Displays the top 5 classification results.

The model is compiled with the RUHMI AI Framework and executed with Ethos-U55 acceleration.

![Image Classification Flow](../../docs/assets/app-flow_img.png)

## Target Environment

- Board: RZ/G3E-EVKIT
- Software: RZ/G3E Ethos-U Package (including RUHMI runtime)
- Peripherals:
  - USB camera
  - HDMI display
  - microSD card (optional)

System configuration:

![Image Classification System](../../docs/assets/app-system-config_img.png)

## Directory Structure

```
.
├── README.md                         // This document
├── exe
│      ├── image_classification       // Application binary
│      ├── labels_mobilenet_v1.txt    // Label file for classification results
│      └── model_movilenetv1		      // AI model directory, compiled using the RUHMI AI Framework
│          ├── config.yaml
│          └── mobilenet_v1_0.25
└── src                               // Application source code
```

## Model Information

| AI Model     | Input size            | Output size   |
| ------------ | --------------------- | ------------- | 
| MobileNet V1 |  int8[1, 224, 224, 3] | int8[1, 1000] |

The model outputs classification scores for 1,000 categories.

- [Model reference](https://github.com/renesas/ruhmi-framework-mcu/tree/main/application_examples/image_classification#model-reference)


## Build

Build is required only when `src/` is included in your release package.

1. Install and source the RZ/G3E toolchain environment.
2. Build the application:

```bash
mkdir -p src/build
cd src/build
cmake ..
make
```

Generated binary: `src/build/image_classification`

## Run

Copy files to RZ/G3E-EVKIT:

```bash
scp -r exe/ root@<TARGET_IP>:/home/root/
```

USB camera mode:

```bash
./image_classification USB
```

Image file mode:

```bash
./image_classification IMAGE <path_to_image>
```

Expected output includes model info, FPS, and Top-5 classification results.

## Notes

- FPS values are reference values only.
- Press `Enter` in the running console to terminate the app.
- Refer to [LICENSE](../../LICENSE.md) in the repository root for license information.
