/**
 * This program times the request/reponse communication pattern.  We time 
 * two types of patterns -- large requests with small reponses (e.g. providing the
 * server with informatino) and small requests with large responses (asking the server for
 * informatio).  Small messages are always one byte.  Big messages size is parameterized.
 * 
 * Usage:
 * 
 *    reqrep uri nmsgs msgsize
 * 
 * Where:
 * *   uri is the communications endpoint
 * *   nmsgs is the nummber of req/rep pairs to excxhange.
 * *   msgsize is the size of the large message.
 * 
 * Output timings include the Time, msgs/sec and kbytes/sec for both large and small
 * REQ.
 * 
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
#include <latch>
#include <chrono>
#include <vector>
#include <chrono>

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

/**
 *  requestor thread:
 * @param uri  - uri to connect to the replier with.
 * @param nreq - number of requests to make.
 * @param size - Size of the request
 */
static void
requestThread(std::string uri, size_t nreq, size_t size) {
    char* request = new char[size];    // Recycle the req buffer.

    // set up the requstor

    int socket = checkstat(
        nn_socket(AF_SP, NN_REQ),
        "Failed to open the request socket."
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Failed to connect to the replier."
    );

    char* reply(nullptr);
    for (int i = 0;  i < nreq; i++) {
        checkstat(
            nn_send(socket, request, size, 0),
            "Failed to make a request"
        );
        reply = nullptr;
        checkstat(
            nn_recv(socket, &reply, NN_MSG, 0),
            "Failed to receive a reply"
        );
        nn_freemsg(reply);
    }
    delete []request;
    checkstat(
        nn_shutdown(socket, endpoint),
        "could not shutdown req endpoint"
    );
    checkstat(
        nn_close(socket),
        "Could not close req socket."
    );
}

/*
   replier - handles requests.

   @param socket - socket we send/receive on.
   @param nreq - Number of requests to handle.
   @param size   Size of the reply.

*/
static void
replier(int socket, size_t nreq, size_t size) {
    char* reply  = new char[size];
    void* request;
    for (int i =0; i < nreq; i++) {
        request = nullptr;
        checkstat(
            nn_recv(socket, &request, NN_MSG, 0),
            "Failed to get  a request"
        );
        nn_freemsg(request);

        checkstat(
            nn_send(socket, reply, size, 0),
            "Failed to send a reply"
        );
    }
    delete []reply;
}

// entry point.

int main(int argc, char** argv) {
    // get parameters, not production:

    std::string uri(argv[1]);
    size_t nmsg = atoi(argv[2]);
    size_t msgsize = atoi(argv[3]);

    // set up the req listener.

    int socket = checkstat(
        nn_socket(AF_SP, NN_REP),
        "Failed to open the reply socket"
    );
    int endpoint = checkstat(
        nn_bind(socket, uri.c_str()),
        "Failed to bind reply socket."
    );
    // Small req, big replies.
    
    std::thread req(requestThread, uri, nmsg, 1);

    // --------------------------  Timing.

    auto brstart = std::chrono::high_resolution_clock::now();
    replier(socket, nmsg, msgsize);
    req.join();
    auto brend =  std::chrono::high_resolution_clock::now();
    // ----------------------------- done.

    // big requests small replies.

    std::thread reqb(requestThread, uri, nmsg, msgsize);

    //--------------------- timing
    auto srstart = std::chrono::high_resolution_clock::now();
    replier(socket, nmsg, 1);
    reqb.join();
    auto srend = std::chrono::high_resolution_clock::now();

    // Shutdown the socket.

    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown reply socket"
    );
    checkstat(
        nn_close(socket),
        "Failed to close reply socket."
    );

    /// Report timigs.

    auto brequestduration = brend - brstart;
    double brtiming = (double)std::chrono::duration_cast<std::chrono::milliseconds>(brequestduration).count()/1000.0;

    double brmsgTiming = (double)nmsg / brtiming;
    double brxferrate  = (double)(nmsg* msgsize)/(brtiming * 1024.0);

    std::cout << "Big request small replies\n";
    std::cout << "Time     : " << brtiming << std::endl;
    std::cout << "Mesg/sec : " << brmsgTiming << std::endl;
    std::cout << "KB/sec   : " << brxferrate << std::endl;

    auto smrequestduration = srend - srstart;
    auto srtiming = (double)std::chrono::duration_cast<std::chrono::milliseconds>(smrequestduration).count()/1000.0;
    double srmsgTiming = (double)nmsg/srtiming;
    double srxferrate = (double)(nmsg*msgsize)/(srtiming * 1024.0);

    std::cout << "Small request big reqplies: \n";
    std::cout << "Time     : " << srtiming << std::endl;
    std::cout << "msg/seq  : " << srmsgTiming << std::endl;
    std::cout << "KB/sec   : " << srxferrate << std::endl;


    return EXIT_SUCCESS;


}