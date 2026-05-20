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
* Description  : RZ/G3E Sample AI Application (Image Classification)
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

/*****************************************
* Definition for AI model
******************************************/
struct ModelConfig {
    std::string model_dir; // directory that stores the RUHMI compilation output
    std::string label_file;	// text file that defines class labels for the model
};

/* Empty since labels will be loaded from label_list file */
static std::vector<std::string> label_file_map = {};

/* Size of input image to the model */
#define MODEL_IN_H (224)
#define MODEL_IN_W (224)
#define MODEL_IN_C (3)

/*****************************************
* Macro for Application
******************************************/
/*Camera Capture Image Information*/
#define CAM_IMAGE_WIDTH             (640)
#define CAM_IMAGE_HEIGHT            (480)
#define CAM_IMAGE_CHANNEL_YUY2      (2)

/* Display Image Information*/
#define IMAGE_OUTPUT_WIDTH          (1920)
#define IMAGE_OUTPUT_HEIGHT         (1080)
#define IMAGE_OUTPUT_CHANNEL_BGRA   (4)

/*Image display out*/
#define DISP_IMAGE_OUTPUT_WIDTH     (1480)
#define DISP_IMAGE_OUTPUT_HEIGHT    (1050)

/*Total Display out*/
#define DISP_OUTPUT_WIDTH           (1920)
#define DISP_OUTPUT_HEIGHT          (1080)

/*Waiting Time*/
#define WAIT_TIME                   (1000) /* microseconds */

/*Timer Related*/
#define CAPTURE_TIMEOUT             (20)  /* seconds */
#define AI_THREAD_TIMEOUT           (20)  /* seconds */
#define EXIT_THREAD_TIMEOUT         (10)  /* seconds */
#define KEY_THREAD_TIMEOUT          (5)   /* seconds */

/*Thresholds*/
#define threshold 0.5

/* Color definitions for display */
namespace display_color {
    static const cv::Scalar White {255, 255, 255};
    static const cv::Scalar Green {0,   255,   0};
    static const cv::Scalar Red   {0,     0, 255};
    static const cv::Scalar Black {0,     0,   0};    
}

#endif
