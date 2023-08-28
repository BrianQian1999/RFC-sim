# Requirements: C++17, yaml-cpp

SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj
YAMLLIB_DIR = ~/yaml-cpp/build

CXX = g++
CXXFLAGS = -std=c++17 -Ofast -I$(INC_DIR) -I ~/yaml-cpp/include
LDFLAGS = -L $(YAMLLIB_DIR) -lyaml-cpp

TARGET = rfc-sim

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@
	rm $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

clean:
	rm -f $(OBJS) $(TARGET)
