# RZ/G3E Image Classification AI Application

## Introduction
This application performs image classification on the RZ/G3E platform using either a USB camera input or a still image file.  
Input images are classified into 1,000 categories, and the results are displayed on an HDMI monitor.

The AI model used in this application is compiled with the RUHMI AI Framework and executed on the Ethos-U55 accelerator.

![Demo_image, RA8P1 AI Appのデモ画像を想定しています。](./images/app-demo.png)

## Target Environment
- **Board**
  - RZ/G3E EVK
- **Software**
  - RZ/G3E Ethos-U Package (including the RZ/G3E RUHMI AI Framework)
- **Peripheral Devices**
  - USB Camera
  - HDMI Display
  - microSD card (optional, used for boot media)

The following figure shows the hardware and system configuration of the Image Classification application on RZ/G3E.

![G3E operation env](./images/app-system-config.png)

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

## AI Model Information

| AI Model     | Input size            | Output size   |
| ------------ | --------------------- | ------------- | 
| MobileNet V1 |  int8[1, 224, 224, 3] | int8[1, 1000] |

The model outputs classification scores for 1,000 categories.

- [Model reference](https://github.com/renesas/ruhmi-framework-mcu/tree/main/application_examples/image_classification#model-reference)

## Application Flow
### Mode: USB Camera

Initialization:
- Step 1. Parse command-line arguments.
- Step 2. Load the AI model.

Inference Loop:
- Step 3. Acquire input frame from the USB camera.
- Step 4. Pre-process input data.
- Step 5. Prepare the input buffer for inference execution.
- Step 6. Run inference using the RUHMI runtime.
- Step 7. Retrieve inference results from the output buffer.
- Step 8. Post-process the inference results.

Steps 3 through 8 are executed repeatedly while the application is running.

### Mode: Image file

In the image file mode, the input image file is read from the file system on the RZ/G3E EVK.

Initialization:
- Step 1. Parse command-line arguments.
- Step 2. Load the AI model.

Inference Execution:
- Step 3. Pre-process input data.
- Step 4. Prepare the input buffer for inference execution.
- Step 5. Run inference using the RUHMI runtime.
- Step 6. Retrieve inference results from the output buffer.
- Step 7. Post-process the inference results.

The inference sequence (Steps 3 through 7) is executed once for the specified input image, and the classification result is displayed.

## Build

1. Prepare the target toolchain (SDK) provided in RZ/G3E Ethos Package and set up the environment.
```bash
$ ./rz-vlp-glibc-x86_64-core-image-weston-cortexa55-smarc-rzg3e-toolchain-x.x.x.sh
$ source <path_to_SDK>/environment-setup-cortexa55-poky-linux
```
2. Build the application by following the commands below.

```bash
$ mkdir -p src/build
$ cd src/build
$ cmake ..
$ make
```

The application binary **image_classification** is generated in the build directory.

## Run

Connect HDMI to the display, and USB-serial between your target board and PC.

Boot the RZ/G3E EVK and copy the application and related files to the target board.  
(e.g.) If you copy the application to your target board using Ethernet, run the following command on your host PC.

```bash
$ scp -r exe/ root@<TARGET_IP>:/home/root/
```

Then, run the AI App on the target board as below:

### USB camera mode

(command)
```bash
$ ./image_classification USB
```
Output:
  - Model directory, including the AI model compiled with the RZ/G3E RUHMI AI Framework
  - FPS (end-to-end)
  - Classification results (Top 5)

### Image file mode

(command)
```bash
$ ./image_classification IMAGE <path_to_image>
```
Output:
  - Model directory, including the AI model compiled with the RZ/G3E RUHMI AI Framework
  - FPS (Pre-process / Inference / Post-process)
  - Classification results (Top 5)

**Note:** FPS values are shown for reference only and are not intended for performance evaluation.

The application can be terminated by pressing the **Enter** key on the console where it is running.


## Licenses
★ここは方針が決まったの上、記載いただけると幸いです。

