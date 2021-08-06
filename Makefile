CC = g++

CFLAGS = -g -Wall -O3
LIBS = -pthread -laio -luring

SRC := src
BUILD := build

SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

all: clean benchmark

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)

benchmark: $(OBJECTS)
	$(CC) $^ -o build/$@ $(CFLAGS) $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) -I$(SRC) -c $< -o $@ $(CFLAGS)