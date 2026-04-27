# RZ/G3E AI Application Specification (Face Detection)

## Introduction
This document describes how to build and run the Face Detection application running on RZ/G3E platform.  
This face detection app gets input from a USB camera or image file and displays the detection results on HDMI monitor.

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

The following figure shows the hardware and system configuration of the Face Detection application on RZ/G3E.

![G3E operation env](./images/app-system-config.png)

## Directory Structure

```
.
├── README.md                           // This document
├── exe
│   ├── face_detection                  // Application binary
│   └── model_yolo-fastest              // AI model directory, compiled using the RUHMI AI Framework
│          ├── config.yaml
│          └── yolo-fastest_192_face_v4
└── src	                                // Application source code
```

## AI Model Information

| AI Model                                                                           | Input size          | Output size                        |
| ---------------------------------------------------------------------------------- | ------------------- | ---------------------------------- |
| [yolo-fastest_192_face_v4.tflite](https://github.com/emza-vs/ModelZoo/tree/master) |  int8 [1,192,192,1] | int8[1,6,6,18]<br>int8[1,12,12,18] |

- [Model reference](https://github.com/renesas/ruhmi-framework-mcu/tree/main/application_examples/face_detection#model-reference)

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

The inference sequence (Steps 3 through 7) is executed once for the specified input image, and the detection result is displayed.

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

The application binary **face_detection** is generated in the build directory.

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
$ ./face_detection USB
```
Output:
  - Model directory, including the AI model compiled with the RZ/G3E RUHMI AI Framework
  - FPS (end-to-end)
  - Number of faces detected

### Image file mode

(command)
```bash
./face_detection IMAGE <path_to_image>
```
Output:
  - Model directory, including the AI model compiled with the RZ/G3E RUHMI AI Framework
  - FPS (Pre-process / Inference / Post-process)
  - Number of faces detected

**Note:** FPS values are shown for reference only and are not intended for performance evaluation.

The application can be terminated by pressing the **Enter** key on the console where it is running.


## Licenses
★ここは方針が決まったの上、記載いただけると幸いです。

