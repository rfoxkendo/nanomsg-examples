PROGRAMS=pushpull
CXXFLAGS=-lnanomsg -g -O0
all: $(PROGRAMS)

pushpull: pushpull.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f $(PROGRAMS)
