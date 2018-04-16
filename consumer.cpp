#include "consumer.h"

#include <string>
#include "bnlogif.h"
#include "algo/aes_algo.h" 

//begin cipher by liulijin 20170606 add
#include "config.h"
//end cipher by liulijin 20170606 add

static int POS_TYPE = 4;
static string PRIX_BSDR_PRODUCER = "/yl/broadcast/bsdr/producer";
static string TYPE_GET_DATA = "getdata";
static string TYPE_GET_TRAN = "gettraninfo";
static string TYPE_GET_TXID = "gettxid";
//Jerry : add 20180305
static string TYPE_GET_REG_DATA = "getregdata";
static string TYPE_GET_REG_DATA_SIZE = "getregdatasize";
static string TYPE_GET_REG_DATA_PRICE = "getregdataprice";
static string TYPE_GET_MARKET_FROM_MARKET ="getmarketinfofrommarket";
static string TYPE_GET_MARKET_FROM_USER	= "getmarketinfofromuser";
static string TYPE_GET_MARKET_FROM_TEC_COMPANY = "getmarketinfofromteccomapany";
static string TYPE_GET_MARKET_FROM_TEC_COMPANY_AS_USER = "getmarketinfofromteccomapanyasuser";

static string TYPE_GET_REG_DATA_FROM_OTHER_USER  = "getregdatafromotheruser";
static string TYPE_REQ_USER_PUBKEY_SYNC	= "requestuserpubkeysync";
static string TYPE_GET_AUTH_TOKEN	= "getauthtoken";
static string TYPE_GET_AUTH_RANDOM	= "getauthrandom"; //Jerry add 20180319
static string TYPE_GET_MARKET_FROM_BN	= "getmarketinfofrombn"; // Jerry add 20180319
static string TYPE_GET_CONSUME_BILL	= "getconsumebill";
static string TYPE_GET_CONSUME_SUM	= "getconsumesum";
static string TYPE_UPDATE_MARKET_BALANCE_FROM_BN_NODE = "updatemarketbalancefrombnnode";// Jerry add 20180404
static string TYPE_ADD_MARKET_TO_BN_NODE ="addmarkettobnnode";
static string TYPE_DISABLE_MARKET_FROM_BN_NODE ="disablemarketfrombnnode";

//New interface add by gd 2018.04.05, Used to notify BSDR to store the specified data on chain
static string TYPE_NOTIFY_ON_CHAIN ="notifyonchain";

static string TYPE_UPDATE_REGISTER_DATA_PRICE = "registerdataprice";
static string TYPE_DISABLE_REGISTER_DATA ="banregdata";

static string TYPE_UPDATE_REGISTER_DATA="updateregdata";
static string TYPE_SET_REGISTER_DATA="setregdata";
Consumer::Consumer() :
    m_pData(NULL)
{
	m_pface = std::unique_ptr<Face>(new Face);
}

Consumer::~Consumer() {
	if (m_pData != NULL) {
		free(m_pData);
		m_pData = NULL;
	}
}

void Consumer::setProNum(int pronum)
{
	m_pronum = pronum;
}

void Consumer::RegisterDataSet(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;
	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_SET_REGISTER_DATA + "/" + param;
	expressInterestName(strName);
}
void Consumer::RegisterDataDisable(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;
	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_DISABLE_REGISTER_DATA + "/" + param;
	expressInterestName(strName);
}
void Consumer::RegisterDataUpdate(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;
	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_UPDATE_REGISTER_DATA + "/" + param;
	expressInterestName(strName);
}

void Consumer::RegisterDataPriceUpdate(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;
	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_UPDATE_REGISTER_DATA_PRICE + "/" + param;
	expressInterestName(strName);
}

void Consumer::UpdateMarketBalanceFromBnNode(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_UPDATE_MARKET_BALANCE_FROM_BN_NODE + "/" + param;
	expressInterestName(strName);
}

void Consumer::AddMarketToBnNode(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_ADD_MARKET_TO_BN_NODE + "/" + param;
	expressInterestName(strName);
}

void Consumer::DisableMarketFromBnNode(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_DISABLE_MARKET_FROM_BN_NODE + "/" + param;
	expressInterestName(strName);
}
//Jerry: add 20180305
void Consumer::GetRegData(const string& strMarket,const char * param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_REG_DATA + "/" + param;
	expressInterestName(strName);
	
}
//Jerry: add 20180305
void Consumer::GetRegDataSize(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_REG_DATA_SIZE + "/" + param;
	expressInterestName(strName);	
}
void Consumer::GetRegDataPrice(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_REG_DATA_PRICE + "/" + param;
	expressInterestName(strName);
}
void Consumer::GetMarketInfoFromMarket(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_MARKET_FROM_MARKET + "/" + param;
	expressInterestName(strName);
}

void Consumer::GetMarketInfoFromUser(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_MARKET_FROM_USER+ "/" + param;
	expressInterestName(strName);
}

void Consumer::GetMarketInfoFromTecCompany(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_MARKET_FROM_TEC_COMPANY + "/" + param;
	expressInterestName(strName);
}

void Consumer::GetMarketInfoFromTecCompanyAsUser(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_MARKET_FROM_TEC_COMPANY_AS_USER + "/" + param;
	expressInterestName(strName);
}

void Consumer::GetMarketInfoFromBn(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;
	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_MARKET_FROM_BN + "/" + param;
	expressInterestName(strName);
}

void Consumer::GetRegDataFromOtherUser(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_REG_DATA_FROM_OTHER_USER + "/" + param;
	expressInterestName(strName);
}

void Consumer::UserPubkeySync(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_REQ_USER_PUBKEY_SYNC + "/" + param;
	expressInterestName(strName);
}

void Consumer::GetAuthRandom(const string& strMarket, const char* param)
{
	 m_success = false;
        std::ostringstream s;
        s<<m_pronum;

        string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
                                        + Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_AUTH_RANDOM + "/" + param;
        expressInterestName(strName);
}


void Consumer::GetAuthToken(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_AUTH_TOKEN + "/" + param;
	expressInterestName(strName);
}


void Consumer::GetConsumeBill(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_CONSUME_BILL + "/" + param;
	expressInterestName(strName);
}

void Consumer::GetConsumeBillSum(const string& strMarket, const char* param)
{
	m_success = false;
	std::ostringstream s;
	s<<m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
					+ Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
					+ TYPE_GET_CONSUME_SUM + "/" + param;
	expressInterestName(strName);
}

void Consumer::expressInterestName(const string& strName)
{
       // printf("expressInterestName: strName is %s\n", strName.c_str());
	Interest interest(Name(strName.c_str()));
	interest.setInterestLifetime(time::milliseconds(10000));
	interest.setMustBeFresh(true);

	if(nullptr == m_pface)
	{
		m_pface = std::unique_ptr<Face>(new Face);
	}

	if (nullptr == m_pface) {
		BN_LOG_ERROR("get block data error[new face failed].");
		m_errormsg = "new face failed";
		return;
	}

	try {
		BN_LOG_DEBUG("express interest:%s", strName.c_str());

		m_pface->expressInterest(interest, bind(&Consumer::onData, this, _1, _2),
				bind(&Consumer::onNack, this, _1, _2),
				bind(&Consumer::onTimeout, this, _1));

		// processEvents will block until the requested data received or timeout occurs
		m_pface->processEvents();
	} catch (std::runtime_error& e) {
		m_pface = nullptr;
		BN_LOG_ERROR("get block date catch error:%s", e.what());
		m_errormsg = "ylfd error";
	} catch (...) {
		m_pface = nullptr;
		
		BN_LOG_ERROR("get block date catch unknow error:");
		m_errormsg = "unknow ylfd error";
	}
}

//New interface add by gd 2018.04.05, Used to notify BSDR to store the specified data on chain
void Consumer::NotifyOnChain(const string& strMarket, const char* param)
{
        m_success = false; 
        std::ostringstream s;
        s<<m_pronum;

        string strName = PRIX_BSDR_PRODUCER + "/" + strMarket + "/"
                                        + Config::Instance()->m_sMacAddr + "/" + s.str() + "/"
                                        + TYPE_NOTIFY_ON_CHAIN + "/" + param;
        expressInterestName(strName);
}

/*
void Consumer::GetBlockData(const string& consumerMarket,
		const string& dataMarket, const string& blocklabel) {
	m_success = false;

	std::ostringstream s;
    s << m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + dataMarket + "/" 
	               + Config::Instance()->m_sMacAddr + "/" + s.str() + "/" 
	               + TYPE_GET_DATA + "/" + consumerMarket + "/" + dataMarket 
	               + blocklabel;
	if (nullptr == m_pface) {
		BN_LOG_ERROR("get block data error[new face failed].");
		m_errormsg = "new face failed";
		return;
	}

	try {
		BN_LOG_DEBUG("express interest:%s", strName.c_str());

		m_pface->expressInterest(interest, bind(&Consumer::onData, this, _1, _2),
				bind(&Consumer::onNack, this, _1, _2),
				bind(&Consumer::onTimeout, this, _1));

		// processEvents will block until the requested data received or timeout occurs
		m_pface->processEvents();
	} catch (std::runtime_error& e) {
		m_pface = nullptr;
		BN_LOG_ERROR("get block date catch error:%s", e.what());
		m_errormsg = "ylfd error";
	} catch (...) {
		m_pface = nullptr;
		
		BN_LOG_ERROR("get block date catch unknow error:");
		m_errormsg = "unknow ylfd error";
	}
}

void Consumer::GetBlockData(const string& consumerMarket,
		const string& dataMarket, const string& blocklabel) {
	m_success = false;

	std::ostringstream s;
    s << m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + dataMarket + "/" 
	               + Config::Instance()->m_sMacAddr + "/" + s.str() + "/" 
	               + TYPE_GET_DATA + "/" + consumerMarket + "/" + dataMarket 
	               + blocklabel;
    expressInterestName(strName);
	// _LOG_ERROR("GetBlockData[name:" << strName << "]");

	
}

void Consumer::GetTranInfo(const string& consumerMarket,
		const string& dataMarket, const string& blocklabel) {
	m_success = false;

	std::ostringstream s;
    s << m_pronum;

	string strName = PRIX_BSDR_PRODUCER + "/" + dataMarket + "/" 
	               + Config::Instance()->m_sMacAddr + "/" + s.str() + "/" 
	               + TYPE_GET_TRAN + "/" + consumerMarket + "/" + dataMarket 
	               + blocklabel;

	// _LOG_TRACE("GetTranInfo[name:" << strName << "]");

	Interest interest(Name(strName.c_str()));
	interest.setInterestLifetime(time::milliseconds(10000));
	interest.setMustBeFresh(true);

	if(nullptr == m_pface)
	{
		m_pface = std::unique_ptr<Face>(new Face);
	}

	if (nullptr == m_pface) {
		BN_LOG_ERROR("et tran info error[new face failed].");
		m_errormsg = "new face failed";
		return;
	}

	try {
		BN_LOG_DEBUG("express interest:%s", strName.c_str());

		m_pface->expressInterest(interest,
				bind(&Consumer::onTranData, this, _1, _2),
				bind(&Consumer::onNack, this, _1, _2),
				bind(&Consumer::onTimeout, this, _1));

		m_pface->processEvents();
	} catch (std::runtime_error& e) {
		BN_LOG_ERROR("get tran info catch error:%s", e.what());
		m_errormsg = "ylfd error";
	} catch (...) {
		BN_LOG_ERROR("get tran info catch unknow error");
		m_errormsg = "unknow ylfd error";
	}

}

void Consumer::GetTxID(const string& dataMarket, const string& blocklabel)
{
	m_success = false;

	std::ostringstream s;
    s << m_pronum;

	string strName = "/yl/broadcast/egz/bsdr/producer/" + dataMarket + "/" 
	               + Config::Instance()->m_sMacAddr + "/" + s.str() + "/gettxid/" 
	               + dataMarket + blocklabel;

	// _LOG_ERROR("GetTxID[name:" << strName << "]");

	Interest interest(Name(strName.c_str()));
	interest.setInterestLifetime(time::milliseconds(10000));
	interest.setMustBeFresh(true);

	if(nullptr == m_pface)
	{
		m_pface = std::unique_ptr<Face>(new Face);
	}

	if (nullptr == m_pface) {
		BN_LOG_ERROR("get txid error[new face failed].");
		m_errormsg = "new face failed";
		return;
	}

	try {
		// _LOG_INFO("express interest:" << strName);

		m_pface->expressInterest(interest,
				bind(&Consumer::onTxIDData, this, _1, _2),
				bind(&Consumer::onNack, this, _1, _2),
				bind(&Consumer::onTimeout, this, _1));

		m_pface->processEvents();
	} catch (std::runtime_error& e) {
		BN_LOG_ERROR("get txid catch error:%s", e.what());
		m_errormsg = "ylfd error";
	} catch (...) {
		BN_LOG_ERROR("get txid catch unknow error.");
		m_errormsg = "unknow ylfd error";
	}
}*/

void Consumer::onData(const Interest& interest, const Data& data) {
	Name dataName(interest.getName());
	Name::Component comType = dataName.get(POS_TYPE);
	std::string strType = comType.toUri();

	Block bck = data.getContent();
	size_t size = bck.value_size();

	//begin decrypt the content by mali 20170606 add
	//128bits key.
	unsigned char key[KEY_BYTE_NUMBERS];
	//Init vector.
	unsigned char iv[KEY_BYTE_NUMBERS];

	
	memcpy(key, KEY, KEY_BYTE_NUMBERS);
	memcpy(iv, INITIAL_VECTOR, KEY_BYTE_NUMBERS);

	int decryptedtext_len, ciphertext_len;
	ciphertext_len = size;

	unsigned char *ciphertextbuf = (unsigned char *) bck.value();

	// if (Config::Instance()->m_sCipherSwitch == 1) {
	// 	hexdump(stdout, "=======ciphertextbuf=======", ciphertextbuf,
	// 			ciphertext_len);
	// }

	unsigned char *decryptplaintextbuf = NULL;

	if (Config::Instance()->m_sCipherSwitch == 1) {
		decryptplaintextbuf = (unsigned char*) malloc(
				MAX_TEXT_SIZE * sizeof(char));
        
	if(NULL==decryptplaintextbuf)
        {
            BN_LOG_ERROR("decryptplaintextbuf malloc failed");
            m_errormsg = "failed to malloc memory space";
            return;
        }
        try
        {
            decryptedtext_len = decrypt(ciphertextbuf, ciphertext_len, key, iv,
                decryptplaintextbuf);
        }
        catch (std::runtime_error& e) 
        {
            BN_LOG_ERROR("producer catch nfd error:%s", e.what());
        }
        catch(...)
        {
            BN_LOG_ERROR("producer process events catch unknow error.");
        }
	}

	// if (Config::Instance()->m_sCipherSwitch == 1) {
	// 	hexdump(stdout, "=======decryptplaintextbuf=======",
	// 			decryptplaintextbuf, decryptedtext_len);
	// }
	//end decrypt the content by mali 20170606 add

	if (m_pData != NULL) {
		free(m_pData);
		m_pData = NULL;
	}

	//begin decrypt the content by mali 20170606 modify
	if (Config::Instance()->m_sCipherSwitch == 1) {
		m_pData = (char *) malloc(decryptedtext_len * sizeof(char) + 1);
		if (NULL == m_pData) {
			m_errormsg = "failed to malloc memory space";
			free(decryptplaintextbuf);
			return;
		}
		memset(m_pData, '\0', decryptedtext_len * sizeof(char) + 1);

		strncpy(m_pData, reinterpret_cast<const char*>(decryptplaintextbuf),
				decryptedtext_len);
		free(decryptplaintextbuf);
	} else {
		m_pData = (char *) malloc(size * sizeof(char) + 1);
		if (NULL == m_pData) {
			m_errormsg = "failed to malloc memory space";
			return;
		}
		memset(m_pData, '\0', size * sizeof(char) + 1);

		strncpy(m_pData, reinterpret_cast<const char*>(bck.value()), size);
	}
	//end decrypt the content by mali 20170606 modify

	// _LOG_ERROR("m_pData:" << m_pData);

	m_success = true;

	// fprintf(stderr, "Consumer::onData[%s]\n", m_pData);
}

/*void Consumer::onTranData(const Interest& interest, const Data& data) {
	Name dataName(interest.getName());
	Name::Component comType = dataName.get(POS_TYPE);
	std::string strType = comType.toUri();

	Block bck = data.getContent();
	size_t size = bck.value_size();

	//begin decrypt the content by marty 20170620 add
	//128bits key.
	unsigned char key[KEY_BYTE_NUMBERS];
	//Init vector.
	unsigned char iv[KEY_BYTE_NUMBERS];

	
	memcpy(key, KEY, KEY_BYTE_NUMBERS);
	memcpy(iv, INITIAL_VECTOR, KEY_BYTE_NUMBERS);

	int decryptedtext_len, ciphertext_len;
	ciphertext_len = size;

	unsigned char *ciphertextbuf = (unsigned char *) bck.value();

	// if (Config::Instance()->m_sCipherSwitch == 1) {
	// 	hexdump(stdout, "=======ciphertextbuf=======", ciphertextbuf,
	// 			ciphertext_len);
	// }

        unsigned char *decryptplaintextbuf = NULL;

        if (Config::Instance()->m_sCipherSwitch == 1) 
        {

            decryptplaintextbuf = (unsigned char*) malloc(MAX_TEXT_SIZE * sizeof(char));
            
            try
            {
                decryptedtext_len = decrypt(ciphertextbuf, ciphertext_len, key, iv,
                    decryptplaintextbuf);
            }
            catch (std::runtime_error& e) 
            {
                BN_LOG_ERROR("producer catch nfd error:%s", e.what());
            }
            catch(...)
            {
                BN_LOG_ERROR("producer process events catch unknow error.");
            }
        }

	// if (Config::Instance()->m_sCipherSwitch == 1) {
	// 	hexdump(stdout, "=======decryptplaintextbuf=======",
	// 			decryptplaintextbuf, decryptedtext_len);
	// }
	//end decrypt the content by marty 20170620 add

	if (m_pData != NULL) {
		free(m_pData);
		m_pData = NULL;
	}

        //begin decrypt the content by marty 20170620 modify
        if (Config::Instance()->m_sCipherSwitch == 1)
        {
            m_pData = (char *) malloc(decryptedtext_len * sizeof(char) + 1);
            if (NULL == m_pData) {
                m_errormsg = "failed to malloc memory space";
                free(decryptplaintextbuf);
                return;
            }
            
            memset(m_pData, '\0', decryptedtext_len * sizeof(char) + 1);

            strncpy(m_pData, reinterpret_cast<const char*>(decryptplaintextbuf),
            decryptedtext_len);
            free(decryptplaintextbuf);
                
        } 
        else 
        {
            m_pData = (char *) malloc(size * sizeof(char) + 1);
            
            if (NULL == m_pData) {
                m_errormsg = "failed to malloc memory space";
                return;
            }
            
            memset(m_pData, '\0', size * sizeof(char) + 1);

            strncpy(m_pData, reinterpret_cast<const char*>(bck.value()), size);
        }
        //end decrypt the content by marty 20170620 modify

    // _LOG_ERROR("m_pData:" << m_pData);

	m_success = true;

	// fprintf(stderr, "Consumer::onTranData[%s]\n", m_pData);
}
*/
void Consumer::onTxIDData(const Interest& interest, const Data& data)
{
	Block bck = data.getContent();
	size_t size = bck.value_size();

	//begin decrypt the content by marty 20170620 add
	//128bits key.
	unsigned char key[KEY_BYTE_NUMBERS];
	//Init vector.
	unsigned char iv[KEY_BYTE_NUMBERS];

	
	memcpy(key, KEY, KEY_BYTE_NUMBERS);
	memcpy(iv, INITIAL_VECTOR, KEY_BYTE_NUMBERS);

	int decryptedtext_len, ciphertext_len;
	ciphertext_len = size;

	unsigned char *ciphertextbuf = (unsigned char *) bck.value();

	// if (Config::Instance()->m_sCipherSwitch == 1) {
	// 	hexdump(stdout, "=======ciphertextbuf=======", ciphertextbuf,
	// 			ciphertext_len);
	// }

    unsigned char *decryptplaintextbuf = NULL;

    if (Config::Instance()->m_sCipherSwitch == 1) 
    {

        decryptplaintextbuf = (unsigned char*) malloc(MAX_TEXT_SIZE * sizeof(char));
        
        try
        {
            decryptedtext_len = decrypt(ciphertextbuf, ciphertext_len, key, iv,
                decryptplaintextbuf);
        }
        catch (std::runtime_error& e) 
        {
            BN_LOG_ERROR("producer catch nfd error:%s", e.what());
        }
        catch(...)
        {
            BN_LOG_ERROR("producer process events catch unknow error.");
        }
    }
	//end decrypt the content by marty 20170620 add

	if (m_pData != NULL) {
		free(m_pData);
		m_pData = NULL;
	}

    //begin decrypt the content by marty 20170620 modify
    if (Config::Instance()->m_sCipherSwitch == 1)
    {
        m_pData = (char *) malloc(decryptedtext_len * sizeof(char) + 1);
        if (NULL == m_pData) {
            m_errormsg = "failed to malloc memory space";
            free(decryptplaintextbuf);
            return;
        }
        
        memset(m_pData, '\0', decryptedtext_len * sizeof(char) + 1);

        strncpy(m_pData, reinterpret_cast<const char*>(decryptplaintextbuf),
        decryptedtext_len);
        free(decryptplaintextbuf);
            
    } 
    else 
    {
        m_pData = (char *) malloc(size * sizeof(char) + 1);
        
        if (NULL == m_pData) {
            m_errormsg = "failed to malloc memory space";
            return;
        }
        
        memset(m_pData, '\0', size * sizeof(char) + 1);

        strncpy(m_pData, reinterpret_cast<const char*>(bck.value()), size);
    }
    //end decrypt the content by marty 20170620 modify

	m_success = true;
}

void Consumer::onNack(const Interest& interest, const lp::Nack& nack) {
	m_errormsg = "Network Nack";
}

void Consumer::onTimeout(const Interest& interest) {
	m_errormsg = "get data timeout";
}

