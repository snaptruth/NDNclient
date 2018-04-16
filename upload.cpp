#include "upload.h"
#include "config.h"
#include "bnlogif.h"
#include <sys/time.h>

//state leval
#define NOTE 0.3
#define WARN 0.2
#define SERIOUSWARN 0.1
//size of array
#define COLUMN 3072
#define ROW 15
//array for state
char memdata[ROW][COLUMN];
char memstate[ROW][COLUMN];
char cpudata[ROW][COLUMN];
char cpustate[ROW][COLUMN];
char diskdata[ROW][COLUMN];
char diskstate[ROW][COLUMN];
char dbdata[ROW][COLUMN];
char dbstate[ROW][COLUMN];
char nodestate[ROW][COLUMN];
char cephstate[COLUMN];

CUpLoad *CUpLoad::m_pUpload = new CUpLoad();

CUpLoad::CUpLoad():m_stateServiceAddress(""), m_pCurl(NULL), m_errCode(0),m_pthread(NULL),m_shouldResume(true),m_strTid(""),m_bnNum(0),m_baarNum(0),m_bsdrNum(0),m_bcrNum(0),m_diskRoute("")
{

}

CUpLoad::~CUpLoad()
{

}

CUpLoad* CUpLoad::getInstance()
{
    return m_pUpload;
}

bool CUpLoad::initialize()
{
    m_stateServiceAddress = Config::Instance()->m_statusSerAddress;
    m_diskRoute = Config::Instance()->m_diskroute;
    //if resolve failed
    if(0 == m_diskRoute.size() || 0 == m_stateServiceAddress.size())
    {
    	 BN_LOG_ERROR("upLoad read config failed");
    	 return false;
    }

	m_bnNum = Config::Instance()->m_bnnum;
    m_baarNum = Config::Instance()->m_baarnum;
    m_bsdrNum = Config::Instance()->m_bsdrnum;
    m_bcrNum = Config::Instance()->m_bcrnum;
    //if resolve failed
    if(0 == m_bnNum || 0 == m_baarNum || 0 == m_bsdrNum || 0 == m_bcrNum)
    {
    	 BN_LOG_ERROR("upLoad read config failed");
    	 return false;
    }

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if(CURLE_OK != res)
    {
        m_errCode = UPLOAD_CURL_ENV_INIT_FAIL;
        BN_LOG_ERROR("upLoad curl_global_init failed");
        return false;
    }
    m_pCurl = curl_easy_init();
    if(NULL == m_pCurl)
    {
        m_errCode = UPLOAD_CURL_ENV_INIT_FAIL;
        BN_LOG_ERROR("upLoad curl_easy_init failed");
        return false;
    }
    return true;
}

bool CUpLoad::run()
{
    m_pthread = new std::thread(&CUpLoad::upLoad, this);
    if(NULL == m_pthread)
    {
        BN_LOG_ERROR("upLoad task start failed");
       // m_errCode = UPLOAD_TASK_START_FAIL;
        return false;
    }
    return true;
}


void CUpLoad::unInitialize()
{
    if(m_pthread)
    {
    	m_shouldResume = true;
        m_pthread->join();
    }
    curl_easy_cleanup(m_pCurl);
    curl_global_cleanup();
}


void CUpLoad::upLoad()
{
    while(m_shouldResume)
    {
        //信息采集&&整合
        getStateInfo();
        //数据写入StateForBn
        write2StateForBn();
        //格式转化
        transJsonFormat();
        //数据发送
        doHttpPost();
        sleep(3);
     }
}

string CUpLoad::getAddress()
{
    std::lock_guard<std::mutex> lock(m_addressMtx);
    return m_stateServiceAddress;
}

bool CUpLoad::setAddress(string& strAddress)
{
    if(strAddress.size() == 0)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_addressMtx);
    m_stateServiceAddress = strAddress;
    return true;
}

bool CUpLoad::setUploadAddress(const char* jasonParam, string& strRet)
{
    if(nullptr == jasonParam)
        return false;
    //step 1:
    string strUrl;
    do{
    	if(m_errCode != UPLOAD_OK)
    	{
    		break;
    	}
        if(!parseJasonParam(jasonParam, strUrl))
        {
            m_errCode = UPLOAD_JSON_PARSE_FAIL;
            break;
        }
    //step 2:
        if(!setAddress(strUrl))
        {
            m_errCode = UPLOAD_SET_ADDRESS_FAIL;
           	BN_LOG_ERROR("upLoad setAddress failed");
        }
    }while(0);
    // step 3:
    return packJsonResult(strRet);
}

bool CUpLoad::parseJasonParam(const char*jasonParam, string& strUrl)
{
	Document url;
	url.Parse(jasonParam);
	if(!url.IsObject())
	{
		BN_LOG_ERROR("upLoad url is not object");
		return false;
	}
	if(!url.HasMember("nodestateuploadurl"))
	{
		BN_LOG_ERROR("upLoad url has no nodestateuploadurl");
		return false;
	}
	strUrl = url["nodestateuploadurl"].GetString();
	if(!url.HasMember("TID"))
	{
		BN_LOG_ERROR("upLoad url has no TID");
		return false;
	}
	m_strTid = url["TID"].GetString();
	return true;
}


bool CUpLoad::packJsonResult(string& strRet)
{
    // according m_strTiild AND m_errCode ,Pack the return Value With Jason Format
	const char* json = "{\"TID\":\"XX\",\"ResultCode\":\"XX\"}";
	 // 1. 把 JSON 解析至 DOM。
	Document document;
	document.Parse(json);
	// 2. 利用 DOM 修改数据。

	char* ResultCode;
	sprintf(ResultCode,"%d", m_errCode);
	document["TID"].SetString(m_strTid.c_str(), strlen(m_strTid.c_str()));
	document["ResultCode"].SetString(ResultCode, strlen(ResultCode));
	strRet = json;
	return true;
}



void CUpLoad::doHttpPost()
{
	char szJsonData[COLUMN];
	memset(szJsonData, 0, sizeof(szJsonData));
	FILE *fd;
	if(NULL == (fd = fopen("upload.json","r")))
	{
		BN_LOG_ERROR("open upload.json error");
	}
	fread(szJsonData, 1, sizeof(szJsonData), fd);
	fclose(fd);

	 CURLcode res;
	 string url = getAddress();

	 if (NULL != m_pCurl)
	 {
		curl_easy_setopt(m_pCurl, CURLOPT_URL, url.c_str());
		curl_slist *plist = curl_slist_append(NULL,
				"Content-Type:application/json;charset=UTF-8");
		curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, plist);
		curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, szJsonData);
	   curl_easy_setopt(m_pCurl, CURLOPT_TIMEOUT, 10);

	   // Perform the request, res will get the return code
				res = curl_easy_perform(m_pCurl);
	   // Check for errors
	   if (res != CURLE_OK)
	   {
		   BN_LOG_ERROR("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
	   }
		else
		{
			m_errCode = UPLOAD_OK;//reset the status code
		}
    }
 }



char* CUpLoad::judgeStateInfo(char* data)
{
	if(atof(data) > 1.0||atof(data) < 0)
	{
		sprintf(data,"Error happen!!");
		return data;
	}
	if(atof(data) > NOTE && atof(data) < 1.0 || fabs(atof(data) - NOTE) < 1e-6)
	{
		sprintf(data,"%0.2f%%Used",100*(1-atof(data+1)));
		return data;
	}
	if(atof(data) > WARN && atof(data) < NOTE || fabs(atof(data) - WARN) < 1e-6)
	{
		sprintf(data,"Over70%,%0.2f%%Used",100*(1-atof(data+1)));
		return data;
	}
	if(atof(data) > SERIOUSWARN && atof(data) < WARN || fabs(atof(data) - SERIOUSWARN) < 1e-6)
	{
		sprintf(data,"Warning!!Over80%,%0.2f%%Used",100*(1-atof(data+1)));
		return data;
	}
	if(atof(data) > 0 && atof(data) < SERIOUSWARN || atof(data) < 1e-6)
	{
		sprintf(data,"SeriousWarning!!!!Over90%,%0.2f%%Used",100*(1-atof(data+1)));
		return data;
	}
}

void CUpLoad::getCephStateInfo()
{
	FILE *fd = NULL;
	char json[COLUMN];
	char temp[COLUMN];
	char *pLine = NULL;
	int i = 0;
	memset( json, 0, sizeof(json));
	memset( cephstate, 0, sizeof(cephstate));
	memset( temp, 0, sizeof(temp));
	if((fd = fopen("stateOfCEPH", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfCEPH Error");
	}
	else
	{
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);

		 pLine = strstr(json, "health:");
		 if(NULL == pLine)
		 {
			 BN_LOG_ERROR("No CEPHinfo");
		 }
		 else
		 {
			 pLine = strstr(pLine, " ");
			 if(NULL == pLine)
			 {
				 perror("Error");
			 }
			 pLine++;
			 while(*pLine != '\n')
			 {
				temp[i] = *pLine;
				 i++;
				 pLine++;
			 }
			 temp[i] = '\0';
			 if(strcmp(temp,"HEALTH_OK") == 0)
			 {

				 sprintf(cephstate,"ok");
			 }
			 else
			 {
				 sprintf(cephstate,"%s",temp);
			 }
		 }
	}
}

void CUpLoad::getMemStateInfo()
{
	FILE *fd = NULL;
	char json[COLUMN];
	memset(memdata, 0, sizeof(memdata));
	memset(memstate, 0, sizeof(memstate));
	memset(json, 0, sizeof(json));

	if((fd = fopen("stateOfMem", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfMem Error");
	}
	else
	{
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document mem;
		if(!mem.Parse(json).HasParseError())
		{
			Value& mem_data = mem["data"];
			//assert(mem_data.IsObject());
			if(mem_data.IsObject())
			{
				//assert(mem_data.HasMember("result"));
				if(mem_data.HasMember("result"))
				{
					if(mem_data["result"].IsArray())
					{
						Value& result = mem_data["result"];
						for(SizeType i = 0; i < mem_data["result"].Capacity(); i++)
						{
							Value& result_i = result[i];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];

							for(int j=0; j<mem_data["result"].Capacity(); j++)
							{
								if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[j]->Attribute("instance")))
								{
									strcpy(memdata[j],value[1].GetString());
									strcpy(memstate[j],judgeStateInfo(memdata[j]));
								}
							}
						}
					}
				}
			}
		}
	}
}

void CUpLoad::getCpuStateInfo()
{
	FILE *fd = NULL;
	char json[COLUMN];
	memset(cpudata, 0, sizeof(cpudata));
	memset(cpustate, 0, sizeof(cpustate));
	memset(json, 0, sizeof(json));

	if((fd = fopen("stateOfBSDR1CPU", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBSDR1CPU Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bsdr1_cpu;
		if(!bsdr1_cpu.Parse(json).HasParseError())
		{
			Value& bsdr1_cpu_data = bsdr1_cpu["data"];
			//assert(bsdr1_cpu_data.IsObject());
			if(bsdr1_cpu_data.IsObject())
			{
				//assert(bsdr1_cpu_data.HasMember("result"));
				if(bsdr1_cpu_data.HasMember("result"))
				{
					if(bsdr1_cpu_data["result"].IsArray() && (bsdr1_cpu_data["result"].Capacity()!=0))
					{
						Value& result = bsdr1_cpu_data["result"];
						Value& result_i = result[0];
						Value& value = result_i["value"];
						Value& metric = result_i["metric"];
						if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[0]->Attribute("instance")))
						{
							strcpy(cpudata[0],value[1].GetString());
							strcpy(cpustate[0],judgeStateInfo(cpudata[0]));
						}
					}
				}
			}
		}
	}
	//BSDR2 CPU状态
	if((fd = fopen("stateOfBSDR2CPU", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBSDR2CPU Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bsdr2_cpu;
		if(!bsdr2_cpu.Parse(json).HasParseError())
		{
			Value& bsdr2_cpu_data = bsdr2_cpu["data"];
			//assert(bsdr2_cpu_data.IsObject());
			if(bsdr2_cpu_data.IsObject())
			{
				//assert(bsdr2_cpu_data.HasMember("result"));
				if(bsdr2_cpu_data.HasMember("result"))
				{
					if(bsdr2_cpu_data["result"].IsArray() && (bsdr2_cpu_data["result"].Capacity()!=0))
					{
						Value& result = bsdr2_cpu_data["result"];
						Value& result_i = result[0];
						Value& value = result_i["value"];
						Value& metric = result_i["metric"];
						if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[1]->Attribute("instance")))
						{
							strcpy(cpudata[1],value[1].GetString());
							strcpy(cpustate[1],judgeStateInfo(cpudata[1]));
						}
					}
				}
			}
		}
	}
		//BSDR3 CPU状态
		if((fd = fopen("stateOfBSDR3CPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfBSDR3CPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document bsdr3_cpu;
			if(!bsdr3_cpu.Parse(json).HasParseError())
			{
				Value& bsdr3_cpu_data = bsdr3_cpu["data"];
//				assert(bsdr3_cpu_data.IsObject());
				if(bsdr3_cpu_data.IsObject())
				{
//					assert(bsdr3_cpu_data.HasMember("result"));
					if(bsdr3_cpu_data.HasMember("result"))
					{
						if(bsdr3_cpu_data["result"].IsArray() && (bsdr3_cpu_data["result"].Capacity()!=0))
						{
							Value& result = bsdr3_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[2]->Attribute("instance")))
							{
								strcpy(cpudata[2],value[1].GetString());
								strcpy(cpustate[2],judgeStateInfo(cpudata[2]));
							}
						}
					}
				}
			}
		}

		//BSDR4 CPU状态
		if((fd = fopen("stateOfBSDR4CPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfBSDR4CPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document bsdr4_cpu;
			if(!bsdr4_cpu.Parse(json).HasParseError())
			{
				Value& bsdr4_cpu_data = bsdr4_cpu["data"];
				//assert(bsdr4_cpu_data.IsObject());
				if(bsdr4_cpu_data.IsObject())
				{
					//assert(bsdr4_cpu_data.HasMember("result"));
					if(bsdr4_cpu_data.HasMember("result"))
					{
						if(bsdr4_cpu_data["result"].IsArray() && (bsdr4_cpu_data["result"].Capacity()!=0))
						{
							Value& result = bsdr4_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[3]->Attribute("instance")))
							{
								strcpy(cpudata[3],value[1].GetString());
								strcpy(cpustate[3],judgeStateInfo(cpudata[3]));
							}
						}
					}
				}
			}
		}

		//BSDR5 CPU状态
		if((fd = fopen("stateOfBSDR5CPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfBSDR5CPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document bsdr5_cpu;
			if(!bsdr5_cpu.Parse(json).HasParseError())
			{
				Value& bsdr5_cpu_data = bsdr5_cpu["data"];
				//assert(bsdr5_cpu_data.IsObject());
				if(bsdr5_cpu_data.IsObject())
				{
					//assert(bsdr5_cpu_data.HasMember("result"));
					if(bsdr5_cpu_data.HasMember("result"))
					{
						if(bsdr5_cpu_data["result"].IsArray() && (bsdr5_cpu_data["result"].Capacity()!=0))
						{
							Value& result = bsdr5_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[4]->Attribute("instance")))
							{
								strcpy(cpudata[4],value[1].GetString());
								strcpy(cpustate[4],judgeStateInfo(cpudata[4]));
							}
						}
					}
				}
			}
		}

		//BCR1 CPU状态
		if((fd = fopen("stateOfBCR1CPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfBCR1CPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document bcr1_cpu;
			if(!bcr1_cpu.Parse(json).HasParseError())
			{
				Value& bcr1_cpu_data = bcr1_cpu["data"];
				//assert(bcr1_cpu_data.IsObject());
				if(bcr1_cpu_data.IsObject())
				{
					//assert(bcr1_cpu_data.HasMember("result"));
					if(bcr1_cpu_data.HasMember("result"))
					{
						if(bcr1_cpu_data["result"].IsArray() && (bcr1_cpu_data["result"].Capacity()!=0))
						{
							Value& result = bcr1_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[5]->Attribute("instance")))
							{
								strcpy(cpudata[5],value[1].GetString());
								strcpy(cpustate[5],judgeStateInfo(cpudata[5]));
							}
						}
					}
				}
			}
		}

		//BCR2 CPU状态
		if((fd = fopen("stateOfBCR2CPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfBCR2CPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document bcr2_cpu;
			if(!bcr2_cpu.Parse(json).HasParseError())
			{
				Value& bcr2_cpu_data = bcr2_cpu["data"];
//				assert(bcr2_cpu_data.IsObject());
				if(bcr2_cpu_data.IsObject())
				{
//					assert(bcr2_cpu_data.HasMember("result"));
					if(bcr2_cpu_data.HasMember("result"))
					{
						if(bcr2_cpu_data["result"].IsArray() && (bcr2_cpu_data["result"].Capacity()!=0))
						{
							Value& result = bcr2_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[6]->Attribute("instance")))
							{
								strcpy(cpudata[6],value[1].GetString());
								strcpy(cpustate[6],judgeStateInfo(cpudata[6]));
							}
						}
					}
				}
			}
		}

		//BCR3 CPU状态
		if((fd = fopen("stateOfBCR3CPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfBCR3CPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document bcr3_cpu;
			if(!bcr3_cpu.Parse(json).HasParseError())
			{
				Value& bcr3_cpu_data = bcr3_cpu["data"];
				//assert(bcr3_cpu_data.IsObject());
				if(bcr3_cpu_data.IsObject())
				{
					//assert(bcr3_cpu_data.HasMember("result"));
					if(bcr3_cpu_data.HasMember("result"))
					{
						if(bcr3_cpu_data["result"].IsArray() && (bcr3_cpu_data["result"].Capacity()!=0))
						{
							Value& result = bcr3_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[7]->Attribute("instance")))
							{
								strcpy(cpudata[7],value[1].GetString());
								strcpy(cpustate[7],judgeStateInfo(cpudata[7]));
							}
						}
					}
				}
			}
		}

		//FBAAR CPU状态
		if((fd = fopen("stateOfFBAARCPU", "r")) == NULL)
		{
			BN_LOG_ERROR("Open stateOfFBAARCPU Error");
		}
		else
		{
			memset( json, 0, sizeof(json));
			fread(json,sizeof(char),sizeof(json),fd);
			fclose(fd);
			Document fbaar_cpu;
			if(!fbaar_cpu.Parse(json).HasParseError())
			{
				Value& fbaar_cpu_data = fbaar_cpu["data"];
//				assert(fbaar_cpu_data.IsObject());
				if(fbaar_cpu_data.IsObject())
				{
//					assert(fbaar_cpu_data.HasMember("result"));
					if(fbaar_cpu_data.HasMember("result"))
					{
						if(fbaar_cpu_data["result"].IsArray() && (fbaar_cpu_data["result"].Capacity()!=0))
						{
							Value& result = fbaar_cpu_data["result"];
							Value& result_i = result[0];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[8]->Attribute("instance")))
							{
								strcpy(cpudata[8],value[1].GetString());
								strcpy(cpustate[8],judgeStateInfo(cpudata[8]));
							}
						}
					}
				}
			}
		}
}



void CUpLoad::getDiskStateInfo()
{
	FILE *fd = NULL;
	char json[COLUMN];
	memset(diskdata, 0, sizeof(diskdata));
	memset(diskstate, 0, sizeof(diskstate));

	if((fd = fopen("stateOfDisk", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfDisk Error");
	}
	else
	{
		memset(json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document disk;
		if(!disk.Parse(json).HasParseError())
		{
			Value& disk_data = disk["data"];
			//assert(disk_data.IsObject());
			if(disk_data.IsObject())
			{
				//assert(disk_data.HasMember("result"));
				if(disk_data.HasMember("result"))
				{
					if(disk_data["result"].IsArray())
					{
						Value& result = disk_data["result"];
						for(SizeType i = 0; i < disk_data["result"].Capacity(); i++)
						{
							Value& result_i = result[i];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							for(int j=0; j<Config::Instance()->m_Instance.size(); j++)
							{
								if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[j]->Attribute("instance")) && !strcmp(metric["mountpoint"].GetString(),m_diskRoute.c_str()))
								{
									strcpy(diskdata[j],value[1].GetString());
									strcpy(diskstate[j],judgeStateInfo(diskdata[j]));
								}
							}
						}
					}
				}
			}
		}
	}
}


void CUpLoad::getDbStateInfo()
{
	FILE *fd = NULL;
	char json[COLUMN];
	memset(dbdata, 0, sizeof(dbdata));
	memset(dbstate, 0, sizeof(dbstate));
	memset(json, 0, sizeof(json));
	if((fd = fopen("stateOfSqlCon", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfSqlCon Error");
	}
	else
	{
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document db;
		if(!db.Parse(json).HasParseError())
		{
			Value& db_data = db["data"];
			//assert(db_data.IsObject());
			if(db_data.IsObject())
			{
				//assert(db_data.HasMember("result"));
				if(db_data.HasMember("result"))
				{
					if(db_data["result"].IsArray())
					{
						Value& result = db_data["result"];
						for(SizeType i = 0; i < db_data["result"].Capacity(); i++)
						{
							Value& result_i = result[i];
							Value& value = result_i["value"];
							Value& metric = result_i["metric"];
							for(int j=0; j<db_data["result"].Capacity(); j++)
							{
								if(!strcmp(metric["instance"].GetString(),Config::Instance()->m_Instance[j]->Attribute("instance")))
								{
									sprintf(dbdata[j],"%sConnection",value[1].GetString());
								}
							}
						}
					}
				}
			}
		}
	}
}


void CUpLoad::getNodeStateInfo()
{
	FILE *fd = NULL;
	char json[COLUMN];
	memset(nodestate, 0, sizeof(nodestate));

	//BCR1_NODE状态
	if((fd = fopen("stateOfBCR1Chain1", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR1Chain1 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr1_chain1;
		if(!bcr1_chain1.Parse(json).HasParseError())
		{
			Value& bcr1_chain1_state = bcr1_chain1["result"];
	//		assert(bcr1_chain1_state.IsObject());
			if(bcr1_chain1_state.IsObject())
			{
				if(bcr1_chain1_state.HasMember("balance"))
				{
					sprintf(nodestate[0],"chain1_balance:%f",bcr1_chain1_state["balance"].GetDouble());
				}
			}
		}
	}

	if((fd = fopen("stateOfBCR1Chain2", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR1Chain2 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr1_chain2;
		if(!bcr1_chain2.Parse(json).HasParseError())
		{
			Value& bcr1_chain2_state = bcr1_chain2["result"];
			//assert(bcr1_chain2_state.IsObject());
			if(bcr1_chain2_state.IsObject())
			{
				if(bcr1_chain2_state.HasMember("balance"))
				{
					sprintf(nodestate[0],"%s,chain2_balance:%f",nodestate[0], bcr1_chain2_state["balance"].GetDouble());
				}
			}
		}
	}

	if((fd = fopen("stateOfBCR1Chain3", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR1Chain3 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr1_chain3;
		if(!bcr1_chain3.Parse(json).HasParseError())
		{
			Value& bcr1_chain3_state = bcr1_chain3["result"];
			//assert(bcr1_chain3_state.IsObject());
            if(bcr1_chain3_state.IsObject())
            {
				if(bcr1_chain3_state.HasMember("balance"))
				{
					sprintf(nodestate[0],"%s,chain3_balance:%f",nodestate[0], bcr1_chain3_state["balance"].GetDouble());
				}
            }
		}
	}


	//BCR2_NODE状态
	if((fd = fopen("stateOfBCR2Chain1", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR2Chain1 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr2_chain1;
		if(!bcr2_chain1.Parse(json).HasParseError())
		{
			Value& bcr2_chain1_state = bcr2_chain1["result"];
			//assert(bcr2_chain1_state.IsObject());
            if(bcr2_chain1_state.IsObject())
            {
				if(bcr2_chain1_state.HasMember("balance"))
				{
					sprintf(nodestate[1],"chain1_balance:%f",bcr2_chain1_state["balance"].GetDouble());
				}
            }
		}
	}

	if((fd = fopen("stateOfBCR2Chain2", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR2Chain2 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr2_chain2;
		if(!bcr2_chain2.Parse(json).HasParseError())
		{
			Value& bcr2_chain2_state = bcr2_chain2["result"];
			//assert(bcr2_chain2_state.IsObject());
			if(bcr2_chain2_state.IsObject())
			{
				if(bcr2_chain2_state.HasMember("balance"))
				{
					sprintf(nodestate[1],"%s,chain2_balance:%f",nodestate[1], bcr2_chain2_state["balance"].GetDouble());
				}
			}
		}
	}

	if((fd = fopen("stateOfBCR2Chain3", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR2Chain3 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr2_chain3;
		if(!bcr2_chain3.Parse(json).HasParseError())
		{
			Value& bcr2_chain3_state = bcr2_chain3["result"];
			//assert(bcr2_chain3_state.IsObject());
            if(bcr2_chain3_state.IsObject())
            {
				if(bcr2_chain3_state.HasMember("balance"))
				{
					sprintf(nodestate[1],"%s,chain3_balance:%f",nodestate[1], bcr2_chain3_state["balance"].GetDouble());
				}
            }
		}
	}

	//BCR3_NODE状态
	if((fd = fopen("stateOfBCR3Chain1", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR3Chain1 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr3_chain1;
		if(!bcr3_chain1.Parse(json).HasParseError())
		{
			Value& bcr3_chain1_state = bcr3_chain1["result"];
			//assert(bcr3_chain1_state.IsObject());
            if(bcr3_chain1_state.IsObject())
            {
				if(bcr3_chain1_state.HasMember("balance"))
				{
					sprintf(nodestate[2],"chain1_balance:%f",bcr3_chain1_state["balance"].GetDouble());
				}
            }
		}
	}

	if((fd = fopen("stateOfBCR3Chain2", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR3Chain2 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr3_chain2;
		if(!bcr3_chain2.Parse(json).HasParseError())
		{
			Value& bcr3_chain2_state = bcr3_chain2["result"];
			//assert(bcr3_chain2_state.IsObject());
			if(bcr3_chain2_state.IsObject())
			{
				if(bcr3_chain2_state.HasMember("balance"))
				{
					sprintf(nodestate[2],"%s,chain2_balance:%f",nodestate[2], bcr3_chain2_state["balance"].GetDouble());
				}
			}
		}
	}

	if((fd = fopen("stateOfBCR3Chain3", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateOfBCR3Chain3 Error");
	}
	else
	{
		memset( json, 0, sizeof(json));
		fread(json,sizeof(char),sizeof(json),fd);
		fclose(fd);
		Document bcr3_chain3;
		if(!bcr3_chain3.Parse(json).HasParseError())
		{
			Value& bcr3_chain3_state = bcr3_chain3["result"];
			//assert(bcr3_chain3_state.IsObject());

            if(bcr3_chain3_state.IsObject())
            {
				if(bcr3_chain3_state.HasMember("balance"))
				{
					sprintf(nodestate[2],"%s,chain3_balance:%f",nodestate[2], bcr3_chain3_state["balance"].GetDouble());
				}
            }
		}
	}
}


void CUpLoad::getStateInfo()
{
	FILE *fd = NULL;

    //需要上報的狀態採集
    if(system("./getinfo.sh") == -1)
	{
		BN_LOG_ERROR("./getinfo.sh, fork error!");
	}

    //內存狀態數據處理
    getMemStateInfo();
    //CPU狀態數據處理
    getCpuStateInfo();
	//磁盤狀態數據處理
    getDiskStateInfo();
    //數據庫狀態數據處理
    getDbStateInfo();
    //Ceph狀態數據處理
    getCephStateInfo();
    //NODE狀態數據處理
    getNodeStateInfo();

	//删除中间文件
	if(system("rm stateOf*") == -1)
	{
		BN_LOG_ERROR("rm stateOf*, fork error!");
	}
}

void CUpLoad::write2StateForBn()
{
	FILE *fd;
	int i;
	if(NULL == (fd = fopen("stateForBn","w+")))
	{
		BN_LOG_ERROR("Open stateForBn Error");
	}
	for(i=0; i<m_bnNum; i++)
	{
		switch(i)
		{
			//BSDR狀態寫入
			case 0:
			{
				fprintf(fd, "%s\n","Name BSDR1");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","Ceph",cephstate);
				fprintf(fd, "%s %s\n","DB",dbdata[i]);
			}break;
			case 1:
			{
				fprintf(fd, "%s\n","Name BSDR2");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","Ceph",cephstate);
				fprintf(fd, "%s %s\n","DB",dbdata[i]);
			}break;
			case 2:
			{
				fprintf(fd, "%s\n","Name BSDR3");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","Ceph",cephstate);
				fprintf(fd, "%s %s\n","DB",dbdata[i]);
			}break;
			case 3:
			{
				fprintf(fd, "%s\n","Name BSDR4");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","Ceph",cephstate);
				fprintf(fd, "%s %s\n","DB",dbdata[i]);
			}break;
			case 4:
			{
				fprintf(fd, "%s\n","Name BSDR5");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","Ceph",cephstate);
				fprintf(fd, "%s %s\n","DB",dbdata[i]);
			}break;
			//BCR狀態寫入
			case 5:
			{
				fprintf(fd, "%s\n","Name BCR1");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","NODE",nodestate[i-m_bsdrNum]);
			}break;
			case 6:
			{
				fprintf(fd, "%s\n","Name BCR2");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","NODE",nodestate[i-m_bsdrNum]);
			}break;
			case 7:
			{
				fprintf(fd, "%s\n","Name BCR3");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
				fprintf(fd, "%s %s\n","NODE",nodestate[i-m_bsdrNum]);
			}break;
			//FBAAR狀態寫入
			case 8:
			{
				fprintf(fd, "%s\n","Name FBAAR");
				fprintf(fd, "%s %s\n","CPU",cpustate[i]);
				fprintf(fd, "%s %s\n","MEM",memstate[i]);
				fprintf(fd, "%s %s\n","DISK",diskstate[i]);
			}break;
		}
	}
	fclose(fd);
}


void CUpLoad::transJsonFormat()
{
	const char* json = "{\"time\":0,\"state\":[{\"Name\":\"BSDR1\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"Ceph\":\"xxx\",\"DB\":\"xxx\"},"
	    		           "{\"Name\":\"BSDR2\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"Ceph\":\"xxx\",\"DB\":\"xxx\"},"
	    		           "{\"Name\":\"BSDR3\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"Ceph\":\"xxx\",\"DB\":\"xxx\"},"
	    		           "{\"Name\":\"BSDR4\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"Ceph\":\"xxx\",\"DB\":\"xxx\"},"
	    		           "{\"Name\":\"BSDR5\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"Ceph\":\"xxx\",\"DB\":\"xxx\"},"
	                       "{\"Name\":\"BCR1\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"NODE\":\"xxx\"},"
	    		           "{\"Name\":\"BCR2\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"NODE\":\"xxx\"},"
	    		           "{\"Name\":\"BCR3\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\",\"NODE\":\"xxx\"},"
	    		           "{\"Name\":\"FBAAR\",\"CPU\":\"xxx\",\"MEM\":\"xxx\",\"DISK\":\"xxx\"}]}";

	char buff[COLUMN];
	char temp[COLUMN];
	char *pLine = NULL;
	FILE* fp = NULL;
	FILE* fd = NULL;
	// 1. 把 JSON 解析至 DOM。
	Document d;
	d.Parse(json);

	// 2. 利用 DOM 修改数据。
	time_t sytime;
	sytime = time(NULL);
	d["time"] = sytime;

	Value& s = d["state"];
	memset(buff, 0, COLUMN);
	memset(temp, 0, COLUMN);
	if((fp = fopen("stateForBn", "r")) == NULL)
	{
		BN_LOG_ERROR("Open stateForBn Error");
	}
	fgets(buff, sizeof(buff), fp);
	if(s.IsArray())
	{
		//更新5台BSDR的状态指标
		for (SizeType i = 0; i < m_bsdrNum; i++)
		{
			Value & v = s[i];
			Document::AllocatorType& allocator = d.GetAllocator();//
			//assert(v.IsObject());
			if(v.IsObject())
			{
				if(v.HasMember("CPU")&&v["CPU"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["CPU"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("MEM")&&v["MEM"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["MEM"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("DISK")&&v["DISK"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["DISK"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("Ceph")&&v["Ceph"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["Ceph"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("DB")&&v["DB"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["DB"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				fgets(buff, sizeof(buff), fp);
			}
		}

		//更新3台BCR的状态指标
		for (SizeType i = m_bsdrNum; i < m_bsdrNum+m_bcrNum; i++)
		{
			Value & v = s[i];
			Document::AllocatorType& allocator = d.GetAllocator();
			//assert(v.IsObject());
			if(v.IsObject())
			{
				if(v.HasMember("CPU")&&v["CPU"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["CPU"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("MEM")&&v["MEM"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["MEM"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("DISK")&&v["DISK"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["DISK"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("NODE")&&v["NODE"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["NODE"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				fgets(buff, sizeof(buff), fp);
			}
		}

		//更新FBAAR的状态指标
		for (SizeType i = m_bsdrNum+m_bcrNum; i < m_bnNum; i++)
		{
			Value & v = s[i];
			Document::AllocatorType& allocator = d.GetAllocator();
			//assert(v.IsObject());
			if(v.IsObject())
			{
				if(v.HasMember("CPU")&&v["CPU"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["CPU"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("MEM")&&v["MEM"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["MEM"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				if(v.HasMember("DISK")&&v["DISK"].IsString())
				{
					fgets(buff, sizeof(buff), fp);
					sscanf(buff, "%*s %s", temp);
					v["DISK"].SetString(temp, strlen(temp), allocator);
				}
				memset(temp, 0, COLUMN);
				fgets(buff, sizeof(buff), fp);
			}
		}
	}
	fclose(fp);

	// 3. 把 DOM 转换成 JSON。
	StringBuffer buffer;
	PrettyWriter<StringBuffer> writer(buffer);
	d.Accept(writer);
	if(NULL == (fd = fopen("upload.json","w")))
	{
		BN_LOG_ERROR("Open upload.json Error");
	}
	fwrite(buffer.GetString(),strlen(buffer.GetString()),1,fd);
	fclose(fd);
}




