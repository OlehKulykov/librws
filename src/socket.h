/*
 *   Copyright (c) 2014 - 2016 Kulykov Oleh <info@resident.name>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */


#ifndef __SOCKET_H__
#define __SOCKET_H__ 1

#include "common.h"

#if defined(RWS_OS_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <errno.h>

#include "error.h"
#include "thread.h"
#include "frame.h"
#include "list.h"

#if defined(RWS_OS_WINDOWS)
typedef SOCKET rws_socket_t;
#define RWS_INVALID_SOCKET INVALID_SOCKET
#else
typedef int rws_socket_t;
#define RWS_INVALID_SOCKET -1
#endif

static const char * k_rws_socket_min_http_ver = "1.1";
static const char * k_rws_socket_sec_websocket_accept = "Sec-WebSocket-Accept";

typedef struct _rws_socket_struct
{
	int port;
	rws_socket_t socket;
	char * scheme;
	char * host;
	char * path;

	char * sec_ws_accept; // "Sec-WebSocket-Accept" field from handshake

	rws_thread work_thread;

	int command;

	rws_bool is_connected; // sock connected + handshake done

	void * user_object;
	rws_on_socket on_connected;
	rws_on_socket on_disconnected;
	rws_on_socket_recvd_text on_recvd_text;
	rws_on_socket_recvd_bin on_recvd_bin;

	void * received;
	size_t received_size; // size of 'received' memory
	size_t received_len; // length of actualy readed message

	_rws_list * send_frames;
	_rws_list * recvd_frames;

	_rws_error * error;

#if defined(RWS_THREAD_SAFE)
	rws_mutex work_mutex;
	rws_mutex send_mutex;
#endif

} _rws_socket;

rws_bool rws_socket_process_handshake_responce(_rws_socket * s);

// receive raw data from socket
rws_bool rws_socket_recv(_rws_socket * s);

// send raw data to socket
rws_bool rws_socket_send(_rws_socket * s, const void * data, const size_t data_size);

_rws_frame * rws_socket_last_unfin_recvd_frame_by_opcode(_rws_socket * s, const rws_opcode opcode);

void rws_socket_process_bin_or_text_frame(_rws_socket * s, _rws_frame * frame);

void rws_socket_process_ping_frame(_rws_socket * s, _rws_frame * frame);

void rws_socket_process_received_frame(_rws_socket * s, _rws_frame * frame);

void rws_socket_idle_recv(_rws_socket * s);

void rws_socket_idle_send(_rws_socket * s);

void rws_socket_wait_handshake_responce(_rws_socket * s);

void rws_socket_send_handshake(_rws_socket * s);

void rws_socket_connect_to_host(_rws_socket * s);

rws_bool rws_socket_create_start_work_thread(_rws_socket * s);

void rws_socket_close(_rws_socket * s);

void rws_socket_resize_received(_rws_socket * s, const size_t size);

void rws_socket_append_recvd_frames(_rws_socket * s, _rws_frame * frame);

void rws_socket_append_send_frames(_rws_socket * s, _rws_frame * frame);

rws_bool rws_socket_send_text_priv(_rws_socket * s, const char * text);

void rws_socket_inform_recvd_frames(_rws_socket * s);

void rws_socket_delete_all_frames_in_list(_rws_list * list_with_frames);

// delete all created, allocated data during work session
void rws_socket_cleanup_session_data(_rws_socket * s);


#if defined(RWS_THREAD_SAFE)
#define rws_socket_work_lock(s) rws_mutex_lock(s->work_mutex);
#define rws_socket_work_unlock(s) rws_mutex_unlock(s->work_mutex);
#define rws_socket_send_lock(s) rws_mutex_lock(s->send_mutex);
#define rws_socket_send_unlock(s) rws_mutex_unlock(s->send_mutex);
#else
#define rws_socket_work_lock(s)
#define rws_socket_work_unlock(s)
#define rws_socket_send_lock(s)
#define rws_socket_send_unlock(s)
#endif



#define COMMAND_IDLE -1
#define COMMAND_NONE 0
#define COMMAND_CONNECT_TO_HOST 1
#define COMMAND_SEND_HANDSHAKE 2
#define COMMAND_WAIT_HANDSHAKE_RESPONCE 3
#define COMMAND_INFORM_CONNECTED 4
#define COMMAND_INFORM_DISCONNECTED 5


#define COMMAND_END 9999



#endif
