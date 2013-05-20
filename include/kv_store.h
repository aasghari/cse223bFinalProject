/*
 * kv_server.h
 *
 *  Created on: May 19, 2013
 *      Author: aasghari
 */

#ifndef KV_SERVER_H_
#define KV_SERVER_H_
#include <map>
#include <set>
#include <vector>
#include "VectorClock.hpp"
class Value
{
	std::string value;
	VectorClock vecClock;

public:
	Value(const std::string& value, const VectorClock& clock);
	bool operator<(const Value& other) const;

};
class KeyValueStore
{
	typedef std::set<Value> ValueList;
	typedef std::map<std::string, ValueList> KeyValueMap;
	KeyValueMap keymap;
	VectorClock myClock;
public:
	KeyValueStore(std::string myId);
	void addValueToList(const std::string& key, const std::string& value);
	ValueList getValues(const std::string& key);
};

#endif /* KV_SERVER_H_ */
