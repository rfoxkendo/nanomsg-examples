/**
 * Demo program for a nanomsg bus.  Busses are sort of like pub/sub but:
 * *   Anyone can publish and all others will receive.
 * *   There's no built in selection of message subsets (can be implemented 
 * by application code).
 * *   Message delivery is not reliable... at least in nng this was true in tests.
 * We'll find out if that's true when we do performance testing, which is when that turned up.
 * 
 * What we'll do is 
 * 1.  Set upt the bus so that the main thread is participant 0 and the remaining
 * n-1 threads are the other positions on the bus.
 * 2. Each participant will send one message to the bus.
 * 3. The particpants will receive (with wait) one message and then enter a loop
 * using nn_poll to determine when 
 */


#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/bus.h>
#include <stdlib.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sstream>
#include <vector>


static const int timeout=1000;     // Ms for poll timeout.

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
 * Generate uris
 *   Given the uri template and a bus size returns a vector of strings that are the URIS for the
 * bus.  Element i of the vector is the URI position i on the bus should listen on.
 * 
 * 
 */
static std::vector<std::string>
generateUris(const std::string& base, size_t size) {
    std::vector<std::string> result;
    const char* format = base.c_str();     // For snprintf.
    char  uriBuffer[100];                  // sb big enough.
    for (int i =0; i < size; i++)  {
        int nchars = snprintf(uriBuffer, sizeof(uriBuffer), format, i);
        if (nchars >= sizeof(uriBuffer)) {
            std::cerr << "URI Buffer overflow in generateUris\n";
            exit(EXIT_FAILURE);
        }
        result.push_back(std::string(uriBuffer));
    }

    return result;
}

// Creates the bus socket and listens on the bus.
// Returns the socket and the endpoint position
//
static std::pair<int, int>   // socket/endpoint
createBusSocket(const std::vector<std::string>& busUris, int position) {
    int socket = checkstat(
        nn_socket(AF_SP, NN_BUS),
        "Failed to open bus socket"
    );
    int endpoint = checkstat(
        nn_bind(socket, busUris[position].c_str()),
        "Failed to bind bus socket"
    );
    return std::pair<int, int>(socket, endpoint);
}


// Connect to the other members of the bus as appropriate for our position.
// Returns the vector of endpoint ids.
static std::vector<int>
connectToBus(int socket, const std::vector<std::string>& busUris, int position) {
    std::vector<int> endpoints;

    auto start = busUris.begin() + (position+1);   // Next URI after me.
    // 0 connects to everyone but the last one:

    if (position == 0) {
        auto end = busUris.end() - 1;     // Just past next to last:

        for (auto p = start; p!= end; p++) {
            int endpt = checkstat(
                nn_connect(socket, p->c_str()),
                "Failed to connect to the bus."
            );
            endpoints.push_back(endpt);
        }
    } else if (start != busUris.end()) {
        // All but the first and last one connecect to the subsequent ones.

        for (auto p = start; p != busUris.end(); p++) {
            int endpt = checkstat(
                nn_connect(socket, p->c_str()),
                "Failed to connect to the bus"
            );
            endpoints.push_back(endpt);
        }

    }
    else {
        // Last connects to the first.

        int endpt = checkstat(
            nn_connect(socket, busUris[0].c_str()),
            "Failed to close the connection loop"
        );
    }

    return endpoints;
}

// This is the business end of a bus member:
// We get the uri template, the size of the bus and position.
// Setup the bus with appropriate delays then
// - Send a message.
// - Recieve a messages.
// - Receive more messages until we timeout.
//
static void busMember(std::string uriTemplate, int size, int position) {
    auto busUris = generateUris(uriTemplate, size);

    // Start listening and wait for everyone to start -- could use a barrier here but...
    // we'll just sleep.

    auto socketAndEndpoint = createBusSocket(busUris, position);
    int socket = socketAndEndpoint.first;
    sleep(2);

    auto endpoints = connectToBus(socket, busUris, position);
    endpoints.push_back(socketAndEndpoint.second);      // Will need to shutdown that one too:
    sleep(2);

    // By now everyone's rocking.
    // Format and send our message:

    std::stringstream strMessage;
    strMessage << "Hello from bus member " << position;
    std::string message(strMessage.str());

    checkstat(
        nn_send(socket, message.c_str(), message.size()+1, 0),
        "Bus member failed to send message"
    );

    char* recvMsg(nullptr);                   // To point ot received messages.

    // Get the first message - we'll always get at least one.

    int nRecv = checkstat(
        nn_recv(socket, &recvMsg, NN_MSG, 0),
        "Failed to receive first message"
    );
    std::cerr << position << " Received : " << recvMsg << std::endl;
    nn_freemsg(recvMsg);
    // Set up to poll:

    nn_pollfd poller = {
        fd: socket,
        events: NN_POLLIN,
        revents: 0
    };

    bool done(false);
    while (!done) {
        int nfds = nn_poll(&poller, 1, timeout);
        if (nfds > 0) {
            recvMsg = nullptr;                    // recvMsg fills this in.
            checkstat(
                nn_recv(socket, &recvMsg, NN_MSG, 0),
                "Failed to receive subsequent message"
            );
            std::cerr << position << " Received : " << recvMsg << std::endl;
            nn_freemsg(recvMsg);
        } else if (nfds == 0) {
            done = true;                 // timed out.
        } else {
            if (nn_errno() != EINTR) {
                // Something bad happended:

                checkstat(nfds, "Failed to poll for data");
            }
        }
    }
    // We get here if the message poll times out which means we have all the messages
    // Yeah, I know I could have counted them too but I wanted to play with nn_poll.

    for (auto ept: endpoints) {
        checkstat(
            nn_shutdown(socket, ept),
            "Failed to shutdown an endpoint."
        );
    }
    checkstat(
        nn_close(socket),
        "Failed to close the socket"
    );

}
int main(int argc, char** argv) {
    // get the parameters -- not production code.

    std::string uriTemplate(argv[1]);
    size_t busSize = atoi(argv[2]);

    // In this case we start the threads first, then run busMember for location 0.  The
    // delays should be sufficient.

    std::vector<std::thread*> bus;
    for (int pos = 1; pos < busSize; pos++) {
        bus.push_back(new std::thread(busMember, uriTemplate, busSize, pos));
    }
    // Now run position 0:

    busMember(uriTemplate, busSize, 0);

    // When it returns the threads should be joinable and deletable.:

    for (auto p: bus) {
        p->join();
        delete p;
    }
    bus.clear();

}


