// 本代码解解决Windows系统调用和Linux系统调用的差异问题，提供类似于Windows系统调用的接口

#include <stdlib.h>
#include <stdbool.h>
#include "eventMutexFunc.h"
#include "shareMemFunc.h"

#if (!defined CHECK_PTR) && (defined DEBUG)
#define CHECK_PTR(ptr)  do{\
    if(NULL == (ptr)) \
        return EVENT_MUX_ERR_INVALID_PARAM;\
    }while(0)

#define DBG_PRT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define CHECK_PTR(ptr) 
#define DBG_PRT(fmt, ...) 
#endif

#if defined(__linux__)
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>

#if defined(_WIN32) || defined(WIN32)
#define snprintf(buf,len, format,...) _snprintf_s(buf, len, len-1, format, __VA_ARGS__)
#endif

typedef struct mux_ctx_s
{
    char szName[32];            /* 名称 */ 
    void *pMutex;      /* 互斥体 */ 
}mux_ctx_t;

/* 使用条件变量定义一种事件类型 */
typedef struct event_ctx_s
{
    char szName[32];            /* 名称 */ 
    pthread_mutex_t mutex;      /* 互斥体 */ 
    pthread_cond_t cond;        /* 条件变量 */ 
    sem_t *pSem;                /* 信号量　*/
    int32_t i32WaiterCnt;       /* 等待人数　*/
    bool bManualReset;          /* 是否自动复位 */ 
    bool bSet;                  /* 设置状态 */ 
    bool bResetting;             /*　是否在重置中　*/
}event_ctx_t;

typedef struct event_info_s{
    char szName[64];
    int64_t i64ShmemHandle;
    void *pShmemVirAddr;
    void *pShmemVirAddrMux;
}event_info_t;

void Sleep(uint32_t uMs)
{
#if 0	//网上测试这个方式比usleep的延时更稳定，待测试
	struct timeval tv;
	tv.tv_sec = uMs*1000 / 1000000L;
	tv.tv_usec = uMs*1000 % 1000000L;
	select(1, NULL, NULL, NULL, &tv);
#else
    usleep(uMs * 1000);
#endif
}

int32_t MutexCreate_(void **pMutex)
{
    CHECK_PTR(pMutex);

    pthread_mutex_t *pMutexTmp = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(NULL == pMutexTmp)
        return EVENT_MUX_ERR_NOMEMORY;

    if(0 == pthread_mutex_init(pMutexTmp, NULL))
    {
        *pMutex = pMutexTmp;
        return EVENT_MUX_ERR_OK;
    }
    free(pMutexTmp);
    return EVENT_MUX_ERR_FAILURE;
}

int32_t MutexDestroy_(void *pMutex)
{
    CHECK_PTR(pMutex);

    pthread_mutex_t *pMutexTmp = (pthread_mutex_t *)pMutex;
    if(0 == pthread_mutex_destroy(pMutexTmp))
    {
        free(pMutexTmp);
        return EVENT_MUX_ERR_OK;
    }
    free(pMutexTmp);
    return EVENT_MUX_ERR_FAILURE;
}

int32_t MutexLock_(void *pMutex)       
{
    CHECK_PTR(pMutex);

    pthread_mutex_t *pMutexTmp = (pthread_mutex_t *)pMutex;
    if(0 == pthread_mutex_lock(pMutexTmp))
        return EVENT_MUX_ERR_OK;
    return EVENT_MUX_ERR_FAILURE;    
}

int32_t MutexTimedLock_(void *pMutex, int32_t i32TimeoutMSec)       
{
    CHECK_PTR(pMutex);

    pthread_mutex_t *pMutexTmp = (pthread_mutex_t *)pMutex;
    struct timespec stTimwSec;
#if 0//相对时间
    struct timeval now; 
    gettimeofday(&now, NULL); 

    /* add the offset to get timeout value */ 
    stTimwSec.tv_nsec = now.tv_usec * 1000 + (i32TimeoutMSec % 1000) * 1000000; 
    stTimwSec.tv_sec = now.tv_sec + i32TimeoutMSec/1000;
    if(stTimwSec.tv_nsec >= 1000000000)
    {
        stTimwSec.tv_nsec -= 1000000000;
        stTimwSec.tv_sec++;
    }
#else//相对时间
    if(i32TimeoutMSec < 0)
    {
        return MutexLock_(pMutex);
    }

    clock_gettime(CLOCK_REALTIME, &stTimwSec);
    uint64_t u64Add;
    uint64_t u64Nsec;
    i32TimeoutMSec = i32TimeoutMSec % 1000;

    u64Nsec = i32TimeoutMSec * 1000 * 1000 + stTimwSec.tv_nsec;
    u64Add = u64Nsec / (1000 * 1000 * 1000);
    stTimwSec.tv_sec += (u64Add + u64Nsec);
    stTimwSec.tv_nsec = u64Nsec % (1000 * 1000 * 1000);
    if(stTimwSec.tv_nsec >= 1000000000)
    {
        stTimwSec.tv_nsec -= 1000000000;
        stTimwSec.tv_sec++;
    }
#endif//相对时间
    if(0 == pthread_mutex_timedlock(pMutexTmp, &stTimwSec))
        return EVENT_MUX_ERR_OK;
    return EVENT_MUX_ERR_FAILURE;
}

int32_t MutexUnLock_(void *pMutex)
{
    CHECK_PTR(pMutex);

    pthread_mutex_t *pMutexTmp = (pthread_mutex_t *)pMutex;
    if(0 == pthread_mutex_unlock(pMutexTmp))
        return EVENT_MUX_ERR_OK;
    return EVENT_MUX_ERR_FAILURE;       
}

int32_t ProcessMutexCreate(const char* pszName, void **pMutex)
{
    CHECK_PTR(pMutex);

    sem_t *p_sem = NULL;
    uint32_t uWaitCnt = 0;
    bool bMutexValid = false;
    int32_t iRet = EVENT_MUX_ERR_OK;
    do
    {
        /* 打开进程锁 */
        p_sem = sem_open(pszName, O_RDWR);  
        if (p_sem == SEM_FAILED) {
            mode_t mask = umask(0);	/* 取消屏蔽的权限位,避免创建的信号量文件无法被其他用户访问 */
            /* 信号量初值为1，代表未锁状态 */
            p_sem = sem_open(pszName, O_CREAT | O_EXCL, 0666, 1); 
            if (p_sem == SEM_FAILED) {
                DBG_PRT("sem_open[%s] failed,errno:%d\n", pszName, errno);
            }
            else
            {
				umask(mask);    /* 恢复屏蔽的权限位 */
                bMutexValid = true;
                break;  
            }
            umask(mask);    /* 恢复屏蔽的权限位 */
        }
        else
        {
            bMutexValid = true;
            break;
        }
        Sleep(5);
        uWaitCnt++;
    } while (uWaitCnt < 200);

    if (bMutexValid == false)
    {
        /* 1S都没有获取到锁 */
        iRet = EVENT_MUX_ERR_TIMEOUT;
        *pMutex = NULL;
    }
    else
    {
        mux_ctx_t *pstMuxCtx = (mux_ctx_t *)calloc(1, sizeof(mux_ctx_t));
        if(NULL == pstMuxCtx)
        {
            sem_close(p_sem);
            sem_unlink(pszName);
            return EVENT_MUX_ERR_NOMEMORY;
        }
        snprintf(pstMuxCtx->szName, sizeof(pstMuxCtx->szName), "%s", pszName);
        pstMuxCtx->pMutex = p_sem;
        *pMutex = pstMuxCtx;
    }    

    return iRet;
}

int32_t ProcessMutexDestroy(void* pMutex)
{
    CHECK_PTR(pMutex);

    mux_ctx_t *pstMuxCtx = (mux_ctx_t *)pMutex;
    int32_t iRet = EVENT_MUX_ERR_OK;
    sem_t *p_sem = (sem_t *)pstMuxCtx->pMutex;
    if(NULL != p_sem)
    {
        if(0 != sem_close(p_sem))
            iRet  = EVENT_MUX_ERR_FAILURE;
    }       
    else
        iRet  = EVENT_MUX_ERR_INVALID_PARAM;         
    sem_unlink(pstMuxCtx->szName);
    return iRet;   
}

int32_t ProcessMutexLock(void* pMutex)
{
    CHECK_PTR(pMutex);

    mux_ctx_t *pstMuxCtx = (mux_ctx_t *)pMutex;
    int32_t iRet = EVENT_MUX_ERR_OK;
    sem_t *p_sem = (sem_t *)pstMuxCtx->pMutex;
    
    if(0 != sem_wait(p_sem))
    {
        iRet = EVENT_MUX_ERR_FAILURE;
    }
    return iRet;
}

int32_t ProcessMutexTimedLock(void* pMutex, int32_t i32TimeoutMSec)
{
    CHECK_PTR(pMutex);

    mux_ctx_t *pstMuxCtx = (mux_ctx_t *)pMutex;
    int32_t iRet = EVENT_MUX_ERR_OK;
    sem_t *p_sem = (sem_t *)pstMuxCtx->pMutex;
    struct timespec ts;
    long secs;
    long add = 0;
    uint64_t u64NSec;

    if (NULL == p_sem)
    {
        return EVENT_MUX_ERR_INVALID_PARAM;   
    }
    
    if(i32TimeoutMSec < 0)
    {
        return ProcessMutexLock(pMutex);
    }
    secs = i32TimeoutMSec / 1000;
    i32TimeoutMSec = i32TimeoutMSec % 1000;
    clock_gettime(CLOCK_REALTIME, &ts);

    u64NSec = (uint64_t)i32TimeoutMSec * 1000 * 1000 + ts.tv_nsec;
    add = u64NSec / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = u64NSec % (1000 * 1000 * 1000);
    if(ts.tv_nsec >= 1000000000)
    {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec++;
    }
    if(0 != sem_timedwait(p_sem, &ts))
    {
        if(ETIMEDOUT == errno)
            iRet = EVENT_MUX_ERR_TIMEOUT;
        else
            iRet = EVENT_MUX_ERR_FAILURE;
    }
            
    return iRet;
}

int32_t ProcessMutexUnLock(void* pMutex)
{
    CHECK_PTR(pMutex);

    mux_ctx_t *pstMuxCtx = (mux_ctx_t *)pMutex;
    int32_t iRet = EVENT_MUX_ERR_OK;
    sem_t *p_sem = (sem_t *)pstMuxCtx->pMutex;
    int32_t i32SemValue = 0;
    
    if(0 == sem_getvalue (p_sem, &i32SemValue))
    {
        if(i32SemValue > 0)
        {
            /*abort process with SIGABORT*/
            #if 0
            raise(6);
            #else
            /*When it is 1, it returns directly to ensure its mutual exclusion*/
            if(1 == i32SemValue)
                return EVENT_MUX_ERR_OK;            
            #endif
        }
    }
    else
        return EVENT_MUX_ERR_FAILURE;
    if(0 != sem_post(p_sem))
    {
        iRet = EVENT_MUX_ERR_FAILURE;
    }
    return iRet;
}

int32_t EventCreate_(void** pEvt, int8_t bManualReset, int8_t bInitialState, const char* pszName)
{
    CHECK_PTR(pEvt);

    event_ctx_t *pstEvent = (event_ctx_t *)calloc(1, sizeof(event_ctx_t));
    if(NULL == pstEvent)
        return EVENT_MUX_ERR_INVALID_PARAM;

    pstEvent->bManualReset = bManualReset;
    pstEvent->bSet = bInitialState;
    pstEvent->bResetting = false;

    // 注意：不提供而外属性，将不支持嵌套使用 
    if (0 != pthread_mutex_init(&pstEvent->mutex, NULL))
    {
        free(pstEvent);
        return EVENT_MUX_ERR_FAILURE;
    }
    
    if (0 != pthread_cond_init(&pstEvent->cond, NULL))
    {
        free(pstEvent);
        return EVENT_MUX_ERR_FAILURE;
    }
    *pEvt = pstEvent;
    return EVENT_MUX_ERR_OK;
}

int32_t EventOpen_(void** pEvt, const char* pszName)
{
    /* linux线程间创建同步事件不创建文件，所以不应该存在创建失败需要调用Open的情况 */

    return EVENT_MUX_ERR_FAILURE;
}

int32_t EventClose_(void* pEvt)
{
    CHECK_PTR(pEvt);

    int32_t iRet1, iRet2;
    event_ctx_t *pstEvent = (event_ctx_t *)(pEvt);
   
    iRet1 = pthread_mutex_destroy(&pstEvent->mutex);
    iRet2 = pthread_cond_destroy(&pstEvent->cond);

    if (0 != iRet1 || 0 != iRet2)
    {
        return EVENT_MUX_ERR_FAILURE;
    }
    free(pstEvent);
    return EVENT_MUX_ERR_OK;
}

int32_t EventSet_(void* pEvt)
{
    CHECK_PTR(pEvt);

    int32_t iRet = 0;
    event_ctx_t *pstEvent = (event_ctx_t *)(pEvt);
    
    do
    {
        iRet = pthread_mutex_lock(&pstEvent->mutex); 
        if (iRet != 0)
            break;
        pstEvent->bResetting = false;
        if(pstEvent->bManualReset)
        {
            pstEvent->bSet = true;
            iRet = pthread_cond_broadcast(&pstEvent->cond);
        }            
        else
            iRet = pthread_cond_signal(&pstEvent->cond);
        if (iRet != 0)
        {
            pthread_mutex_unlock (&pstEvent->mutex);
            break;
        }
        iRet = pthread_mutex_unlock (&pstEvent->mutex);
        if (iRet != 0)
            break;
    }while(0);

    if (iRet != 0)
    {
        return EVENT_MUX_ERR_FAILURE;
    }
    return EVENT_MUX_ERR_OK;
}

int32_t EventWait_(void* pEvt, int32_t u32TimeoutMSec)
{
    CHECK_PTR(pEvt);

    int32_t iRet;  
    int32_t iRet2 = EVENT_MUX_ERR_OK;  
    bool bWaitRest = false;
    event_ctx_t *pstEvent = (event_ctx_t *)(pEvt);
   
    do 
    {
        iRet = pthread_mutex_lock(&pstEvent->mutex);
        if (iRet !=0)
        {
            iRet2 = EVENT_MUX_ERR_FAILURE;
            break;
        }         
        while (true == pstEvent->bResetting)
        {
            if(false == bWaitRest)
            {
                bWaitRest = true;
                pthread_mutex_unlock(&pstEvent->mutex);
            }
            Sleep(5);
        }

        if(true == bWaitRest)
            pthread_mutex_lock(&pstEvent->mutex);

        if (pstEvent->bSet) // 已经被设置，不再等待
        {
            if(false == pstEvent->bManualReset)
                pstEvent->bSet = false;
            iRet = pthread_mutex_unlock(&pstEvent->mutex);
            break;
        }
        pstEvent->i32WaiterCnt++;
        if (u32TimeoutMSec > 0)
        {
            struct timeval now; 
            struct timespec abstime;
            gettimeofday(&now, NULL); 

            /* add the offset to get timeout value */ 
            abstime.tv_nsec = now.tv_usec * 1000 + ((__syscall_slong_t)u32TimeoutMSec % 1000) * 1000000; 
            abstime.tv_sec = now.tv_sec + u32TimeoutMSec/1000;
            if(abstime.tv_nsec >= 1000000000)
            {
                abstime.tv_nsec -= 1000000000;
                abstime.tv_sec++;
            }
            iRet = pthread_cond_timedwait(&pstEvent->cond, &pstEvent->mutex, &abstime);
            if (iRet != 0)
            {
                //if (r == ETIMEDOUT)
                {
                    pthread_mutex_unlock(&pstEvent->mutex);
                    iRet2 = EVENT_MUX_ERR_TIMEOUT;
                }
            }
        }
        else
        {
            iRet = pthread_cond_wait(&pstEvent->cond, &pstEvent->mutex);
            if (iRet != 0)
            {
                if (iRet == ETIMEDOUT)
                {
                    pthread_mutex_unlock(&pstEvent->mutex);
                    iRet2 = EVENT_MUX_ERR_FAILURE;
                }
            }          
        }

        if(false == pstEvent->bManualReset)
            pstEvent->bSet = false;

        pstEvent->i32WaiterCnt--;
        iRet = pthread_mutex_unlock(&pstEvent->mutex);
    }while(0);

    return iRet2;
}

int32_t EventReset_(void* pEvt)
{
    CHECK_PTR(pEvt);

    uint8_t bWait = 0;
    event_ctx_t *pstEvent = (event_ctx_t *)(pEvt);
    int32_t iRet = pthread_mutex_lock(&pstEvent->mutex); 
    if (iRet != 0)
        return EVENT_MUX_ERR_FAILURE;
    pstEvent->bResetting = true;
    while(pstEvent->i32WaiterCnt > 0)
    {
        if(0 == bWait)
        {
            bWait = 1;
            /* 睡一小会儿，再看是否有人还有锁没有释放　*/
            pthread_mutex_unlock(&pstEvent->mutex);
        }
        Sleep(3);
    }
    if(1 == bWait)
        pthread_mutex_lock(&pstEvent->mutex); 
    pstEvent->bSet = false;
    pstEvent->bResetting = false;
    pthread_mutex_unlock(&pstEvent->mutex); 

    return EVENT_MUX_ERR_OK;
}

int32_t ProcessEventCreate(void** pEvt, int8_t bManualReset, int8_t bInitialState, const char* pszName)
{
    CHECK_PTR(pEvt);

    sem_t *pSem = NULL;
    uint32_t uWaitCnt = 0;
    bool bMutexValid = false;
    int64_t i64Handle;
    void *pShmemVirAddr = NULL;
    void *pShmemVirAddrMux = NULL;
    char strTmp[128];
    snprintf(strTmp, sizeof(strTmp), "%s_ShmemMux", pszName);
    if(EVENT_MUX_ERR_OK != MutexCreate(strTmp, &pShmemVirAddrMux))
    {
        return EVENT_MUX_ERR_CREATE_MUX;
    }

    MutexLock(pShmemVirAddrMux);
    pSem = sem_open(pszName, O_RDWR);  
    if (SEM_FAILED != pSem) 
    {
        sem_close(pSem);
        MutexUnLock(pShmemVirAddrMux);
        MutexDestroy(pShmemVirAddrMux);
        return EVENT_MUX_ERR_NAME_EXIST;
    }
    event_info_t *pstEventInfo= (event_info_t *)calloc(1, sizeof(event_info_t));
    if(NULL == pstEventInfo)
    {
        sem_close(pSem);
        MutexUnLock(pShmemVirAddrMux);
        MutexDestroy(pShmemVirAddrMux);        
        return EVENT_MUX_ERR_NOMEMORY;
    }

    pstEventInfo->pShmemVirAddrMux = pShmemVirAddrMux;

    do
    {
        mode_t mask = umask(0);         
        /* 信号量初值为0 */
        pSem = sem_open(pszName, O_CREAT | O_EXCL, 0666, 0); 
        if (SEM_FAILED == pSem) 
        {
            DBG_PRT("sem_open[%s] failed,errno:%d\n", pszName, errno);
            umask(mask);
        }
        else
        {
            umask(mask);
            bMutexValid = true;
            break;
        }                        
        Sleep(20);
        uWaitCnt++;
    } while (uWaitCnt < 50);

    if(false == bMutexValid)
    {
        free(pstEventInfo);
        MutexUnLock(pstEventInfo->pShmemVirAddrMux);
        MutexDestroy(pstEventInfo->pShmemVirAddrMux);
        return EVENT_MUX_ERR_FAILURE;
    }

    snprintf(strTmp, sizeof(strTmp), "%s_Shmem", pszName);
    int32_t iRet = shareMemFunc_Create(&i64Handle, &pShmemVirAddr, sizeof(event_ctx_t), strTmp);
    if(SHARE_MEMORY_ERR_OK != iRet)
    {
        sem_close(pSem);
        MutexUnLock(pstEventInfo->pShmemVirAddrMux);
        MutexDestroy(pstEventInfo->pShmemVirAddrMux);
        free(pstEventInfo);
        return EVENT_MUX_ERR_CREATE_SHMEM;
    }

    event_ctx_t *pstEvent = (event_ctx_t *)pShmemVirAddr;        

    pstEvent->pSem = pSem;
    pstEvent->bManualReset = bManualReset;
    pstEvent->bSet = bInitialState;
    pstEvent->bResetting = false;
    pstEventInfo->i64ShmemHandle = i64Handle;
    pstEventInfo->pShmemVirAddr = pShmemVirAddr;
    snprintf(pstEvent->szName, sizeof(pstEvent->szName), "%s", pszName);
    snprintf(pstEventInfo->szName, sizeof(pstEventInfo->szName), "%s", pszName);

    *pEvt = pstEventInfo; 
    MutexUnLock(pstEventInfo->pShmemVirAddrMux); 

    return iRet;
}

int32_t ProcessEventOpen(void** pEvt, const char* pszName)
{
    CHECK_PTR(pEvt);

    sem_t *pSem = NULL;
    int64_t i64Handle;
    void *pShmemVirAddr = NULL;
    void *pShmemVirAddrMux = NULL;
    char strTmp[128];
    snprintf(strTmp, sizeof(strTmp), "%s_ShmemMux", pszName);
    if(EVENT_MUX_ERR_OK != MutexCreate(strTmp, &pShmemVirAddrMux))
    {
        return EVENT_MUX_ERR_CREATE_MUX;
    }
    MutexLock(pShmemVirAddrMux);

    pSem = sem_open(pszName, O_RDWR);  
    if (SEM_FAILED == pSem) 
    {
        MutexUnLock(pShmemVirAddrMux);
        MutexDestroy(pShmemVirAddrMux);
        return EVENT_MUX_ERR_FAILURE;
    }
    event_info_t *pstEventInfo= (event_info_t *)calloc(1, sizeof(event_info_t));
    if(NULL == pstEventInfo)
    {
        sem_close(pSem);
        MutexUnLock(pShmemVirAddrMux);
        MutexDestroy(pShmemVirAddrMux);        
        return EVENT_MUX_ERR_NOMEMORY;
    }
    pstEventInfo->pShmemVirAddrMux = pShmemVirAddrMux;
    snprintf(strTmp, sizeof(strTmp), "%s_Shmem", pszName);
    int32_t iRet = shareMemFunc_Create(&i64Handle, &pShmemVirAddr, sizeof(event_ctx_t), strTmp);
    if(SHARE_MEMORY_ERR_OK != iRet)
    {
        sem_close(pSem);
        MutexUnLock(pstEventInfo->pShmemVirAddrMux);
        MutexDestroy(pstEventInfo->pShmemVirAddrMux);
        free(pstEventInfo);
        return EVENT_MUX_ERR_CREATE_SHMEM;
    }

    event_ctx_t *pstEvent = (event_ctx_t *)pShmemVirAddr;        

    pstEvent->pSem = pSem;
    pstEventInfo->i64ShmemHandle = i64Handle;
    pstEventInfo->pShmemVirAddr = pShmemVirAddr;
    snprintf(pstEvent->szName, sizeof(pstEvent->szName), "%s", pszName);
    snprintf(pstEventInfo->szName, sizeof(pstEventInfo->szName), "%s", pszName);

    *pEvt = pstEventInfo; 
    MutexUnLock(pstEventInfo->pShmemVirAddrMux); 

    return iRet;
}

int32_t ProcessEventClose(void* pEvt)
{
    CHECK_PTR(pEvt);

    int32_t iRet = 0;
    event_info_t *pstEventInfo = (event_info_t *)(pEvt);
    event_ctx_t *pstEvent = (event_ctx_t *)pstEventInfo->pShmemVirAddr;
    if(SEM_FAILED != pstEvent->pSem)
    {
        /*　这里是原子操作，所以不用加锁　*/
        if(sem_close(pstEvent->pSem))
            iRet  = -1;
    }      
    sem_unlink(pstEvent->szName);  
    shareMemFunc_Destroy(pstEventInfo->i64ShmemHandle);
    free(pstEventInfo);

    return iRet; 
}

int32_t ProcessEventSet(void* pEvt)
{
    CHECK_PTR(pEvt);

    int32_t iRet = 0;
    int32_t i32Value = 0;
    event_info_t *pstEventInfo = (event_info_t *)(pEvt);
    event_ctx_t *pstEvent = (event_ctx_t *)pstEventInfo->pShmemVirAddr;    

    if(0 == sem_getvalue(pstEvent->pSem, &i32Value))
    {
        if(1 == i32Value)
            return EVENT_MUX_ERR_OK;
    }
    else
        return EVENT_MUX_ERR_FAILURE;
    MutexLock(pstEventInfo->pShmemVirAddrMux);
    sem_post(pstEvent->pSem);
    if(true == pstEvent->bManualReset)
        pstEvent->bSet = true;
    pstEvent->bResetting = false;
    MutexUnLock(pstEventInfo->pShmemVirAddrMux);
    
    return iRet;
}

int32_t ProcessEventReset(void* pEvt)
{
    CHECK_PTR(pEvt);

    uint8_t bWait = 0;
    int32_t i32Val = 0;
    event_info_t *pstEventInfo = (event_info_t *)(pEvt);
    event_ctx_t *pstEvent = (event_ctx_t *)pstEventInfo->pShmemVirAddr;  

    MutexLock(pstEventInfo->pShmemVirAddrMux);
    pstEvent->bResetting = true;
    while(pstEvent->i32WaiterCnt > 0)
    {
        if(0 == bWait)
        {
            bWait = 1;
            /* 睡一小会儿，再看是否有人还有锁没有释放　*/
            MutexUnLock(pstEventInfo->pShmemVirAddrMux);
        }
        Sleep(3);
    }

    MutexLock(pstEventInfo->pShmemVirAddrMux);
    pstEvent->bSet = false;
    pstEvent->bResetting = false;
    if(0 == sem_getvalue(pstEvent->pSem, &i32Val) && i32Val > 0)
    {
        for (int32_t i = 0; i < i32Val; i++)
        {
            sem_wait(pstEvent->pSem);
        }        
    }
    MutexUnLock(pstEventInfo->pShmemVirAddrMux);
    return EVENT_MUX_ERR_OK;
}

int32_t ProcessEventWait(void* pEvt, int32_t u32TimeoutMSec)
{
    CHECK_PTR(pEvt);

    uint8_t bWait = 0;
    int32_t iRet = EVENT_MUX_ERR_OK;
    event_info_t *pstEventInfo = (event_info_t *)(pEvt);
    event_ctx_t *pstEvent = (event_ctx_t *)pstEventInfo->pShmemVirAddr;  
    struct timespec ts;
    long lSecs;
    long add = 0;
    uint64_t u64NSec;
    
    MutexLock(pstEventInfo->pShmemVirAddrMux);

    while (true == pstEvent->bResetting)
    {
        if(0 == bWait)
        {
            bWait = 1;
            MutexUnLock(pstEventInfo->pShmemVirAddrMux);
        }
        Sleep(5);
    }
    
    if(1 == bWait)
        MutexLock(pstEventInfo->pShmemVirAddrMux);

    if(pstEvent->bSet)
    {
        if(false == pstEvent->bManualReset)
            pstEvent->bSet = false;
        MutexUnLock(pstEventInfo->pShmemVirAddrMux);   
        return EVENT_MUX_ERR_OK;
    }

    pstEvent->i32WaiterCnt++;

    if(u32TimeoutMSec < 0)
    {
        MutexUnLock(pstEventInfo->pShmemVirAddrMux);  
        if(0 != sem_wait(pstEvent->pSem))
        {
            iRet = EVENT_MUX_ERR_FAILURE;
        }  
        MutexLock(pstEventInfo->pShmemVirAddrMux);
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        lSecs = u32TimeoutMSec / 1000;
        u32TimeoutMSec = u32TimeoutMSec % 1000;

        u64NSec = (uint64_t)u32TimeoutMSec * 1000 * 1000 + ts.tv_nsec;
        add = u64NSec / (1000 * 1000 * 1000);
        ts.tv_sec += (add + lSecs);
        ts.tv_nsec = u64NSec % (1000 * 1000 * 1000);
        if(ts.tv_nsec >= 1000000000)
        {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec++;
        }
        MutexUnLock(pstEventInfo->pShmemVirAddrMux);  
        if(0 != sem_timedwait(pstEvent->pSem, &ts))
        {
            if(ETIMEDOUT == errno)
                iRet = EVENT_MUX_ERR_TIMEOUT;
            else
                iRet = EVENT_MUX_ERR_FAILURE;
        }
        MutexLock(pstEventInfo->pShmemVirAddrMux);   
    }

    if(false == pstEvent->bManualReset)
        pstEvent->bSet = false;

    pstEvent->i32WaiterCnt--;     
    if(true == pstEvent->bSet)
    {
        int32_t i32Val = 0;
        if(0 == sem_getvalue(pstEvent->pSem, &i32Val) && i32Val == 0)
        {
            sem_post(pstEvent->pSem);      
        }
    }        
    MutexUnLock(pstEventInfo->pShmemVirAddrMux);

    return iRet;
}
#endif

#if defined(_WIN32)
int32_t MutexCreate_(void **pMutex)
{
    CHECK_PTR(pMutex);
    BOOL bRet;

    CRITICAL_SECTION    *pCriticalSection = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    if(NULL == pCriticalSection)
        return EVENT_MUX_ERR_NOMEMORY;
    #if 0/*　直接切花上下文　*/
    bRet = InitializeCriticalSection(pCriticalSection);
    #else
    /* 设置h旋转值为４０００，只有尝试这么多次后，才切换上下文*/
    bRet = InitializeCriticalSectionAndSpinCount(pCriticalSection, 4000);
    #endif
    if(bRet)
    {
        *pMutex = pCriticalSection;
        return EVENT_MUX_ERR_OK;
    }
        
    free(pCriticalSection);
    return EVENT_MUX_ERR_FAILURE;
}

int32_t MutexDestroy_(void *pMutex)
{
    CHECK_PTR(pMutex);

    CRITICAL_SECTION    *pCriticalSection = (CRITICAL_SECTION*)pMutex;
    DeleteCriticalSection(pCriticalSection);
    free(pCriticalSection);
    return EVENT_MUX_ERR_OK;
}

int32_t MutexLock_(void *pMutex)       
{
    CHECK_PTR(pMutex);

    CRITICAL_SECTION    *pCriticalSection = (CRITICAL_SECTION*)pMutex;
    EnterCriticalSection(pCriticalSection);
    return EVENT_MUX_ERR_OK;
}

int32_t MutexTimedLock_(void *pMutex, int32_t i32TimeoutMSec)       
{
    CHECK_PTR(pMutex);

    int32_t iRet;
    CRITICAL_SECTION    *pCriticalSection = (CRITICAL_SECTION*)pMutex;
    int32_t i32SleepUnit = 5;

    //阻塞等待
    if(i32TimeoutMSec < 0)
    {
        return MutexLock_(pMutex);
    }
    //超时等待
    iRet = TryEnterCriticalSection(pCriticalSection);
    while(FALSE == iRet)
    {
        if(i32TimeoutMSec <= 0)
            break;
        if(i32TimeoutMSec > 0 && i32TimeoutMSec > i32SleepUnit)
        {
            i32SleepUnit = i32TimeoutMSec;
        }            
        Sleep((DWORD)i32SleepUnit);
        i32TimeoutMSec -= i32SleepUnit;
        iRet = TryEnterCriticalSection(pCriticalSection);
    }
    if(FALSE == iRet)
        return EVENT_MUX_ERR_TIMEOUT;
    return EVENT_MUX_ERR_OK;
}

int32_t MutexUnLock_(void *pMutex)
{
    CHECK_PTR(pMutex);

    CRITICAL_SECTION    *pCriticalSection = (CRITICAL_SECTION*)pMutex;    
    LeaveCriticalSection(pCriticalSection);
    return EVENT_MUX_ERR_OK;
}

int32_t ProcessMutexCreate(const char* pszName, void** pMutex)
{
    CHECK_PTR(pMutex);

    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE mutex;

    mutex = CreateMutex(NULL, false, pszName);
    if(0 == mutex)
    {
        *pMutex = NULL;
        iRet = EVENT_MUX_ERR_FAILURE;
    }
    *pMutex = mutex;
    return iRet;
}

int32_t ProcessMutexDestroy(void* pMutex)
{
    CHECK_PTR(pMutex);

    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE h = (HANDLE)pMutex;

    if(TRUE == CloseHandle(h))
        iRet = EVENT_MUX_ERR_FAILURE;
    return iRet;
}

int32_t ProcessMutexLock(void* pMutex)
{
    CHECK_PTR(pMutex);

    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE h = (HANDLE)pMutex;

    if(WAIT_OBJECT_0 != WaitForSingleObject(h, INFINITE))
        iRet = EVENT_MUX_ERR_FAILURE;
    return iRet;
}

int32_t ProcessMutexTimedLock(void* pMutex, int32_t i32MSec)
{
    CHECK_PTR(pMutex);

    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE h = (HANDLE)pMutex;
    DWORD dwRet;

    if(i32MSec < 0)
    {
        return ProcessMutexLock(h);
    }

    dwRet = WaitForSingleObject(h, (DWORD)i32MSec);
    if(WAIT_TIMEOUT == dwRet)
        iRet = EVENT_MUX_ERR_TIMEOUT;
    else if(WAIT_OBJECT_0 != dwRet)
        iRet = EVENT_MUX_ERR_FAILURE;
    return iRet;    
}

int32_t ProcessMutexUnLock(void* pMutex)
{
    CHECK_PTR(pMutex);

    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE h = (HANDLE)pMutex;

    if(0 == ReleaseMutex(h))
        iRet = EVENT_MUX_ERR_FAILURE;
    return iRet;
}

int32_t EventCreate_(void** pEvt, int8_t bManualReset, int8_t bInitialState, const char* pszName)
{
    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE evt;    

    evt = CreateEventA(NULL, (BOOL)bManualReset, (BOOL)bInitialState, pszName);
    if (NULL == evt)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            return EVENT_MUX_ERR_NAME_EXIST;
        }
        return EVENT_MUX_ERR_FAILURE;
    }
    if(EVENT_MUX_ERR_OK == iRet)
        *pEvt = evt;
    return iRet;
}

int32_t EventOpen_(void** pEvt, const char* pszName)
{
    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE evt;
    evt = OpenEventA(EVENT_ALL_ACCESS, FALSE, pszName);
    if (NULL == evt) 
    {
        DBG_PRT("%s:[%s] DtEventInit failed,errno:%d\r\n", __FUNCTION__, pszName, GetLastError());
        iRet = EVENT_MUX_ERR_FAILURE;
    }
    if(EVENT_MUX_ERR_OK == iRet)
        *pEvt = evt;
    return iRet;    
}

int32_t EventClose_(void* pEvt)
{
    int32_t iRet = EVENT_MUX_ERR_OK;
    HANDLE hEvt = (HANDLE)pEvt;
    if(true == CloseHandle(hEvt))
        return EVENT_MUX_ERR_OK; 
    return EVENT_MUX_ERR_FAILURE;
}

int32_t EventSet_(void* pEvt)
{
    int32_t iRet = 0;

    HANDLE hEvt = (HANDLE)pEvt;
    if(true == SetEvent(hEvt))
        return EVENT_MUX_ERR_OK; 
    return EVENT_MUX_ERR_FAILURE;     
}

int32_t EventReset_(void* pEvt)
{
    int32_t iRet = 0;   
    HANDLE hEvt = (HANDLE)pEvt; 
    if(true == ResetEvent(hEvt))
        return EVENT_MUX_ERR_OK; 
    return EVENT_MUX_ERR_FAILURE; 
}

int32_t EventWait_(void* pEvt, int32_t u32TimeoutMSec)
{
    int32_t iRet = 0;  
    HANDLE hEvt = (HANDLE)pEvt; 
    DWORD dwRet;
    
    if(INFINITE == u32TimeoutMSec || u32TimeoutMSec < 0)
    {
        dwRet = WaitForSingleObject(hEvt, INFINITE);
    }
    else
        dwRet = WaitForSingleObject(hEvt, u32TimeoutMSec);
    switch (dwRet) 
    {

        case WAIT_OBJECT_0:
        case WAIT_ABANDONED:
            break;

        case WAIT_TIMEOUT:
            {
                iRet = EVENT_MUX_ERR_TIMEOUT;
            }            
            break;
        default:
            {
                DBG_PRT("%s->%d:DtEventWait failed,errno:%d\r\n", __FUNCTION__, __LINE__, GetLastError());
                iRet = EVENT_MUX_ERR_FAILURE;
            }
            break;
    }
    return iRet;
}

int32_t ProcessEventCreate(void** pEvt, int8_t bManual, int8_t bInitialState, const char* pszName)
{
    return EventCreate_(pEvt, bManual, bInitialState, pszName);
}

int32_t ProcessEventOpen(void** pEvt, const char* pszName)
{
    return EventOpen_(pEvt, pszName);
}

int32_t ProcessEventClose(void* pEvt)
{
    return EventClose_(pEvt);
}

int32_t ProcessEventSet(void* pEvt)
{
    return EventSet_(pEvt);
}

int32_t ProcessEventReset(void* pEvt)
{
    return EventReset_(pEvt);
}

int32_t ProcessEventWait(void* pEvt, int32_t u32TimeoutMSec)
{
    return EventWait_(pEvt, u32TimeoutMSec);
}
#endif //WIN32 | LINUX

typedef struct handle_info_s{
    char szName[64];    /* 句柄对应的名称　*/
    void *stHandle;     /* 句柄对应的结构体　*/
}handle_info_t;

#define HANDLE_2_INFO(h) handle_info_t *pstHandleInfo; do{pstHandleInfo = (handle_info_t *)(h);}while(0)

int32_t MutexCreate(const char* pszName, void **pMutex)
{
    handle_info_t *pstHandleInfo = (handle_info_t *)calloc(1, sizeof(handle_info_t));
    if(NULL == pstHandleInfo)
        return EVENT_MUX_ERR_NOMEMORY;

    if(NULL == pszName || !strlen(pszName))
    {
        if(EVENT_MUX_ERR_OK == MutexCreate_(&pstHandleInfo->stHandle))
        {
            *pMutex = pstHandleInfo;
            return EVENT_MUX_ERR_OK;
        }
        free(pstHandleInfo);
        return EVENT_MUX_ERR_FAILURE;
    }
    else
    {
        uint32_t u32Lens = strlen(pszName);
        if(u32Lens > sizeof(pstHandleInfo->szName))
            return EVENT_MUX_ERR_NAME_TOO_LONG;
        u32Lens = u32Lens > sizeof(pstHandleInfo->szName) ? sizeof(pstHandleInfo->szName) : u32Lens;
        memset(pstHandleInfo->szName, 0, sizeof(pstHandleInfo->szName));
        memcpy(pstHandleInfo->szName, pszName, u32Lens);
        if(EVENT_MUX_ERR_OK == ProcessMutexCreate(pszName, &pstHandleInfo->stHandle))
        {
            *pMutex = pstHandleInfo;
            return EVENT_MUX_ERR_OK;
        }
        free(pstHandleInfo);
        return EVENT_MUX_ERR_FAILURE;
    }
    return EVENT_MUX_ERR_OK;
}

int32_t MutexDestroy(void* pMutex)
{
    HANDLE_2_INFO(pMutex);
    int32_t iRet = EVENT_MUX_ERR_OK;
    if(NULL == pstHandleInfo->szName || !strlen(pstHandleInfo->szName))
    {
        iRet = MutexDestroy_(pstHandleInfo->stHandle);
        free(pstHandleInfo->stHandle);
    }        
    else
        iRet = ProcessMutexDestroy(pstHandleInfo->stHandle);
    free(pstHandleInfo);

    return iRet;
}

int32_t MutexTimedLock(void* pMutex, int32_t i32TimeoutMSec)
{
    HANDLE_2_INFO(pMutex);
    if(NULL == pstHandleInfo->szName || !strlen(pstHandleInfo->szName))
        return MutexTimedLock_(pstHandleInfo->stHandle, i32TimeoutMSec);
    return ProcessMutexTimedLock(pstHandleInfo->stHandle, i32TimeoutMSec);
}

int32_t MutexLock(void* pMutex)
{
    HANDLE_2_INFO(pMutex);
    if(NULL == pstHandleInfo->szName || !strlen(pstHandleInfo->szName))
        return MutexLock_(pstHandleInfo->stHandle);
    return ProcessMutexLock(pstHandleInfo->stHandle);
}

int32_t MutexUnLock(void* pMutex)
{
    HANDLE_2_INFO(pMutex);
    if(NULL == pstHandleInfo->szName || !strlen(pstHandleInfo->szName))
        return MutexUnLock_(pstHandleInfo->stHandle);
    return ProcessMutexUnLock(pstHandleInfo->stHandle);
}

int32_t EventCreate(void** pEvt, int8_t bManual, int8_t bInitialState, const char* pszName)
{
    int32_t iRet = EVENT_MUX_ERR_OK;
    handle_info_t *pstHandleInfo = (handle_info_t *)calloc(1, sizeof(handle_info_t));
    if(NULL == pstHandleInfo)
        return EVENT_MUX_ERR_NOMEMORY;
        

    if(NULL == pszName || !strlen(pszName))
    {
        iRet = EventCreate_(&pstHandleInfo->stHandle, bManual, bInitialState, pszName);
        if(EVENT_MUX_ERR_OK == iRet)
            *pEvt = pstHandleInfo;
        else
            free(pstHandleInfo);
        return iRet;
    }
    else
    {
        uint32_t u32Lens = strlen(pszName);
        if(u32Lens > sizeof(pstHandleInfo->szName))
            return EVENT_MUX_ERR_NAME_TOO_LONG;
        u32Lens = u32Lens > sizeof(pstHandleInfo->szName) ? sizeof(pstHandleInfo->szName) : u32Lens;
        memset(pstHandleInfo->szName, 0, sizeof(pstHandleInfo->szName));
        memcpy(pstHandleInfo->szName, pszName, u32Lens);
        iRet = ProcessEventCreate(&pstHandleInfo->stHandle, bManual, bInitialState, pszName);
        if(EVENT_MUX_ERR_OK == iRet)
            *pEvt = pstHandleInfo;
        else
            free(pstHandleInfo);
        return iRet;        
    }

    return EVENT_MUX_ERR_FAILURE;
}

int32_t EventOpen(void** pEvt, const char* pszName)
{
    int32_t iRet = EVENT_MUX_ERR_OK;
    handle_info_t *pstHandleInfo = (handle_info_t *)calloc(1, sizeof(handle_info_t));
    if(NULL == pstHandleInfo)
        return EVENT_MUX_ERR_NOMEMORY;
        

    if(NULL == pszName || !strlen(pszName))
    {
        iRet = EventOpen_(&pstHandleInfo->stHandle, pszName);
        if(EVENT_MUX_ERR_OK == iRet)
            *pEvt = pstHandleInfo;
        else
            free(pstHandleInfo);
        return iRet;
    }
    else
    {
        uint32_t u32Lens = strlen(pszName);
        if(u32Lens > sizeof(pstHandleInfo->szName))
            return EVENT_MUX_ERR_NAME_TOO_LONG;
        u32Lens = u32Lens > sizeof(pstHandleInfo->szName) ? sizeof(pstHandleInfo->szName) : u32Lens;
        memset(pstHandleInfo->szName, 0, sizeof(pstHandleInfo->szName));
        memcpy(pstHandleInfo->szName, pszName, u32Lens);
        iRet = ProcessEventOpen(&pstHandleInfo->stHandle, pszName);
        if(EVENT_MUX_ERR_OK == iRet)
            *pEvt = pstHandleInfo;
        else
            free(pstHandleInfo);
        return iRet;        
    }    
}

int32_t EventClose(void* pEvt)
{
    HANDLE_2_INFO(pEvt);
    int32_t iRet = EVENT_MUX_ERR_OK;
    if(!strlen(pstHandleInfo->szName))
    {
        iRet = EventClose_(pstHandleInfo->stHandle);
    }
    else
    {
        iRet = ProcessEventClose(pstHandleInfo->stHandle);
    }
    free(pstHandleInfo);
    return iRet;
}

int32_t EventSet(void* pEvt)
{
    HANDLE_2_INFO(pEvt);
    if(!strlen(pstHandleInfo->szName))
    {
        return EventSet_(pstHandleInfo->stHandle);
    }
    return ProcessEventSet(pstHandleInfo->stHandle);
}

int32_t EventReset(void* pEvt)
{
    HANDLE_2_INFO(pEvt);
    if(!strlen(pstHandleInfo->szName))
    {
        return EventReset_(pstHandleInfo->stHandle);
    }
    return ProcessEventReset(pstHandleInfo->stHandle);
}

int32_t EventWait(void* pEvt, int32_t u32TimeoutMSec)
{
    HANDLE_2_INFO(pEvt);
    if(!strlen(pstHandleInfo->szName))
    {
        return EventWait_(pstHandleInfo->stHandle, u32TimeoutMSec);
    }
    return ProcessEventWait(pstHandleInfo->stHandle, u32TimeoutMSec); 
}
