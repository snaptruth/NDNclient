#ifndef YL_BAAR_PRODUCERMGR
#define YL_BAAR_PRODUCERMGR

#include <ndn-cxx/face.hpp>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include "producer.h"
#include "produceregz.h"

using namespace ndn;

class ProducerMgr
{
protected:
	ProducerMgr();
	~ProducerMgr();
private:
	static ProducerMgr* pProducerMgr;
public:
	static ProducerMgr* Instance();

public:
	bool run();
	void stop();
	std::vector<Producer *> GetProducers()
	{
		std::lock_guard<std::mutex> lck(m_vec_pro_mtx);
		return m_vec_pro;
	}
private:
	void listenBsdrName();
	void monitor();
	void onInterest(const InterestFilter& filter, const Interest& interest);
	void onRegisterFailed(const Name& profix, const std::string& reason);
	

private:
	std::thread *m_pthread;    // thread that receive bsdr name
	std::thread *m_pMonitorThread;
	std::mutex m_resumeMutex;
	volatile bool m_shouldresume;
	std::unique_ptr<Face> m_pface;

	vector<Producer *> m_vec_pro;
	std::mutex m_vec_pro_mtx;
	std::vector<ProducerEgz *> m_vec_proegz;
};


#endif	/* #ifndef YL_BAAR_PRODUCERMGR */