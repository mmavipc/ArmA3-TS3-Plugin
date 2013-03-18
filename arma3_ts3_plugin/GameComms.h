#pragma once
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include <string>
#include <hash_map>
void RunThread(void* unused_args);

struct f3vector
{
	float x;
	float y;
	float z;
};

extern bool KillThread;
extern bool ThreadRunning;
extern TS3Functions ts3funcs;