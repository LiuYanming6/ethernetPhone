/*************************************************************************
> File Name: play_selet.c
> Author: 刘艳明
> Mail: liuyanming@live.cn
> Created Time: 2018年10月10日 星期三 13时40分27秒
> 接收广播发送的select方法实现
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
#include "ae_select.h"
#include <alsa/asoundlib.h>

static snd_pcm_uframes_t frames;
static int size;
static unsigned char currMac[MAC_LEN];
static unsigned char zeroMac[MAC_LEN];


void select_read(int fd, snd_pcm_t *handle, char *buffer) {
    int iLen = -1;
    char *buf = NULL;
    int rc = -1;
    static unsigned long index = 0;

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

    finish:
    memset(buffer, 0, size);

    return;
}

int main() {
    int iRet = -1;
    int fd = -1;
    int rc;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    char *buffer;
    char *buf;
    unsigned long index = 0;

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

    /* Set period size to 32 frames. */
    frames = 32;
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

    // 监听UDP 60000 input
    fd = Net_bind();
    while(1){
        iRet = input_timeout(fd, 2);
        if (iRet == 0){
            //timeout
            memset(currMac, 0, MAC_LEN);
            printf("timeout\n");
        } else if (iRet == 1) {
            //input available
            select_read(fd, handle, buffer);
        } else {
            // -1 if errer
            printf("error\n");
        }
    }

    return 0;
}
