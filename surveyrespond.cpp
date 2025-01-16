/**
 * Demonstrats the surveyrespond pattern. 
 * Survey  broadcasts messages to a responder and
 * gets back messages from a subset (might be the complete set) of responders
 * The survey has a timeout (set with NN_SURVEYOR_DEADLINE) after which responses are dropped.
 * This is how subset reponses can work.
 * Additionally  a new servey messagse cancels any in-progress survey.
 * 
 * What we do:
 *    After setting up the survey endpoint, we spin off 10 threads that are given numbers
 * 0-9.  These threads accept surveys which can consist of the following
 * 
 * *   ALL - all threads respond with their ids.
 * *   EVEN - only even threads respond with their ids.
 * *   ODD  - Only odd threads respond with their ids.
 * *   EXIT - Like ALL but threads exit after responding.
 * 
 *  THe main thread sends an ALL, EVEN ODD and then EXIT surveys.
 *    *  Indicates the survey it's about to perform.
 *    * Outputs the responses received.
 * The survey timout is set to something pretty small like 250ms so the program won't hang too
 * long on survey responses.   There is a default survey time but we're too lazy to fetch it
 * to see if its reasonable.
 * 
 */
#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/survey.h>
#include <stdlib.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sstream>
#include <vector>

// Useful error checking method:
// Returns int since e.g. socket returns the socket on ok.
// else output an error and exists with failure status.
static int
checkstat(int status, const char* msg) {
    if (status < 0) {
        std::cerr << msg << " "  << nn_strerror(nn_errno()) << std::endl;
        exit(EXIT_FAILURE);
    }
    return status;
}

// A responder thread: 

static void
responder(std::string uri, int id) {

    int socket = checkstat(
        nn_socket(AF_SP, NN_RESPONDENT),
        "Unable to open respondent socket."
    );
    int endpoint = checkstat(
        nn_connect(socket, uri.c_str()),
        "Respondent failed to connect to the surveyor."
    );
    std::cerr << "Thread " << id << " ready for surveys\n";
    // Process surveys (or not).
    bool done = false;
    while (!done) {
        char* pBuf(nullptr);
        checkstat(
            nn_recv(socket, &pBuf, NN_MSG, 0),
            "Failed to receive a survey"
        );
        std::string survey(pBuf);
        nn_freemsg(pBuf);
        pBuf = nullptr;
        if (survey == "ALL" ||            // everyone responds
            survey == "EXIT" ||           // respond and exit.
            (survey == "ODD" && (id % 2) != 0) || // Odd and we're odd.
            (survey =="EVEN" && (id %2) == 0)) { // even and we're even.

            std::stringstream strresponse;
            strresponse << id << " Responding to " << survey;
            std::string response(strresponse.str());

            checkstat(
                nn_send(socket, response.c_str(), response.size()+1, 0),
                "Failed to respond to a survey"
            );

        }
        if (survey == "EXIT") done = true; // Exit if requested to.
    }

    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown responder endpoint"
    );
    checkstat(
        nn_close(socket),
        "Failed to close responcder socket"
    );

    // Exit thread.
}
// Conduct a survey and get the replies:

static void 
survey (int socket, const char* request) {
    std::cout << "Survey: " << request << std::endl;
    // send the survey:

    checkstat(
        nn_send(socket, request, strlen(request) + 1, 0),   // +1 for null terminator.
        "failed to start a survey"
    );
    char* pResponse(nullptr);
    bool done = false;
    int nRecv;
    std::cerr << "----------------------- start responses\n";
    while (!done) {
        nRecv = nn_recv(socket, &pResponse, NN_MSG, 0);
        if ((nRecv <= 0) && (nn_errno() == ETIMEDOUT)) {
            std::cerr << "Survey timeout " << nRecv << std::endl;
            done = true;                         // Survey done
        } else {
            checkstat(                       // Make sure not some other error:
                nRecv,
                "Failed to get a survey response"
            );
            // response

            std::cout << "Got response: " << pResponse << " " << std::endl;
            nn_freemsg(pResponse);
            pResponse = nullptr;
        }
    }
    std::cerr << "-------------------end responses\n";
}

// entry point.
int main(int argc,  char**argv) {
    std::string uri(argv[1]);    /// not production code.

    // setup the survey end of things:

    int socket = checkstat(
        nn_socket(AF_SP, NN_SURVEYOR),
        "Unable to open surveyor socket."
    );
    int endpoint = checkstat(
        nn_bind(socket, uri.c_str()),
        "Unable to advertise surveyor."
    );
    // Set the survey lifetime.
#ifdef SETLIFE
    int lifetime =250;    /// milliseconds.
    //  Hope this works.  THe docs are not clear.
    checkstat(
        nn_setsockopt(socket, NN_SURVEYOR,  NN_SURVEYOR_DEADLINE,  &lifetime, sizeof(int)),
        "Failed to set servuey lifetime"
    );
#endif
    std::vector<std::thread*>  responders;
    for(int i=0; i < 10; i++) {
        responders.push_back(new std::thread(responder, uri, i));
    }
    // Wait a bit for everything to work out:

    sleep(1);

    survey(socket, "ALL");
    survey(socket, "EVEN");
    survey(socket, "ODD");
    survey(socket, "EXIT");

    // wait for the threads to all exit and  free them.

    for (auto p : responders) {
        p->join();
        delete p;
    }
    responders.clear();

    // Tear down the survey

    checkstat(
        nn_shutdown(socket, endpoint),
        "Failed to shutdown survey endpoint"
    );
    checkstat(
        nn_close(socket),
        "failed to close survey socket."
    );
    return EXIT_SUCCESS;
}