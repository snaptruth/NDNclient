#ifndef YL_BAAR_Upload
#define YL_BAAR_Upload


#include <thread>
#include <mutex>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <cmath>
#include <curl/curl.h>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <cstdio>
#include "tinyxml/tinyxml.h"

using namespace std;
using namespace rapidjson;









enum UPLOAD_CODE
{
	  UPLOAD_OK                       = 11000,
	  UPLOAD_JSON_PARSE_FAIL          = 11001,
	  UPLOAD_SET_ADDRESS_FAIL         = 11002,
	  UPLOAD_CURL_ENV_INIT_FAIL       = 10411,
};



class CUpLoad
{
public :
	  static CUpLoad* getInstance();

	  bool    initialize();
	  bool    run();
	  void    unInitialize();
	  bool setUploadAddress(const char* jasonParam, string& strRet);
	  CUpLoad();
	  ~CUpLoad();
private:
	  void  getStateInfo();
	  char* judgeStateInfo(char* data);
	  void  getCephStateInfo();
	  void  getMemStateInfo();
	  void  getCpuStateInfo();
	  void  getDiskStateInfo();
	  void  getDbStateInfo();
	  void  getNodeStateInfo();
	  void  write2StateForBn();
	  void  transJsonFormat();
	  bool parseJasonParam(const char*jasonParam, string& strUrl);
	  void  upLoad();
	  void  doHttpPost();
	  string  getAddress();
	  bool  setAddress(string& str);
	  bool packJsonResult(string& strRet);
private:
	  string m_stateServiceAddress;
	  string m_diskRoute;
//	  string m_Bsdr1System;
//	  string m_Bsdr2System;
//	  string m_Bsdr3System;
//	  string m_Bsdr4System;
//	  string m_Bsdr5System;
//	  string m_Bcr1System;
//	  string m_Bcr2System;
//	  string m_Bcr3System;
//	  string m_BaarSystem;
	  int m_bnNum;
	  int m_baarNum;
	  int m_bsdrNum;
	  int m_bcrNum;
	  CURL*   m_pCurl;
	  unsigned int m_errCode;
	  std::thread *m_pthread;
	  bool    m_shouldResume;
	  static CUpLoad* m_pUpload;
	  std::mutex m_addressMtx;
	  std::string m_strTid;
};



#endif
