
#include "producer.h"

#include <thread>
#include "bnlogif.h"
#include "algo/aes_algo.h" 

//begin cipher by liulijin add
#include "config.h"
#include <vector>
#include <algorithm>
//end cipher by liulijin add


static const size_t NDN_MAX_PACKET_SIZE = 7000;
static const size_t DATA_EMPTY_ITEM_SIZE = 300;
static const int CONNECTION_RETRY_TIMER = 1;

#if 1
std::string to_string(size_t n) 
{
    std::ostringstream s;
    s << n;
    return s.str();
}
#endif

Producer::Producer(const string& macaddr, const string& bsdrmarket, int thrnum) :
    m_pface(nullptr),
    m_macaddr(macaddr),
    m_bsdrmarket(bsdrmarket),
    m_thrnum(thrnum),
    m_seq(0),
    plistenThr(NULL),
    m_pSending(NULL),
    m_pSending_tail(NULL),
    m_pCache(NULL)
{
}

Producer::~Producer()
{
    stop();
}

string Producer::getBsdrMarket()
{
    return m_bsdrmarket;
}

int Producer::getThrNum()
{
    return m_thrnum;
}

bool Producer::run()
{
    plistenThr = new std::thread(&Producer::doRun, this);

    if(NULL == plistenThr)
    {
        return false;
    }

    return true;
}

void Producer::doRun()
{
    bool shouldresume = true;

    std::ostringstream s;
    s << m_thrnum;

    string strName = "/yl/broadcast/baar/producer/" + m_macaddr + "/" 
                   + m_bsdrmarket + "/" + s.str();

    do
    {
        try
        {
            m_pface = std::unique_ptr<Face>(new Face);
            if(nullptr == m_pface)
            {
                continue;
            }

            // if(m_listenId != NULL)
            // {
            //     m_face.unregisterPrefix(
            //         m_listenId,
            //         []{},
            //         [](const string& failInfo){}
            //         );
                
            //     m_listenId = NULL;
            // }

            BN_LOG_INTERFACE("run producer:%s", strName.c_str());

            m_listenId = m_pface->setInterestFilter(
                strName,
                bind(&Producer::onInterest, this, _1, _2),
                RegisterPrefixSuccessCallback(),
                bind(&Producer::onRegisterFailed, this, _1, _2)
                );
            
            m_pface->processEvents();
        }
        catch (std::runtime_error& e) 
        {
            BN_LOG_ERROR("producer catch nfd error:%s", e.what());
        }
        catch(...)
        {
            BN_LOG_ERROR("producer process events catch unknow error.");
        }

        m_pface = nullptr;

        if(m_shouldresume)
        {
            sleep(CONNECTION_RETRY_TIMER);
        }

    } while(m_shouldresume);
    
    BN_LOG_INTERFACE("end service[bsdr market:%s][producer number:%d]", m_bsdrmarket.c_str(), m_thrnum);
}

void Producer::stop()
{
    // std::lock_guard<std::mutex> lock(m_resumeMutex);
    BN_LOG_INTERFACE("stop producer[bsdr market:%s][producer number:%d]", m_bsdrmarket.c_str(), m_thrnum);

    m_shouldresume = false;
    if(m_pface != nullptr)
    {
        BN_LOG_INTERFACE("stop producer service");
        m_pface->shutdown();
    }

    if(plistenThr != NULL)
    {
        plistenThr->join();
        plistenThr = NULL;
    }

    m_pface = nullptr;

    BrCache::Instance()->InsertBNLinkData(m_bsdrmarket, m_pSending);
    BrCache::Instance()->InsertBNLinkData(m_bsdrmarket, m_pCache);
}

void Producer::addItemData(string & content, BNData * pBNData)
{
    if(NULL == pBNData)
    {
        return;
    }
	std::ostringstream s;
	s << pBNData->operType;
    string strParam;
    if(NULL == pBNData->param)
    {
        strParam = "";
    }
    else
    {
        strParam = pBNData->param;
    }

    /*string strTimeStamp;
    if(NULL == pBNData->timeStamp)
    {
        strTimeStamp = "";
    }
    else
    {
        strTimeStamp = pBNData->timeStamp;
    }

    string strContent;
    if(NULL == pBNData->content)
    {
        strContent = "";
    }
    else
    {
        strContent = pBNData->content;
    }

    string strLabel;
    if(NULL == pBNData->blockLabel)
    {
        strLabel = "";
    }
    else
    {
        strLabel = pBNData->blockLabel;
    }

    string strMarket;
    if(NULL == pBNData->market)
    {
        strMarket = "";
    }
    else
    {
        strMarket = pBNData->market;
    }

    string strPrice;
    if(NULL == pBNData->price)
    {
        strPrice = "";
    }
    else
    {
        strPrice = pBNData->price;
    }*/

    content += "<Item><param>" + strParam + "</param><OperType>" + s.str() + "</OperType></Item>";

    BN_LOG_DEBUG("addItemData:%s", content.c_str());
}

void Producer::onInterest(const InterestFilter& filter, const Interest& interest)
{
    stringstream strLogInfo;
    strLogInfo << "[market:%s][producer number:" << m_thrnum << "]""receive interest:" << interest;
    BN_LOG_INTERFACE("%s", strLogInfo.str().c_str());

	Name dataName(interest.getName());

    Name::Component comConfirm = dataName.get(-1);
    std::string strConfirm = comConfirm.toUri();
    long lSeqnum;
    std::istringstream is(strConfirm);
    is >> lSeqnum;
//    printf("Jerry test: m_seq [%d], lseqnum[%d]\n", m_seq,lSeqnum);
    if(m_seq == lSeqnum)
    {
        DataNode * ptmp = NULL;
        while(m_pSending != NULL)
        {
            ptmp = m_pSending;
            m_pSending = m_pSending->next;

            delete ptmp->pdata;
            delete ptmp;
        }
    }

    

    ++m_seq;
    m_seq = m_seq % 100;
    m_seq = (m_seq <= 0) ? 1 : m_seq;

    string content("<BrData><Seq>");
    content += to_string(m_seq);
    content += "</Seq>";

    if(m_pSending != NULL)
    {
        DataNode * ptmp = m_pSending;
        while(ptmp != NULL)
        {
            addItemData(content, ptmp->pdata);
            ptmp = ptmp->next;
        }
    }
    else
    {
        if(NULL == m_pCache)
        {
            BrCache::Instance()->GetBNData(m_bsdrmarket, &m_pCache);
        }

        DataNode * pTmpDataNode = NULL;
        size_t packetsize = 0;
        while(m_pCache != NULL)
        {
            BNData * pBNData = m_pCache->pdata;
            size_t memsize = pBNData->getMemSize();
            if(NDN_MAX_PACKET_SIZE < packetsize + memsize + DATA_EMPTY_ITEM_SIZE)
            {
                break;
            }

            packetsize += memsize + DATA_EMPTY_ITEM_SIZE;

            addItemData(content, pBNData);

            pTmpDataNode = m_pCache;
            m_pCache = m_pCache->next;
            pTmpDataNode->next = NULL;
            if(NULL == m_pSending)
            {
                m_pSending = m_pSending_tail = pTmpDataNode;
            }
            else
            {
                m_pSending_tail->next = pTmpDataNode;
                m_pSending_tail = m_pSending_tail->next;
            }
        }
    }

    content += "</BrData>";

    BN_LOG_DEBUG("%s", content.c_str());

    //begin encrypt the content by mali 20170606 add
    /* Encrypt the plaintext */  
    //128bits key.
    unsigned char   key[KEY_BYTE_NUMBERS]; 
    //Init vector.
    unsigned char   iv[KEY_BYTE_NUMBERS];

    /* A 128 bit IV */  
    memcpy(key, KEY, KEY_BYTE_NUMBERS);
    memcpy(iv, INITIAL_VECTOR, KEY_BYTE_NUMBERS);
    
    int  ciphertext_len;
    int plaintext_len;

    unsigned char *plaintextbuf = (unsigned char *)content.c_str();
    unsigned char *ciphertextbuf=NULL;

    if (Config::Instance()->m_sCipherSwitch == 1)
    {
        plaintext_len = strlen((const char*)plaintextbuf);
        ciphertextbuf=(unsigned char *)malloc(MAX_TEXT_SIZE*sizeof(char));
        
        if(NULL==ciphertextbuf)
        {
            BN_LOG_ERROR("ciphertextbuf malloc failed");
            return;
        }
        try{
            ciphertext_len = encrypt(plaintextbuf, strlen((const char *)plaintextbuf), key, iv,
                ciphertextbuf);
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

    // if (Config::Instance()->m_sCipherSwitch == 1)
    // {
    // hexdump(stdout,
    //     "=======plaintextbuf=======",
    //     plaintextbuf,
    //     plaintext_len);

    // hexdump(stdout,
    //     "=======ciphertextbuf=======",
    //     ciphertextbuf,
    //     ciphertext_len);
    // }
    //end encrypt the content by mali 20170606 add

    // Create Data packet
    shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(time::seconds(0));

    //begin encrypt the content by mali 20170606 modify
    if (Config::Instance()->m_sCipherSwitch == 1)
    {
      data->setContent(reinterpret_cast<const uint8_t*>(ciphertextbuf), ciphertext_len);
      free(ciphertextbuf);
    }
    else
    {
      data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
    }
    //end encrypt the content by mali 20170606 modify

    // Sign Data packet with default identity
    m_keyChain.sign(*data);

    // Return Data packet to the requester
    // std::cout << ">> D: " << *data << std::endl;
    if(m_pface != nullptr)
    {
        m_pface->put(*data);
    }
}

void Producer::onRegisterFailed(const Name& profix, const std::string& reason)
{
    BN_LOG_ERROR("onRegisterFailed");
    if(m_pface != nullptr)
    {
        m_pface->shutdown();
    }
}


