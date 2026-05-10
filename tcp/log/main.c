#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CONST_PORT 8080
#define CONST_BUFFER_SIZE 1024

int main() {
    int err;

    int socketFileDescriptor;
    int socketOptions = 1;

    struct sockaddr_in socketAddress;
    int socketAddressSize = sizeof(socketAddress);

    /*** * * ***/

    // Socket

    // Socket : file descriptor
    socketFileDescriptor = socket(
        AF_INET, // __domain
        SOCK_STREAM, // __type
        0 // __protocol
    );
    if (socketFileDescriptor == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Socket : options
    err = setsockopt(
        socketFileDescriptor, // __fd
        SOL_SOCKET, // __level
        SO_REUSEADDR | SO_REUSEPORT, // __optname
        &socketOptions, // __optval
        sizeof(socketOptions) // __optlen
    );
    if (err) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Socket : address
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(CONST_PORT);

    /*** * * ***/

    // Bind
    err = bind(
        socketFileDescriptor, // __fd
        (struct sockaddr *)&socketAddress, // __addr
        sizeof(socketAddress) // __len
    );
    if (err) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    err = listen(socketFileDescriptor, 3);
    if (err) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d...\n", CONST_PORT);

    while (1) {
        int newSocket;

        ssize_t bytesRead;
        char buffer[CONST_BUFFER_SIZE] = {0};

        /*** * * ***/

        // Accept
        newSocket = accept(
            socketFileDescriptor, // __fd
            (struct sockaddr *)&socketAddress, // __addr
            (socklen_t*)&socketAddressSize // __addr_len
        );
        if (newSocket < 0) {
            perror("accept");

            continue;
        }
        printf("Connection accepted\n");

        // Read
        bytesRead = read(newSocket, buffer, CONST_BUFFER_SIZE);
        if (bytesRead < 0) {
            perror("read");

            continue;
        }
        printf("Received: %s\n", buffer);

        /*** * * ***/

        close(newSocket);
    }

    /*** * * ***/

    close(socketFileDescriptor);

    /*** * * ***/

    return 0;
}
