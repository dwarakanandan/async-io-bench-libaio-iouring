CC = g++

CFLAGS = -g -Wall

SOURCE = syncio

all:
	for FILE in $(SOURCE) ; do \
		$(CC) $(CFLAGS) -o build/$$FILE src/$$FILE.cpp; \
	done
