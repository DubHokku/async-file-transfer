TARGET = imaqliq
SETPATH = /usr/local/bin

.PHONY: all clean install uninstall

all: $(TARGET)

# -- make obj
server.o: recipient.cc
	g++ -c -std=c++11 -o server.o recipient.cc

client.o: sender.cc
	g++ -c -std=c++11 -o client.o sender.cc

main.o: $(TARGET).cc
	g++ -c -std=c++11 -lpthread -o main.o $(TARGET).cc

# -- make bin
$(TARGET): server.o client.o main.o
	g++ -o $(TARGET) -lpthread main.o server.o client.o

# -- make install
clean:
	rm -rf $(TARGET) main.o server.o client.o

install:
	install $(TARGET) $(SETPATH)

uninstall:
	rm -rf $(SETPATH)/$(TARGET)
