#ifndef YL_BAAR_BRNAMEPUBLISHER
#define YL_BAAR_BRNAMEPUBLISHER

#include <ndn-cxx/face.hpp>
#include <string>
#include <thread>

using namespace ndn;
using namespace std;

class BrNamePubliser
{
public:
	BrNamePubliser();
	~BrNamePubliser();
public:
	bool run();
	void doNamePub();
	void pubConumerNum();
	void stop();
private:
	void onData(const Interest& interest, const Data& data);
	void onNack(const Interest& interest, const lp::Nack& nack);
	void onTimeout(const Interest& interest);
private:
    volatile bool m_shouldResume;
    std::thread * m_thr1;
    std::thread * m_thr2;
};



#endif	/* #ifndef YL_BAAR_BRNAMEPUBLISHER */