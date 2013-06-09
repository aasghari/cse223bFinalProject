/*
 * kv_server.cpp
 *
 *  Created on: May 19, 2013
 *      Author: aasghari
 */
#include <boost/lexical_cast.hpp>
#include "kv_store.h"
#include "MessageHandler.h"
class KVServer: public MessageHandler::MessageHandlerCallback
{
	KeyValueStore myMap;
	MessageHandler net;
public:
	KVServer(std::string serverID, const char* multicastIP, unsigned short multicastPort)
		:myMap(serverID), net(multicastIP,multicastPort, *this,serverID)
	{

	}
	void handleMessage(const char* message, size_t numBytes)
	{
		debug<<"handleMessage: "<<message<<" num bytes:"<<numBytes<<std::endl;
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
	KVServer kvstore(serverID, mcastIP, mcastPort);
//	if(serverID[0]!='1')
//	{
		for(int i=0; i<10; i++)
		{
			std::stringstream key;
			key<<"test"<<i;
			kvstore.put(key.str(), "test");
		}
//	}
	kvstore.start();

}

