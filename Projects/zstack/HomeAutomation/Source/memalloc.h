
/*
 *
 * */

#ifndef __MO_MEMALLOC_H__
#define __MO_MEMALLOC_H__

#include <hal_types.h>
#include "OSAL_Memory.h"
#include "OSAL.h"
#ifdef __cplusplus
extern "C" {
#endif

#define mo_malloc(_size)    osal_mem_alloc(_size)
#define mo_free(_ptr)       osal_mem_free(_ptr)
//void *mo_calloc(size_t nmemb, size_t size);
//void *mo_realloc(void *ptr, size_t size);

#ifndef memcpy
#define memcpy(dst, src, len) osal_memcpy(dst, src, len)
#endif

#ifndef memset
#define memset(dst, val, len) osal_memset(dst, val, len)
#endif

#ifndef memcmp
#define memcmp(src1, src2, len) osal_memcmp(src1, src2, len)
#endif

#ifndef strlen
#define strlen(string) osal_strlen(string)
#endif

#ifndef UBOUND
#define UBOUND(__ARRAY) (sizeof(__ARRAY)/sizeof(__ARRAY[0]))
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __MO_MEMALLOC_H__ */

