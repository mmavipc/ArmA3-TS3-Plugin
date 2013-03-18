#include "GameComms.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Sddl.h>
#include <ctime>

#pragma comment(lib, "Advapi32.lib")
struct PlayerInfo
{
	TS3_VECTOR *pos;
	clock_t lastUpdate;
	anyID clientID;
	PlayerInfo() : pos(new TS3_VECTOR)
	{
	}

	~PlayerInfo()
	{
		delete pos;
	}
};
std::hash_map<std::string, PlayerInfo*> players;

bool KillThread = false;
bool ThreadRunning = false;

std::string ReadString(HANDLE handle, unsigned int length)
{
	DWORD bytesread = 0;
	DWORD total = 0;
	char *buf = new char[length];
	while(total != length)
	{
		ReadFile(handle, (buf+total), length-total, &bytesread, NULL);
		total += bytesread;
	}
	std::string ret(buf, length);
	delete [] buf;
	return ret;
}

int ReadInt(HANDLE handle)
{
	DWORD bytesread = 0;
	DWORD total = 0;
	char *buf = new char[sizeof(int)];
	while(total != sizeof(int))
	{
		ReadFile(handle, (buf+total), sizeof(int)-total, &bytesread, NULL);
		total += bytesread;
	}
	int ret = *(int*)buf;
	delete [] buf;
	return ret;
}

float ReadFloat(HANDLE handle)
{
	DWORD bytesread = 0;
	DWORD total = 0;
	char *buf = new char[sizeof(float)];
	while(total != sizeof(float))
	{
		ReadFile(handle, (buf+total), sizeof(float)-total, &bytesread, NULL);
		total += bytesread;
	}
	float ret = *(float*)buf;
	delete [] buf;
	return ret;
}

void RunThread(void* unused_args)
{
	ThreadRunning = true;
	PSECURITY_DESCRIPTOR sdsc;
	ULONG size;
	ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)", SDDL_REVISION_1, &sdsc, &size);
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = false;
	sa.lpSecurityDescriptor = sdsc;
	HANDLE pipe = CreateNamedPipe("\\\\.\\pipe\\A3TS3_pipe", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE, 255, 1024, 1024, 0, &sa);
	TS3_VECTOR myPos;
	char *buf = new char[1024];
	clock_t lastMessage = clock();
	bool bRunning = true;
	unsigned int ts_error = 0;
	while(!KillThread)
	{
		for(std::hash_map<std::string, PlayerInfo*>::iterator it = players.begin(); it != players.end(); ++it)
		{
			if(it->second)
			{
				if((clock() - it->second->lastUpdate)/CLOCKS_PER_SEC >= 5)
				{
					TS3_VECTOR zeropos;
					memset(&zeropos, 0, sizeof(TS3_VECTOR));
					ts3funcs.channelset3DAttributes(ts3funcs.getCurrentServerConnectionHandlerID(), it->second->clientID, &zeropos);
					players.erase(it);
				}
			}
		}
		if(bRunning && (clock()-lastMessage)/CLOCKS_PER_SEC >= 5)
		{
			bRunning = false;
			anyID *clients;
			ts_error = ts3funcs.getClientList(ts3funcs.getCurrentServerConnectionHandlerID(), &clients);
			if(ts_error == ERROR_ok)
			{
				TS3_VECTOR zeropos;
				memset(&zeropos, 0, sizeof(TS3_VECTOR));
				for(unsigned int i = 0; clients[i] != 0; i++)
				{
					ts3funcs.channelset3DAttributes(ts3funcs.getCurrentServerConnectionHandlerID(), clients[i], &zeropos);
				}
			}
		}
		DWORD bytesread = 0;
		ReadFile(pipe, buf, 1, &bytesread, NULL);
		DWORD error = GetLastError();
		if(error == ERROR_BROKEN_PIPE)
		{
			CloseHandle(pipe);
			pipe = CreateNamedPipe("\\\\.\\pipe\\A3TS3_pipe", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE, 255, 1024, 1024, 0, &sa);
		}
		if(bytesread != 0)
		{
			lastMessage = clock();
			bRunning = true;
			switch(buf[0])
			{
				case 0: //Player position update
					{
						int length = ReadInt(pipe);
						std::string playerID = ReadString(pipe, length);
						TS3_VECTOR theirpos;
						if(!players[playerID])
						{
							players[playerID] = new PlayerInfo();
						}
						players[playerID]->pos->x = ReadFloat(pipe) - myPos.x;
						players[playerID]->pos->y = ReadFloat(pipe) - myPos.y;
						players[playerID]->pos->z = ReadFloat(pipe) - myPos.z;
						players[playerID]->lastUpdate = clock();

						anyID *clients;
						ts_error = ts3funcs.getClientList(ts3funcs.getCurrentServerConnectionHandlerID(), &clients);
						if(ts_error == ERROR_ok)
						{
							for(unsigned int i = 0; clients[i] != 0; i++)
							{
								char* playerid;
								ts3funcs.getClientVariableAsString(ts3funcs.getCurrentServerConnectionHandlerID(), clients[i], CLIENT_META_DATA, &playerid);
								std::string strPlayerID(playerid);
								if(playerID == strPlayerID)
								{
									players[playerID]->clientID = clients[i];
									ts_error = ts3funcs.channelset3DAttributes(ts3funcs.getCurrentServerConnectionHandlerID(), clients[i], players[playerID]->pos);
								}
							}
						}
					}
					break;
				case 1: //Self playerID update
					{
						int length = ReadInt(pipe);
						std::string playerID = ReadString(pipe, length);
						ts3funcs.setClientSelfVariableAsString(ts3funcs.getCurrentServerConnectionHandlerID(), CLIENT_META_DATA, playerID.c_str());
					}
					break;
				case 2: //Self position update
					{
						TS3_VECTOR position;
						TS3_VECTOR forward;
						myPos.x = ReadFloat(pipe);
						myPos.y = ReadFloat(pipe);
						myPos.z = ReadFloat(pipe);
						position.x = 0;
						position.y = 0;
						position.z = 0;
						forward.x = ReadFloat(pipe);
						forward.y = ReadFloat(pipe);
						forward.z = ReadFloat(pipe);
						TS3_VECTOR up;
						up.x = ReadFloat(pipe);
						up.y = ReadFloat(pipe);
						up.z = ReadFloat(pipe);
						ts_error = ts3funcs.systemset3DListenerAttributes(ts3funcs.getCurrentServerConnectionHandlerID(), &position, &forward, &up);
					}
					break;
			}
		}
		Sleep(10);
	}
	ThreadRunning = false;
}