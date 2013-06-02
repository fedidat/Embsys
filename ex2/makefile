#"-B" flag for force rebuild
#runac output.out $(<args.args)
TARGET = a.out
ARGS = ex2.args
OBJECTS = ex2.o embsys_utilities.o embsys_7segments.o embsys_uart.o embsys_uart_queue.o embsys_flash.o embsys_timer.o
FILES = ex2.c embsys_utilities.h embsys_7segments.h embsys_uart.h embsys_uart_queue.h embsys_flash.h embsys_timer.h embsys_utilities.c embsys_7segments.c embsys_uart.c embsys_uart_queue.c embsys_flash.c embsys_timer.c device.svr3 ex2_ivt.s ex2.args
CC = mcc
CFLAGS = -g

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ device.svr3 ex2_ivt.s
	
tar:
	tar cvf ex2.tar makefile README $(FILES)
	
run:
	mdb $(TARGET) @$(ARGS)

clean:
	rm $(OBJECTS) $(TARGET) ex2_ivt.o

