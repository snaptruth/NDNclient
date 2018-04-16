/*
 * bnlogif.h
 *
 *  Created on: 2017年8月23日
 */

#ifndef BNLOGIF_H_
#define BNLOGIF_H_

#include <assert.h>
#include <stdio.h>
#include <sys/msg.h>
#include <string>
#include "include/bnstruct.h"
#include "include/bnconst.h"

extern int g_iDispatchThreadId;
extern int g_iModuleMask;
extern int g_iLastLogSecond;
extern int g_iLastLogMicroSecond;
extern std::string g_strLastLogSecond;
extern std::string g_strLastLogMicroSecond;

ERRCODE InitBnLog(std::string strModuleName, int iModuleMask);

ERRCODE ClearAllLogFile();

void BnLogEnableDebug(bool bEnableDebug);

void BnLogEnableLog(int iLogMask);

int BnLogPaddingTime(char * pcBuffer,int iBufLength);
int BnLogPaddingThreadName(char * pcBuffer,int iBufLength);

/*
 *  Micro for internal use
 */
#define BN_LOG_DEBUG(fm, ...) \
    {\
        char arrcStrBuffer[2048] = {'\0'};\
        int iStrLength = sizeof(struct ThreadMsg);\
        struct ThreadMsg *pThreadMsg =(struct ThreadMsg *)arrcStrBuffer;\
        pThreadMsg->m_id = g_iModuleMask | BN_MSG_LOG_DEBUG;\
        iStrLength += BnLogPaddingTime(arrcStrBuffer + iStrLength, 2046);\
        iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, "[%s:%d] [%s] ", \
                __FILE__, __LINE__, __FUNCTION__);\
        iStrLength += BnLogPaddingThreadName(arrcStrBuffer + iStrLength, 2046 - iStrLength);\
        if(iStrLength > 0 && iStrLength < 2046 ){\
            iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, fm, ##__VA_ARGS__);\
        }\
        arrcStrBuffer[iStrLength] = '\0';\
        msgsnd(g_iDispatchThreadId, (void *)arrcStrBuffer, iStrLength + 1, IPC_NOWAIT);\
    }

#define BN_LOG_INTERFACE(fm, ...) \
    {\
        char arrcStrBuffer[2048] = {'\0'};\
        int iStrLength = sizeof(struct ThreadMsg);\
        struct ThreadMsg *pThreadMsg =(struct ThreadMsg *)arrcStrBuffer;\
        pThreadMsg->m_id = g_iModuleMask | BN_MSG_LOG_INTERFACE;\
        iStrLength += BnLogPaddingTime(arrcStrBuffer + iStrLength, 2046);\
        iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, "[%s:%d] [%s] ", \
                __FILE__, __LINE__, __FUNCTION__);\
        iStrLength += BnLogPaddingThreadName(arrcStrBuffer + iStrLength, 2046 - iStrLength);\
        if(iStrLength > 0 && iStrLength < 2046 ){\
            iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, fm, ##__VA_ARGS__);\
        }\
        arrcStrBuffer[iStrLength] = '\0';\
        msgsnd(g_iDispatchThreadId, (void *)arrcStrBuffer, iStrLength + 1, IPC_NOWAIT);\
    }

#define BN_LOG_ERROR(fm, ...) \
    {\
        char arrcStrBuffer[2048] = {'\0'};\
        int iStrLength = sizeof(struct ThreadMsg);\
        struct ThreadMsg *pThreadMsg =(struct ThreadMsg *)arrcStrBuffer;\
        pThreadMsg->m_id = g_iModuleMask | BN_MSG_LOG_ERROR;\
        iStrLength += BnLogPaddingTime(arrcStrBuffer + iStrLength, 2046);\
        iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, "[%s:%d] [%s] ", \
                __FILE__, __LINE__, __FUNCTION__);\
        iStrLength += BnLogPaddingThreadName(arrcStrBuffer + iStrLength, 2046 - iStrLength);\
        if(iStrLength > 0 && iStrLength < 2046 ){\
            iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, fm, ##__VA_ARGS__);\
        }\
        arrcStrBuffer[iStrLength] = '\0';\
        msgsnd(g_iDispatchThreadId, (void *)arrcStrBuffer, iStrLength + 1, IPC_NOWAIT);\
    }

#define BN_LOG_FATAL(fm, ...) \
    {\
        char arrcStrBuffer[2048] = {'\0'};\
        int iStrLength = sizeof(struct ThreadMsg);\
        struct ThreadMsg *pThreadMsg =(struct ThreadMsg *)arrcStrBuffer;\
        pThreadMsg->m_id = g_iModuleMask | BN_MSG_LOG_FATAL;\
        iStrLength += BnLogPaddingTime(arrcStrBuffer + iStrLength, 2046);\
        iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, "[%s:%d] [%s] ", \
                __FILE__, __LINE__, __FUNCTION__);\
        iStrLength += BnLogPaddingThreadName(arrcStrBuffer + iStrLength, 2046 - iStrLength);\
        if(iStrLength > 0 && iStrLength < 2046 ){\
            iStrLength += snprintf(arrcStrBuffer + iStrLength, 2046 - iStrLength, fm, ##__VA_ARGS__);\
        }\
        arrcStrBuffer[iStrLength] = '\0';\
        msgsnd(g_iDispatchThreadId, (void *)arrcStrBuffer, iStrLength + 1, IPC_NOWAIT);\
    }

/*
 * Micro for easy use
 */
// 检查函数返回值，不成功即返回错误值，DEBUG版本主动挂起
#define BN_CHECK_RETURN(expression) {\
                ERRCODE returnValue = expression;\
                if(returnValue != SUCCESS){\
                    BN_LOG_ERROR("Return ERRCODE: %d", returnValue);\
                    assert(true);\
                    return returnValue;\
                }\
            }

// 检查函数返回值，不成功即返回错误值，不挂起
#define BN_CHECK_RETURN_NOASSERT(expression) {\
                ERRCODE returnValue = expression;\
                if(returnValue != SUCCESS){\
                    BN_LOG_ERROR("Return ERRCODE: %d", returnValue);\
                    return returnValue;\
                }\
            }

// 检查函数返回值，不成功即返回指定错误码，DEBUG版本主动挂起
#define BN_CHECK_RETURN_NEWCODE(expression,errCode) {\
                ERRCODE returnValue = expression;\
                if(returnValue != SUCCESS){\
                    BN_LOG_ERROR("Return ERRCODE: %d", returnValue);\
                    assert(true);\
                    return errCode;\
                }\
            }

// 检查函数返回值，不成功即返回指定错误码，不挂起
#define BN_CHECK_RETURN_NOASSERT_NEWCODE(expression,errCode) {\
                ERRCODE returnValue = expression;\
                if(returnValue != SUCCESS){\
                    BN_LOG_ERROR("Return ERRCODE: %d", returnValue);\
                    return errCode;\
                }\
            }

// 检查函数返回值，不成功即返回，DEBUG版本主动挂起
#define BN_CHECK_RETURN_RETNULL(expression) {\
                ERRCODE returnValue = expression;\
                if(returnValue != SUCCESS){\
                    BN_LOG_ERROR("Return ERRCODE: %d", returnValue);\
                    assert(true);\
                    return ;\
                }\
            }

// 检查函数返回值，不成功即返回，不挂起
#define BN_CHECK_RETURN_NOASSERT_RETNULL(expression) {\
                ERRCODE returnValue = expression;\
                if(returnValue != SUCCESS){\
                    BN_LOG_ERROR("Return ERRCODE: %d", returnValue);\
                    return ;\
                }\
            }

// 检查指针值是否为空，为空即返回指针为空，DEBUG版本主动挂起
#define BN_CHECK_POINTER(ptr) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    assert(true);\
                    return ERROR_POINTERISNULL;\
                }\
            }

// 检查指针值是否为空，为空即返回指针为空，DEBUG版本主动挂起, Break跳出循环
#define BN_CHECK_POINTER_BREAK(ptr) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    assert(true);\
                    break;\
                }\
            }

// 检查指针值是否为空，为空即返回指针为空，不挂起
#define BN_CHECK_POINTER_NOASSERT(ptr) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    return ERROR_POINTERISNULL;\
                }\
            }

// 检查指针值是否为空，为空即返回指定错误码，DEBUG版本主动挂起
#define BN_CHECK_POINTER_NEWCODE(ptr,errCode) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    assert(true);\
                    return errCode;\
                }\
            }

// 检查指针值是否为空，为空即返回指定错误码，不挂起
#define BN_CHECK_POINTER_NOASSERT_NEWCODE(ptr,errCode) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    return errCode;\
                }\
            }

// 检查指针值是否为空，为空则返回，DEBUG版本主动挂起
#define BN_CHECK_POINTER_VOID(ptr) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    assert(true);\
                    return;\
                }\
            }

// 检查指针值是否为空，为空则返回，不挂起
#define BN_CHECK_POINTER_NOASSERT_VOID(ptr) {\
                if(NULL == ptr){\
                    BN_LOG_ERROR("Pointer is 0x%p!", ptr);\
                    return;\
                }\
            }

// 判断表达式是否错误，错误则返回
#define BN_CHECK_CONDITION_RET(express,err) {\
                if(!(express)){\
                    BN_LOG_ERROR("Express return %d", false);\
                    return err;\
                }\
            }

// 判断表达式是否错误，错误则返回
#define BN_CHECK_CONDITION_RETUNKNOWN(express,str) {\
                if(!(express)){\
                    BN_LOG_ERROR(str);\
                    return ERROR_UNKNOWN;\
                }\
            }


#endif /* BNLOGIF_H_ */
