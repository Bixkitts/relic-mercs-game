#ifndef BB_RELIC_HTML_SERVER
#define BB_RELIC_HTML_SERVER

#include "bbnetlib.h"

void sendForbiddenPacket (Host remotehost);
int  getFileData         (const char *dir, char **outBuffer);

#endif
