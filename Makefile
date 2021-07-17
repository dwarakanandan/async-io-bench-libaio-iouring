CC = g++

CFLAGS = -g -Wall -pthread -O3

SOURCE = syncio

all:
	for FILE in $(SOURCE) ; do \
		$(CC) $(CFLAGS) -o build/$$FILE src/$$FILE.cpp; \
	done
