
#include "brnamepubliser.h"
#include "bnlogif.h"
#include "config.h"

BrNamePubliser::BrNamePubliser() :
    m_thr1(NULL),
    m_thr2(NULL)
{
}

BrNamePubliser::~BrNamePubliser()
{
    stop();
}

bool BrNamePubliser::run()
{
    BN_LOG_INTERFACE("run baar name publisher...");

    m_shouldResume = true;

    m_thr1 = new std::thread(&BrNamePubliser::doNamePub, this);
    if(NULL == m_thr1)
    {
        BN_LOG_ERROR("new baar name publisher thread error.");
        return false;
    }

    m_thr2 = new std::thread(&BrNamePubliser::pubConumerNum, this);
    if(NULL == m_thr2)
    {
        BN_LOG_ERROR("new baar publish consumer num thread error.");
        return false;
    }

    return true;
}

void BrNamePubliser::doNamePub()
{
    string strName("/yl/broadcast/baar/namepublish");
    strName += "/" + Config::Instance()->m_sMacAddr;

    std::unique_ptr<Face> p_face = nullptr;

    while(m_shouldResume)
    {
    	Interest interest(strName);
    	interest.setInterestLifetime(time::milliseconds(50));
    	interest.setMustBeFresh(true);

        try
        {
            if(nullptr == p_face)
            {
                p_face = std::unique_ptr<Face>(new Face);
                if(nullptr == p_face)
                {
                    BN_LOG_ERROR("br name publiser new Face error.");
                    continue;
                }
            }
            printf("name publish,express interest: %s\n", strName.c_str());
            BN_LOG_DEBUG("name pusblish,express interest:%s", strName.c_str());

            p_face->expressInterest(interest,
                bind(&BrNamePubliser::onData, this,  _1, _2),
                bind(&BrNamePubliser::onNack, this, _1, _2),
                bind(&BrNamePubliser::onTimeout, this, _1));

            p_face->processEvents();
        }
    	catch (std::runtime_error& e) 
        {
            BN_LOG_ERROR("brname publisher catch nfd error:%s", e.what());
            p_face = nullptr;
        }
        catch(...)
        {
            BN_LOG_ERROR("brname publisher catch unknow error.");
            p_face = nullptr;
        }

        sleep(Config::Instance()->m_sBnrTimer);

    }
	

	// return true;
}

void BrNamePubliser::pubConumerNum()
{
    std::ostringstream s;
    s << Config::Instance()->m_consumerNum;
    string strConsumerNum = s.str();;

    string strName("/yl/broadcast/baar/publish/consumernum/");
    strName += Config::Instance()->m_sMacAddr + "/" + strConsumerNum;


    std::unique_ptr<Face> p_face = nullptr;

    while(m_shouldResume)
    {
        Interest interest(strName);
        interest.setInterestLifetime(time::milliseconds(50));
        interest.setMustBeFresh(true);

        try
        {
            if(nullptr == p_face)
            {
                p_face = std::unique_ptr<Face>(new Face);
                if(nullptr == p_face)
                {
                    BN_LOG_ERROR("br name publiser new Face error.");
                    continue;
                }
            }
	    printf("name publish, express interest:%s\n", strName.c_str());
            BN_LOG_DEBUG("name pusblish,express interest:%s", strName.c_str());

            p_face->expressInterest(interest,
                bind(&BrNamePubliser::onData, this,  _1, _2),
                bind(&BrNamePubliser::onNack, this, _1, _2),
                bind(&BrNamePubliser::onTimeout, this, _1));

            p_face->processEvents();
        }
        catch (std::runtime_error& e) 
        {
            BN_LOG_ERROR("brname publisher catch nfd error:%s", e.what());
            p_face = nullptr;
        }
        catch(...)
        {
            BN_LOG_ERROR("brname publisher catch unknow error.");
            p_face = nullptr;
        }

        sleep(Config::Instance()->m_sBnrTimer);

    }
}

void BrNamePubliser::stop()
{
    m_shouldResume = false;

    if(m_thr1 != NULL)
    {
        m_thr1->join();
        delete m_thr1;
        m_thr1 = NULL;
    }

    if(m_thr2 != NULL)
    {
        m_thr2->join();
        delete m_thr2;
        m_thr2 = NULL;
    }
}

void BrNamePubliser::onData(const Interest& interest, const Data& data)
{

}
void BrNamePubliser::onNack(const Interest& interest, const lp::Nack& nack)
{
    //_LOG_ERROR("br name publish on nack.");
}
void BrNamePubliser::onTimeout(const Interest& interest)
{
    //_LOG_ERROR("br name publish on timeout.");
}



