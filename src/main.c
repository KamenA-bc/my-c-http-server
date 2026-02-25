#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

	void *parsing_worker(void *args);

int main() {
	int server_fd = -1;
	int *client_fd_ptr;
	int return_status = 1; // Assume error, unless we reach the end of the code

	pthread_t client_connection;

	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);


	int client_addr_len;
	struct sockaddr_in client_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
		.sin_port = htons(4221),
		.sin_addr = { htonl(INADDR_ANY) },
	};

	bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	int connection_backlog = 5;
	listen(server_fd, connection_backlog);



	printf("Waiting for a client to connect...\n");

	while(1) 
	{
		client_addr_len = sizeof(client_addr);
		client_fd_ptr = malloc(sizeof(int));
		*client_fd_ptr = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		pthread_create(&client_connection, NULL, parsing_worker, client_fd_ptr);


	}

	close(server_fd);

	return 0;
}

void *parsing_worker(void *args)
{
	int client_fd = *((int *) args);
	pthread_detach(pthread_self());
	free(args);

	char output[BUFFER_SIZE];
	ssize_t bytes_received = recv(client_fd, output, sizeof(output) - 1, 0);


	char *user_agent = strstr(output, "User-Agent");
	if(user_agent)
	{
		user_agent += 12;

		char *user_end = strstr(user_agent, "\r\n");

		if(user_end)
		{
			*user_end = '\0';
		}
	}


	char *path = strstr(output, " ");
	if (path) 
	{

		path++;

		char *path_end = strstr(path, " ");

		if (path_end) 
		{

			*path_end = '\0';

		}

	}

	char reply[BUFFER_SIZE];
	if(path && strcmp(path, "/") == 0)
	{
		strcpy(reply, "HTTP/1.1 200 OK\r\n\r\n");
	}else if(path && strncmp(path, "/echo/", 6) == 0)
	{
		path += 6;
		sprintf(reply, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(path), path) ;

	}else if(path && strcmp(path, "/user-agent") == 0) 
	{

		sprintf(reply, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(user_agent), user_agent) ;

	}else 
	{

		strcpy(reply, "HTTP/1.1 404 Not Found\r\n\r\n");

	}

	send(client_fd, reply, strlen(reply), 0);

	close(client_fd);
}

