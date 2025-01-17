PROGRAMS=pipeline reqrep
CXXFLAGS=-lnanomsg -g -O0 -std=c++20
all : $(PROGRAMS)

pipeline : pipeline.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

reqrep : reqrep.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f $(PROGRAMS)

