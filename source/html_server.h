#ifndef BB_RELIC_HTML_SERVER
#define BB_RELIC_HTML_SERVER

#include "bbnetlib.h"

// Max length of individual headers
#define HEADER_LENGTH        64
#define MAX_HEADERS          32
// Max size of all the headers put together
#define HEADER_PACKET_LENGTH HEADER_LENGTH*MAX_HEADERS
// The string length for integers and names
// in string form to put into headers
#define STATUS_LENGTH        32

typedef enum HTTPContentType {
    HTTP_FLAG_TEXT_HTML,
    HTTP_FLAG_IMAGE_JPG,
    HTTP_FLAG_IMAGE_PNG,
    HTTP_FLAG_TEXT_JAVASCRIPT,
    HTTP_FLAG_COUNT
} HTTPContentType;

void sendContent            (char* dir, 
                             HTTPContentType type, 
                             Host remotehost);
void sendForbiddenPacket    (Host remotehost);

// A list of files we're allowed to serve
// with "sendContent()"
void createAllowedFileTable (void);
bool isFileAllowed          (const char* inDir, char **outDir);

HTTPContentType getContentTypeEnumFromFilename(char* name);

#endif
