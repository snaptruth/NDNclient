#ifndef YL_BAAR_BRCACHE
#define YL_BAAR_BRCACHE

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <sstream>

using namespace std;

static const int MAX_NUM = 7;

enum STATUS_INS
{
	STATUS_SUCCESS = 1, 
	STATUS_BSDR_NOTEXIT, 
	STATUS_BSDR_BUSY, 
	STATUS_NULL_POINT,
	STATUS_ERROR_UNKNOWN
};

enum PRODUCER_DATA_FUNC
{
	REG_DATA_SET	=1,
	REG_DATA_UPDATE, 
	REG_DATA_DISABLE, 
	REG_DATA_PRICE_UPDATE,
	MARKET_ATTACH_BNNODE,
	MARKET_DISABLE_IN_BNNODE,
	DATA_FUNC_ERR
};

inline string Stat2String(STATUS_INS stat)
{
	string strRet("");
	switch(stat)
	{
		case STATUS_SUCCESS:
		    strRet = "success";
		    break;
		case STATUS_BSDR_NOTEXIT:
		    strRet = "unable to cache data[associated bsdr not start-up]";
		    break;
		case STATUS_BSDR_BUSY:
		    strRet = "unable to cache data[associated bsdr is too busy]";
		    break;
		case STATUS_NULL_POINT:
		    strRet = "unable to cache data[allocate memory fail]";
		    break;
		case STATUS_ERROR_UNKNOWN:
		    strRet = "unable to cache data[unknown error]";
		    break;
		default:
		    strRet = "unable to cache data[unknown error]";
		    break;
	}

	return strRet;
}

class BNData
{
private:
	size_t memsize;
private:
	void reducesize(char* pstr);
	bool setOperType( unsigned int OperType);
	bool setParam(const char* JasonParam);
public:
	char* param;
	unsigned int operType;
	bool setData(const char* param, unsigned int operType);

	size_t getMemSize();
	std::string to_string();
	BNData();
	~BNData();
};
/*class BNData
{
private:
	size_t memsize;
private:
	void reducesize(char * pstr);
public:
	size_t getMemSize();
public:
	char* devNo;
	char* timeStamp;
	char* content;
	char* blockLabel;
	char* market;
	char* price;
	long  seq;

	bool setDevNo(char const * const pDevNO);
	bool setTimeStamp(char const * const pTimeStamp);
	bool setContent(char const * const pContent);
	bool setBlockLabel(char const * const pBlockLabel);
	bool setMarket(char const * const pMarket);
	bool setPrice(char const * const pPrice);
	bool setData(char const * const pDevNO, char const * const pTimeStamp, 
		char const * const pContent, char const * const pBlockLabel, 
		char const * const pMarket, char const * const pPrice);

	std::string toItemString();

	std::string to_string();

	BNData();
	~BNData();
};
*/

class EGZData
{
public:
	EGZData();
	~EGZData();
private:
	size_t memsize;
public:
	size_t getMemSize();
public:
	char* blockLabel;
	char* market;
	char * pjson;

	bool setdata(char const * const plabel, char const * const pmarket, char const * const pdata);
	std::string toItemString();
};

typedef struct tagDataNode
{
	struct tagDataNode * next;
	BNData * pdata;
} DataNode;

typedef struct tagEGZDataNode
{
	struct tagEGZDataNode * next;
	EGZData * pdata;
} EGZDataNode;

class BrDataQueue
{
public:
	BrDataQueue(string market);
	~BrDataQueue();
public:
	STATUS_INS InsertBNLinkData(DataNode *pDataHead);
	STATUS_INS InsertDataNode(DataNode * pNode);
	void GetDataLink(DataNode **plink);

	string GetMarket();
	long GetNodeNum();

private:
	void ReleaseData();

public:
	BrDataQueue * next;
private:
	DataNode * head;
	DataNode * tail;
	long m_nodenum;
	std::mutex m_nodemtx;
	std::condition_variable m_nodecond;

	string m_market;
};

class EgzDataQueue
{
public:
	EgzDataQueue(string market);
	~EgzDataQueue();
public:
	STATUS_INS InsertEgzData(EGZDataNode * pNode);
	void GetEgzDataLink(EGZDataNode **plink);

	string GetMarket();
private:
	void ReleaseData();
public:
	EgzDataQueue * next;
private:
	EGZDataNode * head;
	EGZDataNode * tail;

	long m_nodenum;

	std::mutex m_nodemtx;
	std::condition_variable m_nodecond;

	string m_market;
};

class BrCache
{
protected:
	BrCache();
	~BrCache();

private:
	static BrCache* pBrCache;

public:
	static BrCache* Instance();

public:
	bool NewQueue(string market);
	bool HasQueue(string market);
	//egz
	bool NewEgzQueue(string market);
	bool HasEgzQueue(string market);
	//Jerry mod
	STATUS_INS InsertBNData(const char* param, unsigned int operType );//Jerry: insert DataNode into a specify queue according equal division algorithm
	//STATUS_INS InsertBNData(char const * const pDevNO, char const * const pTimeStamp, 
		//char const * const pContent, char const * const pBlockLabel, 
		//char const * const pMarket, char const * const pPrice);
	STATUS_INS InsertDataNode(DataNode * pDataNode);
	STATUS_INS InsertBNLinkData(string market, DataNode *pDataHead);
	void GetBNData(const string &market, DataNode **plink);

	STATUS_INS InsertEgzDataNode(char const * const pBlockLabel, char const* const pMarket, char const * const pjson);
	STATUS_INS InsertEgzDataNode(EGZDataNode * pDataNode);
	void GetEgzData(string market, EGZDataNode **plink);
private:
	void ReleaseAllQueue();
private:
	BrDataQueue * head;

	EgzDataQueue * m_pEgzQue;

	// EgzDataQueue  m_egzqueue;
};


#endif	/* #ifndef YL_BAAR_BRCACHE */
