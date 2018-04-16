//gsoap ns service name:	baarsvr
//gsoap ns service namespace:	https://www.earthledger.com/svr/baarsvr.wsdl
//gsoap ns service location:	https://localhost:8080
//gsoap ns service style:	rpc
//gsoap ns service encoding:	encoded

//gsoap ns schema namespace: urn:ylapi
struct ns__ResponseInfo
{
	int      ResultCode;
	char *   Description;
};


struct ns__egzRet
{
	char * json;
	char * txid;
};

struct ns__ResponseData
{
	int      ResultCode;
	char *   Description;
	char *   Content;
};

int ns__StateServiceUrlSet(const char* param, struct ns__ResponseData* resInfo);
int ns__MarketQueryfromBn(const char* param, struct ns__ResponseData* resInfo);
int ns__MarketDisableInBnNode(const char* param , struct ns__ResponseData* resInfo);
int ns__MarketAttachBnNode( const char* param, struct ns__ResponseData* resInfo);
int ns__MarketBalanceUpdataFromBnNode(const char* param , struct ns__ResponseData* resData);

int ns__AuthTokenGet( const char* param, struct ns__ResponseData* resData);
int ns__AuthRandomGet(const char* param, struct ns__ResponseData* resData);
int ns__UserPubKeySync( const char* param, struct ns__ResponseData* resData);
int ns__ConsumeBillSumQuery( const char* param, struct ns__ResponseData* resData);
int ns__ConsumeBillQuery(const char* param, struct ns__ResponseData* resData);
int ns__RegDataQueryFromUser( const char* param, struct ns__ResponseData* resData);
int ns__MarketQueryFromUser( const char* param, struct ns__ResponseData* resData);
int ns__MarketQueryFromMarket( const char* param , struct ns__ResponseData* resData);
int ns__MarketQueryFromTecCompany(const char* param, struct ns__ResponseData* resInfo );
int ns__MarketQureyfromTecAsUser( const char* param, struct ns__ResponseData* resInfo);
int ns__RegisterDataPriceUpdate( const char* param, struct ns__ResponseData *resInfo);
//int ns__RegisterDataPriceUpdate(const soap* soap, const char* param, struct ns__ResponseInfo *resInfo);
//int ns__RegisterDataPriceUpdate(const soap* soap, const char* param, struct ns__ResponseInfo *resInfo);
int ns__RegisterDataDisable(const char* param, struct ns__ResponseData * resInfo);
int ns__RegisterDataUpdate( const char *param, struct ns__ResponseData * resInfo);
int ns__RegisterDataSet(const char * param,struct ns__ResponseData * resInfo);
int ns__RegisterDataSizeQuery(  char* param, struct ns__ResponseData* resData);
int ns__RegisterDataPriceQuery( const char* param, struct ns__ResponseData* resData);
int ns__RegisterDataQuery( const char* param, struct ns__ResponseData* resData);

//New interface add by gd 2018.04.05, Used to notify BSDR to store the specified data on chain
int ns__NoticeOnChain( const char* param, struct ns__ResponseInfo* resInfo);
//int ns__RegisterData(char * SrcDeviceNo, char * TimeStamp, char * Content, char * BlockLabel, char * Market, char * Price, struct ns__ResponseInfo * resInfo);
//int ns__Consume(char * Blocklabel, char * ConsumerMarket, char * DataMarket, struct ns__ResponseData * resData);
//int ns__QueryTx(char * Blocklabel, char * ConsumerMarket, char * DataMarket, struct ns__ResponseData * resData);

//int ns__egzReg(char * blocklabel, char * market, char * json, struct ns__egzRet * ret);

//gsoap f schema namespace: urn:form

 

//int f__RegData(struct f__RetInfo * retInfo);
//int f__Consume(struct f__RetData * retData);
//int f__QueryTx(struct f__RetData * retData);
