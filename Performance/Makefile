PROGRAMS=pipeline
CXXFLAGS=-lnanomsg -g -O0 -std=c++20
all : $(PROGRAMS)

pipeline : pipeline.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)
