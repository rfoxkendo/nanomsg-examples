/**
 * This program allows you to time the nanomsg push/pull pattern while varying:
 * 1.  The URI used
 * 2.  Number of messages sent.
 * 3.  Size of each mesage.
 * 4.  THe number of pullers.
 * 
 * Usage:
 *    pipeline uri nmsg msgsize nreceivers
 * Where:
 *    * uri - is the uri the pusher listens on and pullers connect to.
 *    * nmsg - is the number of messages sent.
 *    * msgsize - is the size of each message.
 *    * mreceivers - Is the number of receivers.
 * 
 * Each receiver is a thread.  Because of the way messages are distributed
 * to each puller we can't reliably do the terminate message game.
 * See pullThread.
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
#include <latch>
#include <chrono>
#include <vector>

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
 * pull thread:
 * 
 * @param uri - string that containst he URI of the pusher.
 * @param nmsg - The base number of messages - once we see a messages
 *    with a sequence bigger than this we're done.
 * @param ready - pointer to a latch that is decremented by us when we are
 * ready to recieve data.  The pusher waits for all pullers to be ready
 * before actually starting to time and send messages.
 * @param finished - pointer to a latch we arrive at when we're done
 *     getting messages and have shut down our socket.  The pusher
 *     sends messages until wait_for on finished is true.
 * 
 */
static void
pullThread(std::string uri, size_t nmsg, std::latch* ready,  std::latch* finished) {
    int socket = checkstat(
        nn_socket(AF_SP, NN_PULL),
        "Puller failed to open socket"
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Puller failed to connect to pusher."
    );
    
    uint32_t* msgBuf;
    bool done(false);
    ready->count_down();     // This thread is ready...

    // Start receving messages.

    while(!done) {
        msgBuf = nullptr;
        checkstat(
            nn_recv(socket, &msgBuf, NN_MSG, 0),
            "Failed to  pull a message"
        );
        done = msgBuf[0] >= nmsg;
        nn_freemsg(msgBuf);
    }

    
    finished->arrive_and_wait();     // Otherwise pushes hang >sigh<
    checkstat(nn_shutdown(socket, endpoint), "Puller failed shutdown");
    checkstat(nn_close(socket), "Puller failed close");
}
/// Pushes the messages once all is set up
// Returns the number of messages sent before everyone was doe.
static size_t  
pusher(int socket, size_t msgSize,  std::latch& done) {
    char* msg = new char[msgSize];     // Use the same message buffer.
    uint32_t* seq = reinterpret_cast<uint32_t*>(msg);
    *seq = 0;
    size_t result(0);
    while(! done.try_wait()) {
        int stat =  nn_send(socket, msg, msgSize, NN_DONTWAIT);
        if (stat > 0) {    
            *seq += 1;               // Only count what we can send.
            result++;
        } else if (nn_errno() != EAGAIN) {
            checkstat(stat, "Pusher failed to send message");
        }
        // else just blocked.
    }
    delete []msg;
    return result;
    
}
// entry point

int main(int argc, char** argv) {
    // Get the program parameters.

    std::string uri(argv[1]);
    size_t nmsg = atoi(argv[2]);
    size_t msgsize = atoi(argv[3]);
    size_t nreceivers = atoi(argv[4]);


    // Set up the pull side of things.

    int socket = checkstat(
        nn_socket(AF_SP, NN_PUSH),
        "Failed to create push sockket"
    );
    int endpoint = checkstat(
        nn_bind(socket, uri.c_str()),
        "Failed to bind push socket."
    );

    std::latch allready(nreceivers);    // So we know when all the receivers are ready to go.
    std::latch alldone(nreceivers);     // So we know when to join.
    std::vector<std::thread*> receivers;

    for (int i =0; i < nreceivers; i++) {
        receivers.push_back(new std::thread(pullThread, uri, nmsg, &allready, &alldone));
    }

    // Wait for the to all startt:

    allready.wait();

    ///////////////////////////////////// timed
    auto start = std::chrono::high_resolution_clock::now();
    nmsg = pusher(socket, msgsize, alldone);            // Actual number of messagse.
    // Join the threads so we know they're done

    for (auto p : receivers) {
        p->join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    ////////////////////////////////////// timed

    // Clean up everything

    for (auto p : receivers) {
        delete p;

    }
    receivers.clear();
    checkstat(
        nn_shutdown(socket, endpoint),
        "Pusher failed shutdown"
    );
    checkstat(
        nn_close(socket),
        "Pusher failed socket close"
    );

    // publish the timings

    auto duration = end - start;
    double timing = (double)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()/1000.0;
    double msgTiming =(double)nmsg/timing;
    double xferRate = (double)(nmsg * msgsize)/(timing * 1024.0);   // kb/sec


    std::cout << "Time    : " << timing << std::endl;
    std::cout << "msg/sec : " << msgTiming << std::endl;
    std::cout << "Kb/sec  : " << xferRate << std::endl;


    return EXIT_SUCCESS;
}
