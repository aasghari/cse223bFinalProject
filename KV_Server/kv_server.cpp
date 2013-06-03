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
	KVServer(const char* hostID, const char* multicastIP, unsigned short multicastPort)
		:myMap(hostID), net(multicastIP,multicastPort, *this)
	{

	}
	void handleMessage(char* message, size_t numBytes)
	{
		debug<<"handleMessage: "<<message<<std::endl;
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

	char* hostID = argv[1];
	char* mcastIP= argv[2];
	unsigned short mcastPort=boost::lexical_cast<unsigned short>(argv[3]);
	KVServer kvstore(hostID, mcastIP, mcastPort);
	kvstore.put("test", "test");
	kvstore.start();

}

