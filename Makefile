# Requirements: C++14, yaml-cpp

CXX = g++
CXXFLAGS = -std=c++14 -Ofast
LDFLAGS = -lyaml-cpp

TARGET = rfc-sim.out

SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@
	rm $(OBJS)

%.o: %.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

clean:
	rm -f $(OBJS) $(TARGET)
