
#include "produceregz.h"

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

ProducerEgz::ProducerEgz(const string& macaddr, const string& bsdrmarket, int thrnum) :
    m_macaddr(macaddr),
    m_bsdrmarket(bsdrmarket),
    m_thrnum(thrnum),
    m_pegzface(nullptr),
    pEgzListenThr(NULL),
    m_egzseq(0),
    m_pEgzSending(NULL),
    m_pEgzSendingTail(NULL),
    m_pEgzCathe(NULL)
{
}

ProducerEgz::~ProducerEgz()
{
    stop();
}

string ProducerEgz::getBsdrMarket()
{
    return m_bsdrmarket;
}

int ProducerEgz::getThrNum()
{
    return m_thrnum;
}

bool ProducerEgz::run()
{
    pEgzListenThr = new std::thread(&ProducerEgz::doRunEgz, this);
    if(NULL == pEgzListenThr)
    {
        return false;
    }

    return true;
}

void ProducerEgz::doRunEgz()
{
    std::ostringstream s;
    s << m_thrnum;

    string strName = "/yl/broadcast/egz/baar/producer/" + m_macaddr + "/" 
                   + m_bsdrmarket + "/" + s.str();

    do
    {
        try
        {
            m_pegzface = std::unique_ptr<Face>(new Face);
            if(nullptr == m_pegzface)
            {
                continue;
            }

            BN_LOG_INTERFACE("run egz producer:%s", strName.c_str());
            // std::cout << "run egz producer:" << strName << std::endl << std::endl ;

            m_listenId = m_pegzface->setInterestFilter(
                strName,
                bind(&ProducerEgz::onEgzInterest, this, _1, _2),
                RegisterPrefixSuccessCallback(),
                bind(&ProducerEgz::onEgzRegisterFailed, this, _1, _2)
                );
            
            m_pegzface->processEvents();
        }
        catch (std::runtime_error& e) 
        {
            BN_LOG_ERROR("egz producer catch nfd error:%s", e.what());
        }
        catch(...)
        {
            BN_LOG_ERROR("egz producer process events catch unknow error.");
        }

        m_pegzface = nullptr;

        if(m_shouldresume)
        {
            sleep(CONNECTION_RETRY_TIMER);
        }

    } while(m_shouldresume);
    
    BN_LOG_INTERFACE("end egz service[bsdr market:%s][producer number:%d]",
            m_bsdrmarket.c_str(), m_thrnum);
}

void ProducerEgz::stop()
{
    // std::lock_guard<std::mutex> lock(m_resumeMutex);
    BN_LOG_INTERFACE("producer[bsdr market:%s][producer number:%d]", m_bsdrmarket.c_str(), m_thrnum);

    m_shouldresume = false;
    if(m_pegzface != nullptr)
    {
        BN_LOG_INTERFACE("m_pegzface != nullptr]");
        m_pegzface->shutdown();
    }

    if(pEgzListenThr != NULL)
    {
        pEgzListenThr->join();
        pEgzListenThr = NULL;
    }

    m_pegzface = nullptr;

    // BrCache::Instance()->InsertBNLinkData(m_bsdrmarket, m_pSending);
    // BrCache::Instance()->InsertBNLinkData(m_bsdrmarket, m_pCache);
}

void ProducerEgz::onEgzInterest(const InterestFilter& filter, const Interest& interest)
{
    // _LOG_INFO("onEgzInterest [market:" << m_bsdrmarket << "][producer number:" << m_thrnum << "]""receive interest:" << interest);
    // std::cout << "onEgzInterest [market:" << m_bsdrmarket << "][producer number:" << m_thrnum << "]""receive interest:" << interest<< std::endl << std::endl ;

    Name dataName(interest.getName());

    Name::Component comConfirm = dataName.get(-1);
    std::string strConfirm = comConfirm.toUri();
    long lSeqnum;
    std::istringstream is(strConfirm);
    is >> lSeqnum;

    if(m_egzseq == lSeqnum)
    {
        EGZDataNode * ptmp = NULL;
        while(m_pEgzSending != NULL)
        {
            ptmp = m_pEgzSending;
            m_pEgzSending = m_pEgzSending->next;

            delete ptmp->pdata;
            delete ptmp;
        }
    } 

    ++m_egzseq;
    m_egzseq = m_egzseq % 100;
    m_egzseq = (m_egzseq <= 0) ? 1 : m_egzseq;

    std::ostringstream s;
    s << m_egzseq;

    string content("<BrData><Seq>");
    content += s.str();
    content += "</Seq>";

    if(m_pEgzSending != NULL)
    {
        EGZDataNode * ptmp = m_pEgzSending;
        while(ptmp != NULL)
        {
            content = content + ptmp->pdata->toItemString();
            ptmp = ptmp->next;
        }
    }
    else
    {
        if(NULL == m_pEgzCathe)
        {
            BrCache::Instance()->GetEgzData(m_bsdrmarket, &m_pEgzCathe);
        }

        EGZDataNode * pTmpDataNode = NULL;
        size_t packetsize = 0;
        while(m_pEgzCathe != NULL)
        {
            EGZData * pEgzData = m_pEgzCathe->pdata;
            size_t memsize = pEgzData->getMemSize();
            if(NDN_MAX_PACKET_SIZE < packetsize + memsize + DATA_EMPTY_ITEM_SIZE)
            {
                break;
            }

            packetsize += memsize + DATA_EMPTY_ITEM_SIZE;

            content = content + pEgzData->toItemString();

            pTmpDataNode = m_pEgzCathe;
            m_pEgzCathe = m_pEgzCathe->next;
            pTmpDataNode->next = NULL;
            if(NULL == m_pEgzSending)
            {
                m_pEgzSending = m_pEgzSendingTail = pTmpDataNode;
            }
            else
            {
                m_pEgzSendingTail->next = pTmpDataNode;
                m_pEgzSendingTail = m_pEgzSendingTail->next;
            }
        }
    }

    content += "</BrData>";

    // _LOG_INFO(content);

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
    m_egzkeyChain.sign(*data);

    // Return Data packet to the requester
    // std::cout << ">> D: " << *data << std::endl;
    if(m_pegzface != nullptr)
    {
        m_pegzface->put(*data);
    }
}

void ProducerEgz::onEgzRegisterFailed(const Name& profix, const std::string& reason)
{
    BN_LOG_ERROR("onEgzRegisterFailed");
    if(m_pegzface != nullptr)
    {
        m_pegzface->shutdown();
    }
}


