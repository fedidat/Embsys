#"-B" flag for force rebuild
#runac output.out $(<args.args)
TARGET = ex2.out
OBJECTS = ex2.o embsys_uart.o embsys_uart_queue.o
CC = mcc
CFLAGS = -g

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ ex2.ld ex2_ivt.s
		
clean:
	rm $(OBJECTS) $(TARGET) ex2_ivt.o

