CC = g++

BIN = bin

EXEC = pong_server

SRCS = main.cpp

OBJS = bin/main.o

INCLUDE = .

CFLAGS = -I $(INCLUDE) -Wall -g -Werror
LDFLAGS = 

$(EXEC): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BIN)/%.o: %.cpp
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $^ -o $@


run: $(EXEC)
	./$(EXEC)

clean:
	rm -rf $(BIN) $(EXEC)
