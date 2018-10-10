/*************************************************************************
> File Name: net_op.c
> Author: 刘艳明
> Mail: liuyanming@live.cn
> Created Time: 2018年10月10日 星期三 13时40分27秒
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include "net_op.h"

static struct sockaddr_in serv_addr;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int create_socket() {
    int sockfd, iRet;
    const int opt = 1;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_BROADCAST;
    serv_addr.sin_port = htons(PORT);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *) &opt, sizeof(opt)) < 0)//设置套接字类型
    {
        error("setsockopt");
    }

    return sockfd;
}

int Net_send(unsigned char *buf, int buflen) {
    int iRet;
    int j;
    static int sendfd = -1;

    assert(buf != NULL);
    assert(buflen > 0);

    if (sendfd < 0) {
        sendfd = create_socket();
    }

    iRet = sendto(sendfd, buf, buflen, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (iRet < 0) {
        error("send error");
    }
    fsync(sendfd);

    return iRet;
}

int Net_rcv(int fd, unsigned char *buf, int buflen) {
    int iRet = 0;
    iRet = read(fd, buf, buflen);
    if (iRet < 0) {
        perror("ERROR in read");
    }

    return iRet;
}

int Net_bind() {
    static int bindfd = -1;

    if (bindfd > 0)
        return bindfd;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_BROADCAST;
    serv_addr.sin_port = htons(PORT);
    bindfd = create_socket();

    if (bind(bindfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    return bindfd;
}

/*
 * test for this module
 */
int test_main(int argc, char *argv[]) {
    int sockfd;
    int iRet;
    u_char buf[1024] = {0};
    static char *broStr = "255.255.255.255";

    if ((argc == 2) && (argv[1][1] == 's')) {
        sockfd = Net_bind();
        iRet = Net_rcv(sockfd, buf, 1024);
        printf("%s\n", buf);
    } else {
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_BROADCAST;
        serv_addr.sin_port = htons(PORT);
        Net_send(broStr, strlen(broStr));
    }

    close(sockfd);
    return 0;
}
