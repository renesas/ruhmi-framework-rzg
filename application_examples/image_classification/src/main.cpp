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
* Description  : RZ/G3E Sample AI Application (Image Classification)
 ***********************************************************************************************************************/

/*****************************************
 * Includes
 ******************************************/
/* Definition of Macros & other variables */
#include "define.h"
/* Wayland control*/
#include "wayland.h"
/* Definition for ruhmi runtime wrapper */
#include "MeraRuntimeEthosWrapper.h"

/*****************************************
 * Global Variables
 ******************************************/

/* Multithreading */
static sem_t terminate_req_sem;
static sem_t producer;
static sem_t consumer;
static pthread_t ai_inf_thread;
static pthread_t capture_thread;
static pthread_t exit_thread;
static pthread_t kbhit_thread;
static std::mutex mtx;

/* Flags */
static std::atomic<bool> inference_start{false};
static std::atomic<bool> img_obj_ready{false};
static std::atomic<bool> inference_done{false};

/* Global Variables */
std::string model_file_name;

/* App mode */
enum class AppMode { USB, IMAGE };
static AppMode app_mode;

static Wayland wayland;

std::map<float,int> result;

static struct timespec start_time;
static struct timespec end_time;
static float total_time = 0;
static bool fps_inited = false;
static int fps_count = 0;
const int window_num = 10; 
static std::atomic<float> fps(0.0f);

cv::Mat bgra_image;
cv::Mat yuyv_image;
cv::Mat input_image;
cv::Mat frame_g;

static float font_scale = 0.8f;
static float font_thickness = 1.0f;

constexpr float INPUT_SCALE = 0.00784314f;
constexpr int INPUT_ZERO_POINT = -1;
constexpr float OUTPUT_SCALE = 0.00390625f;
constexpr int OUTPUT_ZERO_POINT = -128;

static MeraRuntimeEthosWrapper runtime;
static ModelConfig model_config;

/*****************************************
 * Function Name     : float_to_string
 * Description       : Convert float to string with precision
 * Arguments         : number = float number to be converted
 *                     precision = int number to set precision
 * Return value      : string = string number
 ******************************************/
std::string float_to_string(float number, int precision = 2)
{
    std::stringstream stream;  
    stream.precision(precision);
    stream << std::fixed << number;  
    return stream.str();
}

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
    if (0 == ret_err)
    {
        join_timeout.tv_sec += join_time;
        ret_err = pthread_timedjoin_np(*p_join_thread, NULL, &join_timeout);
    }
    return ret_err;
}

/*****************************************
 * Function Name     : load_label_file
 * Description       : Load label list text file and return the label list that contains the label.
 * Arguments         : w_label_file_name = filename of label list. must be in txt format
 * Return value      : vector<string> list = list contains labels
 *                     empty if error occurred
 ******************************************/
std::vector<std::string> load_label_file(std::string w_label_file_name)
{
    std::vector<std::string> list = {};
    std::vector<std::string> empty = {};
    std::ifstream infile(w_label_file_name);

    if (!infile.is_open())
    {
        return list;
    }

    std::string line = "";
    while (getline(infile, line))
    {
        list.push_back(line);
        if (infile.fail())
        {
            return empty;
        }
    }

    return list;
}


/*****************************************
 * Function Name : R_exit_Thread
 * Description   : Executes the exit thread
 * Arguments     : threadid = thread identification
 * Return value  : -
 ******************************************/
void *R_exit_Thread(void *threadid)
{
    /* Semaphore Variable*/
    int32_t kh_sem_check = 0;
    /* Variable for checking return value */
    int8_t ret = 0;

    /* Set Standard Input to Non Blocking */
    errno = 0;
    ret = fcntl(0, F_SETFL, O_NONBLOCK);
    if (-1 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to run fctnl(): errno=%d\n", errno);
        goto err;
    }

    while (1)
    {
        /* Get the Termination request semaphore value. If different then 1 Termination was requested*/
        /* Checks if sem_getvalue is executed wihtout issue */
        ret = sem_getvalue(&terminate_req_sem, &kh_sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /* Checks the semaphore value */
        if (1 != kh_sem_check)
        {
            break;
        }
        usleep(10000);
    }

/* Error Processing */
err:
    /* Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto exit_end;

exit_end:
    printf("Exit Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
 * Function Name : R_Capture_Thread
 * Description   : Executes the V4L2 capture with Capture thread.
 * Arguments     : cap_pipeline = gstreamer pipeline
 * Return value  : -
 ******************************************/
void *R_Capture_Thread(void *cap_pipeline)
{
    std::string &gstream = *(static_cast<std::string *>(cap_pipeline));
    /* Semaphore Variable */
    int32_t capture_sem_check = 0;
    int8_t ret = 0;
    cv::Mat g_frame;
    cv::VideoCapture g_cap;

    printf("[INFO] Capture Thread Starting\n");

    g_cap.open(gstream, cv::CAP_GSTREAMER);
    
    if (!g_cap.isOpened())
    {
        std::cout << "[ERROR] Error opening video stream or camera !\n"
                  << std::endl;
        goto err;
    }

    while (1)
    {
        /* Get the Termination request semaphore value. If different then 1 Termination was requested */
        /* Checks if sem_getvalue is executed without issue */
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

        /* Image mode: Through the first frame and display it. After that, keep waiting. */
        if ((app_mode == AppMode::IMAGE) && (inference_done.load()))
        {
            usleep(WAIT_TIME);
            continue;
        }
        g_cap >> g_frame;

        /* Breaking the loop if no video frame is detected */
        if (g_frame.empty())
        {
            std::cout << "[INFO] Video ended or corrupted frame !\n";
            goto err;
        }
        else
        {
            if (!inference_start.load())
            {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    input_image = g_frame.clone();
                }
                inference_start.store(true); /* Flag for AI Inference Thread. */
            }

            if (!img_obj_ready.load())
            {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    yuyv_image = g_frame.clone();
                }
                img_obj_ready.store(true); /* Flag for Main Thread. */
            }
            if (app_mode == AppMode::IMAGE)
            {
                inference_done.store(true);
            }
        }
        usleep(WAIT_TIME);
    }

/* Error Processing */
err:
    sem_trywait(&terminate_req_sem);
    goto capture_end;

capture_end:
    /* To terminate the loop in AI Inference Thread.*/
    inference_start.store(true);

    printf("Capture Thread Terminated\n");
    pthread_exit(NULL);
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

    const int min_size = std::min(img.cols, img.rows);
    int offsetX = (img.cols - min_size) / 2;
    int offsetY = (img.rows - min_size) / 2;
    int x = std::max(0, std::min(offsetX, img.cols - min_size));
    int y = std::max(0, std::min(offsetY, img.rows - min_size));
    cv::Rect roi(x, y, min_size, min_size);
    img = img(roi);

    cv::Size size(MODEL_IN_H, MODEL_IN_W);
    cv::resize(img, img, size);
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    img.convertTo(img, CV_32FC3, 1.0/255.0, 0.0);
    img = img * 2.0f - 1.0f;
    img = img / INPUT_SCALE + static_cast<float>(INPUT_ZERO_POINT);

    /* Prepare input buffer to mera runtime */
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
    auto* input_ptr = reinterpret_cast<int8_t*>(runtime.GetInputPtr(in_name));
    if (!input_ptr)
    {
        fprintf(stderr, "[ERROR] Failed to get input buffer pointer\n");
        return false;
    }
    cv::Mat runtime_in(MODEL_IN_H, MODEL_IN_W, CV_8SC3, input_ptr); 
    img.convertTo(runtime_in, CV_8SC3);

    return true;
}

/*****************************************
* Function Name : postprocess_output
* Description   : Execute post-process and parse output tensor
* Arguments     : -
* Return value  : true if succeeded
*                 false 0 otherwise
******************************************/
static bool postprocess_output()
{
    std::vector<float> output_arr(1);

    /* Get output from ruhmi runtime */
    auto output_info = runtime.GetOutputInfo();
    auto [out_name, out_size, out_type] = output_info[0];
    int8_t* data_ptr = reinterpret_cast<int8_t*>(runtime.GetOutputPtr(out_name));
    size_t output_size_arr = static_cast<size_t>(out_size);

    if (out_type != InOutDataType::INT8)
    {
        fprintf(stderr, "[ERROR] Invalid input data type\n");
        return false;
    }
    output_arr.resize(output_size_arr);
    for (int n = 0; n < output_size_arr; n++)
    {
        /* Cast INT8 output data to FP32.*/
        output_arr[n] = int8_to_float32(data_ptr[n], OUTPUT_SCALE, OUTPUT_ZERO_POINT);
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        result.clear();
        for (int n = 0; n < output_size_arr; n++)
        {
            result[output_arr[n]] = n;
        }
    }
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
    /* Semaphore Variable*/
    int32_t inf_sem_check = 0;
    /* Variable for checking return value */
    int8_t ret = 0;
    std::streamsize model_size;
    std::vector<unsigned char> model_data;
    std::vector<float> output_arr(1);

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
    
    sem_wait(&producer);
    /* Get Pre-process starting time*/
    ret = timespec_get(&start_time, TIME_UTC);
    if (0 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to get AI process Start Time\n");
        goto err;
    }
    for (int fps_count = 0; fps_count < window_num; fps_count++)
    {
        /* Pre-process */
        if (!preprocess_input(input_image))
        {
            fprintf(stderr, "[ERROR] Failed to preprocess input.\n");
            goto err;
        }
        /* Run inference */
        runtime.Run();
        /* Post-process */
        if (!postprocess_output())
        {
            fprintf(stderr, "[ERROR] Failed to Post Process.\n");
            goto err;
        }
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
    sem_post(&consumer);
    inference_start.store(false);
    pthread_exit(NULL);

/* Error Processing */
err:
    /* Set Termination Request Semaphore to 0 */
    sem_trywait(&terminate_req_sem);
    goto ai_inf_end;
/* AI Thread Termination*/
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
    /* Semaphore Variable*/
    int32_t inf_sem_check = 0;
    /* Variable for checking return value */
    int8_t ret = 0;
    /* Variable for Performance Measurement*/
    static struct timespec start_time;
    static struct timespec inf_end_time;
    static struct timespec pre_start_time;
    static struct timespec pre_end_time;
    static struct timespec post_start_time;
    static struct timespec post_end_time;
    std::streamsize model_size;
    std::vector<unsigned char> model_data;

    std::vector<float> output_arr(1);

    printf("[INFO] Inference Thread Starting\n");

    /*Inference Loop Start*/
    while(1)
    {
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
        sem_wait(&producer);
        
        /* Pre-process */
        printf("[DEBUG] Start pre-process\n");
        if (!preprocess_input(input_image))
        {
            fprintf(stderr, "[ERROR] Failed to preprocess input.\n");
            goto err;
        } 
        runtime.Run();
        /* Post-process */
        printf("[DEBUG] Start post-process\n");
        if (!postprocess_output())
        {
            fprintf(stderr, "[ERROR] Failed to Post Process.\n");
            goto err;
        }
        inference_start.store(false);

        sem_post(&consumer);        
    }
    /* End of Inference Loop*/

/* Error Processing */
err:
    /* Set Termination Request Semaphore to 0 */
    sem_trywait(&terminate_req_sem);
    goto ai_inf_end;
/* AI Thread Termination*/
ai_inf_end:
    /* To terminate the loop in Capture Thread.*/
    printf("[INFO] AI Inference Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
 * Function Name    : create_output_frame
 * Description      : create the output frame with space for displaying inference details
 * Arguments        : cv::Mat frame_g, input frame to be displayed in the background
 * Return value     : cv::Mat background, final display frame to be written to gstreamer pipeline
 *****************************************/
cv::Mat create_output_frame(cv::Mat frame_g)
{
    /* Create a black background image of size 1080x720 */
    cv::Mat background(DISP_OUTPUT_HEIGHT, DISP_OUTPUT_WIDTH, frame_g.type(), display_color::Black);
    /* Resize the original image to fit within 960x720 */
    cv::Mat resizedImage;
    cv::resize(frame_g, resizedImage, cv::Size(DISP_IMAGE_OUTPUT_WIDTH, DISP_IMAGE_OUTPUT_HEIGHT));
    /* Copy the resized image to the left side of the background (0 to 960) */
    cv::Rect roi(cv::Rect(0, 0, resizedImage.cols, resizedImage.rows));
    resizedImage.copyTo(background(roi));
    return background;
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
    /* Semaphore Related*/
    int32_t sem_check = 0;
    /* Variable for checking return value */
    int8_t ret = 0;
    uint8_t str_count = 0;
    std::string file_name = "";
    std::string class_name;
    int result_cnt = 0;
    float re_fl;

    /* Initialize wayland */
    ret = wayland.init(IMAGE_OUTPUT_WIDTH, IMAGE_OUTPUT_HEIGHT, IMAGE_OUTPUT_CHANNEL_BGRA);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to initialize Image for Wayland\n");
        goto err;
    }

    printf("Main Loop Starts\n");
    while (1)
    {
        /* Get the Termination request semaphore value. If different then 1 Termination was requested */
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
        /* Check img_obj_ready flag which is set in Capture Thread. */
        if (img_obj_ready.load())
        {
            sem_wait(&consumer);
            bgra_image = yuyv_image;
            result_cnt = 0;
            bgra_image = create_output_frame(bgra_image);
            cv::putText(bgra_image, "Model : mobilenet_v1_0.25", cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 5, 260), cv::FONT_HERSHEY_DUPLEX, font_scale, display_color::White, font_thickness); 
            
            /* Display FPS */
            float fps_local = fps.load(std::memory_order_relaxed);
            if (fps_local != 0)
            {
                if (app_mode == AppMode::IMAGE)
                {
                    /* Image Mode */
                    cv::putText(bgra_image, "FPS (Pre-Inference-Post): ", cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 5, 300), cv::FONT_HERSHEY_DUPLEX, font_scale, display_color::White, font_thickness);
                    cv::putText(bgra_image, float_to_string(fps_local, 1), cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 30, 340), cv::FONT_HERSHEY_DUPLEX, font_scale, display_color::White, font_thickness);
                }
                else
                {
                    /* USB Mode */
                    cv::putText(bgra_image, "FPS (End-to-End) : ", cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 5, 300), cv::FONT_HERSHEY_DUPLEX, font_scale, display_color::White, font_thickness);
                    cv::putText(bgra_image, float_to_string(fps_local, 1), cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 30, 340), cv::FONT_HERSHEY_DUPLEX, font_scale, display_color::White, font_thickness);
                }
            }

            /* Display Classification Result (top5) */
            cv::putText(bgra_image, "Classification Results:", cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 5, 420), cv::FONT_HERSHEY_SIMPLEX, font_scale, display_color::White, font_thickness);
            {
                std::lock_guard<std::mutex> lock(mtx);
                for (auto it = result.rbegin(); it != result.rend(); it++)
                {
                    result_cnt++;
                    if(result_cnt > 5) break;
                    re_fl = (float)(*it).first*100;
                    if (re_fl != re_fl) {continue;}
                    re_fl = std::round(re_fl);
                    int val = ceil(re_fl);
                    class_name = label_file_map[(*it).second];
                    std::cout << "Top " << result_cnt << " ["
                                      << std::right << std::setw(5) << std::fixed << std::setprecision(1) << re_fl
                                      << "% ] : [" << label_file_map[(*it).second] << "]" << std::endl;
                    cv::putText(bgra_image,"Top "+std::to_string(result_cnt)+" ["+std::to_string(val)+"%] : ["+class_name+"]", cv::Point(DISP_IMAGE_OUTPUT_WIDTH + 15, 420 + result_cnt * 40), cv::FONT_HERSHEY_SIMPLEX, font_scale - 0.3, display_color::White, font_thickness);
                    if (result_cnt == 1)
                    {   
                        if(val >= threshold*100)
                        {
                            cv::putText(bgra_image, "Class: "+ std::string(class_name), cv::Point(40, 65), cv::FONT_HERSHEY_SIMPLEX, font_scale, display_color::Green, font_thickness + 1);
                            cv::putText(bgra_image, "Score: "+ float_to_string(re_fl) +"%", cv::Point(40, 113), cv::FONT_HERSHEY_SIMPLEX, font_scale, display_color::Green, font_thickness +1);  
                        }
                        else
                        {
                            cv::putText(bgra_image,"Cannot Identify ! ",cv::Point(40, 60), cv::FONT_HERSHEY_SIMPLEX, font_scale, display_color::Red, font_thickness + 2);
                        }
                    }
                }
            }

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

            /* Display image */
            cv::cvtColor(bgra_image, bgra_image, cv::COLOR_BGR2BGRA);
            wayland.commit(bgra_image.data, NULL);
            
            img_obj_ready.store(0);
            sem_post(&producer);
        }
        /* Wait for 1 TICK */
        usleep(WAIT_TIME);
    }

/* Error Processing */
err:
    sem_trywait(&terminate_req_sem);
    main_ret = 1;
    goto main_proc_end;
/* Main Processing Termination*/
main_proc_end:
    /* To terminate the loop in Capture Thread.*/
    img_obj_ready.store(0);
    printf("Main Process Terminated\n");
    return main_ret;
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
    const char *command = "v4l2-ctl --list-devices";
    /* Open a pipe to the command and execute it */
    FILE *pipe = popen(command, "r");
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
    /* Semaphore Variable*/
    int32_t kh_sem_check = 0;
    /* Variable to store the getchar() value */
    int32_t c = 0;
    /* Variable for checking return value */
    int8_t ret = 0;

    printf("[INFO] Key Hit Thread Starting\n");

    printf("************************************************\n");
    printf("* Press ENTER key to quit. *\n");
    printf("************************************************\n");

    /* Set Standard Input to Non Blocking */
    errno = 0;
    ret = fcntl(0, F_SETFL, O_NONBLOCK);
    if (-1 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to run fctnl(): errno=%d\n", errno);
        goto err;
    }

    while (1)
    {
        /* Get the Termination request semaphore value. If different then 1 Termination was requested*/
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

/* Error Processing */
err:
    /* Set Termination Request Semaphore to 0 */
    sem_trywait(&terminate_req_sem);
    goto key_hit_end;

key_hit_end:
    printf("Key Hit Thread Terminated\n");
    pthread_exit(NULL);
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
        << " ./image_classification USB\n"
        << " ./image_classification IMAGE <image_file> (supported: .jpg/.jpeg/.png)\n";
}

/*****************************************
 * Function Name : parse_arg
 * Description   : Parse command-line arguments and set application mode and input pipeline.
 * Arguments     : argc = number of command-line arguments
 *                 argv = command-line argument values
 * Return value  : true  - valid arguments 
                   false - invalid arguments
 ******************************************/
bool parse_arg (int32_t argc, char * argv[], std::string& gstreamer_pipeline)
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
         gstreamer_pipeline = "v4l2src device=" + media_port + " ! videoconvert ! appsink";
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
            gstreamer_pipeline = "filesrc location=" + image_path + " ! jpegdec ! imagefreeze ! videoconvert ! videoscale ! video/x-raw, format=BGR, width=640, height=480 ! appsink";
        }
        else if (dotPos != std::string::npos && (extension =="png"|| extension =="PNG"))
        {
            gstreamer_pipeline = "filesrc location=" + image_path + " ! pngdec ! imagefreeze ! videoconvert ! videoscale ! video/x-raw, format=BGR, width=640, height=480 ! appsink";
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

int32_t main(int32_t argc, char *argv[])
{
    int8_t main_proc = 0;
    int8_t ret = 0;
    int8_t ret_main = 0;
    /* Multithreading Variables */
    int32_t create_thread_capture = -1;
    int32_t create_thread_ai      = -1;
    int32_t create_thread_exit = -1;
    int32_t create_thread_key = -1;
    int32_t sem_create = -1;
    int32_t sem_producer = -1;
    int32_t sem_consumer = -1;
    /* GStreamer pipeline for camera capture */
    static std::string gstreamer_pipeline = "";
    /* Define model and label */
    model_config.model_dir = "model_mobilenetv1/mobilenet_v1_0.25/build/IP/compilation";
    model_config.label_file = "labels_mobilenet_v1.txt";
    
    /* parse arguments and build pipeline */
    if (!parse_arg(argc, argv, gstreamer_pipeline))
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

    /* Load Label from label_list file */
    printf("[INFO] Trying to load label file\n");
    label_file_map = load_label_file(model_config.label_file);
    if (label_file_map.empty())
    {
        std::cout << "\n[ERROR] Failed to load label file." << std::endl;
        return -1;
    }

    printf("Start RZ/G3E Sample Application (Image classification)\n");

    /* Termination Request Semaphore Initialization*/
    /*Initialized value at 1.*/
    sem_create = sem_init(&terminate_req_sem, 0, 1);
    if (0 != sem_create)
    {
        fprintf(stderr, "[ERROR] Failed to Initialize Termination Request Semaphore\n");
        ret_main = -1;
        goto end_threads;
    }

    sem_producer = sem_init(&producer, 0, 1);
    if (0 != sem_producer)
    {
        fprintf(stderr, "[ERROR] Failed to producer Semaphore\n");
        ret_main = -1;
        goto end_threads;
    }

    sem_consumer = sem_init(&consumer, 0, 0);
    if (0 != sem_consumer)
    {
        fprintf(stderr, "[ERROR] Failed to consumer Semaphore\n");
        ret_main = -1;
        goto end_threads;
    }

    /* Create Capture Thread*/
    create_thread_capture = pthread_create(&capture_thread, NULL, R_Capture_Thread, (void *)&gstreamer_pipeline);
    if (0 != create_thread_capture)
    {
        sem_trywait(&terminate_req_sem);
        fprintf(stderr, "[ERROR] Failed to create Capture Thread\n");
        ret_main = -1;
        goto end_threads;
    }

     /* Create Inference Thread */
    if (app_mode == AppMode::IMAGE)
    {
        /* IMAGE mode */
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

    /* Create exit Thread*/
    create_thread_exit = pthread_create(&exit_thread, NULL, R_exit_Thread, NULL);
    if (0 != create_thread_exit)
    {
        fprintf(stderr, "[ERROR] Failed to create exit Thread\n");
        ret_main = -1;
        goto end_threads;
    }
    /* Detached exit thread */
    pthread_detach(exit_thread);

    /* Create Key Hit Thread*/
    create_thread_key = pthread_create(&kbhit_thread, NULL, R_Kbhit_Thread, NULL);
    if (0 != create_thread_key)
    {
        fprintf(stderr, "[ERROR] Failed to create Key Hit Thread\n");
        ret_main = -1;
        goto end_threads;
    }

    /* Main Processing */
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
        ret = wait_join(&kbhit_thread, EXIT_THREAD_TIMEOUT);
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
    if (0 == sem_producer)
    {
        sem_destroy(&producer);
    }
    if (0 == sem_consumer)
    {
        sem_destroy(&consumer);
    }
    goto end_main;
end_main:
    printf("Application End\n");
    return ret_main;
}
