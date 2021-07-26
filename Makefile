CC = g++

CFLAGS = -g -Wall -laio -pthread -O3

SOURCE = syncio async_libaio
HELPERS = helper

all:
	for FILE in $(SOURCE) ; do \
		$(CC) $(CFLAGS) -o build/$$FILE src/$$FILE.cpp src/$(HELPERS).cpp; \
	done
