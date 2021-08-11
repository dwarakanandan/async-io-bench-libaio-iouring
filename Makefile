CC = g++

CFLAGS = -g -Wall -O3
LUSRC := -L../liburing/src/
LIBS = -pthread -laio -luring

SRC := src
LUINCLUDES := -I../liburing/src/include/
BUILD := build

SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

all: clean benchmark

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)

benchmark: $(OBJECTS)
	$(CC) $(CFLAGS) $(LUSRC) $^ -o build/$@ $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) -I$(SRC) $(LUINCLUDES) $(CFLAGS) $(LUSRC) -c $< -o $@