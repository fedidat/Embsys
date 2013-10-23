#include "embsys_utilities.h"
#include "tx_api.h"
#include "embsys_sms.h"
#include "embsys_sms_slave.h"
#include "embsys_timer.h"

void tx_application_define(void *first) 
{
	/* initialize the SMS, slave mode and timer components */
	sms_init();
	embsys_sms_slave_init();
	enbsys_timer_init(TX_TICK_MS);

	/* enable interrupts */
	_enable();
}

void main()
{
	tx_kernel_enter();
}

