/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AE_H__
#define __AE_H__

#include <time.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* 
    File event structure 
    文件事件结构体
*/

typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE) */

    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    
    //clientData 参数一般是指向 redisClient 的指针
    void *clientData;
} aeFileEvent;

/* 
    Time event structure 
    时间事件结构体

    Redis的时间事件可以分为以下两类：
    （1）定时事件：让一段程序在指定的时间之后执行一次；
    （2）周期性事件：让一段程序每隔指定的时间就执行一次。
    
    一个时间事件是定时事件还是周期性事件取决于时间事件处理器的返回值(ae.c/processTimeEvents/retval = te->timeProc(eventLoop, id, te->clientData))：
        （1）如果事件处理器返回返回ae.h/AE_NOMORE,那么这个事件为定时事件，该事件在到达一次之后就会被删除。
        （2）如果事件处理器返回一个非AE_NOMORE的整数值，那么这个事件为周期性事件，此时，服务器会对时间事件的when属性进行更新，
            让该事件在一段时间之后再次到达，并以这种方式更新、运行
*/
typedef struct aeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    
    //定时回调函数指针
    aeTimeProc *timeProc;
    
    //定时事件清理函数，当删除定时事件的时候会被调用
    aeEventFinalizerProc *finalizerProc;
    
    //clientData 参数一般是指向 redisClient 的指针
    void *clientData;

    //定时事件表采用链表来维护
    struct aeTimeEvent *next;
} aeTimeEvent;

/*
    A fired event 
    触发事件结构体
*/
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;

/* 
    State of an event based program 
    事件循环结构体

    其中，events是aeFileEvent结构的数组，每个aeFileEvent结构表示一个注册的文件事件。setsize表示能处理的文件描述符的最大个数。
    beforesleep函数指针表示在监控事件触发之前，需要调用的函数。apindata表示底层多路复用的私有数据，比如对于select来说，
    该结构保存了读写描述符数组；对于epoll来说，该结构中保存了epoll描述符和epoll_event数组。
*/
typedef struct aeEventLoop {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    
    //记录最大的定时事件 id + 1
    long long timeEventNextId;

    //用于系统时间的矫正
    time_t lastTime;     /* Used to detect system clock skew */
    
    //I/O 事件表
    aeFileEvent *events; /* Registered events */
    
    //被触发的事件
    aeFiredEvent *fired; /* Fired events */
    
    //定时事件表
    aeTimeEvent *timeEventHead;
    
    //事件循环结束标识
    int stop;
    
    //对于不同的 I/O 多路复用技术，有不同的数据，详见各自实现
    void *apidata; /* This is used for polling API specific data */
    
    //新的循环前需要执行的操作
    aeBeforeSleepProc *beforesleep;
} aeEventLoop;

/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#endif
