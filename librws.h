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


#ifndef __LIBRWS_H__
#define __LIBRWS_H__ 1

#include <stdio.h>

// check windows
#if defined(WIN32) || defined(_WIN32) || defined(WIN32_LEAN_AND_MEAN) || defined(_WIN64) || defined(WIN64)
#define RWS_OS_WINDOWS 1
#endif


// extern
#if defined(__cplusplus) || defined(_cplusplus)
#define RWS_EXTERN extern "C"
#else
#define RWS_EXTERN extern
#endif


// attribute
#if defined(__GNUC__)
#if (__GNUC__ >= 4)
#if defined(__cplusplus) || defined(_cplusplus)
#define RWS_ATTRIB __attribute__((visibility("default")))
#else
#define RWS_ATTRIB __attribute__((visibility("default")))
#endif
#endif
#endif


// check attrib and define empty if not defined
#if !defined(RWS_ATTRIB)
#define RWS_ATTRIB
#endif


// dll api
#if defined(RWS_OS_WINDOWS)
#if defined(RWS_BUILD)
#define RWS_DYLIB_API __declspec(dllexport)
#else
#define RWS_DYLIB_API __declspec(dllimport)
#endif
#endif


// check dll api and define empty if not defined
#if !defined(RWS_DYLIB_API)
#define RWS_DYLIB_API
#endif


// combined lib api
#define RWS_API(return_type) RWS_EXTERN RWS_ATTRIB RWS_DYLIB_API return_type


// types

/**
 @brief Boolean type.
 */
typedef unsigned char rws_bool;
#define rws_true 1
#define rws_false 0

/**
 @brief Type of all public objects.
 */
typedef void* rws_handle;

/**
 @brief Socket handle.
 */
typedef rws_handle rws_socket;

/**
 @brief Error object handle.
 */
typedef rws_handle rws_error;

typedef rws_handle rws_mutex;

typedef rws_handle rws_thread;

typedef void (*rws_thread_funct)(void * user_object);

typedef void (*rws_on_socket)(rws_socket socket);

typedef void (*rws_on_socket_recvd_text)(rws_socket socket, const char * text, const unsigned int length);

typedef void (*rws_on_socket_recvd_bin)(rws_socket socket, const void * text, const unsigned int length);

// socket

/**
 @brief Create new socket.
 @return Socket handler or NULL on error.
 */
RWS_API(rws_socket) rws_socket_create(void);

RWS_API(void) rws_socket_delete(rws_socket socket);

RWS_API(void) rws_socket_set_scheme(rws_socket socket, const char * scheme);
RWS_API(const char *) rws_socket_get_scheme(rws_socket socket);

RWS_API(void) rws_socket_set_host(rws_socket socket, const char * host);
RWS_API(const char *) rws_socket_get_host(rws_socket socket);

RWS_API(void) rws_socket_set_path(rws_socket socket, const char * path);
RWS_API(const char *) rws_socket_get_path(rws_socket socket);

RWS_API(void) rws_socket_set_port(rws_socket socket, const int port);
RWS_API(int) rws_socket_get_port(rws_socket socket);

RWS_API(void) rws_socket_set_receive_buffer_size(rws_socket socket, const unsigned int size);
RWS_API(unsigned int) rws_socket_get_receive_buffer_size(rws_socket socket);

RWS_API(rws_error) rws_socket_get_error(rws_socket socket);

/**
 @brief Start connection.
 @return rws_true - all params exists and connection started, otherwice rws_false
 */
RWS_API(rws_bool) rws_socket_connect(rws_socket socket);

/**
 @brief Check is socket has connection to host and handshake(sucessfully done).
 @return trw_true - connected to host and handshacked, otherwice rws_false.
 */
RWS_API(rws_bool) rws_socket_is_connected(rws_socket socket);

RWS_API(rws_bool) rws_socket_send_text(rws_socket socket, const char * text);

RWS_API(void) rws_socket_set_user_object(rws_socket socket, void * user_object);

RWS_API(void *) rws_socket_get_user_object(rws_socket socket);

RWS_API(void) rws_socket_set_on_connected(rws_socket socket, rws_on_socket callback);

RWS_API(void) rws_socket_set_on_disconnected(rws_socket socket, rws_on_socket callback);

RWS_API(void) rws_socket_set_on_received_text(rws_socket socket, rws_on_socket_recvd_text callback);

RWS_API(void) rws_socket_set_on_received_bin(rws_socket socket, rws_on_socket_recvd_bin callback);


// error

typedef enum _rws_error_code
{
	rws_error_code_none = 0,

	rws_error_code_send_handshake,
	rws_error_code_parse_handshake,
	rws_error_code_read_from_socket,
	rws_error_code_connect_to_host
} rws_error_code;

/**
 @return 0 - if error is empty or no error, otherwice error code.
 */
RWS_API(int) rws_error_get_code(rws_error error);


/**
 @return 0 - if error is empty or no error, otherwice HTTP error.
 */
RWS_API(int) rws_error_get_http_error(rws_error error);


RWS_API(const char *) rws_error_get_description(rws_error error);


// mutex

RWS_API(rws_mutex) rws_mutex_create_recursive(void);

RWS_API(void) rws_mutex_lock(rws_mutex mutex);

RWS_API(void) rws_mutex_unlock(rws_mutex mutex);

RWS_API(void) rws_mutex_delete(rws_mutex mutex);


// thread

RWS_API(rws_thread) rws_thread_create(rws_thread_funct thread_function, void * user_object);

RWS_API(void) rws_thread_sleep(const unsigned int millisec);

#endif
