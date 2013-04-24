#"-B" flag for force rebuild
#runac output.out $(<args.args)
TARGET = main.out
OBJECTS = main.o #embsys_clock.o embsys_7segments.o embsys_uart.o main.o
CC = mcc
CFLAGS = -g

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ ex1.ld

clean:
	rm $(OBJECTS) $(TARGET)

