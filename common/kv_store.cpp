/*
 * kv_server.cpp
 *
 *  Created on: May 19, 2013
 *      Author: aasghari
 */

#include "kv_store.h"

Value::Value(const std::string& value, const VectorClock& clock):
	value(value), vecClock(clock)
{

}
bool Value::operator<(const Value& other) const
{
	return this->vecClock < other.vecClock;
}



KeyValueStore::KeyValueStore(std::string myId):
		myClock(myId)
{

}
void KeyValueStore::addValueToList(const std::string& key, const std::string& value)
{
	this->keymap[key].insert(Value(value,++myClock));
}

KeyValueStore::ValueList KeyValueStore::getValues(const std::string& key)
{
	return keymap[key];
}


