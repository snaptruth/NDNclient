/*
 * bnthread.h
 *
 *  Created on: 2017年8月22日
 */

#ifndef INCLUDE_BNTHREAD_H_
#define INCLUDE_BNTHREAD_H_

#include <thread>
//#include <queue>
#include <map>
#include <atomic>
#include <condition_variable>
#include "include/bnconst.h"
#include "include/bnstruct.h"

int SendMsgToQueueId(int iQueueId, int iMsgId, const void *pvData, int iLength);
int SendMsgToQueueIdNoWait(int iQueueId, int iMsgId, const void *pvData, int iLength);

struct BnThreadTimerMsg{
    int m_iMsgQueueId;
    int iMsgId;
    timer_t *tTimerId;
};

class BnThread {
public:
    BnThread(const char* pcThreadName, int iMsgQueueKey, int iPriority = BN_THREAD_PRIORITY_DEFAULT);

    virtual ~BnThread();

    ERRCODE PostMsg(int iMsgId, const void *pvData, int iLength);

    ERRCODE PostMsgNoWait(int iMsgId, const void *pvData, int iLength);

    virtual void OnMessage(int iMsgId, int iLength, void *pvData);

    virtual ERRCODE RegisterTimer();

    ERRCODE RegisterThread();

    void ExitThread();

    void join();

    void SetPriority(int iPriority);

    int GetMsgQueueId(){return m_iMsgQueueId;};

    std::string GetThreadName(){std::string strName = m_pcThreadName; return strName;};

protected:
    void RenameThread(const char* pcThreadName);

    void SetThreadPriority(int iPriority);

    ERRCODE CreateTimer(int iTimerId, int iIntervalMs);

    key_t GetMsgQueueKey(){return m_tMsgQueueKey;};

    void SetMsgQueueKey(key_t tQueueKey){m_tMsgQueueKey = tQueueKey;};

private:
    std::thread::id GetThreadId();

    void Process();

    void DeleteMessageQueue();

    ERRCODE DeleteAllTimer();

    // DO NOT ALLOW
    BnThread(const BnThread&);
    const BnThread& operator=(const BnThread&);

private:
    std::thread* m_pThread;
    char* m_pcThreadName;
    key_t m_tMsgQueueKey;
    int m_iThreadPriority;
    int m_iMsgQueueId;
    std::map<int, struct BnThreadTimerMsg *> m_mapTimerMsg;

};

#endif /* INCLUDE_BNTHREAD_H_ */
