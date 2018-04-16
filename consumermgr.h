#ifndef YL_BAAR_CONSUMERMGR
#define YL_BAAR_CONSUMERMGR

#include <ndn-cxx/face.hpp>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include "consumer.h"

using namespace ndn;

class ConsumerMgr
{
public:
	ConsumerMgr();
	~ConsumerMgr();

private:
	static ConsumerMgr* pConsumerMgr;
public:
	static ConsumerMgr* Instance();

public:
	int MakeCSMHanderPool();
	//Jerry: 20180305 mod this function ,market was got by ed-algorithm
	Consumer * getConsumer(string& market);
	void ReleaseCsm(Consumer * pCsm);


	// int MakeCSMPool();
	// Consumer * GetCsm(std::string market);
	// void putCsm(Consumer * pCsm);


private:
	enum CSM_STATUS{STATUS_IDLE = 1, STATUS_BUY = 2};
	typedef struct 
	{
		CSM_STATUS   nCsmStatus;
		Consumer *  pCsm;
	} CSMHANDLER;

	std::vector<CSMHANDLER *> m_vCsm;
	std::map<Consumer *, int> m_CsmPtr2Index;
	std::mutex m_mutex;

	std::map<string, int> market2inuse;


	// std::vector<Consumer *> m_vecCsm;
	// std::mutex m_vecCmMtx;
};




#endif	/* #ifndef YL_BAAR_CONSUMERMGR */