CC = gcc
TARGET = httpServerd
OBJECT = httpServerd.o
LDFLAGS=-pthread

all: httpServerd

httpServerd: httpServerd.o
	$(CC) -o httpServerd httpServerd.o ${LDFLAGS}

httpServerd.o: ../src/httpServerd.c
	$(CC) $(FLAGS) -c ../src/httpServerd.c ../include/httpServerd.h ${LDFLAGS}

clean:
	rm -f $(OBJECT) $(TARGET) 
