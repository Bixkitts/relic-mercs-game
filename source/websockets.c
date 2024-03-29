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

static void  base64_encode      (const char *input, 
                                 int length,
                                 char *output); 
static int   compute_sha1       (const char *data, 
                                 size_t data_len, 
                                 unsigned char *digest);
static void  extractKeyCode     (char *outString, 
                                 char *httpString, 
                                 ssize_t packetSize);
static int   generateAcceptCode (unsigned char *outCode, 
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

/*
 * this function assumes outData is large enough to hold the payload of inData
 */
/*
 * Returns the length of the decoded data
 */
int decodeWebsocketMessage(char *outData, char *inData, ssize_t dataSize)
{
    const int opcodeLength = 1;
    const int maskLength   = 4;
    int       lengthBytes  = 0; // the amount of bytes that tell us
                                 // the length of the packet
    int       maskIndex    = 0; // index at which to find the MASK
    int       payloadIndex = 0;
    char     *mask         = NULL;
    char     *inPayload    = NULL;

    // 1. First byte = FIN and opcodes
    // Ignore completely!
    

    // 2. Bytes 2-10 payload length
    //      - Read bits 9-15 (inclusive), interpret as uint
    //      This is just byte [1], we chillin

    if (((unsigned char)inData[1] & 0b01111111) <= 125) {
        lengthBytes = 1;
        // we're done, this is the payload length
    }

    // 3. IF masking is used (for incoming messages, it is), the next 4 BYTES contain
    // masking key
    maskIndex    = opcodeLength + lengthBytes; 
    payloadIndex = opcodeLength + lengthBytes + maskLength;

    mask         = &inData[maskIndex];
    inPayload    = &inData[payloadIndex];

    for (int i = 0; i < dataSize - payloadIndex; i++) {
        outData[i] = mask[i % maskLength] ^ inPayload[i];
    }
    return dataSize - payloadIndex;
    // 4. All subsequent bytes are payload
}

/*
 * Make sure outData is larger than inData
 */
int encodeWebsocketMessage(char *outData, char *inData, ssize_t dataSize)
{
    const int headerSize = 2;
    outData[0] = 0x81; // FIN websocket header
    outData[1] = (unsigned char)dataSize & 0b01111111; // Payload size

    memcpy(&outData[2], inData, dataSize);
    return dataSize + headerSize;
}

static int generateAcceptCode(unsigned char *outCode, char *inCode, ssize_t codeLen)
{
    char temp1[WEBSOCK_CODE_LEN] = {0};
    unsigned char temp2[WEBSOCK_CODE_LEN] = {0};
    strncpy (temp1, inCode, codeLen);
    strcat  (temp1, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    if (compute_sha1(temp1, strlen(temp1), temp2) < 0) {
        return -1;
    } 
    base64_encode ((char*)temp2, 20, (char*)outCode);
    printf ("\n%s\n", outCode);
    return 0;
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
        "\n\n";

    char receivedCode[WEBSOCK_CODE_LEN] = { 0 };
    char responseCode[WEBSOCK_CODE_LEN] = { 0 };
    extractKeyCode     (receivedCode, httpString, packetSize);
    if (generateAcceptCode ((unsigned char*)responseCode, receivedCode, WEBSOCK_CODE_LEN) < 0) {
        return;
    }

    strcpy  (response, tempResponse);
    strncat (response, responseCode, WEBSOCK_CODE_LEN);
    strncat (response, finResponse, strlen(finResponse));
    sendDataTCP (response, strnlen(response, WEBSOCK_HEADERS_LEN), remotehost);
}


// TODO: move encryption to it's own file?
static void base64_encode(const char* input, int length, char *outString) 
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

    memcpy(outString, bufferPtr->data, bufferPtr->length);
    outString[bufferPtr->length] = '\0';

    BIO_free_all(bio);
}

static int compute_sha1(const char *data, size_t data_len, unsigned char *digest) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned int digest_len;

    // Create and initialize the message digest context
    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        fprintf(stderr, "Error creating message digest context\n");
        return -1;
    }

    // Set the digest algorithm to SHA-1
    md = EVP_sha1();

    // Initialize the message digest computation
    if (1 != EVP_DigestInit_ex(mdctx, md, NULL)) {
        fprintf(stderr, "Error initializing message digest computation\n");
        return -1;
    }

    // Update the message digest computation with the input data
    if (1 != EVP_DigestUpdate(mdctx, data, data_len)) {
        fprintf(stderr, "Error updating message digest computation\n");
        return -1;
    }

    // Finalize the message digest computation and obtain the digest value
    if (1 != EVP_DigestFinal_ex(mdctx, digest, &digest_len)) {
        fprintf(stderr, "Error finalizing message digest computation\n");
        return -1;
    }

    // Clean up the message digest context
    EVP_MD_CTX_free(mdctx);
}

