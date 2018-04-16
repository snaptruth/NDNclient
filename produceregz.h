#ifndef YL_BAAR_ProducerEgz
#define YL_BAAR_ProducerEgz

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include "brcache.h"

#include <string>
#include <vector>
#include <mutex>
#include <thread>

using namespace ndn;
using namespace std;


class ProducerEgz 
{
public:
	ProducerEgz(const string& macaddr, const string& bsdrmarket, int thrnum);
	~ProducerEgz();

public:
	// void setMacAddr(const string &macaddr);
	bool run();
	void stop();

	string getBsdrMarket();
	int getThrNum();

	//egz
	void doRunEgz();
private:
	//egz
	void onEgzInterest(const InterestFilter& filter, const Interest& interest);
	void onEgzRegisterFailed(const Name& profix, const std::string& reason);

private:
	void addItemData(string & content, BNData * pBNData);

private:
    string m_macaddr;
    string m_bsdrmarket;
    int m_thrnum;

    const ndn::RegisteredPrefixId * m_listenId;

	std::mutex m_resumeMutex;
	volatile bool m_shouldresume;

	////  egz deamo
	std::unique_ptr<Face> m_pegzface;
	KeyChain m_egzkeyChain;

	std::thread * pEgzListenThr;
	long m_egzseq;
	EGZDataNode * m_pEgzSending;
	EGZDataNode * m_pEgzSendingTail;
	EGZDataNode * m_pEgzCathe;
};

#endif	/* #ifndef YL_BAAR_ProducerEgz */
