
#include "consumermgr.h"
#include "config.h"

#include "bnlogif.h"
#include <unistd.h>
#include <sys/time.h>
#include "producermgr.h"

ConsumerMgr* ConsumerMgr::pConsumerMgr = new ConsumerMgr();

ConsumerMgr* ConsumerMgr::Instance()
{
	return pConsumerMgr;
}

ConsumerMgr::ConsumerMgr() 
{

}

ConsumerMgr::~ConsumerMgr()
{

}

int ConsumerMgr::MakeCSMHanderPool()
{
	int nPoolSize = Config::Instance()->m_conumercfg.m_poolsize;

	int i = 0;
	for(; i < nPoolSize; i++)
	{
		CSMHANDLER * pHandler = new CSMHANDLER;
		if(NULL == pHandler)
		{
			break;
		}
		Consumer * pCon = new Consumer();
		if(NULL == pCon)
		{
			delete pHandler; pHandler = NULL;
			break;
		}
		pHandler->pCsm = pCon;
		pHandler->nCsmStatus = STATUS_IDLE;
		m_CsmPtr2Index[pCon] = i;
		m_vCsm.push_back(pHandler);
	}

	return i;
}

Consumer * ConsumerMgr::getConsumer(string& market)
{
    
	std::vector<Producer* > vecProducer = ProducerMgr::Instance()->GetProducers();
	if(!vecProducer.size())
		return NULL;
    printf("Jerry: getConsumer itervec size is %d\n", vecProducer.size());
	std::vector<Producer* >::iterator itervec = vecProducer.begin();
	market = (*itervec)->getBsdrMarket();
    printf("Jerry: getConsumer market is %s\n", market.c_str());
	unsigned int minInUse = 0;
	//bool hasfound = false;
	std::map<string, int>::iterator iterforfoundBsdrInMap;
	
	for(; itervec != vecProducer.end(); itervec++)
	{
		iterforfoundBsdrInMap = market2inuse.find((*itervec)->getBsdrMarket());
		if(iterforfoundBsdrInMap == market2inuse.end())
		{
			market =(*itervec)->getBsdrMarket();
			break;
		}else
		{	
			if(itervec == vecProducer.begin())
			{
				minInUse = iterforfoundBsdrInMap->second + 1;
				market = iterforfoundBsdrInMap->first;
				continue;
			}
			if(iterforfoundBsdrInMap->second < minInUse-1)
			{
				minInUse = iterforfoundBsdrInMap->second+1;
				market = iterforfoundBsdrInMap->first;
			}
		}
	}
	
	printf("Jerry : find selected bsdr %s\n", market.c_str());
	std::lock_guard<std::mutex> lck(m_mutex);

	std::vector<CSMHANDLER *>::iterator iter = m_vCsm.begin();
	//std::string pSelectedBsdr;
	
	for(; iter != m_vCsm.end(); iter++)
	{
		if(STATUS_IDLE == (*iter)->nCsmStatus)
		{
			(*iter)->nCsmStatus = STATUS_BUY;

			int nNum = 0;
		//	Jerry 
			std::map<string, int>::iterator itermap = market2inuse.find(market);
			if(itermap == market2inuse.end())
			{
				market2inuse[market] = 0;
			}
			else
			{
				nNum = itermap->second + 1;
				nNum = nNum % Config::Instance()->m_consumerNum;
				itermap->second = nNum;
			}

			(*iter)->pCsm->setProNum(nNum);

			return (*iter)->pCsm;
		}
	}

	Consumer * ret = NULL;

	int nIncrement = Config::Instance()->m_conumercfg.m_increment;
	int i = 0;
	while(i < nIncrement)
	{
		CSMHANDLER * pHandler = new CSMHANDLER;
		if(NULL == pHandler)
		{
			break;
		}
		Consumer * pCon = new Consumer();
		if(NULL == pCon)
		{
			delete pHandler; pHandler = NULL;
			break;
		}
		pHandler->pCsm = pCon;
		if(NULL ==  ret)
		{
			ret = pCon;

			int nNum = 0;
			std::map<string, int>::iterator itermap = market2inuse.find(market);
			if(itermap == market2inuse.end())
			{
				market2inuse[market] = 0;
			}
			else
			{
				nNum = itermap->second + 1;
				nNum = nNum % Config::Instance()->m_consumerNum;
				itermap->second = nNum;
			}

			pCon->setProNum(nNum);

			pHandler->nCsmStatus = STATUS_BUY;
		}
		else
		{
			pHandler->nCsmStatus = STATUS_IDLE;
		}
		
		m_CsmPtr2Index[pCon] = m_vCsm.size();
		m_vCsm.push_back(pHandler);

		i++;
	}

	return ret;
}

void ConsumerMgr::ReleaseCsm(Consumer * pCsm)
{
	if(NULL == pCsm)
	{
		return;
	}

	std::map<Consumer *, int>::iterator iter = m_CsmPtr2Index.find(pCsm);
	if(iter == m_CsmPtr2Index.end())
	{
		return;
	}
	if(iter->second > m_vCsm.size() - 1)
	{
		return;
	}

	m_vCsm[iter->second]->nCsmStatus = STATUS_IDLE;
}

// int ConsumerMgr::MakeCSMPool()
// {
// 	int nPoolSize = Config::Instance()->m_conumercfg.m_poolsize;

// 	int i = 0;
// 	for(; i < nPoolSize; i++)
// 	{
// 		Consumer * pCon = new Consumer();
// 		if(NULL == pCon)
// 		{
// 			break;
// 		}
// 		m_vecCsm.push_back(pCon);
// 	}

// 	return i;
// }

// Consumer * ConsumerMgr::GetCsm(std::string market)
// {
// 	Consumer * pRet = NULL;

// 	std::lock_guard<std::mutex> lck(m_vecCmMtx);
// 	if(m_vecCsm.size() > 0)
// 	{
// 		pRet = m_vecCsm[0];
// 		m_vecCsm.erase(m_vecCsm.begin());

// 		int nNum = 0;
// 		std::map<string, int>::iterator itermap = market2inuse.find(market);
// 		if(itermap == market2inuse.end())
// 		{
// 			market2inuse[market] = 0;
// 		}
// 		else
// 		{
// 			nNum = itermap->second + 1;
// 			nNum = nNum % Config::Instance()->m_consumerNum;
// 			itermap->second = nNum;
// 		}

// 		pRet->setProNum(nNum);

// 		return pRet;
// 	}

// 	int nIncrement = Config::Instance()->m_conumercfg.m_increment;
// 	int i = 0;
// 	while(i < nIncrement)
// 	{
// 		Consumer * pCon = new Consumer();
// 		if(NULL == pCon)
// 		{
// 			break;
// 		}
// 		if(NULL ==  pRet)
// 		{
// 			pRet = pCon;

// 			int nNum = 0;
// 			std::map<string, int>::iterator itermap = market2inuse.find(market);
// 			if(itermap == market2inuse.end())
// 			{
// 				market2inuse[market] = 0;
// 			}
// 			else
// 			{
// 				nNum = itermap->second + 1;
// 				nNum = nNum % Config::Instance()->m_consumerNum;
// 				itermap->second = nNum;
// 			}

// 			pCon->setProNum(nNum);
// 		}
// 		else
// 		{
// 			m_vecCsm.push_back(pCon);
// 		}

// 		i++;
// 	}

// 	return pRet;
// }

// void ConsumerMgr::putCsm(Consumer * pCsm)
// {
// 	if(NULL == pCsm)
// 	{
// 		return;
// 	}

// 	std::lock_guard<std::mutex> lck(m_vecCmMtx);
// 	m_vecCsm.push_back(pCsm);
// }


// void ConsumerMgr::onInterest(const InterestFilter& filter, const Interest& interest)
// {
// 	_LOG_INFO("recieve bsdr name pubish interest:" << interest);

// 	Name dataName(interest.getName());

// 	Name::Component comBsdrMarket = dataName.get(POS_MARKET_BSDR_NAME_PUB);
//     std::string strBsdrMarket = comBsdrMarket.toUri();

//     time_t tm = ::time(NULL);

//     int * pStatus = new int[Config::Instance()->m_consumerNum];
//     if(NULL == pStatus)
//     {
//     	return;
//     }
//     for(int j= 0; j < Config::Instance()->m_consumerNum + 1; j++)
//     {
//     	pStatus[j] = 0;
//     }

//     std::lock_guard<std::mutex> lck(m_vecMutex);

//     for(int i = 0; i < m_vecConsumer.size(); )
//     {
//     	if(strBsdrMarket == m_vecConsumer[i]->m_market)
//     	{
//     		if(m_vecConsumer[i]->m_consumerNum > Config::Instance()->m_consumerNum)
//     		{
//     			delete m_vecConsumer[i];
//     			m_vecConsumer.erase(m_vecConsumer.begin() + i);

//     			continue;
//     		}
//     		else
//     		{
//     			pStatus[m_vecConsumer[i]->m_consumerNum] = 1;
//     		}
//     	}

//     	i++;
//     }

//     for(int k = 1; k < Config::Instance()->m_consumerNum + 1; k++)
//     {
//     	if(0 == pStatus[k])
//     	{
//     		Consumer * pConsumer = new Consumer(strBsdrMarket, k);
//     		if(NULL == pConsumer)
//     		{
//     			break;
//     		}

//     		m_vecConsumer.push_back(pConsumer);
//     	}
//     }
    
// }

// void ConsumerMgr::onRegisterFailed(const Name& profix, const std::string& reason)
// {
// 	_LOG_ERROR("onRegisterFailed:" << reason);
// 	if(m_pface != nullptr)
// 	{
// 		m_pface->shutdown();
// 	}
// }
