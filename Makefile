PROGRAMS=pushpull reqrep pair
CXXFLAGS=-lnanomsg -g -O0
all: $(PROGRAMS)

pushpull: pushpull.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)


reqrep: reqrep.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

pair: pair.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)
clean:
	rm -f $(PROGRAMS)
