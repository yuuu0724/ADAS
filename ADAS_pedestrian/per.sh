# #!/bin/bash
# # ================================================
# # RK3588 全性能模式脚本（CPU + GPU + NPU）
# # Tested on: LubanCat / Firefly / Rock 5B / Orange Pi 5
# # ================================================

# echo "🚀 正在将 RK3588 CPU、GPU、NPU 全部设置为性能模式..."

# # ---- CPU ----
# if ls /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null 2>&1; then
#     for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
#         echo performance > $i 2>/dev/null
#     done
#     echo "✅ CPU 已切换到 performance 模式"
# else
#     echo "⚠️ 未检测到 CPU 调速文件"
# fi

# # ---- GPU ----
# GPU_PATH="/sys/class/devfreq/fb000000.gpu"
# if [ -d "$GPU_PATH" ]; then
#     echo performance > $GPU_PATH/governor 2>/dev/null
#     # 可选：手动锁最高频率
#     if [ -f "$GPU_PATH/available_frequencies" ]; then
#         MAX_FREQ=$(cat $GPU_PATH/available_frequencies | tr ' ' '\n' | sort -n | tail -1)
#         echo userspace > $GPU_PATH/governor
#         echo $MAX_FREQ > $GPU_PATH/min_freq
#         echo $MAX_FREQ > $GPU_PATH/max_freq
#         echo $MAX_FREQ > $GPU_PATH/userspace/set_freq
#     fi
#     echo "✅ GPU 已锁定最高频率"
# else
#     echo "⚠️ 未检测到 GPU 节点"
# fi

# # ---- NPU ----
# NPU_PATH="/sys/class/devfreq/fdab0000.npu"
# if [ -d "$NPU_PATH" ]; then
#     echo performance > $NPU_PATH/governor 2>/dev/null
#     if [ -f "$NPU_PATH/available_frequencies" ]; then
#         MAX_FREQ=$(cat $NPU_PATH/available_frequencies | tr ' ' '\n' | sort -n | tail -1)
#         echo userspace > $NPU_PATH/governor
#         echo $MAX_FREQ > $NPU_PATH/min_freq
#         echo $MAX_FREQ > $NPU_PATH/max_freq
#         echo $MAX_FREQ > $NPU_PATH/userspace/set_freq
#     fi
#     echo "✅ NPU 已锁定最高频率"
# else
#     echo "⚠️ 未检测到 NPU 节点"
# fi

# echo "✅ 全部设置完成！系统已进入全性能模式。"



# 请切换到root用户

# CPU定频
echo "CPU0-3可用频率/CPU6-7 available frequency:"
sudo cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies
sudo echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
sudo echo 1800000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
echo "CPU0-3当前频率/CPU0-3 current frequency:"
sudo cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq

echo "CPU4-5可用频率/CPU6-7 available frequency:"
sudo cat /sys/devices/system/cpu/cpufreq/policy4/scaling_available_frequencies
sudo echo userspace > /sys/devices/system/cpu/cpufreq/policy4/scaling_governor
sudo echo 2400000 > /sys/devices/system/cpu/cpufreq/policy4/scaling_setspeed
echo "CPU4-5 当前频率/CPU4-5 current frequency:"
sudo cat /sys/devices/system/cpu/cpufreq/policy4/cpuinfo_cur_freq

echo "CPU6-7可用频率:/CPU6-7 available frequency"
sudo cat /sys/devices/system/cpu/cpufreq/policy6/scaling_available_frequencies
sudo echo userspace > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor
sudo echo 2400000 > /sys/devices/system/cpu/cpufreq/policy6/scaling_setspeed
echo "CPU6-7 当前频率/CPU6-7 current frequency:"
sudo cat /sys/devices/system/cpu/cpufreq/policy6/cpuinfo_cur_freq

# NPU定频
echo "NPU可用频率/NPU available frequency:"
sudo cat /sys/class/devfreq/fdab0000.npu/available_frequencies    
sudo echo userspace > /sys/class/devfreq/fdab0000.npu/governor
sudo echo 1000000000 > /sys/class/devfreq/fdab0000.npu/userspace/set_freq
echo "NPU当前频率/NPU current frequency:"
sudo cat /sys/class/devfreq/fdab0000.npu/cur_freq