/*
 * MulticastHandler.cpp
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#include "MessageHandler.h"
class LatestInOrderForHost
{
public:
	const std::string& hostID;
	int highestInorder;
	LatestInOrderForHost(const std::string& hostID, int highestInorder):
		hostID(hostID),highestInorder(highestInorder){}

};
inline bool operator<(const LatestInOrderForHost& rhs, const ::Network::MsgWrapper& lhs)
{
	return rhs.highestInorder<
			VectorClock::decode(lhs.vectorclock()).getClockByID(rhs.hostID);
}
inline bool operator<(const ::Network::MsgWrapper& rhs, const LatestInOrderForHost& lhs)
{
	return VectorClock::decode(rhs.vectorclock()).getClockByID(lhs.hostID)<
			lhs.highestInorder;
}

MessageHandler::MessageHandler(	const char* multicast_address,const short multicast_port,
		const std::set<std::string>& clique,
		MessageHandler::MessageRecievedCallback& callback,std::string& serverID) :
		msgRevCallback(callback),retryFailueCallback(NULL),
	multicaseEndpoint(boost::asio::ip::address::from_string(multicast_address), multicast_port),
	socket_(io_service),resendTimer(io_service),recheckMissingDataTimer(io_service),
	myClock(serverID),currentClique(clique),serverID(serverID),missingDataRetiesLeft(MAX_MISSING_DATA_RETRIES),
	bytesSent(0), bytesRec(0), msgsSent(0), msgsRec(0)
{
	boost::asio::ip::udp::endpoint listen_endpoint;

	if(multicaseEndpoint.address().is_v4())
	{
		listen_endpoint=boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(),multicast_port);
	}
	else
	{
		listen_endpoint=boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(),multicast_port);
	}

	debug<<"socket open ("<<multicast_address<<":"<<multicast_port<<")"<<std::endl;
	socket_.open(multicaseEndpoint.protocol());

//	debug<<"enable broadcast"<<std::endl;
//	socket_.set_option(boost::asio::socket_base::broadcast(true));

	debug<<"setting reuse option"<<std::endl;
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	debug<<"socket binding"<<std::endl;
	socket_.bind(listen_endpoint);
	debug<<"setting loopback option"<<std::endl;
	socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));

	debug<<"joinning mulitcast group:"<<multicaseEndpoint.address().to_string()<<std::endl;
	socket_.set_option(boost::asio::ip::multicast::join_group(multicaseEndpoint.address()));

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
	this->messageHistory[this->myClock.getClockID()].insert(sendMsg.getMsgAsProto());
	//debug to test missing messages
//		if(this->myClock.getMyClock()<8)
//			return;
	this->sendMessage(sendMsg,multicaseEndpoint);
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
void handle_send_to1(boost::shared_ptr<std::string> message,
	      const boost::system::error_code& error,
	      std::size_t bytes_sent)
{}
void MessageHandler::requestMissingData(const boost::asio::ip::udp::endpoint& destination)
{
	debug<<"Requesting missing data from"<<destination.address().to_string()<<std::endl;
	::Network::MsgWrapper missDataWrap;
	missDataWrap.mutable_missingdata();
	debug<<"requesting missing with clock"<<this->myClock<<std::endl;
	missDataWrap.set_vectorclock(this->myClock.encode());
	boost::shared_ptr<std::string> msgShardPtr(new std::string(missDataWrap.SerializeAsString()));
	socket_.async_send_to(boost::asio::buffer(*msgShardPtr), destination,
			boost::bind(&MessageHandler::handle_send_to, this, msgShardPtr,
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred));
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
		dbgmsg << "Ack for message:"<<wrap.ackmsg().msgid();
	}

	if(wrap.has_datamsg())
	{
		dbgmsg<<"data msgid:"<<wrap.datamsg().msgid()<<" msg:"<<wrap.datamsg().clientmsg();
	}
	if(wrap.has_missingdata())
	{
		dbgmsg<<"missdata";

	}
	if (!error)
	{
		debug<<"Successfully sent message:"<<dbgmsg.str()<<std::endl;
	}
	this->setPendingMessageRetryTimer();


}

void MessageHandler::handle_receive_from(boost::shared_array<char> data,const boost::system::error_code& error,
		size_t bytes_recvd, boost::shared_ptr<boost::asio::ip::udp::endpoint> recFrom)
{
	this->asynchWaitForData();
	if (error)
	{
		debug<<"Got Error recieving data:"<<error<<std::endl;
		return; //Silently drop all errors
	}
	else
	{
		std::string recHost=recFrom->address().to_string();
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
			debug<<"Higher than one diff:"<<diffCount<<"...missing data from:"<<recHost<<std::endl;
			if(this->dataSwap[*recFrom] < recMsgClock)
			{
				debug<<"Higher clock than before for host"<<recHost<<std::endl;
				this->dataSwap[*recFrom]=recMsgClock;
				this->setDataSwapTimer();
			}

		}

		if(msgwrap.has_datamsg())
		{
			//Handle data messages
			const ::Network::DataPassMsg& datamsg=msgwrap.datamsg();
			debug<<"MyClock:"<<this->myClock.getClockByID(recMsgClock.getClockID())<<
					" OtherClock:"<<recMsgClock.getMyClock()<<std::endl;
			if(this->myClock.getClockByID(recMsgClock.getClockID())+1==recMsgClock.getMyClock())
			{
				debug<<"Logged in order msgid("<<recMsgClock.getClockID()<<"):"<<datamsg.msgid()<<""<<std::endl;
				this->inOrderLog[recMsgClock.getClockID()]=recMsgClock.getMyClock();
				//clocks are off by only one (ie this message we are processing) merge the clocks
				this->myClock.incrementByID(recMsgClock.getClockID());

				std::set<int>& outordermsgs=this->outOfOrderLog[recMsgClock.getClockID()];
				//sets are order. First element will be the first out of order message we have
				//if it is one bigger than the message we just got, maybe we just missed one message
				//start taking all in-order messages off the out of order list
				std::set<int>::iterator outIT;
				while((outIT=outordermsgs.begin()) != outordermsgs.end())
				{
					debug<<"out:"<<(*outIT) <<"orderlog:"<<(this->inOrderLog[recMsgClock.getClockID()]+1)<<std::endl;
					//if this entry is only one more than the message we just got
					//these are in order
					if(	(*outIT)==(this->inOrderLog[recMsgClock.getClockID()]+1))
					{
						this->inOrderLog[recMsgClock.getClockID()]++;
						this->myClock.incrementByID(recMsgClock.getClockID());
						outordermsgs.erase(outordermsgs.begin());
					}
					else
						break;
				}
			}
			else
			{

				if(!this->outOfOrderLog[recMsgClock.getClockID()].insert(recMsgClock.getMyClock()).second)
				{
					debug<<"duplicate message detected msgid("<<recMsgClock.getClockID()<<"):"<<datamsg.msgid()<<std::endl;
				}
				else
				{
					debug<<"Logged out of order msgid("<<recMsgClock.getClockID()<<"):"<<datamsg.msgid()<<std::endl;
				}
			}

			this->msgRevCallback.handleMessage(datamsg.clientmsg().data(), datamsg.clientmsg().size(), msgwrap.vectorclock());
			this->messageHistory[recMsgClock.getClockID()].insert(msgwrap);
			debug<<"Got msg from:"<<recHost<<" for msgid:"<<datamsg.msgid()<<std::endl;
			//If this is a missing data response the sender as added this to the message
			if(!msgwrap.has_missingdata())
			{
				for(int i=0; i<datamsg.cliqueids_size();i++)
				{
					if(datamsg.cliqueids(i)==this->serverID)
					{
						::Network::MsgWrapper ackWrap;
						ackWrap.mutable_ackmsg()->set_serverid(this->serverID);
						ackWrap.mutable_ackmsg()->set_msgid(datamsg.msgid());
						ackWrap.set_vectorclock(this->myClock.encode());
						debug<<"\tthis serverID:"<<this->serverID<<" is in the requested clique. Sending reply to:"<<
								recHost<<std::endl;
						this->sendMessage(ackWrap,*recFrom);
					}
				}
			}
			else
			{
				//Reset the retry count, we just processed a retrie request
				this->missingDataRetiesLeft=MAX_MISSING_DATA_RETRIES;
			}


		}
		else if(msgwrap.has_ackmsg())
		{
			//Handle ack messages
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
		else if(msgwrap.has_missingdata())
		{
			//handle missing messages
			VectorClock missdataClock=VectorClock::decode(msgwrap.vectorclock());
			debug<<"Missing data request from:"<<missdataClock.getClockID()<<std::endl;
			for(std::map<std::string, int>::const_iterator hostsInClockIT=missdataClock.begin();
					hostsInClockIT!=missdataClock.end();hostsInClockIT++)
			{

				const std::string& hostID=hostsInClockIT->first;
				int missingLatest=hostsInClockIT->second;

				std::set< ::Network::MsgWrapper,DataWithClocksCompare> msgsForHost=this->messageHistory[hostID];
				LatestInOrderForHost finder(hostID,missingLatest+1);

				std::set< ::Network::MsgWrapper,DataWithClocksCompare>::iterator  found;
				found=std::lower_bound(msgsForHost.begin(), msgsForHost.end(),finder);

				if(found!=msgsForHost.end())
				{
					debug<<"missing sending hostId:"<<hostID<< "vecclock:"<<VectorClock::decode(found->vectorclock())<<std::endl;
					//set this as a flag so other side knows this a
					//response for a missing message request
					::Network::MsgWrapper sendMsg=*found;
					sendMsg.mutable_missingdata();
					this->sendMessage(sendMsg,*recFrom);
				}
			}
		}
		else
		{
			debug<<"Unsupported msg type"<<std::endl;
		}

	}
}
void MessageHandler::handle_missingData(const boost::system::error_code& error)
{
	if(!error)
	{
		debug<<"Missing Data Timer went off retries:"<<missingDataRetiesLeft<<std::endl;
		std::map<boost::asio::ip::udp::endpoint, std::set<int> > stillMissing;
		for(std::map<boost::asio::ip::udp::endpoint,VectorClock>::iterator missIT=this->dataSwap.begin();
				missIT!=this->dataSwap.end(); )
		{
			if(this->myClock.clockDiffs(missIT->second)>0 && this->myClock < missIT->second)
			{

				debug<<"missing data from"<<missIT->first.address().to_string()<<std::endl;

				if(missingDataRetiesLeft-- >0)
				{
					//Should really contact a clique member
					//found contact orginal sender
					requestMissingData(missIT->first);
					this->setDataSwapTimer();
				}
				else
				{
					requestMissingData(missIT->first);
				}
				++missIT;
			}
			else
			{
				debug<<"deteting missing data from:"<<missIT->first.address().to_string()<<
										" size:"<<this->dataSwap.size()-1<<std::endl;
				this->dataSwap.erase(missIT++);

			}
		}

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
				this->sendMessage(msg,this->multicaseEndpoint);
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
		debug<<"pending msgs:"<<this->pendingMsgs.size()<<std::endl;
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
void MessageHandler::setDataSwapTimer()
{
	if(this->dataSwap.size()>0)
	{
		//if expired in the past
		if (this->recheckMissingDataTimer.expires_at() <= ::boost::asio::deadline_timer::traits_type::now())
		{
			this->recheckMissingDataTimer.expires_from_now(
					boost::posix_time::seconds(MessageHandler::MAX_TIME_BEFORE_MISSING_DATA_REQUEST));
			this->recheckMissingDataTimer.async_wait(boost::bind(&MessageHandler::handle_missingData, this,
				boost::asio::placeholders::error));
		}
	}
}
void MessageHandler::asynchWaitForData()
{
	boost::shared_array<char> buffer(new char[DATA_MAX_LENGTH]);
	boost::shared_ptr<boost::asio::ip::udp::endpoint> recFrom(new boost::asio::ip::udp::endpoint());
	socket_.async_receive_from(
			boost::asio::buffer(buffer.get(), DATA_MAX_LENGTH), *recFrom,
			boost::bind(&MessageHandler::handle_receive_from, this,
					buffer,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					recFrom));
}

void MessageHandler::startHandler()
{
	//start the timers to expire now so we have a time set for last expiration.
	resendTimer.expires_from_now(boost::posix_time::seconds(0));
	recheckMissingDataTimer.expires_from_now(boost::posix_time::seconds(0));
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
	this->msg.mutable_datamsg()->set_msgid(vectorClock.getMyClock());
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
