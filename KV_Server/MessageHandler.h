/*
 * MulticastHandler.h
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#ifndef MULTICASTHANDLER_H_
#define MULTICASTHANDLER_H_

/*
 * MulticastHandler.cpp
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 *
 *      Code modified from: http://stackoverflow.com/questions/5834041/how-can-i-implement-simple-boost-asio-mulsticast-sender
 */

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <string>
#include <set>
#include <string>
#include <vector>
#include "debug.h"
#include "messages.pb.h"
#include "VectorClock.hpp"
class MessageHandler
{
public:
	class MessageHandlerCallback
	{
	public:
		virtual void handleMessage(const char* message, size_t numBytes)=0;
	};
public:
	MessageHandler(	const char* multicast_address, const short multicast_port,MessageHandler::MessageHandlerCallback& callback, std::string& serverID );
	virtual ~MessageHandler();
	void sendMessage(const char* message);
	void sendMessage(const std::string& message);


	void handle_send_to(const boost::system::error_code& error);
	void handle_receive_from(const boost::system::error_code& error,size_t bytes_recvd);
	void handle_timeout(const boost::system::error_code& error);

	void startHandler();
	void stopHandler();

private:

	class Message
	{
	public:
		Message(const std::string& message, VectorClock& vectorClock, const std::set<std::string>& cliqueIDs);
		Message();//dummy ctor for look ups;
		void recievedReply(const std::string& nodeID);
		bool allRepliesRec() const;
		int getNumRetries() const;
		int getID() const;
		void incNumRetries();
		std::string toBuffer() const;

		inline bool operator==(const Message& other) const {return this->getID()==other.getID(); }
		inline bool operator!=(const Message& other) const {return !this->operator==(other);}
		inline bool operator< (const Message& other) const {return this->getID()<other.getID();}
		inline bool operator> (const Message& other) const {return  other.operator< (*this);}
		inline bool operator<=(const Message& other) const {return !this->operator> (other);}
		inline bool operator>=(const Message& other) const {return !this->operator< (other);}



	private:

		::Network::DataPassMsg msg;
		int numRetries;

	};
	MessageHandler::MessageHandlerCallback& msgRevCallback;
	void asynchWaitForData();
	boost::asio::io_service io_service;
	boost::asio::ip::udp::endpoint sendToEndpoint;
	boost::asio::ip::udp::endpoint recFromEndpoint;
	boost::asio::ip::udp::socket socket_;
	boost::asio::deadline_timer timer_;

	static const int DATA_MAX_LENGTH = 1024;
	char data_[DATA_MAX_LENGTH];//buffer to store data in

	std::map<int,Message> pendingMsgs;
	VectorClock myClock;
	std::set< std::string> currentClique;
	const std::string serverID;
	int bytesSent;//track number of bytes sent
	int bytesRec;//track number of bytes recieved
	int msgsSent;	//Track number of messages sent
	int msgsRec;//track number of messages recieved

};

#endif /* MULTICASTHANDLER_H_ */
