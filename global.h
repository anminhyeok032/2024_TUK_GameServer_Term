#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <array>

//#include "protocol.h"

//#include "Session.h"
//#include "protocol.h"
//#include "Player.h"
//#include "Npc.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

//// global º¯¼ö
constexpr int BUF_SIZE = 200;



//std::array<SESSION*, MAX_NPC + MAX_USER> objects;

enum COMP_KEY
{
	ACCEPT = 0,
	SEND,
	RECV
};

