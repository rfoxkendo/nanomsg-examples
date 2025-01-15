/**
 *  sample nanomessage
 * Push/pull.
 *   The pusher will do the listening and the puller will do
 * the connecting.  The pusher sends one message, the puller
 * outputs it and then exits.  pusher joins the puller.
 * 
 * 
 */
#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <stdlib.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>

// Useful error checking method:
// Returns int since e.g. socket returns the socket on ok.
static int
checkstat(int status, const char* msg) {
    if (status < 0) {
        std::cerr << msg << nn_strerror(nn_errno()) << std::endl;
        exit(EXIT_FAILURE);
    }
    return status;
}


/** Puller thread:
 *  connect to the uri.
 */
static void
puller(std::string uri) {
    int socket = checkstat(
        nn_socket(AF_SP, NN_PULL),
        "Failed to open pull socket."
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Failed to connect to pusher."
    );
    // Lets' get nanomsg to allocated the message:

    char* buf(nullptr);
    int recvbytes = checkstat(
        nn_recv(socket, &buf, NN_MSG, 0),
        "Failed to receive pushed message."

    );
    // We assume the message is a string:

    std::cout << "Pulled " << buf << std::endl;
    nn_freemsg(buf);
    checkstat(
        nn_shutdown(socket, endpoint),
        "Puller failed to shutdown endpoint"
    );
    checkstat(
        nn_close(socket),
        "Puller failed to close socket."
    );
}

int main(int argc, char** argv) {
    std::string uri(argv[1]);     // So we can use all transports.

    int socket = checkstat(
        nn_socket(AF_SP, NN_PUSH),
        "Failed to open push socket"
    );

    int endpoint = checkstat(
        nn_bind(socket, uri.c_str()),
        "Failed to bind pusher."
    );
    // Start the puller and wait abit for it to get started.
    // We could get clever and use a barrier promise/future but that's too complicated.

    std::thread puller_thread(puller, uri);
    sleep(1);                                    // let it get setup.

    const char* message="This is a pushed mesage";
    checkstat(
        nn_send(socket, message ,strlen(message) +1, 0),
        "Unable to push a message"
    );

    // Wait for the puller to finish:

    puller_thread.join();

    // Shutdown our end:

    checkstat(nn_shutdown(socket, endpoint),
        "Unable to shutdownt he pusher's endpoint"
    );
    checkstat(
        nn_close(socket),
        "Unable to shutdown the push socket"
    );

    exit(EXIT_SUCCESS);
}