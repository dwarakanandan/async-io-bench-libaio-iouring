CC = g++

CFLAGS = -g -Wall -O3
SRC := src
BUILD := build

LIBS = -pthread -laio -luring

LAIO_SRC := -L../libaio/src/
LURING_SRC := -L../liburing/src/

LAIO_INCLUDES := -I../libaio/src/
LURING_INCLUDES := -I../liburing/src/include/


SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

all: clean benchmark

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)

benchmark: $(OBJECTS)
	$(CC) $(CFLAGS) $(LAIO_SRC) $(LURING_SRC) $^ -o build/$@ $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) -I$(SRC) $(LAIO_INCLUDES) $(LURING_INCLUDES) -c $< -o $@