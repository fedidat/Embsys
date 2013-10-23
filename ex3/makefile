#"-B" flag for force rebuild
TARGET = a.out
ARGS = ex3.args
OBJECTS = ex3.o embsys_sms_slave.o embsys_uart_queue.o embsys_sms.o embsys_sms_protocol.o embsys_timer.o embsys_network.o embsys_display.o embsys_panel.o embsys_uart.o embsys_utilities.o ivt.o
FILES = ex3.c embsys_sms_slave.c embsys_uart_queue.c embsys_sms.c embsys_sms_protocol.c embsys_timer.c embsys_network.c embsys_display.c embsys_panel.c embsys_uart.c embsys_utilities.c embsys_sms_slave.h embsys_uart_queue.h embsys_sms.h embsys_sms_protocol.h embsys_timer.h embsys_network.h embsys_display.h embsys_panel.h embsys_uart.h embsys_utilities.h tx.a tx_api.h tx_port.h ex3.args
CC = mcc
CFLAGS = -g
AS = mcc
ASFLAGS = -g -c

all: $(TARGET)

$(TARGET): $(OBJECTS) 
	$(CC) $(CFLAGS) $^ -o $@ device.svr3 tx.a

tar:
	tar cvf ex3.tar makefile device.svr3 ivt.s README $(FILES)

run:
	runac $(TARGET) @$(ARGS)

debug:
	mdb $(TARGET) @$(ARGS)

clean:
	rm $(OBJECTS) $(TARGET) embsys_flash.img logfile
