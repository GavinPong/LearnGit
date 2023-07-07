#ifndef __eventMutexFunc_h__
#define __eventMutexFunc_h__

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "utilToolsDef.h"

#ifdef WIN32
#pragma warning(disable:4996)

#include <assert.h>
#include <windows.h>
#include <process.h>
#include <io.h>
#endif

#ifdef __linux__
#include <pthread.h> 
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <semaphore.h>
#include <dlfcn.h>
#endif

#define EVENT_MUX_ERR_OK 1
#define EVENT_MUX_ERR_FAILURE 0
#define EVENT_MUX_ERR_TIMEOUT -1
#define EVENT_MUX_ERR_INVALID_PARAM -2
#define EVENT_MUX_ERR_NOMEMORY -3
#define EVENT_MUX_ERR_NAME_TOO_LONG -4
#define EVENT_MUX_ERR_NAME_EXIST -5
#define EVENT_MUX_ERR_CREATE_MUX -6
#define EVENT_MUX_ERR_CREATE_SHMEM -7

#ifdef __cpulsplus
extern "C" 
{
#endif

/**
 * @brief Create Mutex
 * 
 * @param pszName If it is not null,it can use in process otherwise in thread.
 * @param pMutex It is mutex handle.
 * @return int32_t On success return 1,otherwise retun other value.
 */
UTILTOOLS_API int32_t MutexCreate(const char* pszName, void **pMutex);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t MutexDestroy(void* pMutex);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t MutexTimedLock(void* pMutex, int32_t i32TimeoutMSec);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t MutexLock(void* pMutex);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t MutexUnLock(void* pMutex);
/**
 * @brief Create Event
 * 
 * @param pEvt              It is Event handle
 * @param bManualRest       1-manual rest,0-auto reset
 * @param bInitialState     Initial state of Event
 * @param pszName           If it is not null,it can use in process otherwise in thread.
 * @return  int32_t         On success return 1,otherwise retun other value.
 */
UTILTOOLS_API int32_t EventCreate(void** pEvt, int8_t bManualRest, int8_t bInitialState, const char* pszName);
/**
 * @brief Open it when event it already exists
 * 
 * @param pEvt              It is Event handle
 * @param pszName           Event's name
 * @return int32_t          On success return 1,otherwise retun other value.
 */
UTILTOOLS_API int32_t EventOpen(void** pEvt, const char* pszName);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t EventClose(void* pEvt);
/**
 * @brief Set Event to has Signal, It is going wake up all waiters when bManualRest is true and event is set no signal.
 *        It is going wake up one of all waiters when bManualRest is false and event is set no signal.
 * @note  Always set to no signal after call it. 
 * 
 */
UTILTOOLS_API int32_t EventSet(void* pEvt);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t EventReset(void* pEvt);
/**
 * @brief 
 * 
 */
UTILTOOLS_API int32_t EventWait(void* pEvt, int32_t u32TimeoutMSec);

#ifdef __cpulsplus
}
#endif

#endif/*__eventMutexFunc_h__*/