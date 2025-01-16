/**
 *   Sample for a pubsub.
 * The publisher  listens.  The subscriber connects.
 * The publisher will publish three messages, the last one
 * will start with EXIT   the subscriber will subscribe to 
 * 'EXIT' only
 *
 */
#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <stdlib.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sstream>

// Useful error checking method:
// Returns int since e.g. socket returns the socket on ok.
// else output an error and exists with failure status.
static int
checkstat(int status, const char* msg) {
    if (status < 0) {
        std::cerr << msg << nn_strerror(nn_errno()) << std::endl;
        exit(EXIT_FAILURE);
    }
    return status;
}


/**
 * subscriber thread:
 * 
 */
static void subscriber(std::string uri) {

    // Open the socket set the subscription and connect.
    int socket = checkstat(
        nn_socket(AF_SP, NN_SUB),
        "Could not open subscriber socket."
    );
    const char* subscription="EXIT";
    checkstat(
        nn_setsockopt(socket, NN_SUB, NN_SUB_SUBSCRIBE, subscription, strlen(subscription)),
        "Could not set subscription string."
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Failed to connect to publisher."
    );

    // Get our message:

    char* pMsg(nullptr);
    checkstat(
        nn_recv(socket, &pMsg, NN_MSG, 0),
        "Could not receive message"
    );

    std::cout << "Subscriber got " << pMsg << std::endl;
    nn_freemsg(pMsg);

    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown subscriber endpoint"
    );
    checkstat(
        nn_close(socket),
        "Failed to close subscriber socket."
    );
}

// Entry point:

int main(int argc, char** argv) {
    std::string uri(argv[1]);   /// Not production code.

    // Set up the publisher before starting the subscdriber.

    int socket = checkstat(
        nn_socket(AF_SP, NN_PUB),
        "Could not create publisher socket."
    );
    int endpoint =  checkstat(
        nn_bind(socket, uri.c_str()), 
        "Publisher bind failed."
    );

    // start the subscsriber:

    std::thread subscriberThread(subscriber, uri);
    sleep(1);                        // Give it time to get setup.

    // publish
    const char *  pMessage[3] =  {
        "HELLO - this might start us off",
        "STATUS - might publish a status of some sort.",
        "EXIT - might tell us the publisher is exiting"
    };

    for (int i =0; i < 3; i++) {
        std::cout << "Publishing : " << pMessage[i] << std::endl;
        checkstat(
            nn_send(socket, pMessage[i], strlen(pMessage[i]), 0),
            "Failed to publish a message"
        );
    }

    subscriberThread.join();                     // Should exit right:

    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown publisher endpoint"
    );
    checkstat(
        nn_close(socket),
        "Failed to close publisher socket."
    );

    return EXIT_SUCCESS;
}