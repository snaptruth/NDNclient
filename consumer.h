#ifndef YL_BAAR_CONSUMER
#define YL_BAAR_CONSUMER

#include <ndn-cxx/face.hpp>
#include "tinyxml/tinyxml.h"
#include <string>
#include <map>
#include <mutex>

using namespace ndn;
using namespace std;

class Consumer
{
public:
	Consumer();
	~Consumer();
public:
	void setProNum(int pronum);
	// add by Jerry: 2018 0305
	void GetRegData(const string& strMarket, const char* param);
	void GetRegDataSize(const string& strMarket, const char* param);
	void GetRegDataPrice(const string& strMarket, const char* param);
	void GetMarketInfoFromMarket(const string& strMarket, const char* param);
	void GetMarketInfoFromUser(const string& strMarket, const char* param);
	void GetMarketInfoFromTecCompany(const string& strMarket, const char* param);
	void GetMarketInfoFromBn(const string& strMarket, const char* param);
	void GetAuthRandom(const string& strMarket, const char* param);
	void GetMarketInfoFromTecCompanyAsUser(const string& strMarket, const char* param);
	void GetRegDataFromOtherUser(const string& strMarket, const char* param);
	void UserPubkeySync(const string& strMarket, const char* param);
	void GetAuthToken(const string& strMarket, const char* param);
	void GetConsumeBill(const string& strMarket, const char* param);
	void GetConsumeBillSum(const string& strMarket, const char* param);

	void DisableMarketFromBnNode(const string& strMarket, const char* param);

	void AddMarketToBnNode(const string& strMarket, const char* param);

	void UpdateMarketBalanceFromBnNode(const string& strMarket, const char* param);

        //New interface add by gd 2018.04.05, Used to notify BSDR to store the specified data on chain
    void NotifyOnChain(const string& strMarket, const char* param);
	void RegisterDataPriceUpdate(const string& strMarket, const char* param);
	void RegisterDataDisable(const string& strMarket, const char* param);
	void RegisterDataUpdate(const string& strMarket, const char* param);
	void RegisterDataSet(const string& strMarket, const char* param);
	
	//del by Jerry: 201803015
	//void GetBlockData(const string& consumerMarket, const string& dataMarket, const string& blocklabel);
	//void GetTranInfo(const string& consumerMarket, const string& dataMarket, const string& blocklabel);
	//egz
	//void GetTxID(const string& dataMarket, const string& blocklabel);

private:

	void expressInterestName(const string& strName);
	void onData(const Interest& interest, const Data& data);
	void onTranData(const Interest& interest, const Data& data);
	void onNack(const Interest& interest, const lp::Nack& nack);
	void onTimeout(const Interest& interest);

	//egz
	void onTxIDData(const Interest& interest, const Data& data);
public:
	std::unique_ptr<Face> m_pface;

	int m_pronum;
	
	char * m_pData;
	bool m_success;
	string m_errormsg;

};


#endif	/* #ifndef YL_BAAR_CONSUMER */
