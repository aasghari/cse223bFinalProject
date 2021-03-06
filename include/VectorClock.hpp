/*
 * VectorClock.hpp
 *
 *  Created on: May 19, 2013
 *      Author: aasghari
 */

#ifndef VECTORCLOCK_HPP_
#define VECTORCLOCK_HPP_
#include <string>
#include <sstream>
#include <iostream>
#include <map>
class VectorClock;
inline std::ostream& operator<<(std::ostream &out, const VectorClock &clock);

class VectorClock
{
	friend std::ostream& operator<<(std::ostream &out, const VectorClock &clock);
	std::string myid;
	std::map<std::string, int> clock;

public:
	VectorClock(){}
	VectorClock(const std::string& myid) :
			myid(myid)
	{
		this->clock[myid];
	}
	std::string encode() const
	{
		VectorClock* nonConstsThis=(VectorClock*)this;
		std::stringstream result;
		result<<nonConstsThis->myid<<'\n';
		for(std::map<std::string, int>::iterator it=nonConstsThis->clock.begin();
				it!=nonConstsThis->clock.end();it++)
		{
			result<<it->first<<" "<<it->second<<'\n';
		}
		return result.str();
	}
	static VectorClock decode(const std::string& input )
	{
		VectorClock result;
		std::stringstream value(input);
	    std::string clockID;
	    int clockVal;
	    value>>result.myid;
	    while(value>>clockID)
	    {
	        value>>clockVal;
	        result.clock[clockID]=clockVal;
	    }
	    return result;
	}
	int getMyClock() const
	{
		return ((VectorClock*)this)->clock[myid];
	}
	const std::string& getClockID() const
	{
		return myid;
	}

	bool operator>(const VectorClock& other) const
	{
		VectorClock & nonConstMe=(VectorClock&)(*this);
		bool allGreaterOrEqu = true; //are all other <= mine
		bool strictGreater = false; //is at least one strictly less?
		//loop over other so if its bigger, we pick up its values
		for (std::map<std::string, int>::const_iterator it = other.clock.begin(); it != other.clock.end(); it++)
		{
			const std::string& clkid = it->first;
			int myval = nonConstMe.clock[clkid];
			int otherval = it->second;
			if (myval == 0)
			{
				//never seen this one before, copy its value
				nonConstMe.clock[clkid] = myval = otherval;
			}

			if (myval > otherval)
			{
				strictGreater = true;
			}

			if (myval < otherval)
			{
				allGreaterOrEqu = false;
			}
		}
		return strictGreater && allGreaterOrEqu;
	}
	bool operator<(const VectorClock &other) const
	{
		VectorClock & nonConstMe=(VectorClock&)(*this);
		bool allLessOrEqu = true;        //are all other <= mine
		bool strictLess = false; //is at least one strictly less?
		//loop over other so if its bigger, we pick up its values
		for (std::map<std::string, int>::const_iterator it = other.clock.begin(); it != other.clock.end(); it++)
		{
			const std::string& clkid = it->first;
			int myval = nonConstMe.clock[clkid];
			int otherval = it->second;

			if (myval < otherval)
			{
				strictLess = true;
			}

			if (myval > otherval)
			{
				allLessOrEqu = false;
			}
		}
		return strictLess && allLessOrEqu;
	}

	VectorClock& incMyClock()
	{
		this->clock[this->myid]++;
		return *this;
	}
	VectorClock& operator++()
	{
		// do actual increment
		return this->incMyClock();
	}
	VectorClock operator++(int)
	{
		VectorClock tmp(*this);
		incMyClock();
		return tmp;
	}

	int clockDiffs(const VectorClock& other) const
	{
		int count=0;
		for (std::map<std::string, int>::const_iterator otherIT = other.clock.begin();
				otherIT != other.clock.end(); otherIT++)
		{
			VectorClock* nonConstThis=(VectorClock*)this;
			std::map<std::string, int>::iterator myValIT=nonConstThis->clock.find(otherIT->first);
			int myKnownMsgID=myValIT == this->clock.end()?0:myValIT->second;
			int otherKnownMsgID=otherIT->second;

				count+=otherKnownMsgID>otherKnownMsgID?
						otherKnownMsgID-otherKnownMsgID:otherKnownMsgID-myKnownMsgID;
		}
		return count;
	}
	void incrementByID(const std::string& id)
	{
		this->clock[id]++;
	}
	int getClockByID(const std::string& id) const
	{
		return ((VectorClock*)this)->clock[id];
	}
	std::map<std::string, int>::const_iterator begin()
	{
		return this->clock.begin();
	}
	std::map<std::string, int>::const_iterator end()
	{
		return this->clock.end();
	}

};

inline std::ostream& operator<<(std::ostream &out, const VectorClock &clock)
{
	// Since operator<< is a friend of the Point class, we can access
	// Point's members directly.
	out << "VecClock:" << clock.myid << " [";
	for (std::map<std::string, int>::const_iterator it = clock.clock.begin(); it != clock.clock.end(); it++)
	{
		out << " " << it->first << ":" << it->second;
	}
	out << " ]";

	return out;
}
#endif /* VECTORCLOCK_HPP_ */
