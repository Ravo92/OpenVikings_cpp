#pragma once

#include "NetworkTypes.h"

namespace NNetwork
{
	struct SDynamicNetworkData;
	bool SendDynamicNetworkChatMessage(SDynamicNetworkData* data, NetworkPlayerId recipient, const wchar_t* text);
	void CopyNetworkUtf8ToWide(wchar_t* destination, const char* source, std::int32_t capacity);
	char* Network_GetCallsign();
	bool Network_IsInSession();
	bool Network_GetPlayerName(NetworkPlayerId playerId, char* destination);
	void Network_ShutDown();
	void Network_InitializeCallsign();
}

extern NNetwork::SDynamicNetworkData gDynamicNetworkData;