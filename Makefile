CC = g++

CFLAGS = -g -Wall -O3
LIBS = -pthread -laio

SRC := src
BUILD := build

SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

benchmark: $(OBJECTS)
	$(CC) $^ -o build/$@ $(CFLAGS) $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) -I$(SRC) -c $< -o $@