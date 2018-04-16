GSOAP=./gsoap/soapcpp2
SOAPH=./gsoap/stdsoap2.h
SOAPC=./gsoap/stdsoap2.c
SOAPCPP=./gsoap/stdsoap2.cpp
#CC=g++
LIBS=-lpthread -lz -lssl -lcrypto -lndn-cxx -lboost_system -llog4cxx -lcurl
COFLAGS=-g
CWFLAGS=-Wall -Wno-deprecated-declarations -std=c++11
CIFLAGS=-I./gsoap -I./gsoap/plugin
LOGFLASS=
#LOGFLASS=-D_WITH_LOG4CXXLOG
#CMFLAGS=-DWITH_COOKIES -DWITH_GZIP -DWITH_OPENSSL -DWITH_NO_C_LOCALE -D_WITH_TEST 
CMFLAGS=-DWITH_COOKIES -DWITH_GZIP -DWITH_NO_C_LOCALE 
CFLAGS= $(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS) $(LOGFLASS)
CXXFLAGS =$(COFLAGS) -std=c++11 $(LOGFLASS)
all:		baarsvr
baarsvr:	baarsvr.h baarsvr.c logging.o httpget.o threads.o options.o upload.o\
			producermgr.o producer.o produceregz.o consumermgr.o consumer.o brcache.o brnamepubliser.o \
			config.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o \
			aes_algo.o bnlogif.o\
			$(SOAPH) $(SOAPC)
		$(GSOAP) -c -L baarsvr.h
		$(CXX) $(CFLAGS) -o baarsvr baarsvr.c logging.o httpget.o threads.o options.o upload.o\
			producermgr.o producer.o produceregz.o consumermgr.o consumer.o brcache.o brnamepubliser.o \
			config.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o \
			aes_algo.o bnlogif.o\
			soapC.c soapClient.c soapServer.c $(SOAPC) $(LIBS)
options.o:	opt.h options.h options.c
		$(GSOAP) -cnpopt opt.h
		$(CXX) $(CFLAGS) -c options.c
logging.o:	./gsoap/plugin/logging.h ./gsoap/plugin/logging.c
		$(CXX) $(CFLAGS) -c ./gsoap/plugin/logging.c
httpget.o:      ./gsoap/plugin/httpget.h ./gsoap/plugin/httpget.c
		$(CXX) $(CFLAGS) -c ./gsoap/plugin/httpget.c
threads.o:	./gsoap/plugin/threads.h ./gsoap/plugin/threads.c
		$(CXX) $(CFLAGS) -c ./gsoap/plugin/threads.c

producer.o: producer.h producer.cpp
	    $(CXX) $(CXXFLAGS) -c producer.cpp -o producer.o
upload.o: upload.h upload.cpp
	    $(CXX) $(CXXFLAGS) -c upload.cpp -o upload.o
produceregz.o: produceregz.h produceregz.cpp
	    $(CXX) $(CXXFLAGS) -c produceregz.cpp -o produceregz.o
producermgr.o: producermgr.h producermgr.cpp
		$(CXX) $(CXXFLAGS) -c producermgr.cpp -o producermgr.o
consumermgr.o: consumermgr.h consumermgr.cpp
		$(CXX) $(CXXFLAGS) -c consumermgr.cpp -o consumermgr.o
consumer.o: consumer.h consumer.cpp
	    $(CXX) $(CXXFLAGS) -c consumer.cpp -o consumer.o
brcache.o: brcache.h brcache.cpp
	    $(CXX) $(CXXFLAGS) -c brcache.cpp -o brcache.o
brnamepubliser.o :brnamepubliser.h brnamepubliser.cpp
	    $(CXX) $(CXXFLAGS) -c brnamepubliser.cpp -o brnamepubliser.o
config.o: config.h config.cpp 
		$(CXX) $(CXXFLAGS) -c config.cpp -o config.o

tinystr.o :./tinyxml/tinystr.h ./tinyxml/tinystr.cpp
	    $(CXX) $(CFLAGS) -c ./tinyxml/tinystr.cpp
tinyxml.o :./tinyxml/tinyxml.h ./tinyxml/tinyxml.cpp
	    $(CXX) $(CFLAGS) -c ./tinyxml/tinyxml.cpp
tinyxmlerror.o :./tinyxml/tinyxml.h ./tinyxml/tinyxmlerror.cpp
		$(CXX) $(CFLAGS) -c ./tinyxml/tinyxmlerror.cpp tinyxml.o
tinyxmlparser.o :./tinyxml/tinyxml.h ./tinyxml/tinyxmlparser.cpp
		$(CXX) $(CFLAGS) -c ./tinyxml/tinyxmlparser.cpp tinyxml.o

aes_algo.o : ./algo/aes_algo.h ./algo/aes_algo.c
		$(CXX) $(CXXFLAGS) -c ./algo/aes_algo.c -o aes_algo.o
bnlogif.o:	bnlogif.h ./include/bnconst.h ./bnlogif.h
		$(CXX) $(CXXFLAGS) -c bnlogif.cpp
		
.PHONY: clean distclean
clean:
		rm -f *.o soapH.h soapStub.h soapC.c soapClient.c soapServer.c soapClientLib.c soapServerLib.c
		rm -f *.wsdl *.xsd *.req.xml *.res.xml *.nsmap *.log optC.c optStub.h optH.h upload.json stateForBn
distclean:
		rm -f *.o soapH.h soapStub.h soapC.c soapClient.c soapServer.c soapClientLib.c soapServerLib.c
		rm -f *.wsdl *.xsd  *.req.xml *.res.xml *.nsmap *.log optC.c optStub.h optH.h
		rm -f baarsvr
