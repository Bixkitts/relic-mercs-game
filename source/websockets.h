#ifndef BB_WEBSOCKETS
#define BB_WEBSOCKETS
#include "bbnetlib.h"

#define WEBSOCKET_HEADER_SIZE_MAX 8

void send_web_socket_response(char *http_string,
                              ssize_t packet_size,
                              struct host *remotehost);

int decode_websocket_message(char *out_data, char *in_data, ssize_t data_size);
// returns size of entire websocket packet including header
int write_websocket_header(char in_out_data[static WEBSOCKET_HEADER_SIZE_MAX],
                           ssize_t data_size);
#endif
