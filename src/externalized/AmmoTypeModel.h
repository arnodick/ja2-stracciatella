#pragma once

#include <map>
#include <stdint.h>
#include <string_theory/string>


class JsonObject;
class JsonObjectReader;

struct AmmoTypeModel
{
	AmmoTypeModel(uint16_t index, ST::string && internalName);

	virtual ~AmmoTypeModel();

	virtual void serializeTo(JsonObject &obj) const;

	static AmmoTypeModel* deserialize(JsonObjectReader &obj);

	uint16_t index;
	ST::string internalName;
};

const AmmoTypeModel* getAmmoType(const ST::string& calibreName,
					const std::map<ST::string, const AmmoTypeModel*> &calibreMap);
