#include "shareMemFunc.h"
#include "eventMutexFunc.h"
#include <stdlib.h>

#if defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<errno.h>
#include <sys/mman.h>
#include <unistd.h>
#else

#endif

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

#if defined(_WIN32) || defined(WIN32)
#define snprintf(buf,len, format,...) _snprintf_s(buf, len, len-1, format, __VA_ARGS__)
#endif

#define SHARE_PATH_NAME_STR_MAX 256

typedef struct _share_mem_ctx_s{
    char strName[128];
    /* linux平台使用　*/
    int64_t i64MemSize;
    void *pVirAddr;
    /* windows平台使用 */
    void *pMapView;
    void *hMapping;
    void *hFile;
}share_mem_ctx_t;

#if defined(__linux__)
int32_t shareMemFunc_Create(int64_t *pi64ShareMemHandle, void **pShareMemVirAddr, int64_t i64MemSize, const char *pstrShareMemName)
{
    if(NULL == pstrShareMemName || 0 >= strlen(pstrShareMemName))
        return SHARE_MEMORY_ERR_INVALID_PARAM;

    char strDstName[SHARE_PATH_NAME_STR_MAX];
    int32_t iRet;
    void *muxHandle;
    bool bLocked = 0;
    int32_t i32Fd = -1;
    void *pVirMem = NULL;
    if((strlen(pstrShareMemName) + strlen("_ShmMutex")) >= sizeof(strDstName))
        return SHARE_MEMORY_ERR_NAME_TOO_LONG;

    snprintf(strDstName, sizeof(strDstName), "%s_ShmMutex", pstrShareMemName);
    iRet = MutexCreate(strDstName, &muxHandle);
    if(EVENT_MUX_ERR_OK != iRet)
        return iRet;

    #if 1/* 超时，就算了 */
    iRet = MutexTimedLock(muxHandle, 4000);
    #else/* 必须拿到锁 */
    iRet = MutexLock(muxHandle);
    #endif
    if(EVENT_MUX_ERR_OK == iRet)
        bLocked = 1;
        
    iRet = SHARE_MEMORY_ERR_OK;
    do
    {
        char* home = getenv("HOME"); 
        if(strlen(pstrShareMemName) + strlen(home)  + strlen("/.shmem/")>= sizeof(strDstName))
        {
            iRet = SHARE_MEMORY_ERR_NAME_TOO_LONG;
            break;
        }            
        snprintf(strDstName, sizeof(strDstName), "%s/.shmem", home);
        if(0 != access(strDstName, F_OK))
        {
            if(0 != mkdir(strDstName, 0664))
            {
                iRet = SHARE_MEMORY_ERR_MKDIR_FAILED;
                break;
            }
        }
            
        snprintf(strDstName, sizeof(strDstName), "%s/.shmem/%s", home, pstrShareMemName);
        i32Fd = open(strDstName, O_RDWR);
        if(i32Fd < 0)
        {
            mode_t mode = umask(0);
            i32Fd = open(strDstName, O_RDWR | O_CREAT | O_EXCL, 0664);
            if(i32Fd < 0)
            {
                //umask(mode);
                iRet = SHARE_MEMORY_ERR_OPEN_FILE_FAILED;
                break;
            }
            umask(mode);
        }
        if (ftruncate(i32Fd, (off_t)i64MemSize) < 0)
        {
            DBG_PRT("%s->%d:ftruncate failed with errno:%d\r\n", __FILE__, __LINE__, errno);
            iRet = SHARE_MEMORY_ERR_NOMEMORY;
            break;
        }
        struct stat st;
        if ((fstat(i32Fd, &st)) == -1)
        {
            DBG_PRT("%s->%d:get fstat failed with errno:%d\r\n", __FILE__, __LINE__, errno);
            iRet = SHARE_MEMORY_ERR_FAILURE;
            break;
        }
        pVirMem = (void*)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, i32Fd, 0);
        if (MAP_FAILED == pVirMem)
        {
            DBG_PRT("%s->%d:mmap failed with errno:%d\r\n", __FILE__, __LINE__, errno);
            iRet = SHARE_MEMORY_ERR_MAP_FAILED;
            break;
        }
    }while(0);

    if(i32Fd > 0)
        close(i32Fd);

    if(bLocked)
        MutexUnLock(muxHandle);

    MutexDestroy(muxHandle);

    if(SHARE_MEMORY_ERR_OK != iRet)        
        return iRet;

    share_mem_ctx_t *pstShareMemCtx = (share_mem_ctx_t *)calloc(1, sizeof(share_mem_ctx_t));
    if(NULL == pstShareMemCtx)
    {
        DBG_PRT("%s->%d:calloc sizeof(share_mem_ctx_t) failed errno:%d\r\n", __FILE__, __LINE__, errno);
        iRet = SHARE_MEMORY_ERR_NOMEMORY;
    }
    snprintf(pstShareMemCtx->strName, sizeof(pstShareMemCtx->strName), "%s", pstrShareMemName);
    pstShareMemCtx->i64MemSize = i64MemSize;
    pstShareMemCtx->pVirAddr = pVirMem;  
    *pi64ShareMemHandle = (int64_t)pstShareMemCtx;
    *pShareMemVirAddr = pVirMem;
    
    return iRet; 
}

int32_t shareMemFunc_Destroy(int64_t i64ShareMemHandle)
{
    int32_t iRet = SHARE_MEMORY_ERR_FAILURE;
    share_mem_ctx_t *pstShareMemCtx = (share_mem_ctx_t *)i64ShareMemHandle;
    if(pstShareMemCtx->pVirAddr && pstShareMemCtx->i64MemSize > 0)
    {
        char strDstName[SHARE_PATH_NAME_STR_MAX];
        void *muxHandle = NULL;
        bool bLocked = 0;

        snprintf(strDstName, sizeof(strDstName), "%s_Mutex", pstShareMemCtx->strName);
        iRet = MutexCreate(strDstName, &muxHandle);
        if(EVENT_MUX_ERR_OK == iRet)
        {
            /* 避免死等，等不到，就算了 */
            iRet = MutexTimedLock(muxHandle, 4000);
            if(EVENT_MUX_ERR_OK == iRet)
                bLocked = 1;
            else
                DBG_PRT("%s->%d:Create Mutex[%s] failed with %d\r\n", __FILE__, __LINE__, strDstName, iRet);
        }
        else
        {
            DBG_PRT("%s->%d:Create Mutex[%s] failed with %d\r\n", __FILE__, __LINE__, strDstName, iRet);
        }

        //if(MAP_FAILED != (void *)munmap(pstShareMemCtx->pVirAddr, (size_t)(pstShareMemCtx->i64MemSize)))
        if(-1 != munmap(pstShareMemCtx->pVirAddr, (size_t)(pstShareMemCtx->i64MemSize)))
            iRet = SHARE_MEMORY_ERR_OK;        
        else
            iRet = SHARE_MEMORY_ERR_FAILURE;   

        if(bLocked)
        {
            MutexUnLock(muxHandle);
            MutexDestroy(muxHandle);
            muxHandle = NULL;
        }
        else if(muxHandle)
        {
            MutexDestroy(muxHandle);
        }
    }
    free(pstShareMemCtx);
    return iRet;
}
#endif /* __linux __*/

#if defined(_WIN32)
int32_t shareMemFunc_Create(int64_t *pi64ShareMemHandle, void **pShareMemVirAddr, int64_t i64MemSize, const char *pstrShareMemName)
{
    if(NULL == pstrShareMemName || 0 >= strlen(pstrShareMemName))
        return SHARE_MEMORY_ERR_INVALID_PARAM;

    char strDstName[SHARE_PATH_NAME_STR_MAX];
    int32_t iRet = SHARE_MEMORY_ERR_OK;
    void *muxHandle;
    bool bLocked = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = INVALID_HANDLE_VALUE;
    void *pVirMem = NULL;
    if((strlen(pstrShareMemName) + strlen("_ShmMutex")) >= sizeof(strDstName))
        return SHARE_MEMORY_ERR_NAME_TOO_LONG;

    snprintf(strDstName, sizeof(strDstName), "%s_ShmMutex", pstrShareMemName);
    iRet = MutexCreate(strDstName, &muxHandle);
    if(EVENT_MUX_ERR_OK != iRet)
        return iRet;
      
    #if 1/* 超时，就算了 */
    iRet = MutexTimedLock(muxHandle, 4000);
    #else/* 必须拿到锁 */
    iRet = MutexLock(muxHandle);
    #endif
    if(EVENT_MUX_ERR_OK == iRet)
        bLocked = 1;
        
    iRet = SHARE_MEMORY_ERR_OK;
    do
    {
        char* home = getenv("USERPROFILE"); 
        if(strlen(pstrShareMemName) + strlen(home)  + strlen("/.shmem/")>= sizeof(strDstName))
        {
            iRet = SHARE_MEMORY_ERR_NAME_TOO_LONG;
            break;
        }            
        snprintf(strDstName, sizeof(strDstName), "%s/.shmem", home);
        if(0 != _access(strDstName, 0))
        {
            if(0 == CreateDirectory(strDstName, NULL))
            {
                DBG_PRT("%s->%d:CreateDirectory %s failed,errno %d\r\n", __FILE__, __LINE__, strDstName, GetLastError());
                iRet = SHARE_MEMORY_ERR_MKDIR_FAILED;
                break;
            }
        }
            
        snprintf(strDstName, sizeof(strDstName), "%s/.shmem/%s", home, pstrShareMemName);
        hFile = CreateFile(
            strDstName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,/* 不存在，则创建　*/
            FILE_ATTRIBUTE_HIDDEN,
            NULL
            ); 
        if(INVALID_HANDLE_VALUE == hFile)       
        {
            DBG_PRT("%s->%d:CreateDirectory %s failed,errno %d\r\n", __FILE__, __LINE__, strDstName, GetLastError());
            if(1 == bLocked)
            {
                iRet = SHARE_MEMORY_ERR_OPEN_FILE_FAILED;
                break;
            }
        }
        memset(strDstName, 0, sizeof(strDstName));
		sprintf(strDstName, "%s.shareMem.mapp", pstrShareMemName);

        hMapping = CreateFileMapping(hFile, 0, PAGE_READWRITE, 0, (DWORD)i64MemSize + 0x1000, strDstName);
        if(0 == hMapping)
        {
            int32_t err = GetLastError();
            if(ERROR_ALREADY_EXISTS == err)
            {
                hMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, strDstName);
            }
        }

        if (0 == hMapping)
        {
            DBG_PRT("%s->%d:CreateFileMapping %s failed,errno %d\r\n", __FILE__, __LINE__, strDstName, GetLastError());
            iRet = SHARE_MEMORY_ERR_MAP_FAILED;
            break;
        }	
        pVirMem = (void *)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);	
        if(NULL == pVirMem)
        {
            DBG_PRT("%s->%d:MapViewOfFile %s failed,errno %d\r\n", __FILE__, __LINE__, strDstName, GetLastError());
            iRet = SHARE_MEMORY_ERR_MAP_FAILED;
            break;
        }
    }while(0);

    if(SHARE_MEMORY_ERR_OK != iRet)
    {
        if(pVirMem)
        {
            UnmapViewOfFile(pVirMem);
        }
        if(hMapping > 0)
        {
            CloseHandle(hMapping); 
        }
        if(INVALID_HANDLE_VALUE != hFile)
        {
            CloseHandle(hFile);
        }            
    }


    if(bLocked)
        MutexUnLock(muxHandle);

    MutexDestroy(muxHandle);

    if(SHARE_MEMORY_ERR_OK != iRet)        
        return iRet;

    share_mem_ctx_t *pstShareMemCtx = (share_mem_ctx_t *)calloc(1, sizeof(share_mem_ctx_t));
    if(NULL == pstShareMemCtx)
    {
        DBG_PRT("%s->%d:calloc sizeof(share_mem_ctx_t) failed errno:%d\r\n", __FILE__, __LINE__, errno);
        iRet = SHARE_MEMORY_ERR_NOMEMORY;
    }
    snprintf(pstShareMemCtx->strName, sizeof(pstShareMemCtx->strName), "%s", pstrShareMemName);
    pstShareMemCtx->i64MemSize = i64MemSize;
    pstShareMemCtx->pVirAddr = pVirMem;  
    pstShareMemCtx->hFile = hFile;
    pstShareMemCtx->hMapping = hMapping;
    pstShareMemCtx->pMapView = pVirMem;
    *pi64ShareMemHandle = (int64_t)pstShareMemCtx;
    *pShareMemVirAddr = pVirMem;
    
    return iRet; 
}

int32_t shareMemFunc_Destroy(int64_t i64ShareMemHandle)
{
    int32_t iRet = SHARE_MEMORY_ERR_OK;
    share_mem_ctx_t *pstShareMemCtx = (share_mem_ctx_t *)i64ShareMemHandle;
    if(pstShareMemCtx->pVirAddr && pstShareMemCtx->i64MemSize > 0)
    {
        char strDstName[SHARE_PATH_NAME_STR_MAX];
        void *muxHandle = NULL;
        bool bCreateMux = 0, bLocked = 0;

        snprintf(strDstName, sizeof(strDstName), "%s_Mutex", pstShareMemCtx->strName);
        iRet = MutexCreate(strDstName, &muxHandle);
        if(EVENT_MUX_ERR_OK == iRet)
        {
            bCreateMux = 1;            
            #if 1/* 等不到，就算了 */
            iRet = MutexTimedLock(muxHandle, 4000);
            #else/* 必须等待锁　*/
            iRet = MutexLock(muxHandle);
            #endif
            if(EVENT_MUX_ERR_OK == iRet)
                bLocked = 1;
            else
                DBG_PRT("%s->%d:Logk Mutex[%s] failed with %d\r\n", __FILE__, __LINE__, strDstName, iRet);
        }
        else
        {
            DBG_PRT("%s->%d:Create Mutex[%s] failed with %d\r\n", __FILE__, __LINE__, strDstName, iRet);
        }

        if(pstShareMemCtx->pMapView)
        {
            UnmapViewOfFile(pstShareMemCtx->pMapView);
        }
        if(pstShareMemCtx->hMapping > 0)
        {
            CloseHandle(pstShareMemCtx->hMapping); 
        }
        if(INVALID_HANDLE_VALUE != pstShareMemCtx->hFile)
        {
            CloseHandle(pstShareMemCtx->hFile);
        }  

        if(bLocked)
        {
            MutexUnLock(muxHandle);
            MutexDestroy(muxHandle);
            muxHandle = NULL;
        }
        else if(muxHandle)
        {
            MutexDestroy(muxHandle);
        }
    }
    free(pstShareMemCtx);
    return iRet;
}
#endif

