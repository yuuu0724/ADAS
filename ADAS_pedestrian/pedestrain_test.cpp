#include <iostream>
#include <opencv2/opencv.hpp>
#include "rknn_api.h"

using namespace std;
using namespace cv;

// 模型路径
#define MODEL_PATH "/home/cat/duan/ADAS-pedestrain/model/pedestrain_best.rknn"

// 假设你的模型输入尺寸是 640x480 RGB
#define INPUT_WIDTH 640
#define INPUT_HEIGHT 480
#define INPUT_CHANNEL 3

int main() {
    rknn_context ctx;
    int ret;

    // 1. 初始化 RKNN 模型
    ret = rknn_init(&ctx, MODEL_PATH, 0);
    if (ret != 0) {
        cerr << "[ERROR] rknn_init failed! ret=" << ret << endl;
        return -1;
    }
    cout << "[INFO] RKNN model loaded successfully!" << endl;

    // 2. 打开摄像头
    VideoCapture cap(1);
    if (!cap.isOpened()) {
        cerr << "[ERROR] Cannot open camera!" << endl;
        rknn_destroy(ctx);
        return -1;
    }
    cap.set(CAP_PROP_FRAME_WIDTH, INPUT_WIDTH);
    cap.set(CAP_PROP_FRAME_HEIGHT, INPUT_HEIGHT);

    // 输入缓冲
    Mat frame, rgb_frame;
    uint8_t *input_data = new uint8_t[INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNEL];

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;

        // 转成 RGB
        cvtColor(frame, rgb_frame, COLOR_BGR2RGB);

        // 复制到输入缓冲
        memcpy(input_data, rgb_frame.data, INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNEL);

        // 3. 设置输入
        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].buf = input_data;
        inputs[0].size = INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNEL;
        inputs[0].pass_through = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;

        ret = rknn_inputs_set(ctx, 1, inputs);
        if (ret != 0) {
            cerr << "[ERROR] rknn_inputs_set failed! ret=" << ret << endl;
            break;
        }

        // 4. 推理
        ret = rknn_run(ctx, nullptr);
        if (ret != 0) {
            cerr << "[ERROR] rknn_run failed! ret=" << ret << endl;
            break;
        }

        // 5. 获取输出（假设模型只有一个输出）
        rknn_output outputs[1];
        memset(outputs, 0, sizeof(outputs));
        outputs[0].want_float = 1;

        ret = rknn_outputs_get(ctx, 1, outputs, NULL);
        if (ret != 0) {
            cerr << "[ERROR] rknn_outputs_get failed! ret=" << ret << endl;
            break;
        }

        // 输出结果打印（根据你的模型输出格式自行修改）
        cout << "[INFO] Output data example: " << outputs[0].buf[0] << endl;

        // 释放输出缓冲
        rknn_outputs_release(ctx, 1, outputs);

        // 显示摄像头画面
        imshow("Test Pedestrain RKNN", frame);
        if (waitKey(1) == 27) break; // ESC退出
    }

    delete[] input_data;
    rknn_destroy(ctx);
    return 0;
}
