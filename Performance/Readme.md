## Performance measuring applications and how to use them.

Note these applications are not production code.  If you fail to provide a parameter,
probably they will segfault.

### Push/pull  timings:

Usage:
```
./pipeline uri nmsg msgsize nreceivers
```
Where:

*  uri - is the uri used for the transport endpoints.
*  nmsg - is the number of messages to send.
*  msgsize - is the size of the messages.
*  nreceivers - is the number of receivers.

Note that nreceiver size 1 messages are also sent to tell the pullers they're done and that's
timed as well.  Be sure that nmsg is large enough that will be timing noise.

pipelinetimings.sh - is a script that will accumulate a pile of timings for each transport thpe.
the output is sent to pipelineTimings.log

### REQ/REP timings

Usage:
```
./reqrep uri nmsg msgsize
```

* uri - uri that is the tranport endpoint.
* nmsg - number of REQ/REP pairs.
* msgsize - size of the large message in a REQ/REP transaction.

The program times requests that are msgsize with one byte replies as well as requests that are one
byte with replies that are msgsize.

reqreptimings.sh - is a script that will timing for a numbger of values. Output is written to reqreptimings.log
