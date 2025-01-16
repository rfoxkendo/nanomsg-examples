PROGRAMS=pushpull reqrep pair pubsub surveyrespond
CXXFLAGS=-lnanomsg -g -O0
all: $(PROGRAMS)

pushpull: pushpull.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)


reqrep: reqrep.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

pair: pair.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

pubsub: pubsub.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)


surveyrespond: surveyrespond.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f $(PROGRAMS)
