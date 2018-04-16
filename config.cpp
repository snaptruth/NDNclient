#include "config.h"
//#include "tinyxml/tinyxml.h"
#include "bnlogif.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>  
#include <netinet/in.h>

#include <sstream>

Config* Config::pConfig = new Config();

Config* Config::Instance()
{
	return pConfig;
}

Config::Config()
{

}
Config::~Config()
{

}

bool Config::Init()
{
	if(!ReadConfig("ylca.xml"))
	{
		return false;
	}

    return InitBrInfo();
}

bool Config::ReadConfig(const string& sCfgFileName)
{
	TiXmlDocument doc(sCfgFileName.c_str());
	bool loadOkay = doc.LoadFile();
	//failetoload'phonebookdata.xml'.
	if(!loadOkay){
		BN_LOG_ERROR("Could not load config file %s .Error=%s, Exiting.",
			sCfgFileName.c_str(), doc.ErrorDesc());
		return false;
	}

	TiXmlElement* root = doc.RootElement();

    TiXmlElement* eleLogger = root->FirstChildElement("LOGGER");
    if(NULL == eleLogger)
    {
        m_log2console = 0;
    }
    else
    {
        if(NULL == eleLogger->Attribute("Switch2Console", &m_log2console))
        {
            m_log2console = 0;
        }
       // printf("\n\ntesttesttest%d\n\n",m_log2console);
    }

	TiXmlElement* eleNamePub = root->FirstChildElement("NAMEPUB");
	if(NULL == eleNamePub)
	{
		BN_LOG_ERROR("Element[NAMEPUB] not exist.");
		return false;
	}
	if(NULL == eleNamePub->Attribute("ReExpressTimer", &m_sBnrTimer))
	{
		BN_LOG_ERROR("Attribute[Element:NAMEPUB][Attr:ReExpressTimer] not exist.");
		return false;
	}

    TiXmlElement* eleConsumer = root->FirstChildElement("CONSUMER");
    if(NULL == eleConsumer)
    {
        BN_LOG_ERROR("Element[CONSUMER] not exist.");
        return false;
    }
    if(NULL == eleConsumer->Attribute("ThreadNumber", &m_consumerNum))
    {
        BN_LOG_ERROR("Attribute[Element:CONSUMER][Attr:ThreadNumber] not exist.");
        return false;
    }
    if(NULL == eleConsumer->Attribute("LinkNumber", &m_conumercfg.m_linknum))
    {
        BN_LOG_ERROR("Attribute[Element:CONSUMER][Attr:LinkNumber] not exist.");
        return false;
    }
    if(NULL == eleConsumer->Attribute("PoolSize", &m_conumercfg.m_poolsize))
    {
        BN_LOG_ERROR("Attribute[Element:CONSUMER][Attr:PoolSize] not exist.");
        return false;
    }
    if(NULL == eleConsumer->Attribute("Increment", &m_conumercfg.m_increment))
    {
        BN_LOG_ERROR("Attribute[Element:CONSUMER][Attr:Increment] not exist.");
        return false;
    }

	//begin cipher switch by liulijin 20170606 add
  TiXmlElement* eleCipher = root->FirstChildElement("CIPHERS");
  if(NULL == eleCipher)
  {
    BN_LOG_ERROR("Element[CIPHERS] not exist.");
    return false;
  }
  if(NULL == eleCipher->Attribute("SwitcherNo", &m_sCipherSwitch))
  {
    BN_LOG_ERROR("Attribute[Element:CIPHERS][Attr:SwitcherNo] not exist.");
    return false;
  }
  //end cipher switch by liulijin 20170606 add



	//begin uploaderl by zhonghaijing 20180323 add
TiXmlElement* eleUploadAddr = root->FirstChildElement("UPLOADADDR");
if(NULL == eleUploadAddr)
{
  BN_LOG_ERROR("Element[UPLOADADDR] not exist.");
  return false;
}
if(NULL == eleUploadAddr->Attribute("UpLoadUrl"))
{
  BN_LOG_ERROR("Attribute[Element:UPLOADADDR][Attr:UpLoadUrl] not exist.");
  return false;
}
else
{
	m_statusSerAddress = eleUploadAddr->Attribute("UpLoadUrl");
}
//end uploadurl by zhonghaijing 20180323 add








	   //begin uploaderl by zhonghaijing 20180328 add



//	TiXmlElement* eleBsdr1Sys = root->FirstChildElement("BSDR1system");
//	if(NULL == eleBsdr1Sys)
//	{
//	  BN_LOG_ERROR("Element[BSDR1system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBsdr1Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BSDR1system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bsdr1Sys = eleBsdr1Sys->Attribute("INSTANCE");
//	}
//
//	TiXmlElement* eleBsdr2Sys = root->FirstChildElement("BSDR2system");
//	if(NULL == eleBsdr2Sys)
//	{
//	  BN_LOG_ERROR("Element[BSDR2system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBsdr2Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BSDR2system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bsdr2Sys = eleBsdr2Sys->Attribute("INSTANCE");
//	}
//
//	TiXmlElement* eleBsdr3Sys = root->FirstChildElement("BSDR3system");
//	if(NULL == eleBsdr3Sys)
//	{
//	  BN_LOG_ERROR("Element[BSDR3system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBsdr3Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BSDR3system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bsdr3Sys = eleBsdr3Sys->Attribute("INSTANCE");
//	}
//
//	TiXmlElement* eleBsdr4Sys = root->FirstChildElement("BSDR4system");
//	if(NULL == eleBsdr4Sys)
//	{
//	  BN_LOG_ERROR("Element[BSDR4system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBsdr4Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BSDR4system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bsdr4Sys = eleBsdr4Sys->Attribute("INSTANCE");
//	}
//
//
//	TiXmlElement* eleBsdr5Sys = root->FirstChildElement("BSDR5system");
//	if(NULL == eleBsdr5Sys)
//	{
//	  BN_LOG_ERROR("Element[BSDR5system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBsdr5Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BSDR5system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bsdr5Sys = eleBsdr5Sys->Attribute("INSTANCE");
//	}
//
//
//	TiXmlElement* eleBcr1Sys = root->FirstChildElement("BCR1system");
//	if(NULL == eleBcr1Sys)
//	{
//	  BN_LOG_ERROR("Element[BCR1system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBcr1Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BCR1system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bcr1Sys = eleBcr1Sys->Attribute("INSTANCE");
//	}
//
//
//	TiXmlElement* eleBcr2Sys = root->FirstChildElement("BCR2system");
//	if(NULL == eleBcr2Sys)
//	{
//	  BN_LOG_ERROR("Element[BCR2system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBcr2Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BCR2system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bcr2Sys = eleBcr2Sys->Attribute("INSTANCE");
//	}
//
//
//
//	TiXmlElement* eleBcr3Sys = root->FirstChildElement("BCR3system");
//	if(NULL == eleBcr3Sys)
//	{
//	  BN_LOG_ERROR("Element[BCR3system] not exist.");
//	  return false;
//	}
//	if(NULL == eleBcr3Sys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:BCR3system][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_Bcr3Sys = eleBcr3Sys->Attribute("INSTANCE");
//	}
//
//
//	TiXmlElement* eleBaarSys = root->FirstChildElement("FBAARsystem");
//	if(NULL == eleBaarSys)
//	{
//	  BN_LOG_ERROR("Element[FBAARsystem] not exist.");
//	  return false;
//	}
//	if(NULL == eleBaarSys->Attribute("INSTANCE"))
//	{
//	  BN_LOG_ERROR("Attribute[Element:FBAARsystem][Attr:INSTANCE] not exist.");
//	  return false;
//	}
//	else
//	{
//		m_BaarSys = eleBaarSys->Attribute("INSTANCE");
//	}
//


	   //begin uploaderl by zhonghaijing 20180328 add




    //begin uploaderl by zhonghaijing 20180327 add
	TiXmlElement* eleDiskRoute = root->FirstChildElement("DISKROUTE");
	if(NULL == eleDiskRoute)
	{
	  BN_LOG_ERROR("Element[DISKROUTE] not exist.");
	  return false;
	}
	if(NULL == eleDiskRoute->Attribute("DISKROUTE"))
	{
	  BN_LOG_ERROR("Attribute[Element:DISKROUTE][Attr:DISKROUTE] not exist.");
	  return false;
	}
	else
	{
		m_diskroute = eleDiskRoute->Attribute("DISKROUTE");
	}


	TiXmlElement* eleBnNum = root->FirstChildElement("BNNUM");
	if(NULL == eleBnNum)
	{
		BN_LOG_ERROR("Element[BNNUM] not exist.");
		return false;
	}
	if(NULL == eleBnNum->Attribute("BNNUM", &m_bnnum))
	{
		BN_LOG_ERROR("Attribute[Element:BNNUM][Attr:BNNUM] not exist.");
		return false;
	}


	TiXmlElement* eleBaarNum = root->FirstChildElement("BAARNUM");
	if(NULL == eleBaarNum)
	{
		BN_LOG_ERROR("Element[BAARNUM] not exist.");
		return false;
	}
	if(NULL == eleBaarNum->Attribute("BAARNUM", &m_baarnum))
	{
		BN_LOG_ERROR("Attribute[Element:BAARNUM][Attr:BAARNUM] not exist.");
		return false;
	}


	TiXmlElement* eleBsdrNum = root->FirstChildElement("BSDRNUM");
	if(NULL == eleBsdrNum)
	{
		BN_LOG_ERROR("Element[BSDRNUM] not exist.");
		return false;
	}
	if(NULL == eleBsdrNum->Attribute("BSDRNUM", &m_bsdrnum))
	{
		BN_LOG_ERROR("Attribute[Element:BSDRNUM][Attr:BSDRNUM] not exist.");
		return false;
	}


	TiXmlElement* eleBcrNum = root->FirstChildElement("BCRNUM");
	if(NULL == eleBcrNum)
	{
		BN_LOG_ERROR("Element[BCRNUM] not exist.");
		return false;
	}
	if(NULL == eleBcrNum->Attribute("BCRNUM", &m_bcrnum))
	{
		BN_LOG_ERROR("Attribute[Element:BCRNUM][Attr:BCRNUM] not exist.");
		return false;
	}

	//end uploadurl by zhonghaijing 20180327 add





	//by haijing20180401


	//TiXmlElement* root = doc.RootElement();
	TiXmlElement* eleInstance = root->FirstChildElement("INSTANCE");



		for(TiXmlElement* elem = eleInstance->FirstChildElement("name");NULL!=elem;elem=elem->NextSiblingElement())
		{
			  if(NULL == elem->Attribute("instance"))
	     	 {
	          //fprintf(stderr, "loadXMLtoCreateWallet port empty!\n");
	         // _LOG_INFO("loadXMLtoCreateWallet port empty!");
	          continue;
	     	 }
			TiXmlElement * tmpElem ;
	        tmpElem =(TiXmlElement *) elem->Clone();
	        m_Instance.push_back(tmpElem);

		}
		//printf("\ntesttesttestthe portSize is %d\n",m_Instance.size());
//		printf("\nvector[0] %s\n",m_Instance[0]->Attribute("instance"));
//		printf("\nvector[1] %s\n",m_Instance[1]->Attribute("instance"));











	//by haijing20180401



















	return true;
}

bool Config::InitBrInfo()
{
	struct ifreq ifr;
    struct ifconf ifc;
    char buf[2048];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        BN_LOG_ERROR("socket error.");
        return false;
    }
   
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) 
    {
        BN_LOG_ERROR("ioctl error.");
        return false;
    }
   
    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    char szMac[64];
    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    unsigned char * ptr ;
                    ptr = (unsigned char  *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
                    snprintf(szMac,64,"%02X%02X%02X%02X%02X%02X",*ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
                    m_sMacAddr = szMac;
                    BN_LOG_INTERFACE("mac:%s", szMac);
                    break;
                }
            }
        }else{
            return false;
        }
    }

    return true;
}

