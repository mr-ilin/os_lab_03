CC = gcc
CFLAGS = -pedantic -Wall -std=c11 -g
LDFLAGS = -lpthread
EXECUTABLE = lab3
SRC=main.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(EXECUTABLE) $(LDFLAGS)

# OBJ=$(SRC:.cpp=.o)

# $(EXECUTABLE): $(OBJ)
# 	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)
	
# .cpp.o:
# 	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(EXECUTABLE)