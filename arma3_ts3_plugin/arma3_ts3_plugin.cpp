// arma3_ts3_plugin.cpp : Defines the exported functions for the DLL application.
//
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include <cstdlib>
#include <map>
#include <process.h>
#include "GameComms.h"

TS3Functions ts3funcs;

#define DLLEXPORT __declspec(dllexport)

struct ServerSpecificVars
{
};

std::map<uint64, ServerSpecificVars*> ssv;
uint64 sid = 0;

extern "C"
{
	DLLEXPORT const char* ts3plugin_name()
	{
		return "MMavipc's ArmA3 TS3 Plugin";
	}

	DLLEXPORT const char* ts3plugin_version()
	{
		return "1.0.0";
	}

	DLLEXPORT int ts3plugin_apiVersion()
	{
		return 19;
	}

	DLLEXPORT const char* ts3plugin_author()
	{
		return "Maverick 'MMavipc' Mosher & GitHubians";
	}

	DLLEXPORT const char* ts3plugin_description()
	{
		return "Integrates TS3 voice chat with arma";
	}

	DLLEXPORT void ts3plugin_setFunctionPointers(const struct TS3Functions funcs)
	{
		ts3funcs = funcs;
	}

	DLLEXPORT int ts3plugin_init()
	{
		_beginthread(RunThread, 8192, NULL);
		return 0;
	}

	DLLEXPORT void ts3plugin_shutdown()
	{
		KillThread = true;
		while(ThreadRunning)
		{
		}
	}

	DLLEXPORT void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID)
	{
		if(!ssv[serverConnectionHandlerID])
		{
			ssv[serverConnectionHandlerID] = new ServerSpecificVars;
		}
		sid = serverConnectionHandlerID;
	}

	DLLEXPORT const char* ts3plugin_infoTitle()
	{
		return "ArmATS3";
	}

	DLLEXPORT void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data)
	{
		std::string derp;
		char *name = NULL;
		if(type == PLUGIN_CLIENT)
		{
			ts3funcs.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_META_DATA, &name);
			derp += name;
			*data = (char*)malloc(derp.size()+1);
			(*data)[derp.size()] = 0;
			memcpy(*data, derp.c_str(), derp.size());
		}
	}

	DLLEXPORT void ts3plugin_freeMemory(void* data)
	{
		free(data);
	}

	DLLEXPORT void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels)
	{
		for(int i = 0; i < sampleCount; i++)
		{
			//samples[i] ^= 0xAAAA;
			//samples[i] = ~samples[i];
		}
	}

	DLLEXPORT void ts3plugin_onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance , float* volume )
	{
		float val = 1-(distance/75.0f);
		val = val*val;
		*volume = val;
		if(distance >= 75.0f)
		{
			*volume = 0;
		}
		else if(distance <= 15.0f)
		{
			*volume = 1;
		}
	}
};