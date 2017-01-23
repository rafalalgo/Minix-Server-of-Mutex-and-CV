cmake_minimum_required(VERSION 3.6)
project(projekt)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        etc/usr/o_rc
        etc/usr/rc
        include/cv.h
        include/Makefile
        include/o_Makefile
        lib/libc/sys-minix/cv.c
        servers/cv/includes.h
        servers/cv/main.c
        servers/cv/queue.h
        servers/pm/forkexit.c
        servers/pm/o_forkexit.c
        servers/pm/o_proto.h
        servers/pm/o_signal.c
        servers/pm/o_utility.c
        servers/pm/proto.h
        servers/pm/signal.c
        servers/pm/utility.c)

add_executable(projekt ${SOURCE_FILES})