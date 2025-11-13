#include <pthread.h>
#include <opencv2/opencv.hpp>
#include "yolov5.h"
#include "image_drawing.h"
#include <chrono>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cmath>
#include <threadpool.h>
#include <mqueue.h>

using namespace std;
using namespace cv;
using namespace chrono;

queue<Mat> frame_queue;
queue<Mat> display_queue;

static pthread_mutex_t frame_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t frame_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t display_cond = PTHREAD_COND_INITIALIZER;

atomic<bool> system_exit(false);
static auto last_alarm_time = steady_clock::now() - seconds(10);

#define NPU_CORE_NUM 3
#define MQ_NAME "/adas_audio_mq"
#define MAX_MSG_SIZE 128

threadpool_t pool;
rknn_app_context_t yolov5_ctx[NPU_CORE_NUM];
atomic<int> npu_index(0);

bool use_camera = true;
string video_path = "";

// 使用消息队列发送音频指令
void enqueue_audio(const string &audio_name)
{
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return;
    }
    char msg[MAX_MSG_SIZE];
    strncpy(msg, audio_name.c_str(), sizeof(msg)-1);
    msg[sizeof(msg)-1] = 0;
    mq_send(mq, msg, strlen(msg)+1, 0);
    mq_close(mq);
}

void *Camera_Capture_Thread(void *arg)
{
    VideoCapture cap;
    if (use_camera)
    {
        cap.open(3);
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cout << "[INFO] 使用摄像头输入" << endl;
    }
    else
    {
        cap.open(video_path);
        if (!cap.isOpened())
        {
            cerr << "[ERROR] 无法打开视频文件: " << video_path << endl;
            system_exit = true;
            return NULL;
        }
        cout << "[INFO] 使用视频文件: " << video_path << endl;
    }

    if (!cap.isOpened()) return NULL;

    while (!system_exit)
    {
        Mat frame;
        Mat mirrored_frame;
        cap >> frame;
        if (frame.empty()) continue;
        flip(frame, mirrored_frame, 1);

        pthread_mutex_lock(&frame_mutex);
        if (frame_queue.size() > 5) frame_queue.pop();
        frame_queue.push(mirrored_frame);
        pthread_cond_signal(&frame_cond);
        pthread_mutex_unlock(&frame_mutex);

        usleep(30000);
    }
    return NULL;
}

typedef struct
{
    rknn_app_context_t *ctx;
    image_buffer_t img;
} inference_arg_t;

void run_inference(void *arg)
{
    inference_arg_t *inf = (inference_arg_t *)arg;
    object_detect_result_list od_results;

    if (inference_yolov5_model(inf->ctx, &inf->img, &od_results) != 0)
    {
        free(inf->img.virt_addr);
        free(inf);
        return;
    }

    // 绘制检测框
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result &det = od_results.results[i];
        draw_rectangle(&inf->img, det.box.left,
                       det.box.top,
                       det.box.right - det.box.left,
                       det.box.bottom - det.box.top,
                       COLOR_GREEN, 5);
        const char *cls_name = coco_cls_to_name(det.cls_id);
        char text[128];
        sprintf(text, "%s %.1f%%", cls_name, det.prop * 100);
        draw_text(&inf->img, text, det.box.left, det.box.top - 20, COLOR_RED, 30);

        if (strcmp(cls_name, "pedestrain") == 0)
        {
            auto now = steady_clock::now();
            if (duration_cast<seconds>(now - last_alarm_time).count() > 5)
            {
                last_alarm_time = now;
                enqueue_audio("pedestrian");  // 发送消息队列
                cout << "[ALARM] 注意行人！" << endl;
            }
        }
    }

    // 推送 display_queue
    Mat display_frame(inf->img.height, inf->img.width, CV_8UC3, inf->img.virt_addr);
    Mat bgr_frame;
    cvtColor(display_frame, bgr_frame, COLOR_RGB2BGR);

    pthread_mutex_lock(&display_mutex);
    if (display_queue.size() > 5) display_queue.pop();
    display_queue.push(bgr_frame.clone());
    pthread_cond_signal(&display_cond);
    pthread_mutex_unlock(&display_mutex);

    free(inf->img.virt_addr);
    free(inf);
}

void *Seat_belt_detection_thread(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    while (!system_exit)
    {
        Mat frame;
        pthread_mutex_lock(&frame_mutex);
        while (frame_queue.empty() && !system_exit)
            pthread_cond_wait(&frame_cond, &frame_mutex);
        if (system_exit) {
            pthread_mutex_unlock(&frame_mutex);
            break;
        }
        frame = frame_queue.back();
        frame_queue.pop();
        pthread_mutex_unlock(&frame_mutex);

        Mat rgb_frame;
        cvtColor(frame, rgb_frame, COLOR_BGR2RGB);

        inference_arg_t *inf_arg = (inference_arg_t *)malloc(sizeof(inference_arg_t));
        int cur_npu = npu_index++ % NPU_CORE_NUM;
        inf_arg->ctx = &yolov5_ctx[cur_npu];
        inf_arg->img.format = IMAGE_FORMAT_RGB888;
        inf_arg->img.width = rgb_frame.cols;
        inf_arg->img.height = rgb_frame.rows;
        inf_arg->img.size = rgb_frame.total() * rgb_frame.elemSize();
        inf_arg->img.virt_addr = (unsigned char *)malloc(inf_arg->img.size);
        memcpy(inf_arg->img.virt_addr, rgb_frame.data, inf_arg->img.size);

        threadpool_add(pool, run_inference, inf_arg);
    }
    return NULL;
}

void *DisplayThread(void *arg)
{
    static auto last_time = steady_clock::now();
    static int frame_count = 0;
    static double fps = 0.0;
    Mat frame;

    const int display_interval_ms = 16; // ~60 FPS
    while (!system_exit)
    {
        pthread_mutex_lock(&display_mutex);
        if (!display_queue.empty()) {
            frame = display_queue.back().clone();
            while (display_queue.size() > 0) display_queue.pop();
        }
        pthread_mutex_unlock(&display_mutex);

        if (frame.empty()) {
            usleep(5000);
            continue;
        }

        frame_count++;
        imshow("Pedestrian ADAS", frame);

        auto now = steady_clock::now();
        double elapsed = duration_cast<duration<double>>(now - last_time).count();
        if (elapsed >= 1.0) {
            fps = frame_count / elapsed;
            frame_count = 0;
            last_time = now;
        }

        char fps_text[64];
        sprintf(fps_text, "FPS: %.1f", fps);
        putText(frame, fps_text, Point(20, 40), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);

        if (waitKey(display_interval_ms) == 27) {
            system_exit = true;
            break;
        }
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    if (argc >= 2) { use_camera = false; video_path = argv[1]; }

    cout << "[INFO] 初始化 YOLOV5 模型..." << endl;
    init_post_process();
    for (int i = 0; i < NPU_CORE_NUM; i++)
        if (init_yolov5_model("../model/pedestrian_best.rknn", &yolov5_ctx[i]) != 0) {
            cerr << "[ERROR] 模型初始化失败！" << endl;
            return -1;
        }

    pthread_t Camera_Capture_Thread_tid;
    pthread_t Seat_belt_detection_thread_tid;
    pthread_t DisplayThread_tid;

    threadpool_init(&pool);

    pthread_create(&Camera_Capture_Thread_tid, NULL, Camera_Capture_Thread, NULL);
    pthread_create(&Seat_belt_detection_thread_tid, NULL, Seat_belt_detection_thread, (void *)&pool);
    pthread_create(&DisplayThread_tid, NULL, DisplayThread, NULL);

    pthread_join(Camera_Capture_Thread_tid, NULL);
    pthread_join(Seat_belt_detection_thread_tid, NULL);
    pthread_join(DisplayThread_tid, NULL);

    for (int i = 0; i < NPU_CORE_NUM; i++)
        release_yolov5_model(&yolov5_ctx[i]);

    return 0;
}
