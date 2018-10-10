/*************************************************************************
> File Name: rec.c
> Author: 刘艳明
> Mail: liuyanming@live.cn
> Created Time: 2018年10月10日 星期三 13时40分27秒
> 录音广播
************************************************************************/


#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include<net/if.h>
#include "net_op.h"
#include "common.h"

int main() {
    int rc;
    int size;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    char *buffer;
    char *buf;
    unsigned long index = 0;

    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&handle, "default",
                      SND_PCM_STREAM_CAPTURE, 0);
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
    snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);
    size = frames * 4 + MAC_LEN; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,
                                      &val, &dir);

    memcpy(buffer, getInterMac(), MAC_LEN);
    buf = buffer + MAC_LEN;
    while (1) { // endless loop
        rc = snd_pcm_readi(handle, buf, frames);
        if (rc == -EPIPE) {
            /* EPIPE means overrun */
            fprintf(stderr, "overrun occurred\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            fprintf(stderr,
                    "error from read: %s\n",
                    snd_strerror(rc));
        } else if (rc != (int) frames) {
            fprintf(stderr, "short read, read %d frames\n", rc);
        }
        fprintf(stderr, "send frame index=%ld\n", index++);
        rc = Net_send(buffer, size);
        if (rc != size)
            fprintf(stderr,
                    "short write: wrote %d bytes\n", rc);
    }

    //can't be here
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);

    return 0;
}

