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
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
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
	static const int DATA_MAX_LENGTH = 1024;
	static const int MAX_TIME_BEFORE_RESEND_SEC=3;
	static const int MAX_TIME_BEFORE_MISSING_DATA_REQUEST=1;
	static const int MAX_MISSING_DATA_RETRIES=3;
	static const int MAX_MSG_SEND_RETRIES=3;

	class MessageRecievedCallback
	{
	public:
		virtual void handleMessage(const char* message, size_t numBytes, VectorClock messageClock)=0;
	};
	class RetryFailureCallback
	{
	public:
		virtual void handleRetryFailure(const char* message, size_t numBytes)=0;
	};
public:
	MessageHandler(	const char* multicast_address, const short multicast_port, const std::set<std::string>& clique,
		MessageHandler::MessageRecievedCallback& callback, std::string& serverID );
	void setRetryFailureCallback(MessageHandler::RetryFailureCallback& retFailCB);
	virtual ~MessageHandler();
	void sendMessage(const char* message);
	void sendMessage(const std::string& message);


	void handle_send_to(boost::shared_ptr<std::string> message,
		      const boost::system::error_code& error,
		      std::size_t bytes_sent);
	void handle_receive_from(boost::shared_array<char> data,
			const boost::system::error_code& error,size_t bytes_recvd,
			boost::shared_ptr<boost::asio::ip::udp::endpoint> recFrom);
	void handle_resendTimeout(const boost::system::error_code& error);
	void handle_missingData(const boost::system::error_code& error);
	void startHandler();
	void stopHandler();

private:

	class DataMessage
	{
		friend class MessageHandler;

		const ::Network::MsgWrapper& getMsgAsProto() const;
	public:
		DataMessage(const std::string& message, VectorClock& vectorClock, const std::set<std::string>& cliqueIDs);
		DataMessage(const ::Network::MsgWrapper& msg);
		DataMessage();//dummy ctor for look ups;
		void recievedReply(const std::string& nodeID);
		bool allRepliesRec() const;
		int getNumRetries() const;
		int getID() const;
		time_t getLastRetried() const;
		int incNumRetries();
		VectorClock getVectorClock() const;
		const std::string& getMessage();


		std::string toString();
		inline bool operator==(const DataMessage& other) const {return this->getID()==other.getID(); }
		inline bool operator!=(const DataMessage& other) const {return !this->operator==(other);}
		inline bool operator< (const DataMessage& other) const {return this->getID()<other.getID();}
		inline bool operator> (const DataMessage& other) const {return  other.operator< (*this);}
		inline bool operator<=(const DataMessage& other) const {return !this->operator> (other);}
		inline bool operator>=(const DataMessage& other) const {return !this->operator< (other);}



	private:
		::Network::MsgWrapper msg;
		int numRetries;
		time_t lastRetried;
	};
	class DataWithClocksCompare
	{
	public:

		inline bool operator()(const DataMessage& rhs,const DataMessage& lhs) const {return rhs.getVectorClock()<lhs.getVectorClock(); }
		inline bool operator()(const ::Network::MsgWrapper& rhs, const ::Network::MsgWrapper& lhs)
		{
			return VectorClock::decode(rhs.vectorclock())<	VectorClock::decode(lhs.vectorclock());
		}

	};
	MessageHandler::MessageRecievedCallback& msgRevCallback;
	MessageHandler::RetryFailureCallback* retryFailueCallback;
	void setPendingMessageRetryTimer();
	void setDataSwapTimer();
	void asynchWaitForData();
	void requestMissingData(const boost::asio::ip::udp::endpoint& destination);
	void sendMessage(const DataMessage& msg, const boost::asio::ip::udp::endpoint& destination);
	void sendMessage(const ::Network::MsgWrapper& msg, const boost::asio::ip::udp::endpoint& destination);
	boost::asio::io_service io_service;
	boost::asio::ip::udp::endpoint multicaseEndpoint;
	boost::asio::ip::udp::socket socket_;
	boost::asio::deadline_timer resendTimer;
	boost::asio::deadline_timer recheckMissingDataTimer;


//	char data_[DATA_MAX_LENGTH];//buffer to store data in

	std::map<int,DataMessage> pendingMsgs;
	std::map<boost::asio::ip::udp::endpoint,VectorClock> dataSwap;
	std::map<std::string, std::set< ::Network::MsgWrapper,DataWithClocksCompare> > messageHistory;
	std::map<std::string, int> inOrderLog;
	std::map<std::string, std::set<int> > outOfOrderLog;
	VectorClock myClock;
	std::set< std::string> currentClique;
	const std::string serverID;
	int missingDataRetiesLeft;
	int bytesSent;//track number of bytes sent
	int bytesRec;//track number of bytes recieved
	int msgsSent;	//Track number of messages sent
	int msgsRec;//track number of messages recieved

};

#endif /* MULTICASTHANDLER_H_ */
