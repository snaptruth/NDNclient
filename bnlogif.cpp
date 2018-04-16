/*
 * bnlogif.cpp
 *
 *  Created on: 2017年9月28日
 */
#include "bnlogif.h"

#include <string.h>
#include <malloc.h>
#include "include/bnosutils.h"

#include <sys/prctl.h>
#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

int g_iDispatchThreadId = 0;
int g_iModuleMask = 0;
int g_iLastLogSecond = 0;
int g_iLastLogMicroSecond = 0;
std::string g_strLastLogSecond = "";
std::string g_strLastLogMicroSecond = "";

ERRCODE InitBnLog(std::string strModuleName, int iModuleMask){
    g_iDispatchThreadId = msgget(BM_MSG_QUEUE_LOGDISTRIBUTE, 0666 | IPC_CREAT);

    // Init log thread
    iModuleMask = iModuleMask << 16;
    struct ThreadMsg *pThreadMsg;
    char *pcNewData = NULL;
    int iMsgLength = strModuleName.length() + sizeof(struct ThreadMsg) + 1;
    pcNewData = (char *)malloc(iMsgLength);
    if(NULL == pcNewData){
        return ERROR_NOMEMORY;
    }
    memset(pcNewData, '\0', iMsgLength);
    pThreadMsg = (struct ThreadMsg *)pcNewData;
    pThreadMsg->m_id = iModuleMask | BN_MSG_LOG_CREATE;

    if(strModuleName.length() > 0){
        memcpy(pcNewData + sizeof(struct ThreadMsg), strModuleName.c_str(), strModuleName.length());
    }
    if(-1 == msgsnd(g_iDispatchThreadId, (void*)pcNewData, iMsgLength, IPC_NOWAIT)){
        printf("Can't send message to DispatchThreadId:0x%x\n", g_iDispatchThreadId);
    }
    free(pcNewData);

    // Init
    g_iModuleMask = iModuleMask;

    return SUCCESS;
}

ERRCODE ClearAllLogFile(){
    struct ThreadMsg threadMsg;
    threadMsg.m_id = g_iModuleMask | BN_MSG_LOG_CLEAR;

    msgsnd(g_iDispatchThreadId, (void*)&threadMsg, sizeof(struct ThreadMsg), IPC_NOWAIT);
    return SUCCESS;
}

void BnLogEnableDebug(bool bEnableDebug){
    char arrcStrBuffer[64] = {'\0'};
    struct ThreadMsg *pThreadMsg =(struct ThreadMsg *)arrcStrBuffer;
    pThreadMsg->m_id = g_iModuleMask | BN_MSG_LOG_ENABLE;
    int iLogMask = BN_LOGLEVEL_INTERFACE
                    | BN_LOGLEVEL_ERROR
                    | BN_LOGLEVEL_FATAL;
    if(bEnableDebug){
        iLogMask |= BN_LOGLEVEL_DEBUG;
    }
    memcpy(arrcStrBuffer + sizeof(struct ThreadMsg), &iLogMask, sizeof(int));

    msgsnd(g_iDispatchThreadId, (void *)arrcStrBuffer, sizeof(struct ThreadMsg) + sizeof(int), 0);
}

void BnLogEnableLog(int iLogMask){
    char arrcStrBuffer[64] = {'\0'};
    struct ThreadMsg *pThreadMsg =(struct ThreadMsg *)arrcStrBuffer;
    pThreadMsg->m_id = g_iModuleMask | BN_MSG_LOG_ENABLE;
    memcpy(arrcStrBuffer + sizeof(struct ThreadMsg), &iLogMask, sizeof(int));
    msgsnd(g_iDispatchThreadId, (void *)arrcStrBuffer, sizeof(struct ThreadMsg) + sizeof(int), 0);
}

int BnLogPaddingTime(char * pcBuffer, int iBufLength){
    struct timeval tTimeOfDay;
    gettimeofday(&tTimeOfDay, NULL);

    if(g_iLastLogSecond != tTimeOfDay.tv_sec){
        g_iLastLogSecond = tTimeOfDay.tv_sec;

        struct tm* pTime;
        pTime = localtime(&tTimeOfDay.tv_sec);

        char arrcSecTime[20] = {0};
        snprintf(arrcSecTime, sizeof(arrcSecTime), "%04d-%02d-%02d %02d:%02d:%02d",
                    pTime->tm_year+1900,  pTime->tm_mon+1, pTime->tm_mday,
                    pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
        g_strLastLogSecond = arrcSecTime;
    }
    int iMicroSecond = (int)tTimeOfDay.tv_usec;
    if(g_iLastLogMicroSecond != iMicroSecond){
        g_iLastLogMicroSecond = iMicroSecond;
        char arrcMicroSecTime[8] = {0};
        snprintf(arrcMicroSecTime, sizeof(arrcMicroSecTime), "-%06d", iMicroSecond);
        g_strLastLogMicroSecond = arrcMicroSecTime;
    }


    std::string strTimeInfo = "[" + g_strLastLogSecond + g_strLastLogMicroSecond + "] ";
    int iLength = strTimeInfo.length();
    iLength = (iLength < iBufLength)?iLength : iBufLength;

    memcpy(pcBuffer, strTimeInfo.c_str(), iLength);
    return iLength;
}

int BnLogPaddingThreadName(char * pcBuffer,int iBufLength){
    char arrcThreadName[16] = {'\0'};
#if defined(PR_GET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_GET_NAME, arrcThreadName , 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_get_name_np(pthread_self(), arrcThreadName);

#elif defined(MAC_OSX)
    pthread_getname_np(arrcThreadName);
#endif
    std::string strThreadNameInfo = arrcThreadName;
    strThreadNameInfo = "[" + strThreadNameInfo + "] ";
    memcpy(pcBuffer, strThreadNameInfo.c_str(), strThreadNameInfo.length());
    return strThreadNameInfo.length();  // fix
}

