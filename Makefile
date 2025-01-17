PROGRAMS=pushpull reqrep pair pubsub surveyrespond bus
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

bus: bus.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f $(PROGRAMS)
