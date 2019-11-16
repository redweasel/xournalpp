#include "BackgroundConfig.h"

#include <StringUtils.h>

BackgroundConfig::BackgroundConfig(string config)
{
	for (string s : StringUtils::split(config, ','))
	{
		size_t dotPos = s.find_last_of("=");
		if (dotPos != string::npos)
		{
			string key = s.substr(0, dotPos);
			string value = s.substr(dotPos + 1);
			data[key] = value;
		}
	}
}

BackgroundConfig::~BackgroundConfig() = default;

auto BackgroundConfig::loadValue(string key, string& value) -> bool
{
	auto it = data.find(key);
	if (it != this->data.end())
	{
		value = it->second;
		return true;
	}

	return false;
}

auto BackgroundConfig::loadValue(string key, int& value) -> bool
{
	string str;
	if (loadValue(key, str))
	{
		value = std::stoul(str, nullptr, 10);
		return true;
	}

	return false;
}

auto BackgroundConfig::loadValue(string key, double& value) -> bool
{
	string str;
	if (loadValue(key, str))
	{
		value = std::stoul(str, nullptr, 10);
		return true;
	}

	return false;
}

auto BackgroundConfig::loadValueHex(string key, int& value) -> bool
{
	string str;
	if (loadValue(key, str))
	{
		value = std::stoul(str, nullptr, 16);
		return true;
	}

	return false;
}

