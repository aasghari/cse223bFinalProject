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
#include "debug.h"
class MessageHandler
{
public:
	class MessageHandlerCallback
	{
	public:
		virtual void handleMessage(char* message, size_t numBytes)=0;
	};
public:
	typedef void (*MessageCallbackFun)(std::string& message) ;
	MessageHandler(	const char* multicast_address, const short multicast_port,MessageHandler::MessageHandlerCallback& callback );
	virtual ~MessageHandler();
	void sendMessage(const char* message);
	void sendMessage(const std::string& message);


	void handle_send_to(const boost::system::error_code& error);
	void handle_receive_from(const boost::system::error_code& error,size_t bytes_recvd);
	void handle_timeout(const boost::system::error_code& error);

	void startHandler();
	void stopHandler();

private:
	MessageHandler::MessageHandlerCallback& msgRevCallback;
	void asynchWaitForData();
	boost::asio::io_service io_service;
	boost::asio::ip::udp::endpoint endpoint_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::deadline_timer timer_;

	static const int DATA_MAX_LENGTH = 1024;
	char data_[DATA_MAX_LENGTH];//buffer to store data in

	int bytesSent;//track number of bytes sent
	int bytesRec;//track number of bytes recieved
	int msgsSent;	//Track number of messages sent
	int msgsRec;//track number of messages recieved

};

#endif /* MULTICASTHANDLER_H_ */
