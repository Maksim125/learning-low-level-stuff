#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

struct ServerConfig
{
    int port;
    char address[256];
};

struct ServerConfig server;
struct sockaddr_in server_addr;

void *handle_client(void *arg);

int main(int argc, char *argv[])
{

    server.port = DEFAULT_PORT;

    if (argc > 1)
    {
        server.port = atoi(argv[1]);
    }

    strcpy(server.address, "127.0.0.1");

    server_addr.sin_family = AF_INET; // Use IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server.port);

    printf("Server will run on %s:%d\n", server.address, server.port);

    // bind socket to port
    int server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket failure");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Starting to listen\n");
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (*client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        printf("Accepted connection from client\n");

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_fd) != 0)
        {
            perror("Failed to create thread");
            free(client_fd);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

void *handle_client(void *arg)
{
    printf("Handling some stuff\n");
    int client_fd = *((int *)arg);
    free(arg); // Don't need this dude anymore, he's gone!

    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    if (buffer == NULL)
    {
        perror("Failed to allocate buffer");
        close(client_fd);
        return NULL;
    }

    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0'; // null terminating bytes are fun
        printf("Received: %s\n", buffer);

        const char *status_line = "HTTP/1.1 200 OK\r\n";
        const char *headers = "Content-Type: text/plain\r\nContent-Length: %d\r\n\r\n";
        const char *body = "<html><body><h1>Hey dude! :)</h1></body></html>";

        // Calculate the length of the body
        int body_length = strlen(body);
        // Calculate the length of the headers
        int headers_length = snprintf(NULL, 0, headers, body_length);

        char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
        size_t response_len;

        response_len = strlen(status_line) + headers_length + body_length;
        snprintf(response, response_len + 1, "%s%s%s", status_line, headers, body);

        if (response != NULL)
        {
            // Format the response
            snprintf(response, response_len + 1, "%sContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s",
                     status_line, body_length, body);

            // Send the response
            send(client_fd, response, response_len, 0);

            // Free the allocated memory for the response
            free(response);
        }
        else
        {
            perror("Failed to allocate memory for response");
        }
    }
    else if (bytes_read == 0)
    {
        printf("Client disconnected\n");
    }
    else
    {
        perror("Read failed");
    }

    free(buffer);
    close(client_fd);

    return NULL;
}
