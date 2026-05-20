/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2026 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : define.h
* Version      : v1.00
* Description  : RZ/G3E Sample AI Application (Face Detection)
***********************************************************************************************************************/

#ifndef DEFINE_MACRO_H
#define DEFINE_MACRO_H

/*****************************************
* includes
******************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <cstdio>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <float.h>
#include <atomic>
#include <semaphore.h>
#include <math.h>
#include <climits>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

/*****************************************
* Definition for AI model
******************************************/
struct ModelConfig {
    std::string model_dir; // directory that stores the RUHMI compilation output
};

/* Model Binary */
const static double anchors[] =
{
    38, 77,
    47, 97,
    61, 126,
    14, 26,
    19, 37,
    28, 55,
};

/* Image:: Information for drawing on image */
#define CHAR_SCALE_LARGE            (1.0)
#define CHAR_SCALE_SMALL            (0.9)
#define CHAR_SCALE_VERY_SMALL       (0.6)

/* Number of class to be detected */
#define NUM_CLASS                   (1)
/* Number for [region] layer num parameter */
#define NUM_BB                      (3)
#define NUM_INF_OUT_LAYER           (2)
/* Thresholds */
#define TH_PROB                     (0.5f)
#define TH_NMS                      (0.45f)
/* Size of input image to the model */
#define MODEL_IN_W                  (192)
#define MODEL_IN_H                  (192)

/* Number of grids in the image. The length of this array MUST match with the NUM_INF_OUT_LAYER */
const static uint8_t num_grids[] = { 6, 12 };
/* Number of output */
const static uint32_t INF_OUT_SIZE  = (NUM_CLASS + 5) * NUM_BB * num_grids[0] * num_grids[0]
                                      + (NUM_CLASS + 5) * NUM_BB * num_grids[1] * num_grids[1];

/*****************************************
* Macro for Application
******************************************/

/* Camera Capture Image Information */
#define CAM_IMAGE_WIDTH             (640)
#define CAM_IMAGE_HEIGHT            (480)
#define CAM_IMAGE_CHANNEL_BGR       (3)

/* Camera Capture Information */
#define CAPTURE_STABLE_COUNT        (8)

/* Display Image Information */
#define IMAGE_OUTPUT_WIDTH          (1920)
#define IMAGE_OUTPUT_HEIGHT         (1080)
#define IMAGE_OUTPUT_CHANNEL_BGRA   (4)

/*Image display out*/
#define DISP_IMAGE_OUTPUT_WIDTH     (1480)
#define DISP_IMAGE_OUTPUT_HEIGHT    (1050)

/*Total Display out*/
#define DISP_OUTPUT_WIDTH           (1920)
#define DISP_OUTPUT_HEIGHT          (1080)

/* Image:: Information for drawing on image */
#define CHAR_THICKNESS              (2)
#define CHAR_SCALE_BB               (0.4)
#define CHAR_THICKNESS_BB           (1)
#define LINE_HEIGHT                 (30) /* in pixel */
#define LINE_HEIGHT_OFFSET          (20) /* in pixel */
#define TEXT_WIDTH_OFFSET           (10) /* in pixel */
#define WHITE_DATA                  (0xFFFFFFu) /* in RGB */
#define BLACK_DATA                  (0x000000u) /* in RGB */
#define GREEN_DATA                  (0x00FF00u) /* in RGB */
#define RGB_FILTER                  (0x0000FFu) /* in RGB */
#define BOX_LINE_SIZE               (1)  /* in pixel */
#define BOX_DOUBLE_LINE_SIZE        (1)  /* in pixel */
#define ALIGHN_LEFT                 (1)
#define ALIGHN_RIGHT                (2)
/* For termination method display */
#define TEXT_START_X                (1440) 

/* Timer Related */
#define CAPTURE_TIMEOUT         (20)  /* seconds */
#define AI_THREAD_TIMEOUT       (20)  /* seconds */
#define IMAGE_THREAD_TIMEOUT    (20)  /* seconds */
#define DISPLAY_THREAD_TIMEOUT  (20)  /* seconds */
#define KEY_THREAD_TIMEOUT      (5)   /* seconds */
#define TIME_COEF               (1)
/* Waiting Time */
#define WAIT_TIME               (1000) /* microseconds */

/* Array size */
#define SIZE_OF_ARRAY(array) (sizeof(array)/sizeof(array[0]))

/* Image color channel */
enum class ImageChannel
{
    GRAY = 1,
    BGR  = 3,
    BGRA = 4
};


#endif /* DEFINE_MACRO_H */
