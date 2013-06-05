/*
 * MulticastHandler.cpp
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#include "MessageHandler.h"


MessageHandler::MessageHandler(	const char* multicast_address,const short multicast_port,MessageHandler::MessageHandlerCallback& callback) :
	msgRevCallback(callback),endpoint_(boost::asio::ip::address::from_string(multicast_address), multicast_port), socket_(io_service),
	timer_(io_service), bytesSent(0), bytesRec(0), msgsSent(0), msgsRec(0)
{
	boost::asio::ip::udp::endpoint listen_endpoint;

	if(endpoint_.address().is_v4())
	{
		listen_endpoint=boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(),multicast_port);
	}
	else
	{
		listen_endpoint=boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(),multicast_port);
	}

	debug<<"socket open ("<<multicast_address<<":"<<multicast_port<<")"<<std::endl;
	socket_.open(endpoint_.protocol());

//	debug<<"enable broadcast"<<std::endl;
//	socket_.set_option(boost::asio::socket_base::broadcast(true));

	debug<<"setting reuse option"<<std::endl;
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	debug<<"socket binding"<<std::endl;
	socket_.bind(listen_endpoint);
	debug<<"setting loopback option"<<std::endl;
	socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));

	debug<<"joinning mulitcast group:"<<endpoint_.address().to_string()<<std::endl;
	socket_.set_option(boost::asio::ip::multicast::join_group(endpoint_.address()));





}
MessageHandler::~MessageHandler()
{
	this->stopHandler();
}
void MessageHandler::sendMessage(const char* message)
{
	sendMessage(std::string(message));
}
void MessageHandler::sendMessage(const std::string& message)
{
	this->msgsSent++;
	this->bytesSent += message.size();
	debug<<"sending to:"<<endpoint_.address().to_string()<<std::endl;
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
	this->asynchWaitForData();
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
		this->msgRevCallback.handleMessage(data_, bytes_recvd);
	}

	this->asynchWaitForData();

}
void MessageHandler::handle_timeout(const boost::system::error_code& error)
{
	if (!error)
	{
		//TODO resend the message if not all members of clique have replied
	}
	this->asynchWaitForData();
}

void MessageHandler::asynchWaitForData()
{
	socket_.async_receive_from(
			boost::asio::buffer(data_, DATA_MAX_LENGTH), this->endpoint_,
			boost::bind(&MessageHandler::handle_receive_from, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void MessageHandler::startHandler()
{
	this->asynchWaitForData();

	this->io_service.run();
}
void MessageHandler::stopHandler()
{
	this->io_service.stop();
}

