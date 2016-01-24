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
#include "string.h"

void rws_socket_inform_recvd_frames(_rws_socket * s)
{
	rws_bool is_all_finished = rws_true;
	_rws_frame * frame = NULL;
	_rws_node * cur = s->recvd_frames;
	while (cur)
	{
		frame = (_rws_frame *)cur->value.object;
		if (frame)
		{
			if (frame->is_finished)
			{
				switch (frame->opcode)
				{
					case rws_opcode_text_frame:
						if (s->on_recvd_text) s->on_recvd_text(s, (const char *)frame->data, (unsigned int)frame->data_size);
						break;
					case rws_opcode_binary_frame:
						if (s->on_recvd_bin) s->on_recvd_bin(s, frame->data, (unsigned int)frame->data_size);
						break;
					default: break;
				}
				rws_frame_delete(frame);
				cur->value.object = NULL;
			}
			else
			{
				is_all_finished = rws_false;
			}
		}
		cur = cur->next;
	}
	if (is_all_finished) rws_list_delete_clean(&s->recvd_frames);
}

void rws_socket_read_handshake_responce_value(const char * str, char ** value)
{
	const char * s = NULL;
	size_t len = 0;

	while (*str == ':' || *str == ' ') { str++; }
	s = str;
	while (*s != '\r' && *s != '\n') { s++; len++; }
	if (len > 0) *value = rws_string_copy_len(str, len);
}

rws_bool rws_socket_process_handshake_responce(_rws_socket * s)
{
	const char * str = (const char *)s->received;
	char * sub = NULL;
	float http_ver = -1;
	int http_code = -1;

	rws_error_delete_clean(&s->error);
	sub = strstr(str, "HTTP/");
	if (!sub) return rws_false;
	
	sub += 5;
	if (sscanf(sub, "%f %i", &http_ver, &http_code) != 2)
	{
		http_ver = -1; http_code = -1;
	}

	sub = strstr(str, k_rws_socket_sec_websocket_accept); // "Sec-WebSocket-Accept"
	if (sub)
	{
		sub += strlen(k_rws_socket_sec_websocket_accept);
		rws_socket_read_handshake_responce_value(sub, &s->sec_ws_accept);
	}

	if (http_code != 101 || !s->sec_ws_accept)
	{
		s->error = rws_error_new_code_descr(rws_error_code_parse_handshake,
											(http_code != 101) ? "HTPP code not found or non 101" : "Accept key not found");
		return rws_false;
	}
	return rws_true;
}

// need close socket on error
rws_bool rws_socket_send(_rws_socket * s, const void * data, const size_t data_size)
{
	rws_error_delete_clean(&s->error);
#if defined(RWS_OS_WINDOWS)
	send(s->socket, (const char*)data, data_size, 0);
#else
	send(s->socket, data, data_size, 0);
#endif
	return rws_true;
}

rws_bool rws_socket_recv(_rws_socket * s)
{
	int is_reading = 1, error_number = -1, len = -1;
	char * received = NULL;
	size_t total_len = 0;
	char buff[8192];

	rws_error_delete_clean(&s->error);
	s->received_len = 0;

	while (is_reading)
	{
		len = recv(s->socket, buff, 8192, 0);
		if (len > 0)
		{
			total_len += len;
			if (s->received_size < total_len)
			{
				rws_socket_resize_received(s, total_len);
			}

			received = (char *)s->received;
			if (s->received_len) received += s->received_len;

			memcpy(received, buff, len);
			s->received_len += len;
		}
		else
		{
			is_reading = 0;
		}
	}

	error_number = errno;
	if (error_number != EWOULDBLOCK && error_number != EAGAIN)
	{
		s->error = rws_error_new_code_descr(rws_error_code_read_from_socket, "Error read from socket");
		rws_socket_close(s);
		return rws_false;
	}
	return rws_true;
}

_rws_frame * rws_socket_last_unfin_recvd_frame_by_opcode(_rws_socket * s, const rws_opcode opcode)
{
	_rws_frame * last = NULL;
	_rws_frame * frame = NULL;
	_rws_node * cur = s->recvd_frames;
	while (cur)
	{
		frame = (_rws_frame *)cur->value.object;
		if (frame)
		{
			if (!frame->is_finished && frame->opcode == opcode) last = frame;
		}
		cur = cur->next;
	}
	return last;
}

void rws_socket_process_bin_or_text_frame(_rws_socket * s, _rws_frame * frame)
{
	_rws_frame * last_unfin = rws_socket_last_unfin_recvd_frame_by_opcode(s, frame->opcode);
	if (last_unfin)
	{
		rws_frame_combine_datas(last_unfin, frame);
		last_unfin->is_finished = frame->is_finished;
		rws_frame_delete(frame);
	}
	else
	{
		if (frame->data && frame->data_size) rws_socket_append_recvd_frames(s, frame);
		else rws_frame_delete(frame);
	}
}

void rws_socket_process_ping_frame(_rws_socket * s, _rws_frame * frame)
{
	_rws_frame * pong_frame = rws_frame_create();
	pong_frame->opcode = rws_opcode_pong;
	pong_frame->is_masked = rws_true;
	rws_frame_fill_with_send_data(pong_frame, frame->data, frame->data_size);
	rws_frame_delete(frame);
	rws_socket_append_send_frames(s, pong_frame);
}

void rws_socket_process_conn_close_frame(_rws_socket * s, _rws_frame * frame)
{
	s->command = COMMAND_INFORM_DISCONNECTED;
	s->error = rws_error_new_code_descr(rws_error_code_connection_closed, "Connection was closed by endpoint");
	rws_socket_close(s);
	rws_frame_delete(frame);
}

void rws_socket_process_received_frame(_rws_socket * s, _rws_frame * frame)
{
	switch (frame->opcode)
	{
		case rws_opcode_ping: rws_socket_process_ping_frame(s, frame); break;
		case rws_opcode_text_frame:
		case rws_opcode_binary_frame:
			rws_socket_process_bin_or_text_frame(s, frame);
			break;
		case rws_opcode_connection_close: rws_socket_process_conn_close_frame(s, frame); break;
		default:
			// unprocessed => delete
			rws_frame_delete(frame);
			break;
	}
}

void rws_socket_idle_recv(_rws_socket * s)
{
	_rws_frame * frame = NULL;

	if (!rws_socket_recv(s))
	{
		// sock already closed
		if (s->error) s->command = COMMAND_INFORM_DISCONNECTED;
		return;
	}

	frame = rws_frame_create_with_recv_data(s->received, s->received_len);
	if (frame) rws_socket_process_received_frame(s, frame);
}

void rws_socket_idle_send(_rws_socket * s)
{
	_rws_node * cur = NULL;
	rws_bool sending = rws_true;
	_rws_frame * frame = NULL;

	rws_socket_send_lock(s)
	cur = s->send_frames;
	if (cur)
	{
		while (cur && s->is_connected && sending)
		{
			frame = (_rws_frame *)cur->value.object;
			cur->value.object = NULL;
			if (frame) sending = rws_socket_send(s, frame->data, frame->data_size);
			rws_frame_delete(frame);
			cur = cur->next;
		}
		rws_list_delete_clean(&s->send_frames);
		if (s->error) s->command = COMMAND_INFORM_DISCONNECTED;
	}
	rws_socket_send_unlock(s)
}

void rws_socket_wait_handshake_responce(_rws_socket * s)
{
	if (!rws_socket_recv(s))
	{
		// sock already closed
		if (s->error) s->command = COMMAND_INFORM_DISCONNECTED;
		return;
	}
	if (s->received_len == 0) return;

	if (rws_socket_process_handshake_responce(s))
	{
		s->is_connected = rws_true;
		s->command = COMMAND_INFORM_CONNECTED;
	}
	else
	{
		rws_socket_close(s);
		s->command = COMMAND_INFORM_DISCONNECTED;
	}
}

void rws_socket_send_handshake(_rws_socket * s)
{
	char buff[512];
	char * ptr = buff;
	ptr += sprintf(ptr, "GET %s HTTP/%s\r\n", s->path, k_rws_socket_min_http_ver);

//	ptr += sprintf(ptr, "Host: %s\r\n", s->host);

	if (s->port == 80) ptr += sprintf(ptr, "Host: %s\r\n", s->host);
	else ptr += sprintf(ptr, "Host: %s:%i\r\n", s->host, s->port);

	ptr += sprintf(ptr, "Upgrade: websocket\r\n");
	ptr += sprintf(ptr, "Connection: Upgrade\r\n");
	ptr += sprintf(ptr, "Origin: %s://%s\r\n", s->scheme, s->host);
	ptr += sprintf(ptr, "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n");
	ptr += sprintf(ptr, "Sec-WebSocket-Protocol: chat, superchat\r\n");
	ptr += sprintf(ptr, "Sec-WebSocket-Version: 13\r\n");
	ptr += sprintf(ptr, "\r\n");

	if (rws_socket_send(s, buff, strlen(buff)))
	{
		s->command = COMMAND_WAIT_HANDSHAKE_RESPONCE;
	}
	else
	{
		if (s->error) s->error->code = rws_error_code_send_handshake;
		else s->error = rws_error_new_code_descr(rws_error_code_send_handshake, "Send handshake");
		rws_socket_close(s);
		s->command = COMMAND_INFORM_DISCONNECTED;
	}
}

void rws_socket_connect_to_host(_rws_socket * s)
{
	struct addrinfo hints;
	char portstr[16];
	struct addrinfo * result = NULL;
	int ret = -1;
	struct addrinfo * p = NULL;
	rws_socket_t sock = RWS_INVALID_SOCKET;
#if defined(RWS_OS_WINDOWS)
	unsigned long iMode = 0;
#endif

	memset(&hints, 0, sizeof(hints));
	
	rws_error_delete_clean(&s->error);
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	sprintf(portstr, "%i", s->port);
	ret = getaddrinfo(s->host, portstr, &hints, &result);
	if (ret != 0 || !result)
	{
		s->error = rws_error_new_code_descr(rws_error_code_connect_to_host, gai_strerror(ret));
		s->command = COMMAND_INFORM_DISCONNECTED;
		return;
	}

	p = result;
	while (p != NULL)
	{
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock != RWS_INVALID_SOCKET)
		{
			if (connect(sock, p->ai_addr, p->ai_addrlen) == 0)
			{
				s->socket = sock;
				rws_socket_set_receive_buffer_size(s, 256*1024);
#if defined(RWS_OS_WINDOWS)
				// If iMode != 0, non-blocking mode is enabled.
				iMode = 1;
				ioctlsocket(s->socket, FIONBIO, &iMode);
#else
				fcntl(s->socket, F_SETFL, O_NONBLOCK);
#endif
				break;
			}
#if defined(RWS_OS_WINDOWS)
			closesocket(sock);
#else
			close(sock);
#endif
		}
		p = p->ai_next;
	}

	freeaddrinfo(result);

	if (s->socket == RWS_INVALID_SOCKET)
	{
		s->error = rws_error_new_code_descr(rws_error_code_connect_to_host, "Cant connect to host");
		s->command = COMMAND_INFORM_DISCONNECTED;
	}
	else
	{
		s->command = COMMAND_SEND_HANDSHAKE;
	}
}

static void rws_socket_work_th_func(void * user_object)
{
	_rws_socket * s = (_rws_socket *)user_object;
	while (s->command < COMMAND_END)
	{
		rws_socket_work_lock(s)
		switch (s->command)
		{
			case COMMAND_CONNECT_TO_HOST: rws_socket_connect_to_host(s); break;
			case COMMAND_SEND_HANDSHAKE: rws_socket_send_handshake(s); break;
			case COMMAND_WAIT_HANDSHAKE_RESPONCE: rws_socket_wait_handshake_responce(s); break;
			case COMMAND_IDLE:
				if (s->is_connected) rws_socket_idle_recv(s);
				if (s->is_connected) rws_socket_idle_send(s);
				break;
			default: break;
		}
		rws_socket_work_unlock(s)
		switch (s->command)
		{
			case COMMAND_INFORM_CONNECTED:
				s->command = COMMAND_IDLE;
				if (s->on_connected) s->on_connected(s);
				break;
			case COMMAND_INFORM_DISCONNECTED:
				s->command = COMMAND_END;
				assert(s->socket == RWS_INVALID_SOCKET); //don't forget close socket
				rws_socket_cleanup_session_data(s);
				if (s->on_disconnected) s->on_disconnected(s);
				break;
			case COMMAND_IDLE:
				if (s->recvd_frames) rws_socket_inform_recvd_frames(s);
				break;
			default: break;
		}
		rws_thread_sleep(5);
	}
}

rws_bool rws_socket_create_start_work_thread(_rws_socket * s)
{
	rws_error_delete_clean(&s->error);
	s->command = COMMAND_NONE;
	s->work_thread = rws_thread_create(&rws_socket_work_th_func, s);
	if (s->work_thread)
	{
		s->command = COMMAND_CONNECT_TO_HOST;
		return rws_true;
	}
	return rws_false;
}

void rws_socket_resize_received(_rws_socket * s, const size_t size)
{
	void * res = NULL;
	size_t min = 0;
	if (size == s->received_size) return;

	res = rws_malloc(size);
	assert(res && (size > 0));

	min = (s->received_size < size) ? s->received_size : size;
	if (min > 0 && s->received) memcpy(res, s->received, min);
	rws_free_clean(&s->received);
	s->received = res;
	s->received_size = size;
}

void rws_socket_close(_rws_socket * s)
{
	if (s->socket != RWS_INVALID_SOCKET)
	{
#if defined(RWS_OS_WINDOWS)
		closesocket(s->socket);
#else
		close(s->socket);
#endif
		s->socket = RWS_INVALID_SOCKET;
		s->is_connected = rws_false;
	}
}

void rws_socket_append_recvd_frames(_rws_socket * s, _rws_frame * frame)
{
	_rws_node_value frame_list_var;
	frame_list_var.object = frame;
	if (s->recvd_frames)
	{
		rws_list_append(s->recvd_frames, frame_list_var);
	}
	else
	{
		s->recvd_frames = rws_list_create();
		s->recvd_frames->value = frame_list_var;
	}
}

void rws_socket_append_send_frames(_rws_socket * s, _rws_frame * frame)
{
	_rws_node_value frame_list_var;
	frame_list_var.object = frame;
	if (s->send_frames)
	{
		rws_list_append(s->send_frames, frame_list_var);
	}
	else
	{
		s->send_frames = rws_list_create();
		s->send_frames->value = frame_list_var;
	}
}

rws_bool rws_socket_send_text_priv(_rws_socket * s, const char * text)
{
	size_t len = text ? strlen(text) : 0;
	_rws_frame * frame = NULL;

	if (len <= 0) return rws_false;

	frame = rws_frame_create();
	frame->is_masked = rws_true;
	frame->opcode = rws_opcode_text_frame;
	rws_frame_fill_with_send_data(frame, text, len);
	rws_socket_append_send_frames(s, frame);

	return rws_true;
}

void rws_socket_delete_all_frames_in_list(_rws_list * list_with_frames)
{
	_rws_frame * frame = NULL;
	_rws_node * cur = list_with_frames;
	while (cur)
	{
		frame = (_rws_frame *)cur->value.object;
		if (frame) rws_frame_delete(frame);
		cur->value.object = NULL;
	}
}

void rws_socket_cleanup_session_data(_rws_socket * s)
{
	rws_string_delete_clean(&s->sec_ws_accept);
	s->work_thread = NULL;

	rws_free_clean(&s->received);
	s->received_size = 0;
	s->received_len = 0;

	rws_socket_delete_all_frames_in_list(s->send_frames);
	rws_list_delete_clean(&s->send_frames);
	rws_socket_delete_all_frames_in_list(s->recvd_frames);
	rws_list_delete_clean(&s->recvd_frames);
}

