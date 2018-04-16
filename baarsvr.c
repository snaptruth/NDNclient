#include "soapH.h"
#include "baarsvr.nsmap"	/* namespaces updated 4/4/13 */
#include "options.h"
#ifdef _WITH_TEST
#include "httpget.h"
#endif
#include "logging.h"
#include "threads.h"
#include <signal.h>	/* need SIGPIPE */

#include "producer.h"
#include "brcache.h"
#include "config.h"
#include "consumer.h"
#include "producermgr.h"
#include "consumermgr.h"
#include "brnamepubliser.h"
#include "bnlogif.h"
#include "upload.h"

#define BACKLOG (100)

#ifdef _WITH_TEST
#define AUTH_REALM "gSOAP Web Server Admin demo login: admin guest"
#define AUTH_USERID "admin" /* user ID to access admin pages */
#define AUTH_PASSWD "guest" /* user pw to access admin pages */
#endif

/******************************************************************************\
 *
 *	Thread pool and request queue
 *
 \******************************************************************************/

#define MAX_THR (100)
#define MAX_QUEUE (1000)

static int poolsize = 0;

static int queue[MAX_QUEUE];
static int head = 0, tail = 0;

static MUTEX_TYPE queue_cs;
static COND_TYPE queue_cv;

static const string XML_ITEMNAME_DEVNO = "SrcDeviceNo";
static const string XML_ITEMNAME_TIMESTAMP = "TimeStamp";
static const string XML_ITEMNAME_CONTENT = "Content";
static const string XML_ITEMNAME_BLOCKLABEL = "BlockedLabel";
static const string XML_ITEMNAME_MARKET = "Market";
static const string XML_ITEMNAME_PRICE = "Price";

static const int MSG_LEN_RETURN = 128;

/******************************************************************************\
 *
 *	Program options
 *
 \******************************************************************************/

/* The options.c module parses command line options, HTTP form options, and
 options stored in a config file. Options are defined as follows:
 */

static const struct option default_options[] = { /* "key.fullname", "unit or selection list", text field width, "default value"  */
{ "z.compress", NULL, }, { "c.chunking", NULL, }, { "k.keepalive", "0 1", 1}, {
		"i.iterative", NULL, }, { "v.verbose", NULL, }, { "o.pool", "threads",
		6, (char*) "100" }, { "t.ioTimeout", "seconds", 6, (char*) "5" }, {
		"s.serverTimeout", "seconds", 6, (char*) "0" }, { "d.cookieDomain",
		"host", 20, (char*) "127.0.0.1" }, { "p.cookiePath", "path", 20,
		(char*) "/" }, { "l.logging", "none inbound outbound both", }, { "",
		"port", }, /* takes the rest of command line args */
{ NULL }, /* must be NULL terminated */
};

/* The numbering of these defines must correspond to the option order above */
#define OPTION_z	0
#define OPTION_c	1
#define OPTION_k	2
#define OPTION_i	3
#define OPTION_v	4
#define OPTION_o	5
#define OPTION_t	6
#define OPTION_s	7
#define OPTION_d	8
#define OPTION_p	9
#define OPTION_l	10
#define OPTION_port	11

/******************************************************************************\
 *
 *	Static
 *
 \******************************************************************************/

static struct option *options = NULL;
static time_t start;
static int secure = 0; /* =0: no SSL, =1: support SSL */

/******************************************************************************\
 *
 *	Forward decls
 *
 \******************************************************************************/

void server_loop(struct soap*);
void *process_request(void*); /* multi-threaded request handler */
void *process_queue(void*); /* multi-threaded request handler for pool */
int enqueue(SOAP_SOCKET);
SOAP_SOCKET dequeue();
int http_GET_handler(struct soap*); /* HTTP httpget plugin GET handler */
int copy_file(struct soap*, const char*, const char*); /* copy file as HTTP response */
int check_authentication(struct soap*); /* HTTP authentication check */
void sigpipe_handle(int); /* SIGPIPE handler: Unix/Linux only */
void sigsegv_handle(int);
int CRYPTO_thread_setup();
void CRYPTO_thread_cleanup();

bool format_label(char * label);

/******************************************************************************\
 *
 *	Main
 *
 \******************************************************************************/

int main(int argc, char **argv) {
    InitBnLog("BAAR", BN_LOG_MODULE_BAAR);

    BN_LOG_DEBUG("baarsvr start...");

    BN_CHECK_CONDITION_RETUNKNOWN(Config::Instance()->Init(), "Init config error.");

    BN_CHECK_CONDITION_RETUNKNOWN(ProducerMgr::Instance()->run(), "run producer manager error.");

    BN_CHECK_CONDITION_RETUNKNOWN(ConsumerMgr::Instance()->MakeCSMHanderPool(), "make consumer pool error.");


	BN_CHECK_CONDITION_RETUNKNOWN(CUpLoad::getInstance()->initialize(), "Init upload error.");

	BN_CHECK_CONDITION_RETUNKNOWN(CUpLoad::getInstance()->run(), "run upload error.");
	// if(!ConsumerMgr::Instance()->MakeCSMPool())
	// {
	// 	_LOG_ERROR("consumer manager make consumer pool error.");
	// 	return -1;
	// }

	BrNamePubliser namepub;
	BN_CHECK_CONDITION_RETUNKNOWN(namepub.run(), "run name publisher fail.");

	BN_LOG_DEBUG("run forward success.");

	struct soap soap;
	SOAP_SOCKET master;
	int port = 0;

	start = ::time(NULL);

	options = copy_options(default_options); /* must copy, so option values can be modified */
	if (parse_options(argc, argv, options))
		exit(0);

	if (options[OPTION_port].value)
		port = atol(options[OPTION_port].value);
	if (!port)
		port = 8080;

	BN_LOG_DEBUG("Starting Web server on port %d" , port);

	secure = 1;

	/* Init SSL (can skip or call multiple times, engien inits automatically) */
       #if defined(WITH_OPENSSL) 
	soap_ssl_init();
	/* soap_ssl_noinit(); call this first if SSL is initialized elsewhere */
       #endif
	soap_init2(&soap, SOAP_IO_KEEPALIVE, SOAP_IO_DEFAULT);
	soap_set_mode(&soap, SOAP_C_UTFSTRING);

	/* set up lSSL ocks */
	BN_CHECK_CONDITION_RETUNKNOWN(SOAP_OK == CRYPTO_thread_setup(), "Cannot setup thread mutex");

#if defined(WITH_OPENSSL) || defined(WITH_GNUTLS)
	/* The supplied server certificate "server.pem" assumes that the server is
	 running on 'localhost', so clients can only connect from the same host when
	 verifying the server's certificate.
	 To verify the certificates of third-party services, they must provide a
	 certificate issued by Verisign or another trusted CA. At the client-side,
	 the capath parameter should point to a directory that contains these
	 trusted (root) certificates or the cafile parameter should refer to one
	 file will all certificates. To help you out, the supplied "cacerts.pem"
	 file contains the certificates issued by various CAs. You should use this
	 file for the cafile parameter instead of "cacert.pem" to connect to trusted
	 servers. Note that the client may fail to connect if the server's
	 credentials have problems (e.g. expired).
	 Note 1: the password and capath are not used with GNUTLS
	 Note 2: setting capath may not work on Windows.
	 */
	if (secure && soap_ssl_server_context(&soap,
					SOAP_SSL_DEFAULT,
					"server.pem", /* keyfile: see SSL docs on how to obtain this file */
					"yl123", /* password to read the key file */
					NULL, /* cacert CA certificate, or ... */
					NULL, /* capath CA certificate path */
					NULL, /* DH file (e.g. "dh2048.pem") or numeric DH param key len bits in string (e.g. "2048"), if NULL then use RSA */
					NULL, /* if randfile!=NULL: use a file with random data to seed randomness */
					"baarsvr" /* server identification for SSL session cache (must be a unique name) */
			))
	{
		soap_print_fault(&soap, stderr);
		exit(1);
	}
#endif

#ifdef _WITH_TEST
	/* Register HTTP GET plugin */
	if (soap_register_plugin_arg(&soap, http_get, (void*)http_GET_handler))
	soap_print_fault(&soap, stderr);
#endif

	/* Register logging plugin */
	if (soap_register_plugin(&soap, logging))
		soap_print_fault(&soap, stderr);

	/* Unix SIGPIPE, this is OS dependent (win does not need this) */
	/* soap.accept_flags = SO_NOSIGPIPE; *//* some systems like this */
	/* soap.socket_flags = MSG_NOSIGNAL; *//* others need this */
	signal(SIGPIPE, sigpipe_handle); /* and some older Unix systems may require a sigpipe handler */
	// signal(SIGSEGV, sigsegv_handle);
	soap.bind_flags = SO_REUSEADDR;
	master = soap_bind(&soap, NULL, port, BACKLOG);
	if (!soap_valid_socket(master)) {
		soap_print_fault(&soap, stderr);
		exit(1);
	}
	BN_LOG_DEBUG("Port bind successful: master socket = %d", master);
	MUTEX_SETUP(queue_cs);
	COND_SETUP(queue_cv);
	server_loop(&soap);
	COND_CLEANUP(queue_cv);
	MUTEX_CLEANUP(queue_cs);
	free_options(options);
	soap_end(&soap);
	soap_done(&soap);
	CRYPTO_thread_cleanup();
	CUpLoad::getInstance()->unInitialize();

	THREAD_EXIT;
	return 0;
}

void server_loop(struct soap *soap) {
	struct soap *soap_thr[MAX_THR];
	THREAD_TYPE tid, tids[MAX_THR];
	int req;

	for (req = 1;; req++) {
		SOAP_SOCKET sock;
		int newpoolsize;

		soap->cookie_domain = options[OPTION_d].value; /* set domain of this server */
		soap->cookie_path = options[OPTION_p].value; /* set root path of the cookies */
		soap_set_cookie(soap, "visit", "true", NULL, NULL); /* use global domain/path */
		soap_set_cookie_expire(soap, "visit", 600, NULL, NULL); /* max-age is 600 seconds to expire */

		if (options[OPTION_c].selected)
			soap_set_omode(soap, SOAP_IO_CHUNK); /* use chunked HTTP content (fast) */
		if (options[OPTION_k].selected)
			soap_set_omode(soap, SOAP_IO_KEEPALIVE);
		if (options[OPTION_t].value)
			soap->send_timeout = soap->recv_timeout = atol(
					options[OPTION_t].value);
		if (options[OPTION_s].value)
			soap->accept_timeout = atol(options[OPTION_s].value);
		if (options[OPTION_l].selected == 1 || options[OPTION_l].selected == 3)
			soap_set_logging_inbound(soap, stdout);
		else
			soap_set_logging_inbound(soap, NULL);
		if (options[OPTION_l].selected == 2 || options[OPTION_l].selected == 3)
			soap_set_logging_outbound(soap, stdout);
		else
			soap_set_logging_outbound(soap, NULL);

		newpoolsize = atol(options[OPTION_o].value);

		if (newpoolsize < 0)
			newpoolsize = 0;
		else if (newpoolsize > MAX_THR)
			newpoolsize = MAX_THR;

		if (poolsize > newpoolsize) {
			int job;

			for (job = 0; job < poolsize; job++) {
				while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
					sleep(1);
			}

			for (job = 0; job < poolsize; job++) {
			    BN_LOG_DEBUG("Waiting for thread %d to terminate...", job);
				THREAD_JOIN(tids[job]);
				BN_LOG_DEBUG("Thread %d has stopped", job);
				soap_done(soap_thr[job]);
				free(soap_thr[job]);
			}

			poolsize = 0;
		}

		if (poolsize < newpoolsize) {
			int job;

			for (job = poolsize; job < newpoolsize; job++) {
				soap_thr[job] = soap_copy(soap);
				if (!soap_thr[job])
					break;

				soap_thr[job]->user = (void*) (long) job; /* int to ptr */

				BN_LOG_DEBUG("Starting thread %d", job);
				THREAD_CREATE(&tids[job], (void*(*)(void*) )process_queue,
						(void* )soap_thr[job]);
			}

			poolsize = job;
		}

		/* to control the close behavior and wait states, use the following:
		 soap->accept_flags |= SO_LINGER;
		 soap->linger_time = 60;
		 */
		/* the time resolution of linger_time is system dependent, though according
		 * to some experts only zero and nonzero values matter.
		 */

		sock = soap_accept(soap);
		if (!soap_valid_socket(sock)) {
			if (soap->errnum) {
				soap_print_fault(soap, stderr);
				BN_LOG_ERROR("soap_accept failed, Retry...");
				continue;
			}
			BN_LOG_ERROR("gSOAP Web server timed out");
			break;
		}

		if (options[OPTION_v].selected) {
		    BN_LOG_DEBUG("Request #%d accepted on socket %d connected from IP %d.%d.%d.%d",
		        req, sock,
		        (int)(soap->ip>>24)&0xFF, (int)(soap->ip>>16)&0xFF,
		        (int)(soap->ip>> 8)&0xFF, (int)soap->ip&0xFF);
		}

		if (poolsize > 0) {
			while (enqueue(sock) == SOAP_EOM)
				sleep(1);
		} else {
			struct soap *tsoap = NULL;

			if (!options[OPTION_i].selected)
				tsoap = soap_copy(soap);

			if (tsoap) {
				tsoap->user = (void*) (long) req;
				THREAD_CREATE(&tid, (void*(*)(void*) )process_request,
						(void* )tsoap);
			} else {
#if defined(WITH_OPENSSL) || defined(WITH_GNUTLS)
				if (secure && soap_ssl_accept(soap))
				{
					soap_print_fault(soap, stderr);
					BN_LOG_ERROR("SSL request failed, continue with next call...");
					soap_end(soap);
					continue;
				}
#endif
				/* Keep-alive: frequent EOF faults occur related to KA-timeouts:
				 timeout: soap->error == SOAP_EOF && soap->errnum == 0
				 error:   soap->error != SOAP_OK
				 */
				if (soap_serve(soap)
						&& (soap->error != SOAP_EOF
								|| (soap->errnum != 0
										&& !(soap->omode & SOAP_IO_KEEPALIVE)))) {
				    BN_LOG_ERROR("Request #%d completed with failure 0x%x", req, soap->error);
					soap_print_fault(soap, stderr);
				} else if (options[OPTION_v].selected) {
				    BN_LOG_ERROR("Request #%d completed", req);
				}
				soap_destroy(soap);
				soap_end(soap);
			}
		}
	}

	if (poolsize > 0) {
		int job;

		for (job = 0; job < poolsize; job++) {
			while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
				sleep(1);
		}

		for (job = 0; job < poolsize; job++) {
			BN_LOG_DEBUG("Waiting for thread %d to terminate... ", job);
			THREAD_JOIN(tids[job]);
			BN_LOG_DEBUG("terminated");
			soap_free(soap_thr[job]);
		}
	}
}

/******************************************************************************\
 *
 *	Process dispatcher
 *
 \******************************************************************************/

void *process_request(void *soap) {
	struct soap *tsoap = (struct soap*) soap;

	THREAD_DETACH(THREAD_ID);

#if defined(WITH_OPENSSL) || defined(WITH_GNUTLS)
	if (secure && soap_ssl_accept(tsoap))
	{
		soap_print_fault(tsoap, stderr);
		BN_LOG_ERROR("SSL request failed, continue with next call...");
		soap_destroy(tsoap);
		soap_end(tsoap);
		soap_done(tsoap);
		free(tsoap);
		return NULL;
	}
#endif

	if (soap_serve(tsoap)
			&& (tsoap->error != SOAP_EOF
					|| (tsoap->errnum != 0
							&& !(tsoap->omode & SOAP_IO_KEEPALIVE)))) {
	    BN_LOG_ERROR("Thread %d completed with failure %d", (int)(long)tsoap->user, tsoap->error);
		soap_print_fault(tsoap, stderr);
	} else if (options[OPTION_v].selected) {
	    BN_LOG_DEBUG("Thread %d completed", (int)(long)tsoap->user);
	}

	soap_destroy(tsoap);
	soap_end(tsoap);
	soap_done(tsoap);
	free(soap);

	return NULL;
}

/******************************************************************************\
 *
 *	Thread pool (enabled with option -o<num>)
 *
 \******************************************************************************/

void *process_queue(void *soap) {
	struct soap *tsoap = (struct soap*) soap;

	for (;;) {
		tsoap->socket = dequeue();
		if (!soap_valid_socket(tsoap->socket)) {
			if (options[OPTION_v].selected) {
			    BN_LOG_DEBUG("Thread %d terminating", (int)(long)tsoap->user);
			}
			break;
		}

#if defined(WITH_OPENSSL) || defined(WITH_GNUTLS)
		if (secure && soap_ssl_accept(tsoap))
		{
			soap_print_fault(tsoap, stderr);
			BN_LOG_ERROR("SSL request failed, continue with next call...");
			soap_destroy(tsoap);
			soap_end(tsoap);
			continue;
		}
#endif

		if (options[OPTION_v].selected) {
		    BN_LOG_DEBUG("Thread %d accepted a request", (int)(long)tsoap->user);
		}
		if (soap_serve(tsoap)
				&& (tsoap->error != SOAP_EOF
						|| (tsoap->errnum != 0
								&& !(tsoap->omode & SOAP_IO_KEEPALIVE)))) {
		    BN_LOG_ERROR("Thread %d finished serving request with failure %d",
		            (int)(long)tsoap->user, tsoap->error);
			soap_print_fault(tsoap, stderr);
		} else if (options[OPTION_v].selected) {
		    BN_LOG_DEBUG("Thread %d finished serving request", (int)(long)tsoap->user);
		}
		soap_destroy(tsoap);
		soap_end(tsoap);
	}

	soap_destroy(tsoap);
	soap_end(tsoap);
	soap_done(tsoap);

	return NULL;
}

int enqueue(SOAP_SOCKET sock) {
	int status = SOAP_OK;
	int next;
	int ret;

	if ((ret = MUTEX_LOCK(queue_cs))) {
	    BN_LOG_ERROR("MUTEX_LOCK error %d", ret);
	}

	next = (tail + 1) % MAX_QUEUE;
	if (head == next) {
		/* don't block on full queue, return SOAP_EOM */
		status = SOAP_EOM;
	} else {
		queue[tail] = sock;
		tail = next;

		if (options[OPTION_v].selected) {
		    BN_LOG_DEBUG("enqueue(%d)", sock);
		}

		if ((ret = COND_SIGNAL(queue_cv))) {
		    BN_LOG_DEBUG("COND_SIGNAL error %d", ret);
		}
	}

	if ((ret = MUTEX_UNLOCK(queue_cs))) {
	    BN_LOG_ERROR("MUTEX_UNLOCK error %d", ret);
	}

	return status;
}

SOAP_SOCKET dequeue() {
	SOAP_SOCKET sock;
	int ret;

	if ((ret = MUTEX_LOCK(queue_cs))) {
	    BN_LOG_ERROR("MUTEX_UNLOCK error %d", ret);
	}

	while (head == tail)
		if ((ret = COND_WAIT(queue_cv, queue_cs))) {
		    BN_LOG_ERROR("COND_WAIT error %d", ret);
		}

	sock = queue[head];

	head = (head + 1) % MAX_QUEUE;

	if (options[OPTION_v].selected) {
	    BN_LOG_DEBUG("dequeue(%d)", sock);
	}

	if ((ret = MUTEX_UNLOCK(queue_cs))) {
	    BN_LOG_DEBUG("MUTEX_UNLOCK error %d", ret);
	}

	return sock;
}

#ifdef _WITH_TEST

/******************************************************************************\
 *
 *  HTTP GET handler for httpget plugin
 *
 \******************************************************************************/

int http_GET_handler(struct soap *soap)
{
	/* gSOAP >=2.5 soap_response() will do this automatically for us, when sending SOAP_HTML or SOAP_FILE:
	 if ((soap->omode & SOAP_IO) != SOAP_IO_CHUNK)
	 soap_set_omode(soap, SOAP_IO_STORE); *//* if not chunking we MUST buffer entire content when returning HTML pages to determine content length */
#ifdef WITH_ZLIB
	if (options[OPTION_z].selected && soap->zlib_out == SOAP_ZLIB_GZIP) /* client accepts gzip */
	{	soap_set_omode(soap, SOAP_ENC_ZLIB); /* so we can compress content (gzip) */
		soap->z_level = 9; /* best compression */
	}
	else
	soap_clr_omode(soap, SOAP_ENC_ZLIB); /* so we can compress content (gzip) */
#endif
	/* Use soap->path (from request URL) to determine request: */
	if (options[OPTION_v].selected)
	fprintf(stderr, "HTTP GET Request '%s' to host '%s' path '%s'\n", soap->endpoint, soap->host, soap->path);
	/* we don't like request to snoop around in dirs, so reject when path has a '/' or a '\' or you must at least check for .. to avoid request from snooping around in higher dirs! */
	/* Note: soap->path always starts with '/' so we chech path + 1 */
	if (strchr(soap->path + 1, '/') || strchr(soap->path + 1, '\\'))
	return 403; /* HTTP forbidden */
	if (!soap_tag_cmp(soap->path, "*.html"))
	return copy_file(soap, soap->path + 1, "text/html");
	if (!soap_tag_cmp(soap->path, "*.xml"))
	return copy_file(soap, soap->path + 1, "text/xml");
	if (!soap_tag_cmp(soap->path, "*.jpg"))
	return copy_file(soap, soap->path + 1, "image/jpeg");
	if (!soap_tag_cmp(soap->path, "*.gif"))
	return copy_file(soap, soap->path + 1, "image/gif");
	if (!soap_tag_cmp(soap->path, "*.png"))
	return copy_file(soap, soap->path + 1, "image/png");
	if (!soap_tag_cmp(soap->path, "*.ico"))
	return copy_file(soap, soap->path + 1, "image/ico");
	if (!strncmp(soap->path, "/genivia", 8))
	{	(SOAP_SNPRINTF(soap->endpoint, sizeof(soap->endpoint), strlen(soap->path) + 10), "http://genivia.com%s", soap->path + 8); /* redirect */
		return 307; /* Temporary Redirect */
	}
	/* Check requestor's authentication: */
	if (check_authentication(soap))
	return 401; /* HTTP not authorized */
	/* For example, we can put WSDL and XSD files behind authentication wall */
	if (!soap_tag_cmp(soap->path, "*.xsd")
			|| !soap_tag_cmp(soap->path, "*.wsdl"))
	return copy_file(soap, soap->path + 1, "text/xml");
	return 404; /* HTTP not found */
}

int check_authentication(struct soap *soap)
{	if (soap->userid && soap->passwd)
	{	if (!strcmp(soap->userid, AUTH_USERID) && !strcmp(soap->passwd, AUTH_PASSWD))
		return SOAP_OK;
	}
#ifdef HTTPDA_H
	else if (soap->authrealm && soap->userid)
	{	if (!strcmp(soap->authrealm, AUTH_REALM) && !strcmp(soap->userid, AUTH_USERID))
		if (!http_da_verify_get(soap, (char*)AUTH_PASSWD))
		return SOAP_OK;
	}
#endif
	soap->authrealm = AUTH_REALM;
	return 401;
}

/******************************************************************************\
 *
 *  Copy static page
 *
 \******************************************************************************/

int copy_file(struct soap *soap, const char *name, const char *type)
{	FILE *fd;
	size_t r;
	fd = fopen(name, "rb"); /* open file to copy */
	if (!fd)
	return 404; /* return HTTP not found */
	soap->http_content = type;
	if (soap_response(soap, SOAP_FILE)) /* OK HTTP response header */
	{	soap_end_send(soap);
		fclose(fd);
		return soap->error;
	}
	for (;;)
	{	r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
		if (!r)
		break;
		if (soap_send_raw(soap, soap->tmpbuf, r))
		{	soap_end_send(soap);
			fclose(fd);
			return soap->error;
		}
	}
	fclose(fd);
	return soap_end_send(soap);
}

#endif

bool format_label(char * label) {
	if (NULL == label) {
		return true;
	}

	char * ptmp = (char *) malloc(strlen(label) + 2);
	if (NULL == ptmp) {
		return false;
	}
	memset(ptmp, '\0', strlen(label) + 2);

	char * t1 = ptmp;
	char * t2 = label;

	(*t1++) = '/';

	while ('\0' != (*t2)
			&& !(((*t2) >= 'a' && (*t2) <= 'z')
					|| ((*t2) >= 'A' && (*t2) <= 'Z')
					|| ((*t2) >= '0' && (*t2) <= '9') || '_' == (*t2))) {
		t2++;
	}

	bool flag = false;
	while ((*t2) != '\0') {
		if (((*t2) >= 'a' && (*t2) <= 'z') || ((*t2) >= 'A' && (*t2) <= 'Z')
				|| ((*t2) >= '0' && (*t2) <= '9') || '_' == (*t2)
				|| '/' == (*t2)) {
			if ('/' == (*t2)) {
				if (!flag) {
					(*t1++) = (*t2++);
					flag = true;
				} else {
					t2++;
				}
			} else {
				flag = false;
				(*t1++) = (*t2++);
			}
		} else {
			t2++;
		}

	}

	if ((t1 != (ptmp + 1)) && ('/' == *(t1 - 1))) {
		*(t1 - 1) = '\0';
	} else {
		(*t1) = '\0';
	}

	memset(label, '\0', strlen(label));
	memcpy(label, ptmp, strlen(ptmp));

	free(ptmp);

	return true;
}

int ns__MarketDisableInBnNode(struct soap* soap, const char* param , struct ns__ResponseData* resData)
{
	/*resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;	
	}
	STATUS_INS insRet = BrCache::Instance()->InsertBNData(param,MARKET_DISABLE_IN_BNNODE);
	if(STATUS_SUCCESS != insRet)
	{
		string msg = Stat2String(insRet);
		strcpy(resInfo->Description, msg.c_str());
		BN_LOG_DEBUG("unable to cache data");

		return SOAP_OK;
	}
	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");
	return SOAP_OK;*/
	resData->ResultCode = 1;
        resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
        if(NULL == resData->Description)
        {
                return SOAP_OK;
        }
        if(NULL == param || 0 == strlen(param))
        {
                strcpy(resData->Description, "parameter not add");
                return SOAP_OK;
        }
        string strMarket;
        Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);
         if(NULL == pCsm)
        {
                strcpy(resData->Description, "server busy.");
                return SOAP_OK;
        }

        pCsm->DisableMarketFromBnNode(strMarket, param);
        if (!pCsm->m_success) {
                BN_LOG_ERROR("get data error.");

                if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
                {
                        strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
                }
                else
                {
                        strcpy(resData->Description, pCsm->m_errormsg.c_str());
                }

                ConsumerMgr::Instance()->ReleaseCsm(pCsm);

                return SOAP_OK;
        }
        resData->ResultCode = 0;
        strcpy(resData->Description, "success");
        resData->Content = soap_strdup(soap, pCsm->m_pData);

        ConsumerMgr::Instance()->ReleaseCsm(pCsm);
        return SOAP_OK;
}

int ns__MarketBalanceUpdataFromBnNode(struct soap* soap, const char* param , struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
    resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
    if(NULL == resData->Description)
     {
              return SOAP_OK;
     }
     if(NULL == param || 0 == strlen(param))
     {
            strcpy(resData->Description, "parameter not add");
            return SOAP_OK;
      }
     string strMarket;
     Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);
     if(NULL == pCsm)
      {
                strcpy(resData->Description, "server busy.");
                return SOAP_OK;
      }

      pCsm->UpdateMarketBalanceFromBnNode(strMarket, param);
      if (!pCsm->m_success) {
            BN_LOG_ERROR("get data error.");

             if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
             {
                     strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
              }
              else
              {
                     strcpy(resData->Description, pCsm->m_errormsg.c_str());
              }

              ConsumerMgr::Instance()->ReleaseCsm(pCsm);

              return SOAP_OK;
        }
        resData->ResultCode = 0;
        strcpy(resData->Description, "success");
        resData->Content = soap_strdup(soap, pCsm->m_pData);

        ConsumerMgr::Instance()->ReleaseCsm(pCsm);
        return SOAP_OK;
}
int ns__MarketAttachBnNode(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	
	resData->ResultCode = 1;
    resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
    if(NULL == resData->Description)
     {
              return SOAP_OK;
     }
     if(NULL == param || 0 == strlen(param))
     {
            strcpy(resData->Description, "parameter not add");
            return SOAP_OK;
      }
     string strMarket;
     Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);
     if(NULL == pCsm)
      {
                strcpy(resData->Description, "server busy.");
                return SOAP_OK;
      }

      pCsm->AddMarketToBnNode(strMarket, param);
      if (!pCsm->m_success) {
            BN_LOG_ERROR("get data error.");

             if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
             {
                     strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
              }
              else
              {
                     strcpy(resData->Description, pCsm->m_errormsg.c_str());
              }

              ConsumerMgr::Instance()->ReleaseCsm(pCsm);

              return SOAP_OK;
        }
        resData->ResultCode = 0;
        strcpy(resData->Description, "success");
        resData->Content = soap_strdup(soap, pCsm->m_pData);

        ConsumerMgr::Instance()->ReleaseCsm(pCsm);
        return SOAP_OK;
}

int ns__MarketQueryfromBn(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
        resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
        if(NULL == resData->Description)
        {
                return SOAP_OK;
        }
        if(NULL == param || 0 == strlen(param))
        {
                strcpy(resData->Description, "parameter not add");
                return SOAP_OK;
        }
        string strMarket;
        Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);
         if(NULL == pCsm)
        {
                strcpy(resData->Description, "server busy.");
                return SOAP_OK;
        }

        pCsm->GetMarketInfoFromBn(strMarket, param);
        if (!pCsm->m_success) {
                BN_LOG_ERROR("get data error.");

                if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
                {
                        strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
                }
                else
                {
                        strcpy(resData->Description, pCsm->m_errormsg.c_str());
                }

                ConsumerMgr::Instance()->ReleaseCsm(pCsm);

                return SOAP_OK;
        }
        resData->ResultCode = 0;
        strcpy(resData->Description, "success");
        resData->Content = soap_strdup(soap, pCsm->m_pData);

        ConsumerMgr::Instance()->ReleaseCsm(pCsm);
        return SOAP_OK;
	
}
int ns__AuthRandomGet(struct soap* soap, const char* param, struct ns__ResponseData *resData)
{
	 resData->ResultCode = 1;
        resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
        if(NULL == resData->Description)
        {
                return SOAP_OK;
        }
        if(NULL == param || 0 == strlen(param))
        {
                strcpy(resData->Description, "parameter not add");
                return SOAP_OK;
        }
        string strMarket;
        Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);
	 if(NULL == pCsm)
        {
                strcpy(resData->Description, "server busy.");
                return SOAP_OK;
        }

        pCsm->GetAuthRandom(strMarket, param);
        if (!pCsm->m_success) {
                BN_LOG_ERROR("get data error.");

                if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
                {
                        strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
                }
                else
                {
                        strcpy(resData->Description, pCsm->m_errormsg.c_str());
                }

                ConsumerMgr::Instance()->ReleaseCsm(pCsm);

                return SOAP_OK;
        }
 	resData->ResultCode = 0;
        strcpy(resData->Description, "success");
        resData->Content = soap_strdup(soap, pCsm->m_pData);

        ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
}
int ns__AuthTokenGet(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetAuthToken(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
}

int ns__UserPubKeySync(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->UserPubkeySync(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
}

int ns__ConsumeBillSumQuery(struct soap* soap , const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetConsumeBillSum(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
}

int ns__ConsumeBillQuery(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetConsumeBill(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
}

int ns__RegDataQueryFromUser(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN + 1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}

	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetRegDataFromOtherUser(strMarket, param);

	if (!pCsm->m_success) {
		BN_LOG_DEBUG("get data error");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
		return SOAP_OK;
	}


	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}

int ns__MarketQueryFromUser(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN + 1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}

	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetMarketInfoFromUser(strMarket, param);

	if (!pCsm->m_success) {
		BN_LOG_DEBUG("get data error");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
		return SOAP_OK;
	}


	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}

int ns__MarketQueryFromMarket(struct soap* soap, const char* param , struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN + 1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}

	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetMarketInfoFromMarket(strMarket, param);

	if (!pCsm->m_success) {
		BN_LOG_DEBUG("get data error");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
		return SOAP_OK;
	}


	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}
//int ns__marketQueryfromTecAsUser

int ns__MarketQueryFromTecCompany(struct soap* soap, const char* param, struct ns__ResponseData* resInfo )
{
	resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resInfo->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetMarketInfoFromTecCompany(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");
	resInfo->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}
int ns__MarketQureyfromTecAsUser(struct soap* soap, const char* param, struct ns__ResponseData* resInfo)
{
 	resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resInfo->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetMarketInfoFromTecCompanyAsUser(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");
	resInfo->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}

int ns__RegisterDataPriceUpdate(struct soap* soap, const char* param, struct ns__ResponseData *resInfo)
{
	/*resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;
	}
	STATUS_INS insRet = BrCache::Instance()->InsertBNData(param,REG_DATA_UPDATE);
	if(STATUS_SUCCESS != insRet)
	{
		string msg = Stat2String(insRet);
		strcpy(resInfo->Description, msg.c_str());
		BN_LOG_DEBUG("unable to cache data");

		return SOAP_OK;
	}
	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");*/
	
	resInfo->ResultCode = 1;
		resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
		if(NULL == resInfo->Description)
		{
			return SOAP_OK;
		}
		if(NULL == param || 0 == strlen(param))
		{
			strcpy(resInfo->Description, "parameter not add");
			return SOAP_OK; 
		}
		string strMarket;
		Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data
	
		if(NULL == pCsm)
		{
			strcpy(resInfo->Description, "server busy.");
			return SOAP_OK;
		}
	
		pCsm->RegisterDataPriceUpdate(strMarket, param);
		if (!pCsm->m_success) {
			BN_LOG_ERROR("get data error.");
	
			if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
			{
				strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
			}
			else
			{
				strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
			}
	
			ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	
			return SOAP_OK;
		}
	
		resInfo->ResultCode = 0;
		strcpy(resInfo->Description, "success");
		resInfo->Content = soap_strdup(soap, pCsm->m_pData);
	
		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;

}

//Jerry reg_data_disable
int ns__RegisterDataDisable(struct soap* soap, const char* param, struct ns__ResponseData * resInfo)
{
	resInfo->ResultCode = 1;
		resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
		if(NULL == resInfo->Description)
		{
			return SOAP_OK;
		}
		if(NULL == param || 0 == strlen(param))
		{
			strcpy(resInfo->Description, "parameter not add");
			return SOAP_OK; 
		}
		string strMarket;
		Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data
	
		if(NULL == pCsm)
		{
			strcpy(resInfo->Description, "server busy.");
			return SOAP_OK;
		}
	
		pCsm->RegisterDataDisable(strMarket, param);
		if (!pCsm->m_success) {
			BN_LOG_ERROR("get data error.");
	
			if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
			{
				strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
			}
			else
			{
				strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
			}
	
			ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	
			return SOAP_OK;
		}
	
		resInfo->ResultCode = 0;
		strcpy(resInfo->Description, "success");
		resInfo->Content = soap_strdup(soap, pCsm->m_pData);
	
		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;

}
//Jerry reg_data_update
int ns__RegisterDataUpdate(struct soap* soap,const char *param, struct ns__ResponseData * resInfo)
{
	resInfo->ResultCode = 1;
		resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
		if(NULL == resInfo->Description)
		{
			return SOAP_OK;
		}
		if(NULL == param || 0 == strlen(param))
		{
			strcpy(resInfo->Description, "parameter not add");
			return SOAP_OK; 
		}
		string strMarket;
		Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data
	
		if(NULL == pCsm)
		{
			strcpy(resInfo->Description, "server busy.");
			return SOAP_OK;
		}
	
		pCsm->RegisterDataUpdate(strMarket, param);
		if (!pCsm->m_success) {
			BN_LOG_ERROR("get data error.");
	
			if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
			{
				strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
			}
			else
			{
				strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
			}
	
			ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	
			return SOAP_OK;
		}
	
		resInfo->ResultCode = 0;
		strcpy(resInfo->Description, "success");
		resInfo->Content = soap_strdup(soap, pCsm->m_pData);
	
		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
}
//Jerry DataRegister Add 
int ns__RegisterDataSet(struct soap * soap,const char * param,struct ns__ResponseData * resInfo)
{
 	/*resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;	
	}
	printf("Jerry test: InsertBnData\n");
	STATUS_INS insRet = BrCache::Instance()->InsertBNData(param,REG_DATA_SET);
	if(STATUS_SUCCESS != insRet)
	{
		string msg = Stat2String(insRet);
		strcpy(resInfo->Description, msg.c_str());
		BN_LOG_DEBUG("unable to cache data");

		return SOAP_OK;
	}
	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");
	return SOAP_OK;*/
	resInfo->ResultCode = 1;
		resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
		if(NULL == resInfo->Description)
		{
			return SOAP_OK;
		}
		if(NULL == param || 0 == strlen(param))
		{
			strcpy(resInfo->Description, "parameter not add");
			return SOAP_OK; 
		}
		string strMarket;
		Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data
	
		if(NULL == pCsm)
		{
			strcpy(resInfo->Description, "server busy.");
			return SOAP_OK;
		}
	
		pCsm->RegisterDataSet(strMarket, param);
		if (!pCsm->m_success) {
			BN_LOG_ERROR("get data error.");
	
			if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
			{
				strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
			}
			else
			{
				strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
			}
	
			ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	
			return SOAP_OK;
		}
	
		resInfo->ResultCode = 0;
		strcpy(resInfo->Description, "success");
		resInfo->Content = soap_strdup(soap, pCsm->m_pData);
	
		ConsumerMgr::Instance()->ReleaseCsm(pCsm);
	return SOAP_OK;
	
}


//Jerry ns__reg_data_size_query
int ns__RegisterDataSizeQuery(struct soap* soap, char* param, struct ns__ResponseData* resData)
{
	printf("Jerry: ns_RegisterDataSizeQuery, %s\n",param);
        resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;	
	}
	printf("Jerry : getConsumer()\n");
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data
        printf("Jerry: has found consumer %s\n", strMarket.c_str());
	if(NULL == pCsm)
	{
		printf("Jerry: has not found consumer\n");
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetRegDataSize(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}
//Jerry ns_reg_data_size_query
int ns__RegisterDataPriceQuery(struct soap* soap,const char* param, struct ns__ResponseData* resInfo)
{
	resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket ;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);//Jerry , according nSeqNO to get data

	if(NULL == pCsm)
	{
		strcpy(resInfo->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetRegDataPrice(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");
	resInfo->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}

int ns__StateServiceUrlSet(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description =  (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;
	}
	string strRet;

	if(!CUpLoad::getInstance()->setUploadAddress(param, strRet))
	{
		strcpy(resData->Description, "operation fail");
		if(strRet.size() != 0)
		{
			resData->Content = soap_strdup(soap, strRet.c_str());
		}else
		{
			resData->Content = soap_strdup(soap, "unknown error");//shoudn't be here
		}
		return SOAP_OK;
	}

	strcpy(resData->Description, "Success");
	resData->Content = soap_strdup(soap, strRet.c_str());


	return SOAP_OK ;
}
//Jerry ns_reg_data_query
int ns__RegisterDataQuery(struct soap* soap, const char* param, struct ns__ResponseData* resData)
{
	resData->ResultCode = 1;
	resData->Description = (char*) soap_malloc(soap, MSG_LEN_RETURN +1);
	if(NULL == resData->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resData->Description, "parameter not add");
		return SOAP_OK;	
	}
	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);

	if(NULL == pCsm)
	{
		strcpy(resData->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->GetRegData(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resData->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resData->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}

	resData->ResultCode = 0;
	strcpy(resData->Description, "success");
	resData->Content = soap_strdup(soap, pCsm->m_pData);

	ConsumerMgr::Instance()->ReleaseCsm(pCsm);

	return SOAP_OK;
}

//New interface add by gd 2018.04.05, Used to notify BSDR to store the specified data on chain
int ns__NoticeOnChain(struct soap * soap, const char* param, struct ns__ResponseInfo* resInfo)
{
 	resInfo->ResultCode = 1;
	resInfo->Description = (char*) soap_malloc(soap, 128+1);
	if(NULL == resInfo->Description)
	{
		return SOAP_OK;
	}
	if(NULL == param || 0 == strlen(param))
	{
		strcpy(resInfo->Description, "parameter not add");
		return SOAP_OK;	
	}
	
	//handle start
	printf("input  param: %s\n",param);
       	string strMarket;
	Consumer * pCsm = ConsumerMgr::Instance()->getConsumer(strMarket);

	if(NULL == pCsm)
	{
		strcpy(resInfo->Description, "server busy.");
		return SOAP_OK;
	}

	pCsm->NotifyOnChain(strMarket, param);
	if (!pCsm->m_success) {
		BN_LOG_ERROR("get data error.");

		if(pCsm->m_errormsg.length() > MSG_LEN_RETURN)
		{
			strncpy(resInfo->Description, pCsm->m_errormsg.c_str(), MSG_LEN_RETURN);
		}
		else
		{
			strcpy(resInfo->Description, pCsm->m_errormsg.c_str());
		}

		ConsumerMgr::Instance()->ReleaseCsm(pCsm);

		return SOAP_OK;
	}
	//hand end
        //Content is need handle ? 	
	resInfo->ResultCode = 0;
	strcpy(resInfo->Description, "success");
	return SOAP_OK;
	
}


/******************************************************************************\
 *
 *	OpenSSL
 *
 \******************************************************************************/

#ifdef WITH_OPENSSL
struct CRYPTO_dynlock_value
{
	MUTEX_TYPE mutex;
};

static MUTEX_TYPE *mutex_buf;

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
{
	struct CRYPTO_dynlock_value *value;
	(void)file; (void)line;
	value = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value));
	if (value)
	MUTEX_SETUP(value->mutex);
	return value;
}

static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{
	(void)file; (void)line;
	if (mode & CRYPTO_LOCK)
	MUTEX_LOCK(l->mutex);
	else
	MUTEX_UNLOCK(l->mutex);
}

static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{
	(void)file; (void)line;
	MUTEX_CLEANUP(l->mutex);
	free(l);
}

static void locking_function(int mode, int n, const char *file, int line)
{
	(void)file; (void)line;
	if (mode & CRYPTO_LOCK)
	MUTEX_LOCK(mutex_buf[n]);
	else
	MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function()
{
	return (unsigned long)THREAD_ID;
}

int CRYPTO_thread_setup()
{
	int i;
	mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
	if (!mutex_buf)
	return SOAP_EOM;
	for (i = 0; i < CRYPTO_num_locks(); i++)
	MUTEX_SETUP(mutex_buf[i]);
	CRYPTO_set_id_callback(id_function);
	CRYPTO_set_locking_callback(locking_function);
	CRYPTO_set_dynlock_create_callback(dyn_create_function);
	CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
	CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
	return SOAP_OK;
}

void CRYPTO_thread_cleanup()
{
	int i;
	if (!mutex_buf)
	return;
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_dynlock_create_callback(NULL);
	CRYPTO_set_dynlock_lock_callback(NULL);
	CRYPTO_set_dynlock_destroy_callback(NULL);
	for (i = 0; i < CRYPTO_num_locks(); i++)
	MUTEX_CLEANUP(mutex_buf[i]);
	free(mutex_buf);
	mutex_buf = NULL;
}

#else

/* OpenSSL not used */

int CRYPTO_thread_setup() {
	return SOAP_OK;
}

void CRYPTO_thread_cleanup() {
}

#endif

/******************************************************************************\
 *
 *	SIGPIPE
 *
 \******************************************************************************/

void sigpipe_handle(int x) 
{
	BN_LOG_ERROR("receive SIGPIPE.");
}

void sigsegv_handle(int x)
{
	BN_LOG_ERROR("receive SIGSEGV.");
}

