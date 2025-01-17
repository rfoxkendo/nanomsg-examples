## Nanomessage examples

Note, unless otherwise specified, the programs taka a single parameter,
the URI for the communication endpoint.  

Note, Lessonl learned for running in a container inside a WSL instance...
```tcp://localhost:xyzq``` does not work but ```tcp://127.0.0.1:xyzq``` does.

* pushpull - sets up a push/pull land sends/receives a message
* reqrep - Request/reply example
* pair - Pair pattern example.
* pubsub - demonstrates the publisher/subscriber pattern.
* surveyrespond - demonstrates the survey respond pattern.
* bus - Sets up an arbitrarily sized bus.  Each particpant sends/receives messages.
Usage for this is ```bus base-url bus size``` where:
    *  base-url is a URL with a %d in it.  %d will be replaced by the position of
    of each participant on the bus as each participant must provide a bound address.
    *  size - is a number >1 which is the number of bus members.

    
### Performance measurement apps.

See [Performance measurement](Performance/Readme.md)