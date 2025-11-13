#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MQ_NAME "/adas_audio_mq"
#define MAX_MSG_SIZE 128

int main() {
    mqd_t mq;
    struct mq_attr attr;
    char msg[MAX_MSG_SIZE];

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return -1;
    }

    printf("[INFO] Audio player started.\n");

    while (1) {
        memset(msg, 0, sizeof(msg));
        if (mq_receive(mq, msg, MAX_MSG_SIZE, NULL) >= 0) {
            printf("[INFO] Play audio: %s\n", msg);
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "aplay ../audio/%s.wav", msg);
            system(cmd);
        }
    }
    mq_close(mq);
    mq_unlink(MQ_NAME);
    return 0;
}
