#include "irc_common.h"
#include "irc_logic.h"
#include "irc_listeners.h"
#include "irc_client.h"
#include "irc_rcon.h"

dynvar_t *irc_connected;
cvar_t *irc_server;
cvar_t *irc_port;
cvar_t *irc_nick;

irc_export_t *GetIrcAPI(const irc_import_t *imports) {
	static irc_export_t exports;
	IRC_IMPORT = *imports;
	exports.API = Irc_If_API;
	exports.Init = Irc_If_Init;
	exports.Shutdown = Irc_If_Shutdown;
	exports.Connect = Irc_If_Connect;
	exports.Disconnect = Irc_If_Disconnect;
	exports.AddListener = Irc_Proto_AddListener;
	exports.RemoveListener = Irc_Proto_RemoveListener;
	exports.ERROR_MSG = IRC_ERROR_MSG;
	return &exports;
}

int Irc_If_API(void) {
	return IRC_API_VERSION;
}

qboolean Irc_If_Init(void) {
	irc_connected = IRC_IMPORT.Dynvar_Lookup("irc_connected");
	irc_server = IRC_IMPORT.Cvar_Get("irc_server", "", 0);
	irc_port = IRC_IMPORT.Cvar_Get("irc_port", "", 0);
	irc_nick = IRC_IMPORT.Cvar_Get("irc_nick", "", 0);
	assert(irc_connected);
	Irc_Proto_InitListeners();
	IRC_IMPORT.Dynvar_AddListener(irc_connected, Irc_Logic_Connected_f);
	IRC_IMPORT.Dynvar_AddListener(irc_connected, Irc_Client_Connected_f);
	IRC_IMPORT.Dynvar_AddListener(irc_connected, Irc_Rcon_Connected_f);
	return qtrue;
}

void Irc_If_Shutdown(void) {
	IRC_IMPORT.Dynvar_RemoveListener(irc_connected, Irc_Client_Connected_f);
	IRC_IMPORT.Dynvar_RemoveListener(irc_connected, Irc_Logic_Connected_f);
	IRC_IMPORT.Dynvar_RemoveListener(irc_connected, Irc_Rcon_Connected_f);
	Irc_Proto_TeardownListeners();					// remove remaining listeners (if any)
	Irc_ClearHistory();								// clear history buffer
}

qboolean Irc_If_Connect(void) {
	const char * const server = Cvar_GetStringValue(irc_server);
	const unsigned short port = Cvar_GetIntegerValue(irc_port);
	qboolean *c;
	Irc_Logic_Connect(server, port);						// try to connect
	IRC_IMPORT.Dynvar_GetValue(irc_connected, (void**) &c);	// get connection status
	return !*c;
}

qboolean Irc_If_Disconnect(void) {
	qboolean *c;
	IRC_IMPORT.Dynvar_GetValue(irc_connected, (void**) &c);	// get connection status
	Irc_Logic_Disconnect("");								// disconnect if connected
	return qfalse;											// always succeed
}
