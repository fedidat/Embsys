#include "embsys_utilities.h"
#include "tx_api.h"
#include "embsys_timer.h"
#include "embsys_sms.h"

#define SLEEP_PERIOD 1

TX_THREAD mainInitThread;
CHAR mainInitThreadStack[TX_MINIMUM_STACK];
void mainInitThreadMainFunc(ULONG v)
{
    sms_init();
}

TX_THREAD idleThread;
CHAR idleThreadStack[TX_MINIMUM_STACK];
void idleThreadMainFunc(ULONG v)
{
    while(true)
        _sleep(SLEEP_PERIOD);
}

void tx_application_define(void *first) 
{
	tx_thread_create(&mainInitThread, "Main Init Thread", mainInitThreadMainFunc, 0, mainInitThreadStack,
        TX_MINIMUM_STACK, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&idleThread, "Idle thread", idleThreadMainFunc, 0, idleThreadStack,
        TX_MINIMUM_STACK, TX_MAX_PRIORITIES-1, TX_MAX_PRIORITIES-1, 1,TX_AUTO_START);
    embsys_timer_init(TX_TICK_MS);
    _enable();
}

void main()
{
	tx_kernel_enter();
}

