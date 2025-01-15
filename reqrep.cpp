/**
 * Sample REQ/REP program.
 * 
 * Request thread say "WHO ARE YOU" to the replier who responds with its pid.
 */
#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
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

// requestor sends a request and gets a reply back.
// We do the connect the rep side doese the bind.
//
static void requestor(std::string uri) {
    int socket = checkstat(
        nn_socket(AF_SP, NN_REQ),
        "Failed to open requestor socket"
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Faild to connect to the replier."
    );
    const char* request = "Request";
    checkstat(
        nn_send(socket, request, strlen(request) + 1 , 0),
        "Failed to send request."
    );

    char* buffer(nullptr);
    int replySize = checkstat(
        nn_recv(socket, &buffer, NN_MSG, 0),
        "Failed to receive reply"
    );
    std::cerr << "Reply: " << buffer << std::endl;
    nn_freemsg(buffer);

    checkstat(
        nn_shutdown(socket, endpoint),
        "Faild to shutdown requester's endpoint"

    );
    checkstat(
        nn_close(socket),
        "failed to close requester socket"
    );

}
int main(int argc, char** argv) {
    std::string uri(argv[1]);

    int socket = checkstat(
        nn_socket(AF_SP, NN_REP),
        "Failed to open the reply socket"
    );
    int endpoint = checkstat(
        nn_bind(socket, uri.c_str()),
        "Failed to bind the rep socket"
    );

    // start the requestor.

    std::thread req(requestor, uri);
    char* buffer(nullptr);
    checkstat(
        nn_recv(socket, &buffer, NN_MSG, 0),
        "Failed to receive a request"
    );
    std::cerr << "Request: " << buffer << std::endl;
    nn_freemsg(buffer);
    
    // formulate our reply:

    auto pid = getpid();
    std::stringstream replystream;
    replystream << "I am : " << pid;
    std::string reply = replystream.str();

    /// send it.

    checkstat(
        nn_send(socket, reply.c_str(), reply.size()+1, 0),
        "Failed to send reply."
    );


    req.join();

    // now shutdown.
    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown reply endpoint"
    );
    checkstat(
        nn_close(socket),
        "Failed to close reply socket"
    );

    exit(EXIT_SUCCESS);

} 