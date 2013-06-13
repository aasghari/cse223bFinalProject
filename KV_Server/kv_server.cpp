/*
 * kv_server.cpp
 *
 *  Created on: May 19, 2013
 *      Author: aasghari
 */
#include <boost/lexical_cast.hpp>
#include "kv_store.h"
#include "MessageHandler.h"
class KVServer: public MessageHandler::MessageRecievedCallback,
				public MessageHandler::RetryFailureCallback
{
	KeyValueStore myMap;
	MessageHandler net;
public:
	KVServer(std::string serverID, const char* multicastIP, unsigned short multicastPort, const std::set<std::string>& clique)
		:myMap(serverID), net(multicastIP,multicastPort, clique,*this,serverID)
	{
		net.setRetryFailureCallback(*this);
	}
	void handleMessage(const char* message, size_t numBytes)
	{
		debug<<"handleMessage: "<<message<<" num bytes:"<<numBytes<<std::endl;
	}
	void handleRetryFailure(const char* message, size_t numBytes)
	{
		debug<<"retries failed. shuting down: "<<message<<" num bytes:"<<numBytes<<std::endl;
		net.stopHandler();
	}

	void start()
	{
		net.startHandler();
	}

	void put(std::string key, std::string value)
	{
		myMap.addValueToList(key, value);
		net.sendMessage(key);
	}

};

int main(int argc, char** argv)
{

	char* serverID = argv[1];
	char* mcastIP= argv[2];
	unsigned short mcastPort=boost::lexical_cast<unsigned short>(argv[3]);
	std::set<std::string> clique;
	for(int i=4; i<argc; i++)
	{
		clique.insert(argv[i]);
	}
	KVServer kvstore(serverID, mcastIP, mcastPort,clique);
	if(serverID[0]!='1')
	{
		for(int i=0; i<10; i++)
		{
			std::stringstream key;
			key<<"test"<<i;
			kvstore.put(key.str(), "test");
		}
	}
	kvstore.start();

}

