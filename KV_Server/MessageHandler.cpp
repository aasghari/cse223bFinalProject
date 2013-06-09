/*
 * MulticastHandler.cpp
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#include "MessageHandler.h"


MessageHandler::MessageHandler(	const char* multicast_address,const short multicast_port,MessageHandler::MessageHandlerCallback& callback,std::string& serverID) :
	msgRevCallback(callback),
	sendToEndpoint(boost::asio::ip::address::from_string(multicast_address), multicast_port),
	socket_(io_service),timer_(io_service),
	myClock(serverID),serverID(serverID),bytesSent(0), bytesRec(0), msgsSent(0), msgsRec(0)
{
	this->currentClique.insert(serverID);
	boost::asio::ip::udp::endpoint listen_endpoint;

	if(sendToEndpoint.address().is_v4())
	{
		listen_endpoint=boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(),multicast_port);
	}
	else
	{
		listen_endpoint=boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(),multicast_port);
	}

	debug<<"socket open ("<<multicast_address<<":"<<multicast_port<<")"<<std::endl;
	socket_.open(sendToEndpoint.protocol());

//	debug<<"enable broadcast"<<std::endl;
//	socket_.set_option(boost::asio::socket_base::broadcast(true));

	debug<<"setting reuse option"<<std::endl;
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	debug<<"socket binding"<<std::endl;
	socket_.bind(listen_endpoint);
	debug<<"setting loopback option"<<std::endl;
	socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));

	debug<<"joinning mulitcast group:"<<sendToEndpoint.address().to_string()<<std::endl;
	socket_.set_option(boost::asio::ip::multicast::join_group(sendToEndpoint.address()));

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


	this->myClock++;
	Message msg(message, this->myClock,this->currentClique);
	const MessageHandler::Message& addedMsg=this->pendingMsgs[msg.getID()]=msg;
	debug<<"sending msg:"<<addedMsg.getID()<<" to:"<<sendToEndpoint.address().to_string()<<std::endl;
	socket_.async_send_to(boost::asio::buffer(addedMsg.toBuffer()), sendToEndpoint,
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
		::Network::MsgWrapper msgwrap;
		msgwrap.ParseFromArray(data_, bytes_recvd);
		if(msgwrap.has_datamsg())
		{
			const ::Network::DataPassMsg& datamsg=msgwrap.datamsg();
			this->msgRevCallback.handleMessage(datamsg.clientmsg().data(), datamsg.clientmsg().size());
			::Network::MsgWrapper ackWrap;
			ackWrap.mutable_ackmsg()->set_serverid(this->serverID);
			ackWrap.mutable_ackmsg()->set_msgid(datamsg.msgid());
			ackWrap.mutable_ackmsg()->set_vectorclock(this->myClock.encodeClock());

			debug<<"Got msg from:"<<this->recFromEndpoint.address().to_string()<<" for msgid:"<<datamsg.msgid()<<std::endl;
			for(int i=0; i<datamsg.cliqueids_size();i++)
			{
				if(datamsg.cliqueids(i)==this->serverID)
				{
					debug<<"\tthis serverID:"<<this->serverID<<" is in the requested clique. Sending reply to:"<<
							this->recFromEndpoint.address().to_string()<<std::endl;
					socket_.async_send_to(boost::asio::buffer(ackWrap.SerializeAsString()), this->recFromEndpoint,
												boost::bind(&MessageHandler::handle_send_to, this, boost::asio::placeholders::error));
				}
			}

		}
		else if(msgwrap.has_ackmsg())
		{
			const ::Network::AckMsg& ackmsg=msgwrap.ackmsg();
			int msgID=(int)ackmsg.msgid();
			std::map<int,Message>::iterator msgLookup=this->pendingMsgs.find(msgID);
			debug<<"Got Ack from:"<<ackmsg.serverid()<<" for msgid:"<<msgID<<std::endl;
			if(msgLookup == this->pendingMsgs.end())
			{
				debug<<"\tNo record of msgid:"<<msgID<<" ignoring"<<std::endl;
				return;
			}

			Message& pendmsg=msgLookup->second;
			pendmsg.recievedReply(ackmsg.serverid());
			if(pendmsg.allRepliesRec())
			{
				debug<<"\tLast ack for msgid:"<<msgID<<" removing from pending list"<<std::endl;
				this->pendingMsgs.erase(msgLookup);
			}
		}
		else
		{
			debug<<"Unsupported msg type"<<std::endl;
		}
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
			boost::asio::buffer(data_, DATA_MAX_LENGTH), this->recFromEndpoint,
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

MessageHandler::Message::Message(const std::string& message, VectorClock& vectorClock, const std::set<std::string>& cliqueIDs):numRetries(0)
{
	for(std::set<std::string>::iterator it=cliqueIDs.begin(); it!= cliqueIDs.end();it++)
		this->msg.add_cliqueids(*it);
	this->msg.set_clientmsg(message);
	this->msg.set_vectorclock(vectorClock.encodeClock());
	this->msg.set_msgid(vectorClock.getMyTime());


}
MessageHandler::Message::Message()
{
}

void MessageHandler::Message::recievedReply(const std::string& nodeID)
{
	google::protobuf::RepeatedPtrField< ::google::protobuf::string >* mutableIDs=this->msg.mutable_cliqueids();

	int index=0;
	for(google::protobuf::RepeatedPtrField< ::google::protobuf::string >::iterator it=mutableIDs->begin();
			it!=mutableIDs->end(); it++, index++)
	{
		if((*it)==nodeID)
		{
			mutableIDs->SwapElements(index, mutableIDs->size()-1);
			mutableIDs->ReleaseLast();
			break;
		}
	}

}
bool MessageHandler::Message::allRepliesRec() const
{
	return this->msg.cliqueids().size()<=0;
}
int MessageHandler::Message::getNumRetries() const
{
	return this->numRetries;
}
int MessageHandler::Message::getID() const
{
	return this->msg.msgid();
}
void MessageHandler::Message::incNumRetries()
{
	this->numRetries++;
}
std::string  MessageHandler::Message::toBuffer() const
{
	::Network::MsgWrapper wrap;
	wrap.mutable_datamsg()->CopyFrom(this->msg);
	return wrap.SerializeAsString();
}
