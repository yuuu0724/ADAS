# YOLOv5_RK3588_object_detect

该工程主要用于编译生成可执行程序。**单独运行该可执行程序无法播放音频**，需要结合 `audio_player` 模块才能正常播报音频，但目标检测功能可以正常工作。

---

## 1. 项目代码介绍

| 文件 | 功能说明 |
|------|---------|
| `src/main.cpp` | 主程序运行文件 |
| `src/postprocess.cpp` | 模型推理后的后处理代码 |
| `src/yolov5.cpp` | 模型初始化、推理、反初始化等函数 |
| `include/postprocess.h`, `include/yolov5.h` | 函数声明文件 |
| `imoc_display.cpp`| 手动用 moc生成的文件，用于支持Qt的信号与槽、QObject 类的元信息以及 Q_PROPERTY 等特性、moc display.h -o moc_display.cpp|
| `display.cpp`| Qt显示配置文件|
---

## 2. 配置文件说明

- `3rdparty/`：第三方库  
- `build/`：编译输出目录  
- `inputimage/`：输入图片文件夹  
- `outputimage/`：输出图片文件夹  
- `model/`：RKNN 模型文件及标签名 TXT 文件所在文件夹  
- `rknn_lib/`：瑞芯微官方动态库 `librknnrt.so` 所在位置  
- `per.sh`：配置 CPU 和 NPU 频率为最高的 shell 脚本  

> ⚠️ 注意：执行 `per.sh` 可以提升推理性能。

---

## 3. 编译运行步骤

```bash
# 进入编译目录
cd build

# 配置工程
cmake ..

# 编译生成可执行文件
make

# 运行可执行程序
./myADAS
