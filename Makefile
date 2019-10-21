TARGET = imaqliq
SETPATH = /usr/local/bin

.PHONY: all clean install uninstall

all: $(TARGET)

# -- make obj
server.o: recipient.c
	g++ -c -std=c++11 -lpthread -o server.o recipient.c

client.o: sender.c
	g++ -c -std=c++11 -o client.o sender.c

main.o: $(TARGET).c
	g++ -c -std=c++11 -o main.o $(TARGET).c

# -- make bin
$(TARGET): server.o client.o main.o
	g++ -o $(TARGET) main.o server.o client.o

# -- make install
clean:
	rm -rf $(TARGET) main.o server.o client.o

install:
	install $(TARGET) $(SETPATH)

uninstall:
	rm -rf $(SETPATH)/$(TARGET)
