#"-B" flag for force rebuild
#runac output.out $(<args.args)
TARGET = ex1.out
OBJECTS = ex1.o embsys_clock.o embsys_7segments.o embsys_uart.o embsys_uart_queue.o
FILES = ex1.c embsys_clock.h embsys_clock.c embsys_7segments.h embsys_7segments.c embsys_uart.h embsys_uart.c embsys_uart_queue.h embsys_uart_queue.c ex1.ld ex1.args
CC = mcc
CFLAGS = -g

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ ex1.ld
	
tar:
	tar cvf ex1.tar makefile README $(FILES)
	
clean:
	rm $(OBJECTS) $(TARGET)

