#ifndef YL_BAAR_PRODUCER
#define YL_BAAR_PRODUCER

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include "brcache.h"

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <time.h>

using namespace ndn;
using namespace std;


class Producer 
{
public:
	Producer(const string& macaddr, const string& bsdrmarket, int thrnum);
	~Producer();

public:
	// void setMacAddr(const string &macaddr);
	bool run();
	void doRun();
	void stop();

	string getBsdrMarket();
	int getThrNum();
	volatile time_t m_lastHeartbeat;

private:
	void onInterest(const InterestFilter& filter, const Interest& interest);
	void onRegisterFailed(const Name& profix, const std::string& reason);

private:
	void addItemData(string & content, BNData * pBNData);

private:
	std::unique_ptr<Face> m_pface;
	KeyChain m_keyChain;
    
    string m_macaddr;
    string m_bsdrmarket;
    int m_thrnum;

    std::thread * plistenThr;

    const ndn::RegisteredPrefixId * m_listenId;

	std::mutex m_resumeMutex;
	volatile bool m_shouldresume;

	long m_seq;

    DataNode * m_pSending;
    DataNode * m_pSending_tail;
	DataNode * m_pCache;
};

#endif	/* #ifndef YL_BAAR_PRODUCER */
