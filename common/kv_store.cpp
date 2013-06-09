/*
 * kv_server.cpp
 *
 *  Created on: May 19, 2013
 *      Author: aasghari
 */

#include "kv_store.h"




KeyValueStore::KeyValueStore(std::string myId)
{

}
void KeyValueStore::addValueToList(const std::string& key, const std::string& value)
{
	this->keymap[key].insert(value);
}

KeyValueStore::ValueList KeyValueStore::getValues(const std::string& key)
{
	return keymap[key];
}


