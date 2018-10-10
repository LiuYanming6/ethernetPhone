/*************************************************************************
> File Name: play.c
> Author: 刘艳明
> Mail: liuyanming@live.cn
> Created Time: 2018年10月10日 星期三 13时40分27秒
> 接收广播发送, 用到了 redis 的 ae 模块， 有点大材小用， select实现可以满足业务需要
************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<net/if.h>
#include <memory.h>
#include <sys/ioctl.h>
#include "ae.h"
#include "net_op.h"
#include "common.h"

#include <alsa/asoundlib.h>

static char *buffer;
static int size;
static snd_pcm_t *handle;
/* Set period size to 32 frames. */
static snd_pcm_uframes_t frames = 32;
static unsigned char currMac[MAC_LEN];
static unsigned char zeroMac[MAC_LEN];

int initSND() {
    int fd = -1;
    int rc;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;

    /* Open PCM device for playback. */
    rc = snd_pcm_open(&handle, "default",
                      SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        fprintf(stderr,
                "unable to open pcm device: %s\n",
                snd_strerror(rc));
        exit(1);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params,
                                 SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, NUM_CHANNEL);

    /* 44100 bits/second sampling rate (CD quality) */
    val = RATE;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                    &val, &dir);

    snd_pcm_hw_params_set_period_size_near(handle,
                                           params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr,
                "unable to set hw parameters: %s\n",
                snd_strerror(rc));
        exit(1);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames,
                                      &dir);
    size = frames * 4 + MAC_LEN; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);


    return 0;
}

int play(char *buf) {
    int rc = -1;
    static unsigned long index = 0;

    rc = snd_pcm_writei(handle, buf, frames);
    if (rc == -EPIPE) {
        /* EPIPE means underrun */
        fprintf(stderr,
                "underrun occurred\n");
        snd_pcm_prepare(handle);
    } else if (rc < 0) {
        fprintf(stderr,
                "error from writei: %s\n",
                snd_strerror(rc)
        );
    } else if (rc != (int) frames) {
        fprintf(stderr,
                "short write, write %d frames\n",
                rc);
    }
    fprintf(stderr, "Write frame index=%ld\n", index++);

    return 0;
}

/*
 * return 2000;  start next 2 second
 *        -1 //stop and execute finalPro
 * 超时，认为这段讲话结束，准备结束新的讲话
 */
int startPro(struct aeEventLoop *eventLoop, long long id, void *clientData) {

    printf("startPro\n");
    memset(currMac, 0, MAC_LEN);

    return -1;
}

/*
 *
 *  aeCreateTimeEvent(el, 5000, startPro, NULL, finalPro);
 */
void finalPro(struct aeEventLoop *eventLoop, void *clientData) {

    printf("finalPro\n");

    return;
}

void acceptUdpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int iLen = -1;
    char *buf = NULL;

    iLen = read(fd, buffer, size);
    if (iLen == 0) {
        fprintf(stderr, "end of file on input\n");
        return;
    } else if (iLen != size) {
        fprintf(stderr,
                "short read: read %d bytes\n",
                iLen);
    }

    if (!memcmp(buffer, getInterMac(), MAC_LEN)){
        // from self, drop
        printf("drop\n");
        goto finish;
    } else if (!memcmp(currMac, zeroMac, MAC_LEN)) {
        // first voice in, lock it.
        printf("first voice\n");
        memcpy(currMac, buffer, MAC_LEN);
    } else if (memcmp(currMac, buffer, MAC_LEN)) {
        // this is already a node speaking, drop it.
        printf("not first voice\n");
        goto finish;
    }

    buf = buffer + MAC_LEN;
    play(buf);

    if (el->timeEventNextId > 0) {
        aeDeleteTimeEvent(el, el->timeEventNextId - 1);
        el->timeEventNextId = 0;
    }
    aeCreateTimeEvent(el, 2000, startPro, NULL, NULL);
    printf("timeEventNextId %lld\n", el->timeEventNextId);

finish:
    memset(buffer, 0, size);

    return;
}

int main() {
    int serverfd = 0;

    initSND();

    aeEventLoop *el = aeCreateEventLoop(9);
    if (el == NULL) {
        printf("aeCreateEventLoop failed\n");
    }

    // 监听UDP 60000 input
    serverfd = Net_bind();
    if (aeCreateFileEvent(el, serverfd, AE_READABLE,
                          acceptUdpHandler, NULL) == AE_ERR) {
        printf("aeCreateFileEvent error \n");
    }

    printf("loop forever\n");
    aeMain(el);
    aeDeleteEventLoop(el);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);

    return 0;
}
