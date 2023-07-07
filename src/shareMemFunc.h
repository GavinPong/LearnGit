#ifndef __shareMemFunc_h__
#define __shareMemFunc_h__

#include <stdint.h>
#include "utilToolsDef.h"

#define SHARE_MEMORY_ERR_OK 1
#define SHARE_MEMORY_ERR_FAILURE 0
#define SHARE_MEMORY_ERR_TIMEOUT -1
#define SHARE_MEMORY_ERR_INVALID_PARAM -2
#define SHARE_MEMORY_ERR_NOMEMORY -3
#define SHARE_MEMORY_ERR_NAME_TOO_LONG -4
#define SHARE_MEMORY_ERR_SIZE_TOO_LARGE -5
#define SHARE_MEMORY_ERR_OPEN_FILE_FAILED -6
#define SHARE_MEMORY_ERR_MAP_FAILED -7
#define SHARE_MEMORY_ERR_MKDIR_FAILED -8

#define SHARE_MEM_MODE_RD       1 << 0 /*read mode*/
#define SHARE_MEM_MODE_RDWR     1 << 1 /*Write mode*/

/**
 * @brief Create share memory
 * 
 * @param pi64ShareMemHandle   pointter of store handle of share memory
 * @param pShareMemVirAddr     pointer of virtual address of share memory
 * @param i64MemSize           size of share memory
 * @param pstrShareMemName     name of share memory,it must be the only one
 * @return int32_t on success return 1,otherwise retun other value
 */
UTILTOOLS_API int32_t shareMemFunc_Create(int64_t *pi64ShareMemHandle, void **pShareMemVirAddr, int64_t i64MemSize, const char *pstrShareMemName);
/**
 * @brief destroy share memory
 * 
 * @param i64ShareMemHandle handle of share memory
 * @return int32_t on success return 1,otherwise retun other value
 */
UTILTOOLS_API int32_t shareMemFunc_Destroy(int64_t i64ShareMemHandle);


#endif