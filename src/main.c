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
char *base_directory = NULL;

int main(int argc, char **argv) {

	for(int i = 0; i<argc; i++)
	{
		if(strcmp(argc[i], "--directory") == 0)
		{
			if(i + 1 <argc)
			{
				base_directory = argv[i + 1];
				break;
			}
		}
	}

	int *client_fd_ptr;
	int return_status = 1; //Assume there was an error unless we reach the end of the code;

	pthread_t client_connection;

	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);


	int client_addr_len;
	struct sockaddr_in client_addr;

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd == -1)
	{
		printf("Socket creating failed: %s...\n", strerror(errno));
		goto end;
	}

	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
	{
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		goto cleanup;
	}

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
		.sin_port = htons(4221),
		.sin_addr = { htonl(INADDR_ANY) },
	};

	if(bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
	{
		printf("Bind failed: %s \n", strerror(errno));
		goto cleanup;
	}

	int connection_backlog = 5;
	if(listen(server_fd, connection_backlog) == -1)
	{
		printf("Listen failed: %s \n", strerror(errno));
		goto cleanup;
	}



	printf("Waiting for a client to connect...\n");

	while(1) 
	{
		client_addr_len = sizeof(client_addr);
		client_fd_ptr = malloc(sizeof(int));
		*client_fd_ptr = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

		if(*client_fd_ptr == -1)
		{
			printf("Accept failed: %s \n", strerror(errno));
			free(client_fd_ptr);
			continue;
		}

		if(pthread_create(&client_connection, NULL, parsing_worker, client_fd_ptr) != 0)
		{
			printf("Failed to creat thread!\n");
			close(*client_fd_ptr);
			free(client_fd_ptr);
			continue;
		}


	}

	return_status = 0;

cleanup:
	close(server_fd);
end:
	return return_status;
}


void *parsing_worker(void *args)
{
	int client_fd = *((int *) args);
	pthread_detach(pthread_self());
	free(args);

	char output[BUFFER_SIZE];
	ssize_t bytes_received = recv(client_fd, output, sizeof(output) - 1, 0);
	if(bytes_received == -1)
	{
		printf("Error receiving request: %s...\n", strerror(errno));
		goto cleanup;
	}
	output[bytes_received] = '\0';

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

	char *reply;
	if(path && strcmp(path, "/") == 0)
	{
		strcpy(reply, "HTTP/1.1 200 OK\r\n\r\n");
	}else if(path && strncmp(path, "/echo/", 6) == 0)
	{
		path += 6;
		sprintf(reply, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(path), path) ;

	}else if(path && strncmp(path, "/files/", 7) == 0)
	{
		path += 7;

		char full_path[BUFFER_SIZE];
		if (base_directory != NULL) {
			snprintf(full_path, sizeof(full_path), "%s/%s", base_directory, path);
		} else {
			snprintf(full_path, sizeof(full_path), "%s", path);
		}

		if(access(full_path, F_OK) == 0)
		{
			FILE *fp = fopen(path, "rb");
			fseek(fp, 0, SEEK_END);
			int res = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char file_buffer[res + 1];
			size_t bytes_read = fread(file_buffer, 1, res, fp);
			file_buffer[bytes_read] = '\0';
			fclose(fp);

			snprintf(reply, sizeof(reply), "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n%s", res, file_buffer);

		}else
		{
			sprintf(reply, "HTTP/1.1 404 Not Found\r\n\r\n");
		}
	}else if(path && strcmp(path, "/user-agent") == 0) 
	{
		if(user_agent != NULL){
			sprintf(reply, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(user_agent), user_agent) ;
		}

	}else 
	{

		strcpy(reply, "HTTP/1.1 404 Not Found\r\n\r\n");

	}

	if(send(client_fd, reply, strlen(reply), 0) == -1)
	{
		printf("Send failed: %s \n", strerror(errno));
		goto cleanup;
	}

cleanup:
	close(client_fd);
}
