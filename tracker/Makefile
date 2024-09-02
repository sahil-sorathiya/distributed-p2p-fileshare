CC = g++
CFLAGS = -Wall -I/usr/include/openssl
LDFLAGS = -lssl -lcrypto
DEPS = headers.h  
OBJ = classes/Groups.o classes/Socket.o classes/Users.o classes/Utils.o handlers.o tracker.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

tracker: $(OBJ)
	$(CC) $(CFLAGS) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o tracker ./classes/*.o