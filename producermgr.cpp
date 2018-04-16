#include "producermgr.h"
#include "bnlogif.h"
#include "config.h"
#include "brcache.h"

static const int CONNECTION_RETRY_TIMER = 1;
static const int POS_MARKET_BSDR_NAME_PUB = 4;
static const int POS_THR_NUM_BSDR_CONSUMER = 5;

static const int MONITOR_TIMER             = 10;
static const int MONITOR_WAIT_RESPONE_TIME = 30;

ProducerMgr* ProducerMgr::pProducerMgr = new ProducerMgr();

ProducerMgr* ProducerMgr::Instance()
{
	return pProducerMgr;
}

ProducerMgr::ProducerMgr() : m_pthread(NULL), m_pface(nullptr), m_pMonitorThread(NULL)
{
}
ProducerMgr::~ProducerMgr()
{

}

bool ProducerMgr::run()
{
	BN_LOG_INTERFACE("run producer manager...");

	m_shouldresume = true;

	if(m_pthread != NULL)
	{
		return true;
	}

	m_pthread = new std::thread(&ProducerMgr::listenBsdrName, this);
	if(NULL == m_pthread)
	{
		BN_LOG_ERROR("run producer manager error[new std::thread m_pthread fail]");
		return false;
	}
	m_pMonitorThread = new std::thread(&ProducerMgr::monitor, this);
	if(NULL == m_pMonitorThread)
	{
		BN_LOG_ERROR("run producermgr monitor error");
		return false;
	}

	return true;
}


void ProducerMgr::stop()
{
	m_shouldresume = false;

	if(m_pface != nullptr)
	{
		m_pface->shutdown();
	}

	if(m_pthread != NULL)
	{
		m_pthread->join();
		m_pthread = NULL;
	}
	if(m_pMonitorThread != NULL)
	{
		m_pMonitorThread ->join();
		m_pMonitorThread = NULL;
	}

}

void ProducerMgr::monitor()
{
	while(m_shouldresume)
	{
		time_t tm = ::time(NULL);

		//_LOG_INFO("monitor[time:" << tm << "]");

		{
			std::lock_guard<std::mutex> lck(m_vec_pro_mtx);
			for(int i = 0; i < (int)m_vec_pro.size();)
			{
				if((tm - m_vec_pro[i]->m_lastHeartbeat) > MONITOR_WAIT_RESPONE_TIME)
		    	{
		    		//_LOG_INFO("realese:" << *(m_vec_pro[i]));
		    		m_vec_pro[i]->stop();
		    		delete m_vec_pro[i];

		    		m_vec_pro.erase(m_vec_pro.begin() + i);
		    		continue;
		    	}

				i++;
			}
		}

		sleep(MONITOR_TIMER);
	}
}
void ProducerMgr::listenBsdrName()
{
	do
	{
		try
		{
			if(nullptr == m_pface)
			{
				m_pface = std::unique_ptr<Face>(new Face);
				if(nullptr == m_pface)
				{
					continue;
				}
			}

			BN_LOG_INTERFACE("producer mgr listen:/yl/broadcast/bsdr/namepublish");
			// std::cout << "producer mgr listen:/yl/broadcast/bsdr/namepublish" << std::endl << std::endl ;

			m_pface->setInterestFilter("/yl/broadcast/bsdr/namepublish",
				bind(&ProducerMgr::onInterest, this, _1, _2),
				RegisterPrefixSuccessCallback(),
				bind(&ProducerMgr::onRegisterFailed, this, _1, _2));

			m_pface->processEvents();
		}
		catch (std::runtime_error& e)
		{
			BN_LOG_ERROR("ProducerMgr catch nfd error:%s", e.what());
		}
		catch(...)
		{
			BN_LOG_ERROR("ProducerMgr process events catch unknow error.");
		}

		m_pface = nullptr;

		if(m_shouldresume)
		{
			sleep(CONNECTION_RETRY_TIMER);
		}

	} while(m_shouldresume);

	BN_LOG_INTERFACE("leave listenBsdrName.");
}

void ProducerMgr::onInterest(const InterestFilter& filter, const Interest& interest)
{
    stringstream strLogInfo;
    strLogInfo << "recieve bsdr name pubish interest:" << interest;
	BN_LOG_INTERFACE("%s", strLogInfo.str().c_str());

	Name dataName(interest.getName());

	Name::Component comBsdrMarket = dataName.get(POS_MARKET_BSDR_NAME_PUB);
    std::string strBsdrMarket = comBsdrMarket.toUri();

    Name::Component comThrNum = dataName.get(POS_THR_NUM_BSDR_CONSUMER);
    string strThrnum = comThrNum.toUri();
    int thrnum = atoi(strThrnum.c_str());

    BN_LOG_INTERFACE("Timer[bsdr market:%s][consumer number:%d]", strBsdrMarket.c_str(), thrnum);
    printf("bsdr market:%s: consumer numer %d", strBsdrMarket.c_str(), thrnum);
    time_t tm = ::time(NULL);

    // int *p = new int[thrnum];
    // if(NULL == p)
    // {

    // }
    // for(int i = 0; i < thrnum; i++)
    // {
    // 	p[i] = 0;
    // }

    if(!BrCache::Instance()->HasQueue(strBsdrMarket))
    {
        printf("Jerry: BrcacheNewQueue %s\n", strBsdrMarket.c_str());
    	BrCache::Instance()->NewQueue(strBsdrMarket);
    }

    int count = 0;
	{
		std::lock_guard<std::mutex> lck(m_vec_pro_mtx);
		vector<Producer *>::iterator iter = m_vec_pro.begin();
    	while(iter != m_vec_pro.end())
    	{
    		if((*iter)->getBsdrMarket() == strBsdrMarket)
    		{
    			BN_LOG_INTERFACE("[bsdr market:%s][producer number:%d]", strBsdrMarket.c_str(), (*iter)->getThrNum());

    			if((*iter)->getThrNum() > thrnum)
    			{
    				(*iter)->stop();
    				m_vec_pro.erase(iter);
    				continue;
    			}
				(*iter)->m_lastHeartbeat = tm;
    			count++;
    		}

    	++iter;
    	}
    }

    if(count < thrnum + 1)
    {
    	count++;
    	for(; count < thrnum + 1; count++)
    	{
    		BN_LOG_INTERFACE("start new producer[bsdr market:%s][producer number:%d]", strBsdrMarket.c_str(), count);

	    	Producer * pProducer = new Producer(Config::Instance()->m_sMacAddr, strBsdrMarket, count);
	    	if(NULL == pProducer)
	    	{
	    		break;
	    	}

	    	if(!pProducer->run())
	    	{
	    		delete pProducer;
	    		break;
	    	}
	    	else
	    	{
	    		std::lock_guard<std::mutex> lck(m_vec_pro_mtx);
	    		m_vec_pro.push_back(pProducer);
	    	}
    	}
    }

    //egz
    if(!BrCache::Instance()->HasEgzQueue(strBsdrMarket))
    {
    	BrCache::Instance()->NewEgzQueue(strBsdrMarket);
    }
    
    int count_egz = 0;
    vector<ProducerEgz *>::iterator iteregz = m_vec_proegz.begin();
    while(iteregz != m_vec_proegz.end())
    {
    	if((*iteregz)->getBsdrMarket() == strBsdrMarket)
    	{
    		BN_LOG_INTERFACE("[bsdr market:%s][producer number:%d]", strBsdrMarket.c_str(), (*iteregz)->getThrNum());

			if((*iteregz)->getThrNum() > thrnum)
			{
				(*iteregz)->stop();
				m_vec_proegz.erase(iteregz);
				continue;
			}

			count_egz++;
		}

    	++iteregz;
    }

    if(count_egz < thrnum + 1)
    {
    	count_egz++;
    	for(; count_egz < thrnum + 1; count_egz++)
    	{
    		BN_LOG_INTERFACE("start new ProducerEgz[bsdr market:%s][producer number:%d]", strBsdrMarket.c_str(), count_egz);

	    	ProducerEgz * pProducer = new ProducerEgz(Config::Instance()->m_sMacAddr, strBsdrMarket, count_egz);
	    	if(NULL == pProducer)
	    	{
	    		break;
	    	}

	    	if(!pProducer->run())
	    	{
	    		delete pProducer;
	    		break;
	    	}
	    	else
	    	{
	    		m_vec_proegz.push_back(pProducer);
	    	}
    	}
    }
}

void ProducerMgr::onRegisterFailed(const Name& profix, const std::string& reason)
{
	BN_LOG_ERROR("ProducerMgr onRegisterFailed:%s", reason.c_str());
	if(m_pface != nullptr)
	{
		m_pface->shutdown();
	}
}

