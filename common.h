/*************************************************************************
	> File Name: common.h
	> Author: 刘艳明
	> Mail: liuyanming@live.cn
	> Created Time: 2018年09月29日 星期六 18时39分07秒
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H

#define NUM_CHANNEL 2
#define RATE        11025 //44100 22050  11025
#define ALSA_PCM_NEW_HW_PARAMS_API
#define MAC_LEN 6

inline u_char *getInterMac() {
    //char *inter = "adhoc0";
    char *inter = "eth0";
    int s, i;
    static struct ifreq ifr = {0};

    if (0 != strcmp(ifr.ifr_name, inter)) {
        char mac[MAC_LEN] = {0};
        s = socket(AF_INET, SOCK_DGRAM, 0);
        strcpy(ifr.ifr_name, inter);
        ioctl(s, SIOCGIFHWADDR, &ifr);
        for (i = 0; i < MAC_LEN; i++)
            mac[i] = ifr.ifr_hwaddr.sa_data[i];
        close(s);
    }
    return ifr.ifr_hwaddr.sa_data;
}

#endif
