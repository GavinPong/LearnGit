#include <stdint.h>
#include <stdio.h>
#include "eventMutexFunc.h"
#include "shareMemFunc.h"
#include <thread>

using namespace std;

#ifdef __linux__
#define Sleep(uMs) usleep((uMs) * 1000);
#endif

#define THREAD_CNT 10

int32_t i32Count = 0;
void *g_pThreadMutex = NULL;
void *g_pProcessMutex = NULL;
void *g_pThreadEvent = NULL;
void *g_pProcessEvent = NULL;
void *g_pShareMemVirAddr = NULL;
int64_t g_i64ShareMemHandle = 0;

void threadRdWrMutex()
{
    MutexLock(g_pThreadMutex);
    i32Count++;
    MutexUnLock(g_pThreadMutex);
    printf("i32Count:%d\r\n", i32Count);
}

void testThreadMutex()
{
    thread szThread[THREAD_CNT];

    for (size_t i = 0; i < THREAD_CNT; i++)
    {
        szThread[i] = thread(threadRdWrMutex);
    }

    for (size_t i = 0; i < THREAD_CNT; i++)
    {
        szThread[i].join();
    }
}

void processRdWrMutex()
{
    //printf("%s->%d:start process mux lock:\r\n", __FILE__, __LINE__);
    MutexLock(g_pProcessMutex);
    //printf("%s->%d:end process mux lock:\r\n", __FILE__, __LINE__);
    (*((int64_t *)g_pShareMemVirAddr))++;
    //printf("%s->%d:start process mux unlock:\r\n", __FILE__, __LINE__);
    MutexUnLock(g_pProcessMutex);
    //printf("%s->%d:end process mux unlock:\r\n", __FILE__, __LINE__);
    printf("%s->%d:i32Count:%ld\r\n", __FILE__, __LINE__, (*((int64_t *)g_pShareMemVirAddr)));
}

void testProcessMutex()
{
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);
    thread szThread[THREAD_CNT];

    for (size_t i = 0; i < THREAD_CNT; i++)
    {
        szThread[i] = thread(processRdWrMutex);
    }

    for (size_t i = 0; i < THREAD_CNT; i++)
    {
        szThread[i].join();
    }
}

#define EVENT_THREAD_WAIT_CNT 10
#define EVENT_THREAD_NOTIFY_CNT 1

void eventWaitThread(int32_t i32ThreadIndx)
{
    printf("start wait thread[%d] event,\r\n", i32ThreadIndx);
    EventWait(g_pThreadEvent, -1);
    printf("end wait thread[%d] event,\r\n", i32ThreadIndx);
}

void eventNotifyThread()
{
    printf("start notify thread event,\r\n");
    Sleep(5000);
    #if 1
    EventSet(g_pThreadEvent);
    #else
    EventReset(g_pThreadEvent);
    #endif
    printf("end notify thread event,\r\n");
}

void testThreadEvent()
{
    thread szEventWaitThread[EVENT_THREAD_WAIT_CNT];
    thread szEventNotifyThread[EVENT_THREAD_NOTIFY_CNT];

    for (size_t i = 0; i < EVENT_THREAD_WAIT_CNT; i++)
    {
        szEventWaitThread[i] = thread(eventWaitThread, i);
    }
    szEventNotifyThread[0] = thread(eventNotifyThread);
    
    for (size_t i = 0; i < EVENT_THREAD_WAIT_CNT; i++)
    {
        szEventWaitThread[i].join();
    }    
    
    szEventNotifyThread[0].join();
}

void eventWaitProcess(int32_t i32ThreadIndx)
{
    printf("start wait thread[%d] event,\r\n", i32ThreadIndx);
    EventWait(g_pProcessEvent, -1);
    printf("end wait thread[%d] event,\r\n", i32ThreadIndx);
}

void eventNotifyProcess()
{
    printf("start notify thread event,\r\n");
    Sleep(5000);
    #if 1
    EventSet(g_pProcessEvent);
    #else
    EventReset(g_pProcessEvent);
    #endif
    printf("end notify thread event,\r\n");
}

void testProcessWaitEvent()
{
    thread szEventWaitThread[EVENT_THREAD_WAIT_CNT];
    thread szEventNotifyThread[EVENT_THREAD_NOTIFY_CNT];

    for (size_t i = 0; i < EVENT_THREAD_WAIT_CNT; i++)
    {
        szEventWaitThread[i] = thread(eventWaitProcess, i);
    }
    
    for (size_t i = 0; i < EVENT_THREAD_WAIT_CNT; i++)
    {
        szEventWaitThread[i].join();
    }    
}

void testProcessNotifyEvent()
{
    thread szEventNotifyThread[EVENT_THREAD_NOTIFY_CNT];
    szEventNotifyThread[0] = thread(eventNotifyProcess);
    
    for (size_t i = 0; i < EVENT_THREAD_NOTIFY_CNT; i++)
    {
        szEventNotifyThread[i].join();
    }    
}

void testThread()
{
    MutexCreate("", &g_pThreadMutex);
    if(EVENT_MUX_ERR_OK != EventCreate(&g_pThreadEvent, 1, 0, NULL))
        printf("%s->%d:EventCreate failed\r\n", __FILE__, __LINE__);
    testThreadMutex();
    testThreadEvent();
}

void testThreadByName()
{
    //printf("%s->%d:\r\n", __FILE__, __LINE__);
    MutexCreate("TestMux", &g_pThreadMutex);
    if(EVENT_MUX_ERR_OK != EventCreate(&g_pThreadEvent, 1, 0, "TestEvent"))
        printf("%s->%d:EventCreate failed\r\n", __FILE__, __LINE__);
    
    testThreadMutex();
    testThreadEvent();

    EventClose(g_pThreadEvent);
    MutexDestroy(g_pThreadMutex);
}

void testProcess1()
{
    int32_t iRet;
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);
    iRet = MutexCreate("TestMux", &g_pProcessMutex);
    if(EVENT_MUX_ERR_OK != iRet)
    {
        printf("%s->%d:MutexCreate[%s] failed with %d\r\n", __FILE__, __LINE__, "TestMux", iRet);
        return;    
    }
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);
    iRet = EventCreate(&g_pProcessEvent, 0, 0, "TestEvent");
    if(EVENT_MUX_ERR_OK != iRet)
    {
        if(EVENT_MUX_ERR_NAME_EXIST == iRet)
            iRet = EventOpen(&g_pProcessEvent, "TestEvent");
    }
    if(EVENT_MUX_ERR_OK != iRet)
    {
        printf("%s->%d:EventOpen[%s] failed with %d\r\n", __FILE__, __LINE__, "TestMux", iRet);
        return;
    }    
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);
    if(EVENT_MUX_ERR_OK != iRet)
        printf("%s->%d:EventCreate failed with %d\r\n", __FILE__, __LINE__, iRet);
    iRet = shareMemFunc_Create(&g_i64ShareMemHandle, &g_pShareMemVirAddr, sizeof(int64_t), "test");
    if(SHARE_MEMORY_ERR_OK != iRet)
        printf("%s->%d:shareMemFunc_Create failed with %d\r\n", __FILE__, __LINE__, iRet);
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);
    testProcessMutex();
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);
    testProcessWaitEvent();
    //printf("%s->%d:start testProcessMutex:\r\n", __FILE__, __LINE__);

    shareMemFunc_Destroy(g_i64ShareMemHandle);
    EventClose(g_pProcessEvent);
    MutexDestroy(g_pProcessMutex);
}

void testProcess2()
{    
    int32_t iRet = MutexCreate("TestMux", &g_pProcessMutex);
    if(EVENT_MUX_ERR_OK != iRet)
    {
        printf("%s->%d:MutexCreate[%s] failed with %d\r\n", __FILE__, __LINE__, "TestMux", iRet);
        return;
    }

    iRet = EventCreate(&g_pProcessEvent, 0, 0, "TestEvent");
    if(EVENT_MUX_ERR_OK != iRet)
    {
        if(EVENT_MUX_ERR_NAME_EXIST == iRet)
            iRet = EventOpen(&g_pProcessEvent, "TestEvent");            
    }
    if(EVENT_MUX_ERR_OK != iRet)
    {
        printf("%s->%d:EventOpen[%s] failed with %d\r\n", __FILE__, __LINE__, "TestMux", iRet);
        return;
    }
    iRet = shareMemFunc_Create(&g_i64ShareMemHandle, &g_pShareMemVirAddr, sizeof(int64_t), "test");
    if(SHARE_MEMORY_ERR_OK != iRet)
        printf("%s->%d:shareMemFunc_Create failed with %d\r\n", __FILE__, __LINE__, iRet);        
    testProcessMutex();
    testProcessNotifyEvent();

    shareMemFunc_Destroy(g_i64ShareMemHandle);
    EventClose(g_pProcessEvent);
    MutexDestroy(g_pProcessMutex);
}

int32_t main(int32_t i32Argc, char *pstrArgv[])
{
    if(atoi(pstrArgv[1]) == 1)
    {
        testThread();
    }
    else if(atoi(pstrArgv[1]) == 2)
    {
        testThreadByName();
    }
    else if(atoi(pstrArgv[1]) == 3)
    {
        testProcess1();
    }
    else if(atoi(pstrArgv[1]) == 4)
    {
        testProcess2();
    }
    else
        printf("%s->%d:intput param is invalid\r\n", __FILE__, __LINE__);

    return 0;
}