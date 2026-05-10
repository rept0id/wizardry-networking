#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

#define CONST_PORT 8080
#define CONST_BUFFER_SIZE 1024
#define CONST_DEST1_ADDR "192.168.2.8"
#define CONST_DEST1_PORT "8080"
#define CONST_DEST2_ADDR "192.168.2.12"
#define CONST_DEST2_PORT "8080"

int isHealthyBackend(const char *addr) {
    char ping_cmd[100];

    snprintf(ping_cmd, sizeof(ping_cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", addr);

    return system(ping_cmd) == 0;
}

int connectBackend(const char *addr, int port) {
    int backendSocket;

    struct sockaddr_in backend_addr;

    /*** * * ***/

    backendSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (backendSocket < 0) {
        perror("socket");
        return -1;
    }

    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &backend_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(backendSocket);
        return -1;
    }
    if (connect(backendSocket, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
        perror("connect");
        close(backendSocket);
        return -1;
    }

    /*** * * ***/

    return backendSocket;
}

int main() {
    int err;

    int socketFileDescriptor;
    int socketOptions = 1;

    struct sockaddr_in socketAddress;
    int socketAddressSize = sizeof(socketAddress);

    int backendIndex = 0;

    fd_set forwardSet;

    /*** * * ***/

    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    err = setsockopt(socketFileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &socketOptions, sizeof(socketOptions));
    if (err) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(CONST_PORT);

    /*** * * ***/

    err = bind(socketFileDescriptor, (struct sockaddr *)&socketAddress, sizeof(socketAddress));
    if (err) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    err = listen(socketFileDescriptor, 3);
    if (err) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Load balancer listening on port %d...\n", CONST_PORT);

    while (1) {
        int newSocket;

        char buffer[CONST_BUFFER_SIZE] = {0};

        int backendSocket = -1;
        const char *backends[][2] = {
            {CONST_DEST1_ADDR, CONST_DEST1_PORT},
            {CONST_DEST2_ADDR, CONST_DEST2_PORT}
        };

        int backendsCount = sizeof(backends)/sizeof(backends[0]);

        /*** * * ***/

        newSocket = accept(socketFileDescriptor, (struct sockaddr *)&socketAddress, (socklen_t*)&socketAddressSize);
        if (newSocket < 0) {
            perror("accept");
            continue;
        }
        printf("Connection accepted\n");

        for (int i = 0; i < backendsCount; i++) {
            int idx = (backendIndex + i) % backendsCount;
            if (isHealthyBackend(backends[idx][0])) {
                backendSocket = connectBackend(backends[idx][0], atoi(backends[idx][1]));
                if (backendSocket >= 0) {
                    backendIndex = (idx + 1) % backendsCount;
                    break;
                }
            }
        }

        if (backendSocket < 0) {
            printf("No healthy backends available\n");
            close(newSocket);
            continue;
        }

        while (1) {
            int e;

            FD_ZERO(&forwardSet);
            FD_SET(newSocket, &forwardSet);
            FD_SET(backendSocket, &forwardSet);

            e = select(FD_SETSIZE, &forwardSet, NULL, NULL, NULL);
            if (e) {
                perror("select");

                break;
            }
            e = FD_ISSET(newSocket, &forwardSet);
            if (e) {
                ssize_t bytesRead;

                bytesRead = read(newSocket, buffer, CONST_BUFFER_SIZE);

                if (bytesRead <= 0) {
                    break;
                }

                send(backendSocket, buffer, bytesRead, 0);
            }
            e = FD_ISSET(backendSocket, &forwardSet);
            if (e) {
                ssize_t bytesRead;

                bytesRead = read(backendSocket, buffer, CONST_BUFFER_SIZE);

                if (bytesRead <= 0) {
                    break;
                }

                send(newSocket, buffer, bytesRead, 0);
            }
        }

        /*** * * ***/

        close(backendSocket);
        close(newSocket);
    }

    /*** * * ***/

    close(socketFileDescriptor);

    /*** * * ***/

    return 0;
}
