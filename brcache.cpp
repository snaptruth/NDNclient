#include "brcache.h"
#include "tinyxml/tinyxml.h"
#include "bnlogif.h"

static const size_t MAX_PACKET_SIZE = 7000;
static const size_t EMPTY_ITEM_SIZE = 300;

BrCache* BrCache::pBrCache = new BrCache();

BrCache* BrCache::Instance()
{
	return pBrCache;
}

BrCache::BrCache() : head(NULL), m_pEgzQue(NULL)
{ 
}

BrCache::~BrCache()
{
	ReleaseAllQueue();
}

bool BrCache::NewQueue(string market)
{
	if(market.empty())
	{
		return true;
	}

	if(HasQueue(market))
	{
		return true;
	}

	BN_LOG_INTERFACE("[market:%s]", market.c_str());

	BrDataQueue * pQueue = new BrDataQueue(market);
	if(NULL == pQueue)
	{
		return false;
	}

	if(NULL == head)
	{
		head = pQueue;
		return true;
	}

	BrDataQueue * ptmp = head;
	while(ptmp->next != NULL)
	{
		ptmp = ptmp->next;
	}

	ptmp->next = pQueue;

	return true;
}

bool BrCache::HasQueue(string market)
{
	BrDataQueue * ptmp = head;
	while(ptmp != NULL)
	{
		if(ptmp->GetMarket() == market)
		{
			return true;
		}
		ptmp = ptmp->next;
	}

	return false;
}

bool BrCache::NewEgzQueue(string market)
{
	if(market.empty())
	{
		return true;
	}

	if(HasEgzQueue(market))
	{
		return true;
	}

	BN_LOG_INTERFACE("[market:%s]", market.c_str());

	EgzDataQueue * pQueue = new EgzDataQueue(market);
	if(NULL == pQueue)
	{
		return false;
	}

	if(NULL == m_pEgzQue)
	{
		m_pEgzQue = pQueue;
		return true;
	}

	EgzDataQueue * ptmp = m_pEgzQue;
	while(ptmp->next != NULL)
	{
		ptmp = ptmp->next;
	}

	ptmp->next = pQueue;

	return true;
}

bool BrCache::HasEgzQueue(string market)
{
	EgzDataQueue * ptmp = m_pEgzQue;
	while(ptmp != NULL)
	{
		if(ptmp->GetMarket() == market)
		{
			return true;
		}
		ptmp = ptmp->next;
	}

	return false;
}

void BrCache::ReleaseAllQueue()
{
	if(NULL == head)
	{
		return;
	}

	BrDataQueue * ptmp = NULL;
	while(head != NULL)
	{
		 ptmp = head;
		 head = head->next;

		 delete ptmp; ptmp = NULL;
	}
}

STATUS_INS BrCache::InsertBNLinkData(string market, DataNode *pDataHead)
{
	BrDataQueue * ptmp = head;
	while(ptmp != NULL)
	{
		if(ptmp->GetMarket() == market)
		{
			return ptmp->InsertBNLinkData(pDataHead);
		}
		ptmp = ptmp->next;
	}
	
	return STATUS_BSDR_NOTEXIT;
}

STATUS_INS BrCache::InsertDataNode(DataNode * pDataNode)
{
	BrDataQueue * ptmp = head;
	if(ptmp == NULL)
		return STATUS_NULL_POINT;
	std::string selectedMarket = ptmp->GetMarket();
	BrDataQueue * pSelectedQueue = head;
	long min_node = ptmp->GetNodeNum();
	while(ptmp != NULL)
	{	
	    if(ptmp->GetNodeNum() == 0)
	    {
		ptmp = pSelectedQueue;
		selectedMarket = ptmp->GetMarket();
		break;
	    //	return ptmp->InsertDataNode(pDataNode);
	    }
		if((ptmp->GetNodeNum())< min_node)
		{
			min_node = ptmp->GetNodeNum();
			pSelectedQueue = ptmp;
			selectedMarket = ptmp->GetMarket();
		}
		//if(ptmp->GetMarket() == string(pDataNode->pdata->market))
		//{
			//return ptmp->InsertDataNode(pDataNode);
		//}
		ptmp = ptmp->next;
	}
	printf("Jerry test: insertDataNode strMarket %s\n", pSelectedQueue->GetMarket().c_str());
	return pSelectedQueue->InsertDataNode(pDataNode);  
	
	return STATUS_BSDR_NOTEXIT;
}

STATUS_INS BrCache::InsertBNData(const char* param , unsigned int operType)
{
	BNData * pBNdata = new BNData();
	if(NULL == pBNdata)
	{
		BN_LOG_ERROR("new BNData failed.");

		return STATUS_NULL_POINT;
	}

	if(!pBNdata->setData(param, operType))
	{
		printf("invoke BNData.setData failed.[param:%s]",
			((NULL == param) ? "" : param));

		delete pBNdata;
		return STATUS_NULL_POINT;
	}

	DataNode * pDataNode = new DataNode();
	if(NULL == pDataNode)
	{
		BN_LOG_ERROR("InsertBNData failed[new DataNode fail]");

		delete pBNdata;
		return STATUS_NULL_POINT;
	}

	pDataNode->pdata = pBNdata;
	pDataNode->next = NULL;

	BN_LOG_DEBUG("InsertBNData:%s",  pBNdata->to_string().c_str());

	STATUS_INS bRet = InsertDataNode(pDataNode);
	if(bRet != STATUS_SUCCESS)
	{
		delete pBNdata;
		delete pDataNode;
	}

	return bRet;
}

void BrCache::GetBNData(const string &market, DataNode **plink)
{
	BrDataQueue * ptmp = head;
	while(ptmp != NULL)
	{
		if(ptmp->GetMarket() == market)
		{
			ptmp->GetDataLink(&(*plink));
			break;
		}
		ptmp = ptmp->next;
	}
}

STATUS_INS BrCache::InsertEgzDataNode(EGZDataNode * pDataNode)
{
	// return m_egzqueue.InsertEgzData(pDataNode);

	EgzDataQueue * ptmp = m_pEgzQue;
	while(ptmp != NULL)
	{
		if(ptmp->GetMarket() == string(pDataNode->pdata->market))
		{
			return ptmp->InsertEgzData(pDataNode);
		}
		ptmp = ptmp->next;
	}
	
	return STATUS_BSDR_NOTEXIT;
}

void BrCache::GetEgzData(string market, EGZDataNode **plink)
{
	// m_egzqueue.GetEgzDataLink(&(*plink));

	EgzDataQueue * ptmp = m_pEgzQue;
	while(ptmp != NULL)
	{
		if(ptmp->GetMarket() == market)
		{
			ptmp->GetEgzDataLink(&(*plink));
			break;
		}
		ptmp = ptmp->next;
	}
}

STATUS_INS BrCache::InsertEgzDataNode(char const * const pBlockLabel, char const* const pMarket, char const * const pjson)
{
	EGZData * pEgzdata = new EGZData();
	if(NULL == pEgzdata)
	{
		BN_LOG_ERROR("new EGZData failed.[blocklabel:%s][json:%s]",
			((NULL == pBlockLabel) ? "" : pBlockLabel),
			((NULL == pjson) ? "" : pjson));

		return STATUS_NULL_POINT;
	}

	if(!pEgzdata->setdata(pBlockLabel, pMarket, pjson))
	{
		BN_LOG_ERROR("invoke EGZData.setdata failed.[blocklabel:%s][market:%s][json:%s]",
			((NULL == pBlockLabel) ? "" : pBlockLabel),
			((NULL == pMarket) ? "" : pMarket),
			((NULL == pjson) ? "" : pjson));

		delete pEgzdata;
		return STATUS_NULL_POINT;
	}

	EGZDataNode * pEGZDataNode = new EGZDataNode();
	if(NULL == pEGZDataNode)
	{
		BN_LOG_ERROR("InsertEGZData failed[new EGZDataNode fail]");

		delete pEgzdata;
		return STATUS_NULL_POINT;
	}

	pEGZDataNode->pdata = pEgzdata;
	pEGZDataNode->next = NULL;

	STATUS_INS bRet = InsertEgzDataNode(pEGZDataNode);
	if(bRet != STATUS_SUCCESS)
	{
		delete pEgzdata;
		delete pEGZDataNode;
	}

	return bRet;
}

BrDataQueue::BrDataQueue(string market) : 
    next(NULL), head(NULL), tail(NULL), m_nodenum(0), m_market(market)
{
}

BrDataQueue::~BrDataQueue()
{
	ReleaseData();
}

string BrDataQueue::GetMarket()
{
	return m_market;
}

long BrDataQueue::GetNodeNum()
{
	return m_nodenum;
}
STATUS_INS BrDataQueue::InsertDataNode(DataNode * pNode)
{
	if(NULL == pNode)
	{
		return STATUS_SUCCESS;;
	}

	{
		std::lock_guard<std::mutex> lck(m_nodemtx);

		if(NULL == head)
		{
			head = pNode;
			tail = head;
			tail->next = NULL;
		}
		else
		{
			tail->next = pNode;
			tail = tail->next;
			tail->next = NULL;
		}
               
		m_nodenum++;
		printf("Jerry : insertDataNode***************** m_nodenum[%d],m_market[%s]\n", m_nodenum, m_market.c_str());	
	}

	if(m_nodenum > 10)
	{
		m_nodecond.notify_one();
	}

	return STATUS_SUCCESS;
}

STATUS_INS BrDataQueue::InsertBNLinkData(DataNode *pDataHead)
{
	if(NULL == pDataHead)
	{
		return STATUS_SUCCESS;
	}

	{
		std::lock_guard<std::mutex> lck(m_nodemtx);

		if(NULL == head)
		{
			head = pDataHead;
			tail = pDataHead;
		}
		else
		{
			tail->next = pDataHead;
		}

		while(tail->next != NULL)
		{
			m_nodenum++;
			tail = tail->next;
		}
	}

	if(m_nodenum > 10)
	{
		m_nodecond.notify_one();
	}

	return STATUS_SUCCESS;
}

void BrDataQueue::GetDataLink(DataNode **plink)
{
	std::unique_lock<std::mutex> lck(m_nodemtx);

	if(NULL == head)
	{
		if(std::cv_status::timeout == m_nodecond.wait_for(lck, std::chrono::seconds(2)))
		{
			if(NULL == head)
			{
				return;
			}
		}
	}

	*plink = head;
	m_nodenum = 0;

	head = tail = NULL;
}

void BrDataQueue::ReleaseData()
{
	std::lock_guard<std::mutex> lck(m_nodemtx);

	DataNode * ptmp = NULL;
	while(head != NULL)
	{
		ptmp = head;
		head = head->next;

		delete ptmp->pdata;
		delete ptmp; ptmp = NULL;
	}
	m_nodenum = 0;
}

EgzDataQueue::EgzDataQueue(string market) : m_market(market), next(NULL), head(NULL), tail(NULL), m_nodenum(0)
{
}

EgzDataQueue::~EgzDataQueue()
{
	ReleaseData();
}

string EgzDataQueue::GetMarket()
{
	return m_market;
}

void EgzDataQueue::ReleaseData()
{
	std::lock_guard<std::mutex> lck(m_nodemtx);

	EGZDataNode * ptmp = NULL;
	while(head != NULL)
	{
		ptmp = head;
		head = head->next;

		delete ptmp->pdata;
		delete ptmp; ptmp = NULL;
	}
}

STATUS_INS EgzDataQueue::InsertEgzData(EGZDataNode * pNode)
{
	if(NULL == pNode)
	{
		return STATUS_SUCCESS;
	}

	{
		std::lock_guard<std::mutex> lck(m_nodemtx);

		if(NULL == head)
		{
			head = pNode;
			tail = head;
			tail->next = NULL;
		}
		else
		{
			tail->next = pNode;
			tail = tail->next;
			tail->next = NULL;
		}	
	}

	m_nodecond.notify_one();

	return STATUS_SUCCESS;
}

void EgzDataQueue::GetEgzDataLink(EGZDataNode **plink)
{
	std::unique_lock<std::mutex> lck(m_nodemtx);

	if(NULL == head)
	{
		if(std::cv_status::timeout == m_nodecond.wait_for(lck, std::chrono::seconds(2)))
		{
			if(NULL == head)
			{
				return;
			}
		}
	}

	*plink = head;

	head = tail = NULL;
}

BNData::BNData()
{
	memsize = 0;
	param =	NULL;
	operType = DATA_FUNC_ERR;
	/*devNo = NULL;
	timeStamp = NULL;
	content = NULL;
	blockLabel = NULL;
	market = NULL;
	price = NULL;
	seq = -1;
	*/
}

BNData::~BNData()
{
	BN_LOG_DEBUG("release: %s", to_string().c_str());

	if(param != NULL)
	{
		free(param);
		param = NULL;
	}
	/*if(timeStamp != NULL)
	{
		free(timeStamp);
		timeStamp = NULL;
	}
	if(content != NULL)
	{
		free(content);
		content = NULL;
	}
	if(blockLabel != NULL)
	{
		free(blockLabel);
		blockLabel = NULL;
	}
	if(market != NULL)
	{
		free(market);
		market = NULL;
	}
	if(price != NULL)
	{
		free(price);
		price = NULL;
	}*/
}

void BNData::reducesize(char * pstr)
{
	if(NULL == pstr)
	{
		return;
	}

	size_t len = strlen(pstr);
	memsize -= len;
}

size_t BNData::getMemSize()
{
	return memsize;
}

bool BNData::setParam(const char* jasonParam)
{// Jerry : here need to be mod
	if(NULL == jasonParam)
	{
		
		return false;
	}

	if(param != NULL)
	{
		reducesize(param);

		free(param);
		param = NULL;
	}

	size_t len = 0;
	len = strlen(jasonParam);
	param = (char *)malloc(len + 1);
	if(NULL == param)
	{
		return false;
	}

	memsize += len;

	memcpy(param, jasonParam, len);
	param[len] = '\0';
	printf("Jerry test: setparam success\n");
	return true;
}

bool BNData::setOperType( unsigned int OperType)
{
	if(OperType >= DATA_FUNC_ERR )
		return false;
	operType = OperType;
	printf("Jerry test: setOperType success\n");
	return true;
}

bool BNData::setData(const char* const param, unsigned int operType)
{
	if(NULL == param || operType >= DATA_FUNC_ERR)
	{
		return false;
	}

	if(!setParam(param))
	{
		return false;
	}

	if(!setOperType(operType))
	{
		return false;
	}


	return true;
}

/*std::string BNData::toItemString()
{
	string strDevNo;
    if(NULL == devNo)
    {
        strDevNo = "";
    }
    else
    {
        strDevNo = devNo;
    }

    string strTimeStamp;
    if(NULL == timeStamp)
    {
        strTimeStamp = "";
    }
    else
    {
        strTimeStamp = timeStamp;
    }

    string strContent;
    if(NULL == content)
    {
        strContent = "";
    }
    else
    {
        strContent = content;
    }

    string strLabel;
    if(NULL == blockLabel)
    {
        strLabel = "";
    }
    else
    {
        strLabel = blockLabel;
    }

    string strMarket;
    if(NULL == market)
    {
        strMarket = "";
    }
    else
    {
        strMarket = market;
    }

    string strPrice;
    if(NULL == price)
    {
        strPrice = "";
    }
    else
    {
        strPrice = price;
    }

    return "<Item><DevNo>" + strDevNo + "</DevNo><TimeStamp>" + strTimeStamp + "</TimeStamp><Content><![CDATA["
        +  strContent + "]]></Content><BlockLabel>" + strLabel + "</BlockLabel><Market>" + strMarket + "</Market><Price>"
        +  strPrice + "</Price></Item>";
}*/

std::string BNData::to_string()
{
	std::string ret = "";

	std::ostringstream s;
	s << operType;

	ret = ret + "[param: " + ((NULL == param)? "":param )+ "][operType: " + s.str() + "]";
	//ret = ret + "[devNo:" + ((NULL == devNo) ? "" : devNo) + "][timeStamp:" + 
	   //   ((NULL == timeStamp) ? "" : timeStamp) +"][content:" + ((NULL == content) ? "" : content)
	    // + "][blockLabel:" + ((NULL == blockLabel) ? "" : blockLabel) + "][market:" + ((NULL == market) ? "" : market)
	    // + "][price:" + ((NULL == price) ? "" : price) + "][seq:" + s.str() + "]";

	return ret;
}

EGZData::EGZData() : memsize(0), blockLabel(NULL), market(NULL), pjson(NULL)
{
	
}

EGZData::~EGZData()
{
	if(blockLabel != NULL)
	{
		free(blockLabel);
		blockLabel = NULL;
	}
	if(market != NULL)
	{
		free(market);
		market = NULL;
	}
	if(pjson != NULL)
	{
		free(pjson);
		pjson = NULL;
	}
}


size_t EGZData::getMemSize()
{
	return memsize;
}

bool EGZData::setdata(char const * const plabel, char const * const pmarket, char const * const pdata)
{
	if(NULL == plabel || NULL == pmarket || NULL == pdata)
	{
		return false;
	}

	if(blockLabel != NULL)
	{
		free(blockLabel);
		blockLabel = NULL;
	}

	if(market != NULL)
	{
		free(market);
		market = NULL;
	}

	if(pjson != NULL)
	{
		free(pjson);
		pjson = NULL;
	}

	memsize = 0;

	//label
	size_t tmpsize = strlen(plabel);
	blockLabel = (char *)malloc(tmpsize + 1);
	if(NULL == blockLabel)
	{
		return false;
	}
	memsize += tmpsize + 1;
	memcpy(blockLabel, plabel, tmpsize);
	blockLabel[tmpsize] = '\0';

	//market
	tmpsize = strlen(pmarket);
	market = (char*)malloc(tmpsize + 1);
	if(NULL == market)
	{
		return false;
	}
	memsize += tmpsize + 1;
	memcpy(market, pmarket, tmpsize);
	market[tmpsize] = '\0';

	tmpsize = strlen(pdata);
	pjson = (char *)malloc(tmpsize + 1);
	if(NULL == pjson)
	{
		return false;
	}
	memsize = tmpsize + 1;
	memcpy(pjson, pdata, memsize - 1);
	pjson[memsize - 1] = '\0';

	return true;
}

std::string EGZData::toItemString()
{
	string strblocklabel;
	if(NULL == blockLabel)
	{
		strblocklabel = "";
	}
	else
	{
		strblocklabel = blockLabel;
	}

	string strMarket;
	if(NULL == market)
	{
		strMarket = "";
	}
	else
	{
		strMarket = market;
	}

	string strJson;
    if(NULL == pjson)
    {
        strJson = "";
    }
    else
    {
        strJson = pjson;
    }

    return "<Item><BlockLabel>" + strblocklabel + "</BlockLabel><Market>" + strMarket + "</Market><Json>" + strJson + "</Json></Item>";
}
