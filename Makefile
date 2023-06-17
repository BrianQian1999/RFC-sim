# Requirements: C++14, yaml-cpp

SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj
TEST_DIR = ./unittest

CXX = g++
CXXFLAGS = -std=c++14 -Ofast -I$(INC_DIR)
LDFLAGS = -lyaml-cpp

TARGET = rfc-sim

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@
	rm $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

TEST_TARGET = test
TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(TEST_SRCS))
test: $(TEST_TARGET)
$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@
	rm $(TEST_OBJS)

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)
