
#import "ViewController.h"
#include "librws.h"
#include "frame.h"
#include "list.h"

static rws_socket _socket = NULL;

static void on_socket_received_text(rws_socket socket, const char * text, const unsigned int length)
{
	char buff[8*1024];
	memcpy(buff, text, length);
	buff[length] = 0;

	printf("\nSocket text: <%s>", buff);
}

static void on_socket_received_bin(rws_socket socket, const void * data, const unsigned int length)
{
	char buff[8*1024];
	memcpy(buff, data, length);
	buff[length] = 0;

	printf("\nSocket bin: <%s>", buff);
}

static void on_socket_connected(rws_socket socket)
{
	printf("\n socket connected");

	const char * s =
	"{\"version\":\"1.0\",\"supportedConnectionTypes\":[\"websocket\"],\"minimumVersion\":\"1.0\",\"channel\":\"/meta/handshake\"}";

	rws_socket_send_text(socket, s);
}

static void on_socket_disconnected(rws_socket socket)
{
	rws_error error = rws_socket_get_error(socket);
	if (error)
	{
		printf("\nsocket disconnect with code, error: %i, %s",
			   rws_error_get_code(error),
			   rws_error_get_description(error));
	}
}

void rws_thread_funct1(void * user_object)
{
	printf("\n THREAD 1");
}
void rws_thread_funct2(void * user_object)
{
	printf("\n THREAD 2");
}
void rws_thread_funct3(void * user_object)
{
	printf("\n THREAD 3");
}
void rws_thread_funct4(void * user_object)
{
	printf("\n THREAD 4");
}


static void rws_test()
{
	rws_thread_create(&rws_thread_funct1, NULL);
	rws_thread_create(&rws_thread_funct2, NULL);
	rws_thread_create(&rws_thread_funct3, NULL);
	rws_thread_create(&rws_thread_funct4, NULL);

	rws_socket socket = rws_socket_create();
	assert(socket);
	_socket = socket;

	const char * scheme = "ws";
	const char * host = "echo.websocket.org";
	const char * path = "/";
	int port = 80;

	scheme = "ws";
	host = "messages.presentain.com";
	path = "/faye";
	port = 80;

//	scheme = "wss";
//	host = "echo.websocket.org";
//	path = "/";
//	port = 443;


	rws_socket_set_scheme(socket, scheme);								printf("%i\n", (int)__LINE__);
	assert(strcmp(rws_socket_get_scheme(socket), scheme) == 0);			printf("%i\n", (int)__LINE__);

	rws_socket_set_host(socket, host);									printf("%i\n", (int)__LINE__);
	assert(strcmp(rws_socket_get_host(socket), host) == 0);				printf("%i\n", (int)__LINE__);

	rws_socket_set_path(socket, path);									printf("%i\n", (int)__LINE__);
	assert(strcmp(rws_socket_get_path(socket), path) == 0);				printf("%i\n", (int)__LINE__);

	rws_socket_set_port(socket, port);									printf("%i\n", (int)__LINE__);
	assert(rws_socket_get_port(socket) == port);						printf("%i\n", (int)__LINE__);

	rws_socket_set_on_disconnected(socket, &on_socket_disconnected);
	rws_socket_set_on_connected(socket, &on_socket_connected);
	rws_socket_set_on_received_text(socket, &on_socket_received_text);
	rws_socket_set_on_received_bin(socket, &on_socket_received_bin);

	rws_socket_connect(socket);
}


@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad
{
	[super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
	rws_test();
}

- (void)didReceiveMemoryWarning
{
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

@end
