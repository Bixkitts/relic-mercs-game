#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "websockets.h"
#include "error_handling.h"
#include "helpers.h"

#define WEBSOCK_HEADERS_LEN 512
#define WEBSOCK_CODE_LEN    64

static char *base64_encode      (const unsigned char *input, 
                                 int length); 
static void  extractKeyCode     (char *outString, 
                                 char *httpString, 
                                 ssize_t packetSize);
static void  generateAcceptCode (char *outCode, 
                                 char *inCode, 
                                 ssize_t codeLen);

static void extractKeyCode(char *outString, char *httpString, ssize_t packetSize)
{
    const char *pattern = "Sec-WebSocket-Key: ";
    int index  =  stringSearch (httpString, pattern, packetSize);
    index      += strnlen      (pattern, packetSize); 
    int index2 =  charSearch   (&httpString[index], '\n', packetSize-index);
    index2     += index;
    int codeLen = index2 - index;
    memcpy (outString, &httpString[index], codeLen);
    outString[codeLen-1] = '\0';
}

static void generateAcceptCode(char *outCode, char *inCode, ssize_t codeLen)
{

}
void sendWebSocketResponse(char *httpString, ssize_t packetSize, Host remotehost)
{
    char  response[WEBSOCK_HEADERS_LEN]  = { 0 };
    // We append the calculated hash to this
    // Then build the response
    char *tempResponse = 
        "HTTP/1.1 101 Switching Protocols\n"
        "Upgrade: websocket\n"
        "Connection: Upgrade\n"
        "Sec-WebSocket-Accept: ";
    char *finResponse =
        "Sec-WebSocket-Protocol: chat\n\n";

    char receivedCode[WEBSOCK_CODE_LEN] = { 0 };
    char responseCode[WEBSOCK_CODE_LEN] = { 0 };
    extractKeyCode  (receivedCode, httpString, packetSize);
    generateAcceptCode (responseCode, receivedCode, strlen(response));

    strcpy(response, tempResponse);
}


// TODO: might move this to own file or smthn
static char* base64_encode(const unsigned char* input, int length) 
{
    BIO     *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new  (BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new  (BIO_s_mem());
    bio = BIO_push (b64, bio);

    BIO_write       (bio, input, length);
    BIO_flush       (bio);
    BIO_get_mem_ptr (bio, &bufferPtr);

    char *buffer = (char *)malloc(bufferPtr->length + 1);
    memcpy(buffer, bufferPtr->data, bufferPtr->length);
    buffer[bufferPtr->length] = '\0';

    BIO_free_all(bio);

    return buffer;
}
