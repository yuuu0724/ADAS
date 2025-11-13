#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

pid_t pid_audio = 0;
pid_t pid_seatbelt = 0;
pid_t pid_pedestrian = 0;

void cleanup(int signum)
{
    printf("\n[INFO] Caught signal %d, terminating child processes...\n", signum);
    if (pid_audio > 0) kill(pid_audio, SIGTERM);
    if (pid_seatbelt > 0) kill(pid_seatbelt, SIGTERM);
    if (pid_pedestrian > 0) kill(pid_pedestrian, SIGTERM);
    exit(0);
}

int main(int argc, char *argv[])
{
    // 捕捉 Ctrl+C
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // 启动音频播放进程
    pid_audio = fork();
    if (pid_audio < 0)
    {
        perror("[ERROR] fork() failed for audio process");
        return -1;
    }
    else if (pid_audio == 0)
    {
        printf("[INFO] Audio process started (PID %d)\n", getpid());
        execl("./audio_player",
              "./audio_player", NULL);
        perror("[ERROR] execl failed for audio_process");
        exit(-1);
    }

    sleep(1); // 给音频进程初始化 MQ 留时间

    // 启动行人检测程序
    pid_pedestrian = fork();
    if (pid_pedestrian < 0)
    {
        perror("[ERROR] fork() failed for pedestrian process");
        return -1;
    }
    else if (pid_pedestrian == 0)
    {
        printf("[INFO] Pedestrian process started (PID %d)\n", getpid());
        if (argc >= 2)
        {
            execl("../video/pedestrain.mp4",
                  "../video/pedestrain.mp4", argv[1], NULL);
        }
        else
        {
            execl("./pedestrian",
                  "./pedestrian", NULL);
        }
        perror("[ERROR] execl failed for pedestrian_app");
        exit(-1);
    }

    // 启动安全带检测程序
    pid_seatbelt = fork();
    if (pid_seatbelt < 0)
    {
        perror("[ERROR] fork() failed for seatbelt process");
        return -1;
    }
    else if (pid_seatbelt == 0)
    {
        printf("[INFO] Seatbelt process started (PID %d)\n", getpid());
        if (argc >= 2)
        {
            execl("../video/seatbelt.mp4",
                  "../video/seatbelt.mp4", argv[1], NULL);
        }
        else
        {
            execl("./seatbelt",
                  "./seatbelt", NULL);
        }
        perror("[ERROR] execl failed for seatbelt_app");
        exit(-1);
    }

    // 父进程等待所有子进程退出
    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0)
    {
        printf("[INFO] Process %d exited with status %d\n", wpid, status);
    }

    printf("[INFO] All child processes have exited.\n");
    return 0;
}
