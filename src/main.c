#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main() {
	int server_fd = -1;
	int client_fd = -1;
	int return_status = 1; // Assume error unless we reach the end of the code

	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);


	int client_addr_len;
	struct sockaddr_in client_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) 
	{
		printf("Socket creation failed: %s...\n", strerror(errno));
		goto cleanup;
	}

	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) 
	{
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		goto cleanup;
	}

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
		.sin_port = htons(4221),
		.sin_addr = { htonl(INADDR_ANY) },
	};

	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) 
	{
		printf("Bind failed: %s \n", strerror(errno));
		goto cleanup;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) 
	{
		printf("Listen failed: %s \n", strerror(errno));
		goto cleanup;
	}

	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

	client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	if (client_fd == -1) \
	{
        	printf("Accept failed: %s\n", strerror(errno));
        	goto cleanup;
    	}

	printf("Client connected\n");

	char output[1028];
	ssize_t bytes_received = recv(client_fd, output, sizeof(output) - 1, 0);
	if (bytes_received == -1) 
	{
		printf("Error receiving request: %s...\n", strerror(errno));
		goto cleanup;
	}


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

	char reply[1024];
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

	if(send(client_fd, reply, strlen(reply), 0) == -1)
	{
		printf("Send failed: %s\n", strerror(errno));
        	goto cleanup;
	}

	return_status = 0;
	
	cleanup:
		if(client_fd != -1)
		{
			close(client_fd);
		}
		if(server_fd != -1)
		{
			close(server_fd);
		}

	return return_status;
}

