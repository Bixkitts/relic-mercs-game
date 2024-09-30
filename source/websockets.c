#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

#include "error_handling.h"
#include "helpers.h"
#include "websockets.h"

#define WEBSOCK_HEADERS_LEN 512
#define WEBSOCK_CODE_LEN    64

static void base64_encode(const char *input, int length, char *output);
static int compute_sha1(const char *data,
                        size_t data_len,
                        unsigned char *digest);
static void extractKeyCode(char *outString,
                           char *httpString,
                           ssize_t packetSize);
static int generateAcceptCode(unsigned char *outCode,
                              char *inCode,
                              ssize_t codeLen);

static void extractKeyCode(char *outString,
                           char *httpString,
                           ssize_t packetSize)
{
    const char *pattern = "Sec-WebSocket-Key: ";
    int index           = stringSearch(httpString, pattern, packetSize);
    index += strnlen(pattern, packetSize);
    int index2 = charSearch(&httpString[index], '\n', packetSize - index);
    index2 += index;
    int codeLen = index2 - index;
    memcpy(outString, &httpString[index], codeLen);
    outString[codeLen - 1] = '\0';
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
    // The 9th bit is the MASK bit, that's why it's 0.
    const unsigned char lengthCode = (unsigned char)inData[1] & 0b01111111;
    // By default, payload length is the same as the length code
    // up to 125 bytes
    ssize_t payloadLength    = (ssize_t)lengthCode;
    ssize_t maxPayloadLength = 0;
    // The amount of bytes that make up the number that indicates
    // the amount of bytes that make up the payload :))
    int lengthBytes = 1;

    int maskIndex    = 0; // index at which to find the MASK
    int payloadIndex = 0;
    char *mask       = NULL;
    char *inPayload  = NULL;

    // 1. First byte = FIN and opcodes
    // Ignore completely!

    /* 2. Bytes 2-10 payload length
     * The lengthCode indicates how many bytes we need to tell
     * provide the length of the packet.
     * Magic numbers are from websocket spec.
     */
    if (lengthCode == 126) {
        lengthBytes   = 3;
        payloadLength = (uint16_t)inData[2];
    }
    else if (lengthCode == 127) {
        lengthBytes   = 9;
        payloadLength = (uint64_t)inData[2];
    }

    // 3. IF masking is used (for incoming messages, it is), the next 4 BYTES
    // contain masking key
    maskIndex    = opcodeLength + lengthBytes;
    payloadIndex = opcodeLength + lengthBytes + maskLength;

    mask      = &inData[maskIndex];
    inPayload = &inData[payloadIndex];

    maxPayloadLength = dataSize - payloadIndex;

    if (payloadLength > maxPayloadLength) {
        fprintf(stderr, "Malformed websocket packet received, ignoring...");
        return 0;
    }

    // Here the actual payload from the client is decoded.
    for (int i = 0; i < payloadLength; i++) {
        outData[i] = mask[i % maskLength] ^ inPayload[i];
    }
    return payloadLength;
}

/*
 * inOutData needs to be a buffer
 * with WEBSOCKET_HEADER_SIZE_MAX free space
 *
 * Returns the size of the websocket header
 * written to the buffer in bytes.
 */
int writeWebsocketHeader(char inOutData[static WEBSOCKET_HEADER_SIZE_MAX],
                         ssize_t dataSize)
{
    /* Refer to the websocket spec for how
     * this encoding works
     */
    int headerSize = 0;
    // 1. Websocket FIN Header
    inOutData[0] = 0x82;
    // 2. Weirdest part of the
    //    websocket spec (writing payload length).
    if (dataSize < 126) {
        inOutData[1] = (unsigned char)dataSize & 0b01111111;
        headerSize   = 2;
    }
    else {
        inOutData[1] =
            (unsigned char)126 & 0b01111111; // 126 is a magic number for a
                                             // 16 bit payload length.
        // Network byte order (big endian)
        inOutData[2] = (unsigned char)((dataSize >> 8) & 0xFF);
        inOutData[3] = (unsigned char)(dataSize & 0xFF);
        headerSize   = 4;
    }

    return headerSize;
}

static int generateAcceptCode(unsigned char *outCode,
                              char *inCode,
                              ssize_t codeLen)
{
    const int codeMaxLen                  = 20;
    char temp1[WEBSOCK_CODE_LEN]          = {0};
    unsigned char temp2[WEBSOCK_CODE_LEN] = {0};
    strncpy(temp1, inCode, codeLen);
    // Constant string that's part of the websocket standard
    strcat(temp1, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    if (compute_sha1(temp1, strlen(temp1), temp2) < 0) {
        return -1;
    }
    base64_encode((char *)temp2, codeMaxLen, (char *)outCode);
#ifdef DEBUG
    printf("\n%s\n", outCode);
#endif
    return 0;
}

void sendWebSocketResponse(char *httpString,
                           ssize_t packetSize,
                           struct host *remotehost)
{
    char response[WEBSOCK_HEADERS_LEN] = {0};
    // We append the calculated hash to this
    // Then build the response
    const char *tempResponse = "HTTP/1.1 101 Switching Protocols\n"
                               "Upgrade: websocket\n"
                               "Connection: Upgrade\n"
                               "Sec-WebSocket-Accept: ";
    const char *finResponse  = "\n\n";

    char receivedCode[WEBSOCK_CODE_LEN] = {0};
    char responseCode[WEBSOCK_CODE_LEN] = {0};
    extractKeyCode(receivedCode, httpString, packetSize);
    if (generateAcceptCode((unsigned char *)responseCode,
                           receivedCode,
                           WEBSOCK_CODE_LEN) < 0) {
        return;
    }

    strcpy(response, tempResponse);
    strncat(response, responseCode, WEBSOCK_CODE_LEN);
    strncat(response, finResponse, strlen(finResponse));
    send_data_tcp(response, strnlen(response, WEBSOCK_HEADERS_LEN), remotehost);
}

// TODO: move encryption to it's own file?
static void base64_encode(const char *input, int length, char *outString)
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    memcpy(outString, bufferPtr->data, bufferPtr->length);
    outString[bufferPtr->length] = '\0';

    BIO_free_all(bio);
}

static int compute_sha1(const char *data,
                        size_t data_len,
                        unsigned char *digest)
{
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
    return 0;
}
