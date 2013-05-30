#"-B" flag for force rebuild
#runac output.out $(<args.args)
TARGET = ex2.out
OBJECTS = ex2.o embsys_7segments.o embsys_uart.o embsys_uart_queue.o embsys_flash.o embsys_timer.o
FILES = ex2.c embsys_7segments.h embsys_uart.h embsys_uart_queue.h embsys_flash.h embsys_timer.h embsys_7segments.c embsys_uart.c embsys_uart_queue.c embsys_flash.c embsys_pager.c embsys_timer.c ex2.args ex2.ld
CC = mcc
CFLAGS = -g

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ ex2.ld ex2_ivt.s
	
tar:
	tar cvf ex1.tar makefile README $(FILES)
	
clean:
	rm $(OBJECTS) $(TARGET) ex2_ivt.o

