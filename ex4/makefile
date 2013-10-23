#"-B" flag for force rebuild
TARGET = a.out
ARGS = ex4.args
OBJECTS = ex4.o embsys_sms.o embsys_sms_protocol.o embsys_timer.o embsys_network.o embsys_display.o embsys_panel.o embsys_utilities.o embsys_flash.o embsys_persistence.o ivt.o
FILES = ex4.c embsys_sms.c embsys_sms_protocol.c embsys_timer.c embsys_network.c embsys_display.c embsys_panel.c embsys_utilities.c embsys_persistence.c embsys_sms.h embsys_sms_protocol.h embsys_timer.h embsys_network.h embsys_display.h embsys_panel.h embsys_flash.c embsys_flash.h embsys_persistence.h embsys_utilities.h tx.a tx_api.h tx_port.h ex4.args
CC = mcc
CFLAGS = -g
AS = mcc
ASFLAGS = -g -c

all: $(TARGET)

$(TARGET): $(OBJECTS) 
	$(CC) $(CFLAGS) $^ -o $@ device.svr3 tx.a

tar:
	tar cvf ex4.tar makefile device.svr3 ivt.s README $(FILES)

run:
	runac $(TARGET) @$(ARGS)

debug:
	mdb $(TARGET) @$(ARGS)

clean:
	rm $(OBJECTS) $(TARGET) embsys_flash.img logfile
