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

static void base64_encode(const char *input, int length, char *out_string);
static int compute_sha1(const char *data,
                        size_t data_len,
                        unsigned char *digest);
static void extract_key_code(char *out_string,
                             char *http_string,
                             ssize_t packet_size);
static int generate_accept_code(unsigned char *out_code,
                                char *in_code,
                                ssize_t code_len);

static void extract_key_code(char *out_string,
                             char *http_string,
                             ssize_t packet_size)
{
    const char *pattern = "Sec-WebSocket-Key: ";
    int index           = string_search(http_string, pattern, packet_size);
    index += strnlen(pattern, packet_size);
    int index2 = char_search(&http_string[index], '\n', packet_size - index);
    index2 += index;
    int code_len = index2 - index;
    memcpy(out_string, &http_string[index], code_len);
    out_string[code_len - 1] = '\0';
}

/*
 * this function assumes outData is large enough to hold the payload of inData
 */
/*
 * Returns the length of the decoded data
 */
int decode_websocket_message(char *out_data, char *in_data, ssize_t data_size)
{
    const int opcode_length = 1;
    const int mask_length   = 4;
    // The 9th bit is the MASK bit, that's why it's 0.
    const unsigned char length_code = (unsigned char)in_data[1] & 0b01111111;
    // By default, payload length is the same as the length code
    // up to 125 bytes
    ssize_t payload_length     = (ssize_t)length_code;
    ssize_t max_payload_length = 0;
    // The amount of bytes that make up the number that indicates
    // the amount of bytes that make up the payload :))
    int length_bytes = 1;

    int mask_index    = 0; // index at which to find the MASK
    int payload_index = 0;
    char *mask        = NULL;
    char *in_payload  = NULL;

    // 1. First byte = FIN and opcodes
    // Ignore completely!

    /* 2. Bytes 2-10 payload length
     * The lengthCode indicates how many bytes we need to tell
     * provide the length of the packet.
     * Magic numbers are from websocket spec.
     */
    if (length_code == 126) {
        length_bytes   = 3;
        payload_length = (uint16_t)in_data[2];
    }
    else if (length_code == 127) {
        length_bytes   = 9;
        payload_length = (uint64_t)in_data[2];
    }

    // 3. IF masking is used (for incoming messages, it is), the next 4 BYTES
    // contain masking key
    mask_index    = opcode_length + length_bytes;
    payload_index = opcode_length + length_bytes + mask_length;

    mask       = &in_data[mask_index];
    in_payload = &in_data[payload_index];

    max_payload_length = data_size - payload_index;

    if (payload_length > max_payload_length) {
        fprintf(stderr, "Malformed websocket packet received, ignoring...");
        return 0;
    }

    // Here the actual payload from the client is decoded.
    for (int i = 0; i < payload_length; i++) {
        out_data[i] = mask[i % mask_length] ^ in_payload[i];
    }
    return payload_length;
}

/*
 * inOutData needs to be a buffer
 * with WEBSOCKET_HEADER_SIZE_MAX free space
 *
 * Returns the size of the websocket header
 * written to the buffer in bytes.
 */
int write_websocket_header(char in_out_data[static WEBSOCKET_HEADER_SIZE_MAX],
                           ssize_t data_size)
{
    /* Refer to the websocket spec for how
     * this encoding works
     */
    int header_size = 0;
    // 1. Websocket FIN Header
    in_out_data[0] = 0x82;
    // 2. Weirdest part of the
    //    websocket spec (writing payload length).
    if (data_size < 126) {
        in_out_data[1] = (unsigned char)data_size & 0b01111111;
        header_size    = 2;
    }
    else {
        in_out_data[1] =
            (unsigned char)126 & 0b01111111; // 126 is a magic number for a
                                             // 16 bit payload length.
        // Network byte order (big endian)
        in_out_data[2] = (unsigned char)((data_size >> 8) & 0xFF);
        in_out_data[3] = (unsigned char)(data_size & 0xFF);
        header_size    = 4;
    }

    return header_size;
}

static int generate_accept_code(unsigned char *out_code,
                                char *in_code,
                                ssize_t code_len)
{
    const int code_max_len                = 20;
    char temp1[WEBSOCK_CODE_LEN]          = {0};
    unsigned char temp2[WEBSOCK_CODE_LEN] = {0};
    strncpy(temp1, in_code, code_len);
    // Constant string that's part of the websocket standard
    strcat(temp1, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    if (compute_sha1(temp1, strlen(temp1), temp2) < 0) {
        return -1;
    }
    base64_encode((char *)temp2, code_max_len, (char *)out_code);
#ifdef DEBUG
    printf("\n%s\n", out_code);
#endif
    return 0;
}

void send_web_socket_response(char *http_string,
                              ssize_t packet_size,
                              struct host *remotehost)
{
    char response[WEBSOCK_HEADERS_LEN] = {0};
    // We append the calculated hash to this
    // Then build the response
    const char *temp_response = "HTTP/1.1 101 Switching Protocols\n"
                                "Upgrade: websocket\n"
                                "Connection: Upgrade\n"
                                "Sec-WebSocket-Accept: ";
    const char *fin_response  = "\n\n";

    char received_code[WEBSOCK_CODE_LEN] = {0};
    char response_code[WEBSOCK_CODE_LEN] = {0};
    extract_key_code(received_code, http_string, packet_size);
    if (generate_accept_code((unsigned char *)response_code,
                             received_code,
                             WEBSOCK_CODE_LEN) < 0) {
        return;
    }

    strcpy(response, temp_response);
    strncat(response, response_code, WEBSOCK_CODE_LEN);
    strncat(response, fin_response, strlen(fin_response));
    send_data_tcp(response, strnlen(response, WEBSOCK_HEADERS_LEN), remotehost);
}

// TODO: move encryption to it's own file?
static void base64_encode(const char *input, int length, char *out_string)
{
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    memcpy(out_string, buffer_ptr->data, buffer_ptr->length);
    out_string[buffer_ptr->length] = '\0';

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
