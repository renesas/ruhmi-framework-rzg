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
* File Name    : main.cpp
* Version      : v1.00
* Description  : RZ/G3E Sample Application for Face Detection
***********************************************************************************************************************/

/*****************************************
* Includes
******************************************/
/* Definition of Macros & other variables */
#include "define.h"
/* Image control */
#include "image.h"
/* Wayland control */
#include "wayland.h"
/*box drawing*/
#include "box.h"
/* Definition for ruhmi runtime wrapper */
#include "MeraRuntimeEthosWrapper.h"

/*****************************************
* Global Variables
******************************************/
/* Multithreading*/
static sem_t terminate_req_sem;
static pthread_t ai_inf_thread;
static pthread_t kbhit_thread;
static pthread_t capture_thread;
static pthread_t hdmi_thread;
static std::mutex mtx;

/* Flags */
static std::atomic<bool> inference_start{false};
static std::atomic<uint8_t> img_obj_ready   (0);
static std::atomic<uint8_t> hdmi_obj_ready  (0);

static Image img;
/* Image to be displayed on GUI*/
cv::Mat input_image;
cv::Mat capture_image;
cv::Mat proc_image;
cv::Mat display_image;

/* GStreamer pipeline for camera capture */
static std::string gstreamer_pipeline = "";

/* Processing Time */
static double pre_time = 0;
static double post_time = 0;
static double ai_time = 0;

/* App mode */
enum class AppMode { USB, IMAGE };
static AppMode app_mode;

static Wayland wayland;

static struct timespec start_time;
static struct timespec end_time;
static float total_time = 0;
static bool fps_inited = false;
static int fps_count = 0;
const int window_num = 10; 
static std::atomic<float> fps(0.0f);

static std::vector<detection> det;
static std::vector<detection> print_det;
static bool display_padding = false;

/* Obtail from command line arguments */
std::string model_file_name;

constexpr float INPUT_SCALE = 0.00392118f;
constexpr int INPUT_ZERO_POINT = -128;
constexpr float OUTPUT_SCALE0 = 0.13408391f;
constexpr int OUTPUT_ZERO_POINT0 = 47;
constexpr float OUTPUT_SCALE1 = 0.18535926f;
constexpr int OUTPUT_ZERO_POINT1 = 10;

static MeraRuntimeEthosWrapper runtime;
static ModelConfig model_config;

/*****************************************
 * Function Name     : int8_to_float32
 * Description       : Convert int8 value to float32.
 * Arguments         : value = int8 number
 *                     scale = dequantization scale
 *                     zero_point = dequantization zero point
 * Return value      : float = float32 number
 ******************************************/
float int8_to_float32(int8_t value, float scale, int32_t zero_point)
{
    return (static_cast<float>(value) - static_cast<float>(zero_point)) * scale;
}

/*****************************************
* Function Name : timedifference_msec
* Description   : compute the time differences in ms between two moments
* Arguments     : t0 = start time
*                 t1 = stop time
* Return value  : the time difference in ms
******************************************/
static double timedifference_msec(struct timespec t0, struct timespec t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1000000.0;
}

/*****************************************
* Function Name : wait_join
* Description   : waits for a fixed amount of time for the thread to exit
* Arguments     : p_join_thread = thread that the function waits for to Exit
*                 join_time = the timeout time for the thread for exiting
* Return value  : 0 if successful
*                 not 0 otherwise
******************************************/
static int8_t wait_join(pthread_t *p_join_thread, uint32_t join_time)
{
    int8_t ret_err;
    struct timespec join_timeout;
    ret_err = clock_gettime(CLOCK_REALTIME, &join_timeout);
    if ( 0 == ret_err )
    {
        join_timeout.tv_sec += join_time;
        ret_err = pthread_timedjoin_np(*p_join_thread, NULL, &join_timeout);
    }
    return ret_err;
}

/*****************************************
* Function Name : sigmoid
* Description   : Helper function for YOLO Post processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
double sigmoid(double x)
{
    return 1.0/(1.0 + exp(-x));
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(float val[NUM_CLASS])
{
    float max_num = -FLT_MAX;
    float sum = 0;
    int32_t i;
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        max_num = std::max(max_num, val[i]);
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= (float) exp(val[i] - max_num);
        sum+= val[i];
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= val[i]/sum;
    }
}

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset
* Arguments     : n = output layer number
                  offs = offset to access the bounding box attributes
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int32_t yolo_index(uint8_t n, int32_t offs, int32_t channel)
{
    uint8_t num_grid = num_grids[n];
    return offs + channel * num_grid *  num_grid;
}

/*****************************************
* Function Name : yolo_offset
* Description   : Get the offset nuber to access the bounding box attributes
*                 To get the actual value of bounding box attributes, use yolo_index() after this function.
* Arguments     : n = output layer number [0~1].
                  b = Number to indicate which bounding box in the region [0~2]
*                 y = Number to indicate which region [0~12]
*                 x = Number to indicate which region [0~12]
* Return value  : offset to access the bounding box attributes.
******************************************/
int32_t yolo_offset(uint8_t n, uint8_t b, uint8_t y, uint8_t x)
{
    uint8_t num = num_grids[n];
    int32_t prev_layer_num = 0;

    for (uint8_t i = 0; i < n; ++i)
    {
        prev_layer_num += NUM_BB * (NUM_CLASS + 5) * num_grids[i] * num_grids[i];
    }
    return prev_layer_num + b * (NUM_CLASS + 5) + (y * num + x) * NUM_BB * (NUM_CLASS + 5);
}

/*****************************************
* Function Name : R_Post_Proc
* Description   : Process CPU post-processing for YOLO
* Arguments     : floatarr = output address
* Return value  : -
******************************************/
void R_Post_Proc(float* floatarr)
{
    /* This mutex processing is for measuring the exact processing time and is not suitable for production */
    mtx.lock();

    /* Following variables are required for correct_yolo_boxes in Darknet implementation */
    /* Note: This implementation refers to the "darknet detector test" */
    float new_w, new_h;
    float correct_w = (float)CAM_IMAGE_WIDTH;
    float correct_h = (float)CAM_IMAGE_HEIGHT;
    if ((float) (MODEL_IN_W / correct_w) < (float) (MODEL_IN_H/correct_h) )
    {
        new_w = (float) MODEL_IN_W;
        new_h = correct_h * MODEL_IN_W / correct_w;
    }
    else
    {
        new_w = correct_w * MODEL_IN_H / correct_h;
        new_h = MODEL_IN_H;
    }

    int32_t n = 0;
    int32_t b = 0;
    int32_t y = 0;
    int32_t x = 0;
    int32_t offs = 0;
    int32_t i = 0;
    float tx = 0;
    float ty = 0;
    float tw = 0;
    float th = 0;
    float tc = 0;
    float center_x = 0;
    float center_y = 0;
    float box_w = 0;
    float box_h = 0;
    float objectness = 0;
    uint8_t num_grid = 0;
    uint8_t anchor_offset = 0;
    float classes[NUM_CLASS];
    float max_pred = 0;
    int32_t pred_class = -1;
    float probability = 0;
    detection d;

    /* Clear the detected result list */
    det.clear();
    
    /*Post Processing Start */
    for (n = 0; n < NUM_INF_OUT_LAYER; n++)
    {
        num_grid = num_grids[n];
        anchor_offset = 2 * NUM_BB * n;
        
        for(y = 0; y < num_grid; y++)
        {
            for(x = 0; x < num_grid; x++)
            {
                for(b = 0; b < NUM_BB; b++)
                {
                    offs = yolo_offset(n, b, y, x);
                    tx = floatarr[offs];
                    ty = floatarr[offs + 1];
                    tw = floatarr[offs + 2];
                    th = floatarr[offs + 3];
                    tc = floatarr[offs + 4];
                    /* Compute the bounding box */
                    /* Get_yolo_box*/
                    center_x = ((float) x + sigmoid(tx)) / (float) num_grid;
                    center_y = ((float) y + sigmoid(ty)) / (float) num_grid;
                    box_w = (float) exp(tw) * anchors[anchor_offset+2*b+0] / (float) MODEL_IN_W;
                    box_h = (float) exp(th) * anchors[anchor_offset+2*b+1] / (float) MODEL_IN_H;
                    /* Adjustment for VGA size */
                    /* correct_yolo_boxes */
                    center_x = (center_x - (MODEL_IN_W - new_w) / 2. / MODEL_IN_W) / ((float) new_w / MODEL_IN_W);
                    center_y = (center_y - (MODEL_IN_H - new_h) / 2. / MODEL_IN_H);
                    center_x = round(center_x * CAM_IMAGE_WIDTH);
                    center_y = round(center_y * CAM_IMAGE_WIDTH);
                    box_w = round(box_w * CAM_IMAGE_WIDTH);
                    box_h = round(box_h * CAM_IMAGE_WIDTH);
                    objectness = sigmoid(tc);
                    Box bb = {center_x, center_y, box_w, box_h};
                    /* Get the class prediction */
                    for (i = 0; i < NUM_CLASS; i++)
                    {
                        classes[i] = sigmoid(floatarr[offs+5+i]);
                    }
                    max_pred = 0;
                    pred_class = -1;
                    for (i = 0; i < NUM_CLASS; i++)
                    {
                        if (classes[i] > max_pred)
                        {
                            pred_class = i;
                            max_pred = classes[i];
                        }
                    }
                    /* Store the result into the list if the probability is more than the threshold */
                    probability = max_pred * objectness;
                    if (probability > TH_PROB)
                    {
                        d = {bb, pred_class, probability};
                        det.push_back(d);
                    }
                }
            }
        }
    }
    /* Non-Maximum Supression filter */
    filter_boxes_nms(det, det.size(), TH_NMS);
    
    mtx.unlock();
}

/*****************************************
* Function Name : draw_bounding_box
* Description   : Draw bounding box on image. 
*                 Must be called before resizing the display image.
* Arguments     : -
* Return value  : -
******************************************/
void draw_bounding_box(void)
{
    /* This mutex processing is for measuring the exact processing time and is not suitable for production */
    mtx.lock();
    std::stringstream stream;
    uint32_t color = GREEN_DATA;

    print_det.clear();
    /* Draw bounding box on RGB image */
    for (size_t i = 0; i < det.size(); i++)
    {
        /* Skip the overlapped bounding boxes */
        if (det[i].prob == 0) continue;
        print_det.push_back(det[i]);
        /* Draw the bounding box on the image */
        stream << std::fixed << std::setprecision(2) << det[i].prob;
        img.draw_rect((int)det[i].bbox.x, (int)det[i].bbox.y, 
            (int)det[i].bbox.w, (int)det[i].bbox.h, color);
    }
    mtx.unlock();
}

/*****************************************
* Function Name : print_result
* Description   : print the result on display.
* Arguments     : -
* Return value  : 0 if succeeded
*               not 0 otherwise
******************************************/
int8_t print_result(Image* img, cv::Mat& proc_image)
{
    /* This mutex processing is for measuring the exact processing time and is not suitable for production */
    mtx.lock();
    std::stringstream stream;
    std::string str = "";
    double total_time = ai_time + pre_time + post_time;
    uint8_t str_count = 1;
    uint32_t draw_offset_x = TEXT_START_X + TEXT_WIDTH_OFFSET;
    uint8_t time_width = 5;
    uint8_t time_precision = 1;
    uint8_t result_width = 5;
    uint8_t result_precision = 1;
    uint32_t i = 0;
    std::string file_name = "";
    size_t file_name_pos = model_file_name.find_last_of('/');
    if(file_name_pos == std::string::npos)
    {
        file_name = model_file_name;
    }
    else
    {
    	file_name = model_file_name.substr(file_name_pos + 1);
    }

    /* Insert empty lines */
    str_count++;

    /* Display the model on BGR image */
    stream.str("");
    stream << "Model: ";
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, LINE_HEIGHT_OFFSET + (LINE_HEIGHT * str_count++), CHAR_SCALE_LARGE, WHITE_DATA, proc_image);
    stream.str("");
    stream << "yolo-fastest_192_face_v4";
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, LINE_HEIGHT_OFFSET + (LINE_HEIGHT * str_count++), CHAR_SCALE_SMALL, WHITE_DATA, proc_image);

    /* Insert empty lines */
    str_count++;

	/* Display the label for FPS */
    stream.str("");
    if (app_mode == AppMode::IMAGE)
    {
        stream << "FPS (Pre-Inference-Post): ";
    }
    else
    {
        /* USB mode */
        stream << "FPS (End-to-End) : ";
    }
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, LINE_HEIGHT_OFFSET + (LINE_HEIGHT * str_count++), CHAR_SCALE_LARGE, WHITE_DATA, proc_image);	
    /* Display FPS */
    stream.str("");
    if (fps > 0.0f)
    {
        stream << std::fixed << std::setprecision(2) << fps;
    }
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, LINE_HEIGHT_OFFSET + (LINE_HEIGHT * str_count++), CHAR_SCALE_SMALL, WHITE_DATA, proc_image);


    /* Insert empty lines */
    str_count++;


    /* Draw No of Faces on BGR image */
    stream.str("");
    stream << "No of Faces";
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, LINE_HEIGHT_OFFSET + (LINE_HEIGHT * str_count++), CHAR_SCALE_LARGE, WHITE_DATA, proc_image);
	/* Draw No of Faces on BGR image */
    stream.str("");
    stream << std::setw(2) << std::setfill('0') << print_det.size();
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, LINE_HEIGHT_OFFSET + (LINE_HEIGHT * str_count++), CHAR_SCALE_SMALL, WHITE_DATA, proc_image);

    /* Draw the termination method at the bottom */
    stream.str("");
    stream << "To terminate the application,";
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, 
                            IMAGE_OUTPUT_HEIGHT - LINE_HEIGHT*3, CHAR_SCALE_VERY_SMALL, WHITE_DATA, proc_image);
    stream.str("");
    stream << "press [Super]+[Tab] and press ENTER key.";
    str = stream.str();
    img->write_string_rgb(str, ALIGHN_LEFT, draw_offset_x, 
                            IMAGE_OUTPUT_HEIGHT - LINE_HEIGHT*2, CHAR_SCALE_VERY_SMALL, WHITE_DATA, proc_image);

    mtx.unlock();

    return 0;
}

/*****************************************
* Function Name : preprocess_input
* Description   : Execute pre-process and set input
* Arguments     : -
* Return value  : true if succeeded
*                 false 0 otherwise
******************************************/
static bool preprocess_input(cv::Mat& input_image)
{
    cv::Mat img = input_image.clone();
    /* resize */
    cv::Size size(MODEL_IN_W, MODEL_IN_H);
    cv::resize(img, img, size, 0, 0, cv::INTER_LINEAR);

    /* convert to grayscale */
    switch (img.channels())
    {
        case static_cast<int>(ImageChannel::GRAY):
        /* grayscale image, skip */
        break;
        case static_cast<int>(ImageChannel::BGR):
            cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        break;
        case static_cast<int>(ImageChannel::BGRA):
            cv::cvtColor(img, img, cv::COLOR_BGRA2GRAY);
        break;
        default:
            fprintf(stderr, "[ERROR] Unsupported channel count\n");
            return false;
    }

    /* get input buffer info */
    auto input_info = runtime.GetInputInfo();
    if (input_info.empty()) {
        fprintf(stderr, "[ERROR] No input tensors\n");
        return false;
    }
    const auto& [in_name, in_size, in_type] = input_info[0];
    if (in_type != InOutDataType::INT8)
    {
        fprintf(stderr, "[ERROR] Invalid input data type\n");
        return false;
    }
    if (in_size != MODEL_IN_W * MODEL_IN_H * 1)
    {
        fprintf(stderr, "[ERROR] Invalid input data size\n");
        return false;       
    }
    auto* input_ptr = reinterpret_cast<int8_t*>(runtime.GetInputPtr(in_name));
    if (!input_ptr)
    {
        fprintf(stderr, "[ERROR] Failed to get input buffer pointer\n");
        return false;
    }

    /* quantize & write directly into runtime input buffer */
    cv::Mat runtime_in(MODEL_IN_H, MODEL_IN_W, CV_8SC1, input_ptr);
    img.convertTo(runtime_in, CV_8SC1, 1.0f / (255.0f * INPUT_SCALE), static_cast<float>(INPUT_ZERO_POINT));

    return true;
}

/*****************************************
* Function Name : R_Inf_Img_Thread
* Description   : Executes the inference thread (IMAGE mode)
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Inf_Img_Thread(void *threadid)
{
    /* Semaphore Variable */
    int32_t inf_sem_check = 0;

    /* Variable for getting Inference output data*/
    void* output_ptr;
    uint32_t out_size;

    /* Inference Variables */
    struct timespec tv;
    int8_t inf_status = 0;

    /* Variable for checking return value */
    int8_t ret = 0;
    /* Variable for Performance Measurement */
    static struct timespec start_time;
    static struct timespec inf_end_time;
    static struct timespec pre_start_time;
    static struct timespec pre_end_time;
    static struct timespec post_start_time;
    static struct timespec post_end_time;
    std::streamsize model_size;
    std::vector<unsigned char> model_data;
    std::vector<float> output_arr(1);
    std::vector<float> output_arr0(1);
    std::vector<float> output_arr1(1);

    printf("[INFO] Inference Thread Starting\n");

    while(1)
    {
        /* Get the Termination request semaphore value. If different then 1 Termination was requested*/
        /* Checks if sem_getvalue is executed without issue */
        errno = 0;
        ret = sem_getvalue(&terminate_req_sem, &inf_sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /* Check the semaphore value */
        if (1 != inf_sem_check)
        {
            goto ai_inf_end;
        }
        /* Check if image frame from Capture Thread is ready.*/
        if (inference_start.load())
        {
            break;
        }
        usleep(WAIT_TIME);
    }

    /* Get Pre-process starting time*/
    ret = timespec_get(&start_time, TIME_UTC);
    if (0 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to get AI process Start Time\n");
        goto err;
    }
    
    for (int fps_count = 0; fps_count < window_num; fps_count++)
    {
        /*Pre-process */
        if (!preprocess_input(input_image))
        {
            fprintf(stderr, "[ERROR] Failed to preprocess input.\n");
            goto err;
        }
        /* Inference */
        runtime.Run();

        /* Prepare for post-process, get output buffer */
        auto output_info = runtime.GetOutputInfo();
        /*(1,6,6,18)*/
        const auto& [out_name0, out_size0, out_type0] = output_info[0];
        size_t output_size0 = out_size0;
        int8_t* data_ptr0 = reinterpret_cast<int8_t*>(runtime.GetOutputPtr(out_name0));
        output_arr0.resize(output_size0);
        for (int n = 0; n < output_size0; n++)
        {
            /* Cast INT8 output data to FP32 */
            output_arr0[n] = int8_to_float32(data_ptr0[n], OUTPUT_SCALE0, OUTPUT_ZERO_POINT0);
        }

        /*(1,12,12,18)*/
        const auto& [out_name1, out_size1, out_type1] = output_info[1];
        size_t output_size1 = out_size1; 
        int8_t* data_ptr1 = reinterpret_cast<int8_t*>(runtime.GetOutputPtr(out_name1));
        output_arr1.resize(output_size1);
        
        for (int n = 0; n < output_size1; n++)
        {
            /* Cast INT8 output data to FP32 */
            output_arr1[n] = int8_to_float32(data_ptr1[n], OUTPUT_SCALE1, OUTPUT_ZERO_POINT1);
        }

        output_arr.clear();
        output_arr.reserve(output_arr0.size() + output_arr1.size());
        output_arr.insert(output_arr.end(), output_arr0.begin(), output_arr0.end());
        output_arr.insert(output_arr.end(), output_arr1.begin(), output_arr1.end());

        /*Post-process */
        R_Post_Proc(output_arr.data());
    }    
    /* Get Post-process End Time*/
    ret = timespec_get(&end_time, TIME_UTC);
    if ( 0 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to Get Post-process End Time\n");
        goto err;
    }
    /* Calculate FPS (Image mode) */
    total_time = (float)((timedifference_msec(start_time, end_time)));
    fps.store(1000.0f * window_num / total_time, std::memory_order_relaxed);
    inference_start.store(false);
    pthread_exit(NULL);

/* Error Processing*/
err:
    /* Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto ai_inf_end;
/*AI Thread Termination */
ai_inf_end:
    /* To terminate the loop in Capture Thread.*/
    printf("[INFO] AI Inference Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
* Function Name : R_Inf_Thread
* Description   : Executes the inference thread
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Inf_Thread(void *threadid)
{
    /* Semaphore Variable */
    int32_t inf_sem_check = 0;

    /* Variable for getting Inference output data*/
    void* output_ptr;
    uint32_t out_size;

    /* Inference Variables */
    struct timespec tv;
    int8_t inf_status = 0;

    /* Variable for checking return value */
    int8_t ret = 0;
    std::streamsize model_size;
    std::vector<unsigned char> model_data;
    std::vector<float> output_arr(1);
    std::vector<float> output_arr0(1);
    std::vector<float> output_arr1(1);

    printf("Inference Thread Starting\n");

    /* Inference Loop Start */
    while(1)
    {
        while(1)
        {
            /* Gets the Termination request semaphore value. If different then 1 Termination was requested */
            /* Checks if sem_getvalue is executed wihtout issue */
            errno = 0;
            ret = sem_getvalue(&terminate_req_sem, &inf_sem_check);
            if (0 != ret)
            {
                fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
                goto err;
            }
            /* Checks the semaphore value */
            if (1 != inf_sem_check)
            {
                goto ai_inf_end;
            }
            /* Checks if image frame from Capture Thread is ready */
            if (inference_start.load())
            {
                break;
            }
            usleep(WAIT_TIME);
        }

        /*Pre-process */
        mtx.lock();
        if (!preprocess_input(input_image))
        {
            fprintf(stderr, "[ERROR] Failed to preprocess input.\n");
            goto err;
        }
        mtx.unlock();

        /* Inference */
        runtime.Run();

        inference_start.store(false);

        /* Prepare for post-process, get output buffer */
        auto output_info = runtime.GetOutputInfo();
        /*(1,6,6,18)*/
        const auto& [out_name0, out_size0, out_type0] = output_info[0];
        size_t output_size0 = out_size0;
        int8_t* data_ptr0 = reinterpret_cast<int8_t*>(runtime.GetOutputPtr(out_name0));
        output_arr0.resize(output_size0);
        for (int n = 0; n < output_size0; n++)
        {
            /* Cast INT8 output data to FP32 */
            output_arr0[n] = int8_to_float32(data_ptr0[n], OUTPUT_SCALE0, OUTPUT_ZERO_POINT0);
        }

        /*(1,12,12,18)*/
        const auto& [out_name1, out_size1, out_type1] = output_info[1];
        size_t output_size1 = out_size1; 
        int8_t* data_ptr1 = reinterpret_cast<int8_t*>(runtime.GetOutputPtr(out_name1));
        output_arr1.resize(output_size1);
        
        for (int n = 0; n < output_size1; n++)
        {
            /* Cast INT8 output data to FP32 */
            output_arr1[n] = int8_to_float32(data_ptr1[n], OUTPUT_SCALE1, OUTPUT_ZERO_POINT1);
        }

        output_arr.clear();
        output_arr.reserve(output_arr0.size() + output_arr1.size());
        output_arr.insert(output_arr.end(), output_arr0.begin(), output_arr0.end());
        output_arr.insert(output_arr.end(), output_arr1.begin(), output_arr1.end());

        /*Post-process */
        R_Post_Proc(output_arr.data());
    }
    /* End of Inference Loop*/

/* Error Processing*/
err:
    /* Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto ai_inf_end;
/*AI Thread Termination */
ai_inf_end:
    /* To terminate the loop in Capture Thread.*/
    printf("[INFO] AI Inference Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
* Function Name : R_Capture_Thread
* Description   : Executes the V4L2 capture with Capture thread.
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Capture_Thread(void *threadid)
{
    std::string &gstream = gstreamer_pipeline;
    printf("[INFO] GStreamer pipeline: %s\n", gstream.c_str());

    /* Semaphore Variable */
    int32_t capture_sem_check = 0;
    int8_t ret = 0;
    /* Counter to wait for the camera to stabilize */
    uint8_t capture_stabe_cnt = CAPTURE_STABLE_COUNT;

    cv::VideoCapture g_cap;
    cv::Mat g_frame;
    cv::Mat padding_frame((CAM_IMAGE_WIDTH - CAM_IMAGE_HEIGHT) / 2, CAM_IMAGE_WIDTH, CV_8UC3, cv::Scalar(0));

    printf("Capture Thread Starting\n");

    g_cap.open(gstream, cv::CAP_GSTREAMER);
    if (!g_cap.isOpened())
    {
        fprintf(stderr, "[ERROR] Failed to open camera\n");
        goto err;
    }

    while(1)
    {
        /* Gets the Termination request semaphore value. If different then 1 Termination was requested */
        /* Checks if sem_getvalue is executed wihtout issue */
        errno = 0;
        ret = sem_getvalue(&terminate_req_sem, &capture_sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /* Checks the semaphore value */
        if (1 != capture_sem_check)
        {
            goto capture_end;
        }

        /* Capture camera image and stop updating the capture buffer */
        g_cap.read(g_frame);

        /* Breaking the loop if no video frame is detected */
        if (g_frame.empty())
        {
            fprintf(stderr, "[ERROR] Failed to get capture image\n");
            goto err;
        }
        else
        {
            /* Do not process until the camera stabilizes, because the image is unreliable until the camera stabilizes */
            if( capture_stabe_cnt > 0 )
            {
                capture_stabe_cnt--;
            }
            else
            {
                if (!inference_start.load())
                {
                    /* Copy captured image to Image object. This will be used in Main Thread */
                    mtx.lock();
                    /* Image: CAM_IMAGE_WIDTH*CAM_IMAGE_HEIGHT (BGR) */
                    input_image = g_frame.clone();
                    
                    /*Add padding for keeping the aspect ratio: CAM_IMAGE_WIDTH*CAM_IMAGE_WIDTH (BGR) */
                    cv::vconcat(padding_frame, input_image, input_image);
                    cv::vconcat(input_image, padding_frame, input_image);
                    
                    mtx.unlock();
                    inference_start.store(true); /* Flag for AI Inference Thread */
                }

                if (!img_obj_ready.load())
                {
                    mtx.lock();
                    capture_image = g_frame.clone();
                    img.set_mat(capture_image);
                    mtx.unlock();
                    img_obj_ready.store(1); /* Flag for Img Thread */
                }
            }
        }
    } /* End of Loop*/

/* Error Processing*/
err:
    sem_trywait(&terminate_req_sem);
    goto capture_end;

capture_end:
    g_cap.release();
    /* To terminate the loop in AI Inference Thread */
    inference_start.store(true);

    printf("Capture Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
 * Function Name : query_device_status
 * Description   : function to check USB device is connectod.
 * Arguments     : device_type = function to check USB device is connectod.
 * Return value  : media_port, media port that device is connectod. 
 ******************************************/
std::string query_device_status(std::string device_type)
{
    std::string media_port = "";
    /* Linux command to be executed */
    const char* command = "v4l2-ctl --list-devices";
    /* Open a pipe to the command and execute it */ 
    FILE* pipe = popen(command, "r");
    if (pipe == nullptr)
    {
        std::cerr << "[ERROR] Unable to open the pipe." << std::endl;
        return media_port;
    }
    /* Read the command output line by line */
    char buffer[128];
	char *p = nullptr;
    size_t found;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        std::string response = std::string(buffer);
        found = response.find(device_type);
        if (std::string::npos != found)
        {
            p = fgets(buffer, sizeof(buffer), pipe);
            if(p == nullptr)
            {
                return media_port;
            }
            media_port = std::string(buffer);
            pclose(pipe);
            /* return media port */
            return media_port;
        } 
    }
    pclose(pipe);
    /* return media port */
    return media_port;
}

/*****************************************
* Function Name : R_Kbhit_Thread
* Description   : Executes the Keyboard hit thread (checks if enter key is hit)
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Kbhit_Thread(void *threadid)
{
    /* Semaphore Variable */
    int32_t kh_sem_check = 0;
    /* Variable to store the getchar() value */
    int32_t c = 0;
    /* Variable for checking return value */
    int8_t ret = 0;

    printf("[INFO] Key Hit Thread Starting\n");

    printf("************************************************\n");
    printf("* Press ENTER key to quit. *\n");
    printf("************************************************\n");

    /* Set Standard Input to Non Blocking*/
    errno = 0;
    ret = fcntl(0, F_SETFL, O_NONBLOCK);
    if (-1 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to run fctnl(): errno=%d\n", errno);
        goto err;
    }

    while(1)
    {
        /* Get the Termination request semaphore value. If different then 1 Termination was requested */
        /* Check if sem_getvalue is executed wihtout issue */
        ret = sem_getvalue(&terminate_req_sem, &kh_sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /* Checks the semaphore value */
        if (1 != kh_sem_check)
        {
            goto key_hit_end;
        }

        c = getchar();
        if (c == '\n' || c == '\r')
        {
            /* When key is pressed */
            printf("[INFO] Key Detected\n");
            goto err;
        }
        else
        {
            /* When nothing is pressed */
            usleep(WAIT_TIME);
        }
    }

/* Error Processing*/
err:
    /* Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto key_hit_end;

key_hit_end:
    printf("Key Hit Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
* Function Name : R_Main_Process
* Description   : Runs the main process loop
* Arguments     : -
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t R_Main_Process()
{
    /* Main Process Variables */
    int8_t main_ret = 0;
    /* Semaphore Related */
    int32_t sem_check = 0;
    /* Variable for checking return value */
    int8_t ret = 0;

    /* Initialize wayland */
    ret = wayland.init(IMAGE_OUTPUT_WIDTH, IMAGE_OUTPUT_HEIGHT, IMAGE_OUTPUT_CHANNEL_BGRA);
    if(0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to initialize Image for Wayland\n");
        goto err;
    }

    printf("Main Loop Starts\n");
    while(1)
    {
        /* Gets the Termination request semaphore value. If different then 1 Termination was requested */
        errno = 0;
        ret = sem_getvalue(&terminate_req_sem, &sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /* Checks the semaphore value */
        if (1 != sem_check)
        {
            goto main_proc_end;
        }

        /* Check img_obj_ready flag which is set in Capture Thread */
        if (img_obj_ready.load())
        {           
            /* Draw bounding box on image */
            draw_bounding_box();

            /* Convert output image size */
            img.convert_size(CAM_IMAGE_WIDTH, IMAGE_OUTPUT_WIDTH, display_padding);

            /* Displays AI Inference Results on display */
            display_image = img.get_mat().clone(); 
            print_result(&img, display_image);

            /* Calculate FPS (USB mode) */
            if (app_mode == AppMode::USB)
            {
                ret = timespec_get(&end_time, TIME_UTC);
                if ( 0 == ret)
                {
                    fprintf(stderr, "[ERROR] Failed to Get End Time\n");
                    goto err;
                }
                if (!fps_inited) {
                    start_time = end_time;
                    fps_inited = true;
                }

                fps_count++;
                
                if (fps_count >= window_num) {
                    total_time = (float)((timedifference_msec(start_time, end_time)));
                    /* Calculate fps (Image mode) */
                    if (total_time > 0.0f)
                    {
                        fps.store(1000.0f * window_num / total_time, std::memory_order_relaxed);
                    }
                    fps_count = 0;
                    start_time = end_time;
                }
            }
            /* Update Wayland */
            cv::cvtColor(display_image, display_image, cv::COLOR_BGR2BGRA);
            wayland.commit(display_image.data, NULL);
            img_obj_ready.store(0);
        }
        /* Wait for 1 TICK */
        usleep(WAIT_TIME);
    }

/* Error Processing*/
err:
    sem_trywait(&terminate_req_sem);
    main_ret = 1;
    goto main_proc_end;
/* Main Processing Termination */
main_proc_end:
    img_obj_ready.store(0);
    printf("Main Process Terminated\n");
    return main_ret;
}

/*****************************************
 * Function Name : print_usage_info
 * Description   : function to print usage info.
 * Arguments     : -
 * Return value  : -
 ******************************************/
void print_usage_info()
{
    std::cout
        << "[INFO] usage:\n"
        << " ./face_detection USB\n"
        << " ./face_detection IMAGE <image_file> (supported: .jpg/.jpeg/.png)\n";
}

/*****************************************
 * Function Name : parse_arg
 * Description   : Parse command-line arguments and set application mode and input pipeline.
 * Arguments     : argc = number of command-line arguments
 *                 argv = command-line argument values
 * Return value  : true  - valid arguments 
                   false - invalid arguments
 ******************************************/
bool parse_arg (int32_t argc, char * argv[])
{ 
    if (argc < 2)
    {
        std::cout << "\n[ERROR] Please specify required arguments and a valid input source." << std::endl;
        print_usage_info();
        return false;
    }

    const std::string mode = argv[1];

    if (mode == "USB")
    {
        std::cout << "[INFO] USB CAMERA mode\n";
        app_mode = AppMode::USB;
        std::string media_port = query_device_status("usb");
        gstreamer_pipeline = "v4l2src device=" + media_port +" ! video/x-raw, width="+std::to_string(CAM_IMAGE_WIDTH)+", height="+std::to_string(CAM_IMAGE_HEIGHT)+" ,framerate=30/1,pixel-aspect-ratio=1/1 ! videoconvert ! appsink -v";
    }
    else if (mode == "IMAGE")
    {
        std::cout << "[INFO] IMAGE mode \n";
        app_mode = AppMode::IMAGE;

        if (argc < 3)
        {
            std::cout << "\n[ERROR] No image files specified." << std::endl;
            return false; 
        }

        const std::string image_path = argv[2];
         /* Extract the file extension */
        size_t dotPos = image_path.find_last_of('.');
        std::string extension = image_path.substr(dotPos + 1);
        if (dotPos != std::string::npos && (extension == "jpg" || extension == "JPG" || extension == "jpeg" || extension == "JPEG")) 
        {
            gstreamer_pipeline = "filesrc location=" + image_path + " ! jpegdec ! imagefreeze ! videoconvert ! videoscale ! video/x-raw, format=BGR, width=640, height=480, pixel-aspect-ratio=1/1 ! appsink";
        }
        else if (dotPos != std::string::npos && (extension =="png"|| extension =="PNG"))
        {
            gstreamer_pipeline = "filesrc location=" + image_path + " ! pngdec ! imagefreeze ! videoconvert ! videoscale ! video/x-raw, format=BGR, width=640, height=480, pixel-aspect-ratio=1/1 ! appsink";
        }
        else
        {
            std::cout << "\n[ERROR] Invalid input file. Please specify a .jpg/.jpeg or .png file." << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "\n[ERROR] Please specify required arguments and a valid input source." << std::endl;
        print_usage_info();
        return false;
    }
    return true;
}

int32_t main(int32_t argc, char * argv[])
{
    int8_t main_proc = 0;
    int8_t ret = 0;
    int32_t ret_main = 0;
    /* Multithreading Variables */
    int32_t create_thread_ai = -1;
    int32_t create_thread_key = -1;
    int32_t create_thread_capture = -1;
    int32_t sem_create = -1;
    /* Define model */
    model_config.model_dir = "model_yolo-fastest/yolo-fastest_192_face_v4/build/IP/compilation";

    /* parse arguments and build pipeline */
    if (!parse_arg(argc, argv))
    {
        return -1;
    }

    /* Load AI Model */
    printf("[INFO] Trying to load AI model\n");
    if (!runtime.LoadModel(model_config.model_dir))
    {
        std::cout << "\n[ERROR] Failed to load AI model." << std::endl;
        return -1;
    }

    /* Check the aspect ratio of camera input and display */
    float camera_ratio = (float) CAM_IMAGE_WIDTH / CAM_IMAGE_HEIGHT;
    float display_ratio = (float) IMAGE_OUTPUT_WIDTH / IMAGE_OUTPUT_HEIGHT;
    if (camera_ratio != display_ratio)
    {
        /* If different, set padding on Wayland display*/
        display_padding = true;
    }

    printf("Start RZ/G3E Sample Application (Face Detection)\n");

    /* Initialize Image object */
    ret = img.init(CAM_IMAGE_WIDTH, CAM_IMAGE_HEIGHT, CAM_IMAGE_CHANNEL_BGR, 
                    IMAGE_OUTPUT_WIDTH, IMAGE_OUTPUT_HEIGHT, IMAGE_OUTPUT_CHANNEL_BGRA);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to initialize Image object\n");
        ret_main = ret;
        goto end_free_malloc;
    }

    /* Termination Request Semaphore Initialization */
    /*Initialized value at 1.*/
    sem_create = sem_init(&terminate_req_sem, 0, 1);
    if (0 != sem_create)
    {
        fprintf(stderr, "[ERROR] Failed to Initialize Termination Request Semaphore\n");
        ret_main = -1;
        goto end_threads;
    }

    /* Create Key Hit Thread */
    create_thread_key = pthread_create(&kbhit_thread, NULL, R_Kbhit_Thread, NULL);
    if (0 != create_thread_key)
    {
        fprintf(stderr, "[ERROR] Failed to create Key Hit Thread\n");
        ret_main = -1;
        goto end_threads;
    }

    /* Create Inference Thread */
    if (app_mode == AppMode::IMAGE)
    {
        create_thread_ai = pthread_create(&ai_inf_thread, NULL, R_Inf_Img_Thread, NULL);
    }
    else
    {
        /* USB mode */
        create_thread_ai = pthread_create(&ai_inf_thread, NULL, R_Inf_Thread, NULL);
    }
    if (0 != create_thread_ai)
    {
        sem_trywait(&terminate_req_sem);
        fprintf(stderr, "[ERROR] Failed to create AI Inference Thread\n");
        ret_main = -1;
        goto end_threads;
    }

    /* Create Capture Thread */
    create_thread_capture = pthread_create(&capture_thread, NULL, R_Capture_Thread, NULL);
    if (0 != create_thread_capture)
    {
        sem_trywait(&terminate_req_sem);
        fprintf(stderr, "[ERROR] Failed to create Capture Thread\n");
        ret_main = -1;
        goto end_threads;
    }

    /* Main Processing*/
    main_proc = R_Main_Process();
    if (0 != main_proc)
    {
        fprintf(stderr, "[ERROR] Error during Main Process\n");
        ret_main = -1;
    }
    goto end_threads;

end_threads:
    if (0 == create_thread_capture)
    {
        ret = wait_join(&capture_thread, CAPTURE_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit Capture Thread on time\n");
            ret_main = -1;
        }
    }
    if (0 == create_thread_ai)
    {
        ret = wait_join(&ai_inf_thread, AI_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit AI Inference Thread on time\n");
            ret_main = -1;
        }
    }
    if (0 == create_thread_key)
    {
        ret = wait_join(&kbhit_thread, KEY_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit Key Hit Thread on time\n");
            ret_main = -1;
        }
    }

    /* Delete Terminate Request Semaphore */
    if (0 == sem_create)
    {
        sem_destroy(&terminate_req_sem);
    }

    /* Exit wayland */
    wayland.exit();

    goto end_free_malloc;

end_free_malloc:
    printf("Application End\n");
    return ret_main;
}