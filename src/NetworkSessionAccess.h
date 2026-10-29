#pragma once

#include "NetworkTypes.h"

namespace NNetwork
{
	bool Network_IsInSession();
	bool Network_GetPlayerName(NetworkPlayerId playerId, char* destination);
	void Network_ShutDown();
	void Network_InitializeCallsign();
}