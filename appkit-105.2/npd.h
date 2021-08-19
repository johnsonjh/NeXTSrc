/*
	npd.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
*/

#ifndef NPD_H
#define NPD_H

#import <mach.h>
#import <sys/message.h>

#define NPD_PUBLIC_PORT   "npd_port"

/* message IDs for connect messages */
#define NPD_INFO         100
#define NPD_REMOVE       101
#define NPD_CONNECT      102

/* message IDs for receive messages */
#define NPD_SEND_HEADER  1
#define NPD_SEND_PAGE    2
#define NPD_SEND_TRAILER 3

typedef struct npd_con_msg {
    msg_header_t   head;
    msg_type_t     printer_type;
    char           printer[1024];
    msg_type_t     host_type;
    char           host[1024];
    msg_type_t     user_type;
    char           user[1024];
    msg_type_t     copies_type;
    int            copies;
} npd_con_msg;

typedef struct npd_receive_msg {
    msg_header_t   head;
    msg_type_long_t type;
    char *data;
} npd_receive_msg;

#endif


