/*
 * MulticastHandler.cpp
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#include "MessageHandler.h"


MessageHandler::MessageHandler(	const char* multicast_address,const short multicast_port) :
		endpoint_(boost::asio::ip::address::from_string(multicast_address), multicast_port), socket_(io_service, endpoint_.protocol()),
		timer_(io_service), bytesSent(0), bytesRec(0), msgsSent(0), msgsRec(0)
{
}
MessageHandler::~MessageHandler()
{
	this->stopHandler();
}
void MessageHandler::sendMulticast(const char* message)
{
	sendMulticast(std::string(message));
}
void MessageHandler::sendMulticast(const std::string& message)
{
	this->msgsSent++;
	this->bytesSent += message.size();
	socket_.async_send_to(boost::asio::buffer(message), endpoint_,
			boost::bind(&MessageHandler::handle_send_to, this, boost::asio::placeholders::error));
}
void MessageHandler::handle_send_to(const boost::system::error_code& error)
{
	if (!error)
	{
		debug<<"Successfully sent message"<<std::endl;
	}
	else
	{
		//TODO resend the message
	}
}

void MessageHandler::handle_receive_from(const boost::system::error_code& error,
		size_t bytes_recvd)
{
	if (error)
	{
		debug<<"Got Error:"<<error<<std::endl;
		return; //Silently drop all errors
	}
	else
	{
		this->msgsRec++;
		this->bytesRec+=bytes_recvd;
		//TODO do something with received data, maybe pass it off to application
		std::cout.write(data_, bytes_recvd);
		std::cout << std::endl;
	}

	socket_.async_receive_from(
			boost::asio::buffer(data_, DATA_MAX_LENGTH), this->endpoint_,
			boost::bind(&MessageHandler::handle_receive_from, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

}
void MessageHandler::handle_timeout(const boost::system::error_code& error)
{
	if (!error)
	{
		//TODO resend the message if not all members of clique have replied
	}
}


void MessageHandler::startHandler()
{
	this->io_service.run();
}
void MessageHandler::stopHandler()
{
	this->io_service.stop();
}

