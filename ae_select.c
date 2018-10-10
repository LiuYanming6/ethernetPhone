/*************************************************************************
> File Name: ae_select.c
> Author: 刘艳明
> Mail: liuyanming@live.cn
> Created Time: 2018年10月10日 星期三 13时40分27秒
************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "ae_select.h"

int input_timeout(int filedes, unsigned int seconds) {
    fd_set set;
    struct timeval timeout;

    /* Initialize the file descriptor set. */
    FD_ZERO (&set);
    FD_SET (filedes, &set);

    /* Initialize the timeout data structure. */
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    /* select returns 0 if timeout, 1 if input available, -1 if error. */
    return select(FD_SETSIZE, &set, NULL, NULL,
                  &timeout);
}
