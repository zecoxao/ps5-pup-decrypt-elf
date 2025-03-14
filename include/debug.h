#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <unistd.h>

extern int g_debug_sock;

#define SOCK_LOG(format, ...)                                          \
{                                                                            \
    char _macro_printfbuf[512];                                              \
    int _macro_size = sprintf(_macro_printfbuf, format, ##__VA_ARGS__);      \
    write(g_debug_sock, _macro_printfbuf, _macro_size);                             \
} while(0);

typedef struct notify_request {
  char useless1[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t*, size_t, int);



#define printf_notification(fmt, ...) \
{   notify_request_t req; \
	bzero(&req, sizeof req); \
	snprintf(req.message, sizeof req.message, fmt, ##__VA_ARGS__); \
	sceKernelSendNotificationRequest(0, &req, sizeof req, 0); \
} while(0);


#include "time.h"

void notify(char* message);
unsigned char GetElapsed(unsigned long ResetInterval);

extern int sock;
extern time_t prevtime;

#define SSIZET_FMT "%zd"

#endif
