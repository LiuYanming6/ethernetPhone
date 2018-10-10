cmake_minimum_required(VERSION 3.12)
project(aeTest C)

set(CMAKE_C_STANDARD 99)

#add_executable(play play.c ae.c ae_epoll.c net_op.c ae_select.c)
add_executable(play play_select.c ae_select.c net_op.c ae_select.c ae_select.h)
add_executable(rec rec.c net_op.c)
