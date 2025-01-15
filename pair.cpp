/**
 *  Demonstrates the pair communication pattern.
 *  The only distinction between 'sender' and 'receiver'
 *   is who listens and who connects.
 * 
 * In our case, unlike the req/rep which has a request/response message ordering
 * discipline, we both send then both receive which, I think, will work as in pair
 * it's truly peer.
 * 
 */
#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
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


// Thing that does the communication; per Dr. Suess.
static void
thing(int socket, const char* name) {
    std::stringstream strmessage;
    strmessage << name << " says hi";
    std::string msg(strmessage.str());
    checkstat(
        nn_send(socket, msg.c_str(), msg.size()+1, 0),
        "Thing failed to send a message"
    );

    char* buf(nullptr);
    checkstat(
        nn_recv(socket, &buf, NN_MSG, 0),
        "THing failed to receive emssage"
    );
    std::cerr << name << " got " << buf << std::endl;;
    nn_freemsg(buf);
}

/**
 * thing2
 *    The thread that connects.
 */
static void
thing2(std::string uri) {
    int socket = checkstat(
        nn_socket(AF_SP, NN_PAIR),
        "Failed to open thing2 socket."
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Failed to connect to pair uri"
    );

    thing(socket, "thing2");  // Do the communication.

    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown thing 2 endpoint"
    );

    checkstat(
        nn_close(socket),
        "Failed to close thing2 socket"
    );
}

// entry point  is thing 2:

int main(int argc, char** argv) {
    std::string uri(argv[1]);

    // set up the listner.

    int socket = checkstat(
        nn_socket(AF_SP, NN_PAIR),
        "Failed to open thing 1 socket"
    );
    int endpoint = checkstat(
        nn_bind(socket, uri.c_str()),
        "Failed to bind thing1 socket"
    );

    // Now we can start thing2:

    std::thread partner(thing2, uri);


    thing(socket, "thing 1");

    partner.join();


    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdownn thing1 end point"
    );
    checkstat(
        nn_close(socket),
        "failed to close thing1 socket."
    );

    exit(EXIT_SUCCESS);
}