#ifndef YL_BAAR_CONFIG
#define YL_BAAR_CONFIG

#include <string>
#include <vector>
#include "tinyxml/tinyxml.h"

using namespace std;

class Config
{
protected:
	Config();
	~Config();

private:
	static Config* pConfig;
public:
	static Config* Instance();

public:
	bool Init();
private:
	bool ReadConfig(const string& sCfgFileName = "");
	bool InitBrInfo();

public:
	struct ConsumerCFG
	{
		int m_linknum;
		int m_poolsize;
		int m_increment;
		int m_maxsize;
	};
	struct ConsumerCFG m_conumercfg;

	int m_log2console;

	int m_sBnrTimer;
	int m_consumerNum;   // consumer number
	//begin cipher switch by liulijin 20170606 add
	int m_sCipherSwitch;
	//end cipher switch by liulijin 20170606 add
	//begin uploadaddr by zhonghaijing add
	int m_bnnum;
	int m_baarnum;
	int m_bsdrnum;
	int m_bcrnum;
	string m_statusSerAddress;
	string m_diskroute;
	vector<TiXmlElement*> m_Instance;//20180401
	//end uploadaddr by zhonghaijing add

	string m_sHttpsSvr;
	string m_sPort;

	string m_sMacAddr;
};




#endif	/* #ifndef YL_BAAR_CONFIG */
