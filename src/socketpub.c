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


#include "../librws.h"
#include "socket.h"
#include "memory.h"

// public
rws_bool rws_socket_connect(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (!s) return rws_false;
	if (s->port <= 0 || !s->scheme || !s->host || !s->path) return rws_false;
	return rws_socket_create_start_work_thread(s);
}

rws_bool rws_socket_send_text(rws_socket socket, const char * text)
{
	_rws_socket * s = (_rws_socket *)socket;
	rws_bool r = rws_false;
	if (!s) return r;
	rws_socket_send_lock(s)
	r = rws_socket_send_text_priv(s, text);
	rws_socket_send_unlock(s)
	return r;
}

rws_socket rws_socket_create(void)
{
	_rws_socket * s = (_rws_socket *)rws_malloc_zero(sizeof(_rws_socket));
	if (!s) return NULL;

	s->port = -1;
	s->socket = RWS_INVALID_SOCKET;
	s->command = COMMAND_NONE;

#if defined(RWS_THREAD_SAFE)
	s->work_mutex = rws_mutex_create_recursive();
	s->send_mutex = rws_mutex_create_recursive();
#endif

	return s;
}

void rws_socket_delete(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (!s) return;

	rws_socket_close(s);

	rws_string_delete(s->scheme);
	rws_string_delete(s->host);
	rws_string_delete(s->path);

	rws_string_delete(s->sec_ws_accept);

	rws_error_delete(s->error);

	rws_free(s->received);
	rws_socket_delete_all_frames_in_list(s->send_frames);
	rws_list_delete_clean(&s->send_frames);
	rws_socket_delete_all_frames_in_list(s->recvd_frames);
	rws_list_delete_clean(&s->recvd_frames);

#if defined(RWS_THREAD_SAFE)
	rws_mutex_delete(s->work_mutex);
	rws_mutex_delete(s->send_mutex);
#endif

	rws_free(s);
}

void rws_socket_set_scheme(rws_socket socket, const char * scheme)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (!s) return;
	rws_string_delete_clean(&s->scheme);
	if (s) s->scheme = rws_string_copy(scheme);
}

const char * rws_socket_get_scheme(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->scheme : NULL;
}

void rws_socket_set_host(rws_socket socket, const char * host)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (!s) return;
	rws_string_delete_clean(&s->host);
	if (s) s->host = rws_string_copy(host);
}

const char * rws_socket_get_host(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->host : NULL;
}

void rws_socket_set_path(rws_socket socket, const char * path)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (!s) return;
	rws_string_delete_clean(&s->path);
	if (s) s->path = rws_string_copy(path);
}

const char * rws_socket_get_path(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->path : NULL;
}

void rws_socket_set_port(rws_socket socket, const int port)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (s) s->port = port;
}

int rws_socket_get_port(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->port : -1;
}

void rws_socket_set_receive_buffer_size(rws_socket socket, const unsigned int size)
{
	_rws_socket * s = (_rws_socket *)socket;
	unsigned int buffSize = size;
	int res = 0;
	if (!s) return;
	if (s->socket == RWS_INVALID_SOCKET) return;

	res = setsockopt(s->socket, SOL_SOCKET, SO_RCVBUF, (char *)&buffSize, sizeof(unsigned int));
	if (res == 0) return;
}

unsigned int _rws_socket_get_receive_buffer_size(rws_socket_t socket)
{
	unsigned int size = 0;
#if defined(RWS_OS_WINDOWS)
	int len = sizeof(unsigned int);
	if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&size, &len) == -1) size = 0;
#else
	socklen_t len = sizeof(unsigned int);
	if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &size, &len) == -1) size = 0;
#endif
	return size;
}

unsigned int rws_socket_get_receive_buffer_size(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (!s) return 0;
	if (s->socket == RWS_INVALID_SOCKET) return 0;
	return _rws_socket_get_receive_buffer_size(s->socket);
}

rws_error rws_socket_get_error(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->error : NULL;
}

void rws_socket_set_user_object(rws_socket socket, void * user_object)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (s) s->user_object = user_object;
}

void * rws_socket_get_user_object(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->user_object : NULL;
}

void rws_socket_set_on_connected(rws_socket socket, rws_on_socket callback)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (s) s->on_connected = callback;
}

void rws_socket_set_on_disconnected(rws_socket socket, rws_on_socket callback)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (s) s->on_disconnected = callback;
}

void rws_socket_set_on_received_text(rws_socket socket, rws_on_socket_recvd_text callback)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (s) s->on_recvd_text = callback;
}

void rws_socket_set_on_received_bin(rws_socket socket, rws_on_socket_recvd_bin callback)
{
	_rws_socket * s = (_rws_socket *)socket;
	if (s) s->on_recvd_bin = callback;
}

rws_bool rws_socket_is_connected(rws_socket socket)
{
	_rws_socket * s = (_rws_socket *)socket;
	return s ? s->is_connected : rws_false;
}

