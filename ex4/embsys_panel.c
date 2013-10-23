#include "embsys_panel.h"

#define PADSTAT (0x1E0)
#define PADIER  (0x1E1)
#define PANEL_INTERRUPT_ENABLE 0x1FFF

void (*panelCallback)(button key);

/* initialization */
unsigned int embsys_panel_init(void (*callback)(button key))
{
    if(!callback)
        return 1;
    
    /* store callback and enable interrupts */
	panelCallback = callback;
	_sr(PANEL_INTERRUPT_ENABLE, PADIER);
    return 0;
}

/* ISR for the input panel, asserted on IRQ9 */
void buttonPressedISR() {
	/* callback with button value */
	(*panelCallback)((int)_lr(PADSTAT));
}

