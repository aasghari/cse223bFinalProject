/*
 * MulticastHandler.cpp
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#include "MessageHandler.h"

MessageHandler::MessageHandler(	const char* multicast_address,const short multicast_port,
		const std::set<std::string>& clique,
		MessageHandler::MessageRecievedCallback& callback,std::string& serverID) :
		msgRevCallback(callback),retryFailueCallback(NULL),
	sendToEndpoint(boost::asio::ip::address::from_string(multicast_address), multicast_port),
	socket_(io_service),resendTimer(io_service),missingDataTimer(io_service),
	myClock(serverID),currentClique(clique),serverID(serverID),bytesSent(0), bytesRec(0), msgsSent(0), msgsRec(0)
{
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
void MessageHandler::setRetryFailureCallback(MessageHandler::RetryFailureCallback& retFailCB)
{
	this->retryFailueCallback=&retFailCB;
}
void MessageHandler::sendMessage(const char* message)
{
	sendMessage(std::string(message));
}
void MessageHandler::sendMessage(const std::string& message)
{
	this->msgsSent++;
	this->bytesSent += message.size();


	DataMessage msg(message, ++this->myClock,this->currentClique);
	const DataMessage& sendMsg=this->currentClique.size()>0?this->pendingMsgs[msg.getID()]=msg:msg;
//	if(this->myClock.getMyTime()<8)
//		return;
	this->sendMessage(sendMsg,sendToEndpoint);
}
void MessageHandler::sendMessage(const ::Network::MsgWrapper& msg, const boost::asio::ip::udp::endpoint& destination)
{
	boost::shared_ptr<std::string> msgShardPtr(new std::string(msg.SerializeAsString()));
	socket_.async_send_to(boost::asio::buffer(*msgShardPtr), destination,
			boost::bind(&MessageHandler::handle_send_to, this, msgShardPtr,
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred));

}
void MessageHandler::sendMessage(const DataMessage& msg, const boost::asio::ip::udp::endpoint& destination)
{
	debug<<"sending msg:"<<msg.getID()<<" to:"<<destination.address().to_string()<<std::endl;
	sendMessage(msg.getMsgAsProto(), destination);
}
void MessageHandler::handle_send_to(boost::shared_ptr<std::string> message,
	      const boost::system::error_code& error,
	      std::size_t bytes_sent)
{
	::Network::MsgWrapper wrap;
	wrap.ParseFromString(*message);
	std::stringstream dbgmsg;

	if(wrap.has_ackmsg())
	{
		dbgmsg << "Sending Ack for message:"<<wrap.ackmsg().msgid();
	}

	if(wrap.has_datamsg())
	{
		dbgmsg<<"Sending data msgid:"<<wrap.datamsg().msgid()<<" msg:"<<wrap.datamsg().clientmsg();
	}
	if (!error)
	{
		debug<<"Successfully sent message:"<<dbgmsg.str()<<std::endl;

	}
	this->setPendingMessageRetryTimer();
}

void MessageHandler::handle_receive_from(boost::shared_array<char> data,const boost::system::error_code& error,
		size_t bytes_recvd)
{

	if (error)
	{
		debug<<"Got Error recieving data:"<<error<<std::endl;
		return; //Silently drop all errors
	}
	else
	{
		this->msgsRec++;
		this->bytesRec+=bytes_recvd;
		//TODO do something with received data, maybe pass it off to application
		::Network::MsgWrapper msgwrap;
		msgwrap.ParseFromArray(data.get(), bytes_recvd);
		VectorClock recMsgClock=VectorClock::decode(msgwrap.vectorclock());
		//If there is more than one outstanding difference, we are missing data!
		int diffCount=0;
		if((diffCount=this->myClock.clockDiffs(recMsgClock))>1)
		{
			debug<<"Higher than one diff:"<<diffCount<<"...missing data"<<std::endl;
			this->setPendingMissingMessageTimer();
		}
		else
		{
			//clocks are off by only one (ie this message we are processing) merge the clocks
			this->myClock.merge(recMsgClock);

		}
		if(msgwrap.has_datamsg())
		{
			const ::Network::DataPassMsg& datamsg=msgwrap.datamsg();
			this->log.insert(DataMessage(msgwrap));
			this->msgRevCallback.handleMessage(datamsg.clientmsg().data(), datamsg.clientmsg().size());


			debug<<"Got msg from:"<<this->recFromEndpoint.address().to_string()<<" for msgid:"<<datamsg.msgid()<<std::endl;
			for(int i=0; i<datamsg.cliqueids_size();i++)
			{
				if(datamsg.cliqueids(i)==this->serverID)
				{
					::Network::MsgWrapper ackWrap;
					ackWrap.mutable_ackmsg()->set_serverid(this->serverID);
					ackWrap.mutable_ackmsg()->set_msgid(datamsg.msgid());
					ackWrap.set_vectorclock(this->myClock.encode());
					debug<<"\tthis serverID:"<<this->serverID<<" is in the requested clique. Sending reply to:"<<
							this->recFromEndpoint.address().to_string()<<std::endl;
					this->sendMessage(ackWrap,this->recFromEndpoint);

				}
			}


		}
		else if(msgwrap.has_ackmsg())
		{
			const ::Network::AckMsg& ackmsg=msgwrap.ackmsg();
			std::map<int,DataMessage>::iterator msgLookup=this->pendingMsgs.find(ackmsg.msgid());
			debug<<"Got Ack from:"<<ackmsg.serverid()<<" for msgid:"<<ackmsg.msgid()<<std::endl;
			if(msgLookup == this->pendingMsgs.end())
			{
				debug<<"\tNo record of msgid:"<<ackmsg.msgid()<<" ignoring"<<std::endl;
				return;
			}

			DataMessage& pendmsg=msgLookup->second;
			pendmsg.recievedReply(ackmsg.serverid());
			if(pendmsg.allRepliesRec())
			{
				debug<<"\tLast ack for msgid:"<<ackmsg.msgid()<<" removing from pending list"<<std::endl;
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
void MessageHandler::handle_missingData(const boost::system::error_code& error)
{
	if(!error)
	{
		debug<<"Missing Data Timer went off"<<std::endl;
	}
	else
	{
		debug<<"Missing Data Timer went off with error:"<<error.message()<<std::endl;
	}
}
void MessageHandler::handle_resendTimeout(const boost::system::error_code& error)
{
	if(!error)
	{
		debug<<"Resend Timer went off"<<std::endl;
	}
	else
	{
		debug<<"Resend Timer went off with error:"<<error.message()<<std::endl;
	}

	//TODO resend the message if not all members of clique have replied
	for(std::map<int,DataMessage>::iterator it=this->pendingMsgs.begin();
			it!=this->pendingMsgs.end();/*not incremented*/)
	{
		DataMessage& msg=it->second;
		time_t lastChecked=msg.getLastRetried();
		double timeDiff=difftime(time(NULL),lastChecked);
		if(timeDiff>this->MAX_TIME_BEFORE_RESEND_SEC)
		{

			if(msg.incNumRetries()<=MAX_MSG_SEND_RETRIES)
			{
				//If we haven't exhausted our retries, try sending agian
				debug<<"Resedning "<<msg.toString()<<std::endl;
				this->sendMessage(msg,this->sendToEndpoint);
				++it;
			}
			else
			{
				debug<<"Msg retries exhausted. Giving up:"<<msg.toString()<<std::endl;
				if(retryFailueCallback!=NULL)
				{
					retryFailueCallback->handleRetryFailure(msg.getMessage().data(),
							msg.getMessage().size());
				}
				//If we have, give up, retrying
				this->pendingMsgs.erase(it++);
			}
		}
	}


	this->setPendingMessageRetryTimer();

}
void MessageHandler::setPendingMessageRetryTimer()
{
	if(this->pendingMsgs.size()>0)
	{
		//in expired in the past
		if (resendTimer.expires_at() <= ::boost::asio::deadline_timer::traits_type::now())
		{
			this->resendTimer.expires_from_now(
					boost::posix_time::seconds(MessageHandler::MAX_TIME_BEFORE_RESEND_SEC));
			this->resendTimer.async_wait(boost::bind(&MessageHandler::handle_resendTimeout, this,
				boost::asio::placeholders::error));
		}
	}
}
void MessageHandler::setPendingMissingMessageTimer()
{
	//if expired in the past
	if (this->missingDataTimer.expires_at() <= ::boost::asio::deadline_timer::traits_type::now())
	{
		this->missingDataTimer.expires_from_now(
				boost::posix_time::seconds(MessageHandler::MAX_TIME_BEFORE_MISSING_DATA_REQUEST));
		this->missingDataTimer.async_wait(boost::bind(&MessageHandler::handle_missingData, this,
			boost::asio::placeholders::error));
	}
}
void MessageHandler::asynchWaitForData()
{
	boost::shared_array<char> buffer(new char[DATA_MAX_LENGTH]);
	socket_.async_receive_from(
			boost::asio::buffer(buffer.get(), DATA_MAX_LENGTH), this->recFromEndpoint,
			boost::bind(&MessageHandler::handle_receive_from, this,
					buffer,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void MessageHandler::startHandler()
{
	//start the timers to expire now so we have a time set for last expiration.
	resendTimer.expires_from_now(boost::posix_time::seconds(0));
	missingDataTimer.expires_from_now(boost::posix_time::seconds(0));
	this->asynchWaitForData();
	this->io_service.run();
}
void MessageHandler::stopHandler()
{
	this->io_service.stop();
	this->socket_.close();
	this->io_service.reset();
}

MessageHandler::DataMessage::DataMessage(const std::string& message, VectorClock& vectorClock, const std::set<std::string>& cliqueIDs):
		numRetries(0), lastRetried(time(NULL))
{
	for(std::set<std::string>::iterator it=cliqueIDs.begin(); it!= cliqueIDs.end();it++)
		this->msg.mutable_datamsg()->add_cliqueids(*it);
	this->msg.mutable_datamsg()->set_clientmsg(message);
	this->msg.mutable_datamsg()->set_msgid(vectorClock.getMyTime());
	this->msg.set_vectorclock(vectorClock.encode());
}
MessageHandler::DataMessage::DataMessage(const ::Network::MsgWrapper& msg):msg(msg)
{
}
MessageHandler::DataMessage::DataMessage()
{
}
const ::Network::MsgWrapper& MessageHandler::DataMessage::getMsgAsProto() const
{
	return this->msg;
}

void MessageHandler::DataMessage::recievedReply(const std::string& nodeID)
{
	google::protobuf::RepeatedPtrField< ::google::protobuf::string >* mutableIDs=this->msg.mutable_datamsg()->mutable_cliqueids();

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
time_t MessageHandler::DataMessage::getLastRetried() const
{
	return this->lastRetried;
}
bool MessageHandler::DataMessage::allRepliesRec() const
{
	return this->msg.datamsg().cliqueids().size()<=0;
}
int MessageHandler::DataMessage::getNumRetries() const
{
	return this->numRetries;
}
int MessageHandler::DataMessage::getID() const
{
	return this->msg.datamsg().msgid();
}
int MessageHandler::DataMessage::incNumRetries()
{
	this->lastRetried=time(NULL);
	return ++this->numRetries;
}
VectorClock MessageHandler::DataMessage::getVectorClock() const
{
	return this->msg.vectorclock();
}
const std::string& MessageHandler::DataMessage::getMessage()
{
	return this->msg.datamsg().clientmsg();
}
std::string MessageHandler::DataMessage::toString()
{
	std::stringstream os;
	if(this->getMsgAsProto().has_ackmsg())
	{
		os << "Ack for message"<<this->msg.ackmsg().msgid();
	}

	if(this->getMsgAsProto().has_datamsg())
	{
		os<<"data msgid:"<<this->msg.datamsg().msgid()<<" msg:"<<this->msg.datamsg().clientmsg();
	}
	os<<" retries:"<<this->getNumRetries()<<" lastRetried:"<<this->getLastRetried();
	return os.str();
}
