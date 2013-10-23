#include "embsys_sms.h"
#include "embsys_panel.h"
#include "embsys_display.h"
#include "embsys_network.h"
#include "embsys_persistence.h"

/* Time intervals */
#define PANEL_TIMER_DURATION (150/TX_TICK_MS)
#define NETWORK_PERIODIC_SEND_DURATION (1000/TX_TICK_MS)

/* Thread Stack Sizes */
#define PANEL_THREAD_STACK_SIZE (1024)
#define NETWORK_SEND_THREAD_STACK_SIZE (1024)
#define NETWORK_RECEIVE_THREAD_STACK_SIZE (1024)
#define NETWORK_SEND_THREAD_STACK_SIZE (1024)
#define PANEL_PRIORITY (1)
#define NETWORK_PRIORITY (1)

/* SMS sizes */
#define SMS_FILE_NAME_LENGTH 5
#define SMS_BLOCK_SIZE (sizeof(SMS_DELIVER))
#define BLOCK_OVERHEAD (sizeof(void*))
#define SMS_POOL_REAL_SIZE (MAX_NUM_SMS*(SMS_BLOCK_SIZE+BLOCK_OVERHEAD))
#define SMS_DB_BLOCK_SIZE (sizeof(smsNode_t))
#define SMS_DB_POOL_REAL_SIZE (MAX_NUM_SMS*(SMS_DB_BLOCK_SIZE+BLOCK_OVERHEAD))
#define PROBE_MESSAGE_SIZE 8
#define PROBE_MESSAGE_MAX_SIZE 22
#define NETWORK_BUFFER_SIZE 4

/* Display defines */
#define MESSAGE_LISTING_SCREEN_BOTTOM "New   Delete"
#define MESSAGE_DISPLAY_SCREEN_BOTTOM "Back  Delete"
#define MESSAGE_EDIT_SCREEN_BOTTOM "Back  Delete"
#define MESSAGE_NUMBER_SCREEN_BOTTOM "Back  Delete"
#define BLANK_LINE "            "
#define DISPLAY_THREAD_STACK_SIZE 1024
#define DISPLAY_THREAD_PRIORITY 0

/* SMS linked list for the database */
typedef struct
{
    pSmsNode_t head;
    pSmsNode_t tail;
    int size;
} SmsLinkedList;

/* Network send event flags */
typedef enum
{
    SEND_PROBE = 1,
    SEND_PROBE_ACK = 2,
} NetworkSendFlags;

/* Button to character mapping */
const char gButton1[] = {'.',',','?','1'};
const char gButton2[] = {'a','b','c','2'};
const char gButton3[] = {'d','e','f','3'};
const char gButton4[] = {'g','h','i','4'};
const char gButton5[] = {'j','k','l','5'};
const char gButton6[] = {'m','n','o','6'};
const char gButton7[] = {'p','q','r','s','7'};
const char gButton8[] = {'t','u','v','8'};
const char gButton9[] = {'w','x','y','z','9'};
const char gButton0[] = {' ','0'};

/* Input panel thread */
TX_THREAD inputPanelThread;
char inputPanelThreadStack[PANEL_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP inputPanelEventFlags;
void inputPanelThreadMainFunction(unsigned long int v);

/* Display thread */
TX_THREAD displayThread;
char displayThreadStack[DISPLAY_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP displayRefreshEventFlags;
TX_EVENT_FLAGS_GROUP displayIdleEventFlags;
void displayThreadMainLoop(unsigned long int v);

/* Network receive thread */
TX_THREAD nwReceiveThread;
char nwReceiveThreadStack[NETWORK_RECEIVE_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP nwReceiveEventFlags;
void nwReceiveThreadMainFun(unsigned long int v);

/* Network send thread */
TX_THREAD nwSendThread;
char nwSendThreadStack[NETWORK_SEND_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP nwSendEventFlags;
void nwSendThreadMainFun(unsigned long int v);

/* Timers, mutex, block pools */
TX_TIMER continuousButtonPressTimer;
TX_TIMER periodicSendTimer;
TX_MUTEX smsMutex;
TX_BLOCK_POOL smsPool;
TX_BLOCK_POOL smsLinkedListPool;

/* Local SMS variables */
char smsBlockPool[SMS_POOL_REAL_SIZE];
char smsDatabaseBlockPool[SMS_DB_POOL_REAL_SIZE];
SmsLinkedList smsDatabase;
pSmsNode_t firstSmsOnScreen;
pSmsNode_t selectedSms;
SMS_SUBMIT currentEditSms;
SMS_PROBE smsProbe;
int inEditRecipientIdLen;
unsigned int fileNameCounter;

/* Input panel variables */
button pressedButton;
bool continuousButtonPressFlag = false;
button lastPressedButton;
int inContinuousEditNum;

/* Network buffers */
desc_t transmitBuffer[NETWORK_BUFFER_SIZE];
desc_t receiveBuffer[NETWORK_BUFFER_SIZE];
unsigned char receivedData[NETWORK_BUFFER_SIZE*NETWORK_MTU];
char smsMsgBuffer[NETWORK_MTU];
char smsProbeBuffer[PROBE_MESSAGE_MAX_SIZE];

/* Display variables */
screen_type currentScreen;
unsigned char gPacketReceivedBuffer[NETWORK_MTU];
bool refreshDisplayFlag = true;
INT selectedLineIndex = 0;
INT lastSelectedLineIndex = 0;

/* Callbacks */
void periodicSendTimerCB(unsigned long int v);
void buttonPressedCB(button b);
void continuousButtonPressTimerCB(unsigned long int v);
void displayDoneCB();
void rxDoneCB(unsigned char buffer[], unsigned int size, unsigned int length);
void txDoneCB(const unsigned char *buffer, unsigned int size);
void rxPacketDropCB(packet_dropped_reason_t);
void txPacketDropCB(transmit_error_reason_t, unsigned char *buffer, unsigned int size, unsigned int length);

/* ThreadX related prototypes */
TX_STATUS createTimers();
TX_STATUS createThreads();
TX_STATUS createEventFlags();

/* Input panel related prototypes */
void setRefreshDisplay();
void refreshDisplay();
void renderScreen();
void renderMessageListingScreen();
void renderMessageDisplayScreen();
void renderMessageEditScreen();
void renderMessageNumberScreen();
void setMessageListingLineInfo(pSmsNode_t pSms,int serialNumber,char* pLine);
void deleteSmsFromScreen(const pSmsNode_t pFirstSms,const pSmsNode_t pSelectedSms);
int getScreenHeight(const screen_type screen);

/* Display related prototypes */
void handleDisplayScreen(button b);
void handleEditScreen(button but);
void handleAddCharToEditSms(const char buttonX[]);
void handleListingScreen(button b);
void handleNumberScreen(button but);
char getNumberFromButton(button but);
void handleButtonPressed(const button b);

/* SMS handling prototypes */
unsigned int getSmsCount();
void handlePacket();
void deleteSms(const pSmsNode_t pFirstSms,const pSmsNode_t pSelectedSms);
void sendEditSms();
int getSmsSerialNumber(const pSmsNode_t pSms);
void updateBufferCyclic();

/* Persistence-related prototypes */
unsigned int findFileName();
void writeSmsToFileSystem(const message_type type, pSmsNode_t pNewSms, char* data);
void addSmsToLinkedList(pSmsNode_t pNewSms);
void addSmsToLinkedListEnd(pSmsNode_t pNewSms);
void fillSmsNodeFields(char* fileName, const message_type type, pSmsNode_t pNewSms,char* data);
void addSmsToDatabase(void* pSms,const message_type type);
void removeSmsFromDatabase(const pSmsNode_t pSms);
void fillLinkedList(unsigned fileCount);
void createAndAddSmsLinkNodeToDB(char* fileName, const message_type type, char* data);
void getSmsByFileName(const unsigned int fileName, unsigned* smsSize, char* data);

/* initialization */
TX_STATUS sms_init()
{
    /* Create a memory pool for 100 SMS from address smsBlockPool of size sizeof(SMS_DELIVER) each */
    unsigned int status = tx_block_pool_create(&smsPool, "SMS Pool", SMS_BLOCK_SIZE, smsBlockPool, SMS_POOL_REAL_SIZE);
    if(status != TX_SUCCESS) 
    	return status;
    /* Create a memory pool for 100 nodes from smsLinkedListPool of size sizeof(smsNode_t) each */        
    status = tx_block_pool_create(&smsLinkedListPool, "Sms linked list Pool",
        SMS_DB_BLOCK_SIZE, smsDatabaseBlockPool, SMS_DB_POOL_REAL_SIZE);
    /* if that failed then delete the first pool */
    if(status != TX_SUCCESS)
    {
            tx_block_pool_delete(&smsPool);
            return status;
    }
        
    /* initialize the SMS persistence and the database */
	fileNameCounter = 0;
	embsys_persistence_init();
	unsigned fileCount = 0;
	embsys_persistence_count(&fileCount);
    smsDatabase.head = NULL;
    smsDatabase.tail = NULL;
    smsDatabase.size = 0;
	/* fill the database, unless the flash is empty of SMSs */
	if(fileCount!=0)
		fillLinkedList(fileCount);

	/* initilialize the display and input panel */
    status = display_init(displayDoneCB);
    if(status != 0)
        return TX_PTR_ERROR;
    status = embsys_panel_init(buttonPressedCB);
    if(status != 0)
        return TX_PTR_ERROR;

    /* add the device id to the probe sms */
    memcpy(smsProbe.device_id,DEVICE_ID,strlen(DEVICE_ID));

	/* init the screen, SMS variables and the database */
    memset(&currentEditSms,0,sizeof(SMS_SUBMIT));
    currentScreen = MESSAGE_LISTING_SCREEN;
    inEditRecipientIdLen = 0;
    inContinuousEditNum = 0;
    firstSmsOnScreen = smsDatabase.head;
    selectedSms = smsDatabase.head;

    /* initialize the network */
    network_call_back_t callBacks;
    network_init_params_t initParams;
    callBacks.dropped_cb = rxPacketDropCB;
    callBacks.recieved_cb = rxDoneCB;
    callBacks.transmit_error_cb = txPacketDropCB;
    callBacks.transmitted_cb = txDoneCB;
    initParams.callback_list = callBacks;

    for(int i=0 ; i<NETWORK_BUFFER_SIZE; ++i)
    {
        receiveBuffer[i].pBuffer = (unsigned int)(receivedData+i*NETWORK_MTU);
        receiveBuffer[i].buff_size = NETWORK_MTU;
    }

    initParams.transmit_buffer = transmitBuffer;
    initParams.tx_buffer_size = NETWORK_BUFFER_SIZE;
    initParams.receive_buffer = receiveBuffer;
    initParams.rx_buffer_size = NETWORK_BUFFER_SIZE;

    status = network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);
    if(status != 0)
        return status;

    status = network_init(&initParams);
    if(status != 0)
    	return TX_START_ERROR;
		
    /* create the timers, the mutex, the threads and the event flags */
    status = tx_mutex_create(&smsMutex,"SMS Mutex",TX_INHERIT);
    if (status != TX_SUCCESS)
        return status;
    status = createTimers();
    if(status != TX_SUCCESS) 
		return status;
    status = createEventFlags();
    if(status != TX_SUCCESS) 
		return status;
    status = createThreads();
    if(status != TX_SUCCESS) 
		return status;

	/* finish the initialization by refreshing the screen */
	setRefreshDisplay();
	refreshDisplay();
   
	return TX_SUCCESS;
}

/* creates the timers */
TX_STATUS createTimers()
{
    TX_STATUS status;
    status = tx_timer_create(&continuousButtonPressTimer, "Continuous Button Press Timer",
            continuousButtonPressTimerCB, 0, PANEL_TIMER_DURATION, 0, TX_NO_ACTIVATE);
    if(status != TX_SUCCESS) return status;

    status = tx_timer_create(&periodicSendTimer, "Periodic send timer", periodicSendTimerCB,
            0, NETWORK_PERIODIC_SEND_DURATION, NETWORK_PERIODIC_SEND_DURATION, TX_AUTO_ACTIVATE);
    return status;
}

/* call back when the periodic probe timer is alerted */
void periodicSendTimerCB(unsigned long int v)
{
    /* indicate if the probe thread acks or only probes*/
    tx_event_flags_set(&nwSendEventFlags,SEND_PROBE,TX_OR);
}

/* creates the threads */
TX_STATUS createThreads()
{
    TX_STATUS status = tx_thread_create(&inputPanelThread, "Input panel Thread", inputPanelThreadMainFunction, 0, 
    	inputPanelThreadStack, PANEL_THREAD_STACK_SIZE, PANEL_PRIORITY, PANEL_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    if(status != TX_SUCCESS) 
    	return status;

    status = tx_thread_create(&nwReceiveThread, "Network receive Thread", nwReceiveThreadMainFun, 0, nwReceiveThreadStack, 
    	NETWORK_RECEIVE_THREAD_STACK_SIZE, NETWORK_PRIORITY, NETWORK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
	if(status != TX_SUCCESS) 
		return status;

    status = tx_thread_create(&nwSendThread, "Network send Thread", nwSendThreadMainFun, 0, nwSendThreadStack, 
    	NETWORK_SEND_THREAD_STACK_SIZE, NETWORK_PRIORITY,  NETWORK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
	if(status != TX_SUCCESS) 
		return status;
		
	status = tx_thread_create(&displayThread, "Display Thread", displayThreadMainLoop, 0, displayThreadStack, 
		DISPLAY_THREAD_STACK_SIZE, DISPLAY_THREAD_PRIORITY, DISPLAY_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    return status;
}

/* creates the event flags */
TX_STATUS createEventFlags()
{
    TX_STATUS status;
    status = tx_event_flags_create(&inputPanelEventFlags,"Input panel Event Flags");
    if(status != TX_SUCCESS) 
    	return status;

    status = tx_event_flags_create(&nwReceiveEventFlags,"Network receive Event Flags");
    if(status != TX_SUCCESS) 
    	return status;

    status = tx_event_flags_create(&nwSendEventFlags,"Network send Event Flags");
	if (status != TX_SUCCESS)
		return status;
	
	status = tx_event_flags_create(&displayRefreshEventFlags,"Display refresh Event Flags");
	if (status != TX_SUCCESS)
		return status;
	status = tx_event_flags_create(&displayIdleEventFlags,"Display idle Event Flags");
    return status;
}

/* gui main loop */
void displayThreadMainLoop(unsigned long int v)
{
	unsigned long int actualFlag;
        renderScreen();
	while(true)
	{
        /* wait until the display is idle */
        tx_event_flags_get(&displayIdleEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);
        /* wait until someone signals to refresh the screen */
		tx_event_flags_get(&displayRefreshEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);
        renderScreen();
	}
}

/* callback when the display line drawing is done */
void displayDoneCB()
{
	/* signal the thread that the operation is done */
	tx_event_flags_set(&displayIdleEventFlags,1,TX_OR);
}

/* signal the events flag to have the display thread refresh the screen */
void refreshDisplay()
{
	tx_event_flags_set(&displayRefreshEventFlags,1,TX_OR);
}

/* render the current screen */
void renderScreen()
{
	tx_mutex_get(&smsMutex,TX_WAIT_FOREVER);
	switch(currentScreen)
	{
		case MESSAGE_LISTING_SCREEN:
			renderMessageListingScreen();
			break;
		case MESSAGE_EDIT_SCREEN:
			renderMessageEditScreen();
			break;
		case MESSAGE_NUMBER_SCREEN:
			renderMessageNumberScreen();
			break;
		case MESSAGE_DISPLAY_SCREEN:
			renderMessageDisplayScreen();
			break;
	}
	tx_mutex_put(&smsMutex);
}

/* render the listing screen */
void renderMessageListingScreen()
{
	unsigned int smsSerialNumber;
	char line[SCREEN_WIDTH];
	pSmsNode_t pMessage,pSelectedMessage;

	/* put the first message in the iterator pMessage and store it in firstMsg for check */
	pSmsNode_t firstMsg = pMessage = firstSmsOnScreen;
	pSelectedMessage = selectedSms;

	/* get the index of the first message */
	smsSerialNumber = getSmsSerialNumber(pMessage);

	/* fill every line except the last */
	for(int i = 0 ; i < SCREEN_HEIGHT-1 ; ++i)
	{
		/* if the message list is done, fill with blank lines */
		if (!pMessage)
			display_set_row(i,false,BLANK_LINE,SCREEN_WIDTH);
		else
		{
			if (pMessage == smsDatabase.head)
				smsSerialNumber = 0;
			setMessageListingLineInfo(pMessage,smsSerialNumber,line);
			display_set_row(i, (pMessage==pSelectedMessage), line, SCREEN_WIDTH);

			if (pMessage==pSelectedMessage)
			{
				selectedLineIndex = i;
				lastSelectedLineIndex = i;
			}

			pMessage = pMessage->pNext;
			if (pMessage == firstMsg)
				pMessage = NULL;

			smsSerialNumber++;
		}
	}

	/* fill the last line and flush */
	display_set_row(SCREEN_HEIGHT-1,true,MESSAGE_LISTING_SCREEN_BOTTOM,SCREEN_WIDTH);
	refreshDisplayFlag = false;
	display_flush();
}

/* render the edit screen */
void renderMessageEditScreen()
{
	unsigned int i;
	if (refreshDisplayFlag)
	{
		/* clear the screen and display bottom line, then clear flag */
		for(i = 0 ; i < SCREEN_HEIGHT-1 ; ++i)
			display_set_row(i,false,BLANK_LINE,SCREEN_WIDTH);
		display_set_row(SCREEN_HEIGHT-1,true,MESSAGE_EDIT_SCREEN_BOTTOM,SCREEN_WIDTH);
		refreshDisplayFlag = false;
	}
	else
	{
		SMS_SUBMIT* pMessage = &currentEditSms;

		/* get the index of the last row */
		if (pMessage->data_length == 0)
			i = 0;
		else
			i = (pMessage->data_length-1)/SCREEN_WIDTH;

		/* print the last row (add spaces if needed) */
		unsigned int lastRowLen;
		if(pMessage->data_length != 0 && (pMessage->data_length % SCREEN_WIDTH) == 0)
			lastRowLen = SCREEN_WIDTH;
		else
			lastRowLen = pMessage->data_length % SCREEN_WIDTH;
			
		char line[SCREEN_WIDTH];
		/* blank line, then copy the old row and set */
		memset(line,' ',SCREEN_WIDTH);
		memcpy(line,pMessage->data + SCREEN_WIDTH*i,lastRowLen);
		display_set_row(i,false,line,SCREEN_WIDTH);

		/* if the user pressed delete and the current line jumped up, clear the next line */
		if (lastRowLen == SCREEN_WIDTH && lastPressedButton == BUTTON_NUMBER_SIGN)
			display_set_row(i+1,false,BLANK_LINE,SCREEN_WIDTH);
	}

	display_flush();
}

/* render the number screen */
void renderMessageNumberScreen()
{
	unsigned int i;
	SMS_SUBMIT* pMessage = &currentEditSms;
	char line[SCREEN_WIDTH];

	for(i = 0 ; (i < ID_MAX_LENGTH) && (pMessage->recipient_id[i] != (char)NULL) ; ++i)
		line[i] = pMessage->recipient_id[i];

	memset(line+i,' ',SCREEN_WIDTH-i);
	display_set_row(0,false,line,SCREEN_WIDTH);

	/* if refresh, clear the rest of the screen and draw the bottom */
	if (refreshDisplayFlag)
	{
		for(i = 1 ; i < SCREEN_HEIGHT-1 ; ++i)
			display_set_row(i,false,BLANK_LINE,SCREEN_WIDTH);
		display_set_row(SCREEN_HEIGHT-1,true,MESSAGE_NUMBER_SCREEN_BOTTOM,SCREEN_WIDTH);

		refreshDisplayFlag = false;
	}

	display_flush();
}

/* render the display screen */
void renderMessageDisplayScreen()
{
	if (refreshDisplayFlag)
	{
		unsigned int i, dataLength;
		pSmsNode_t pMessage = selectedSms;
		char* pMessageBuffer;
		char line[SCREEN_WIDTH];

		memset(line,' ',SCREEN_WIDTH);
		unsigned length;


		if (pMessage->type == INCOMING_MESSAGE)
		{					
			SMS_DELIVER inMsg;
			SMS_DELIVER* pInMsg = &inMsg;
			length = sizeof(SMS_DELIVER);
			getSmsByFileName(pMessage->fileName, &length, (char*)pInMsg);

			
			/* timestamp */
			line[0]=pInMsg->timestamp[7];
			line[1]=pInMsg->timestamp[6];
			line[2]=':';
			line[3]=pInMsg->timestamp[9];
			line[4]=pInMsg->timestamp[8];
			line[5]=':';
			line[6]=pInMsg->timestamp[11];
			line[7]=pInMsg->timestamp[10];
			display_set_row(1,true,line,SCREEN_WIDTH);

			/* clear, then sender ID */
			memset(line,' ',SCREEN_WIDTH);
			for(i=0 ; (pInMsg->sender_id[i] != (char)NULL) && (i < ID_MAX_LENGTH); ++i)
				line[i] = pInMsg->sender_id[i];
			for(; i<ID_MAX_LENGTH; ++i)
				line[i] = ' ';
				
			/* set data */
			dataLength = pInMsg->data_length;
			pMessageBuffer = pInMsg->data;
		}
		else /* OUTCOMING_MESSAGE */
		{
			/* no timestamp (see README), so empty line */
			display_set_row(1,true,line,SCREEN_WIDTH);
			
			/* recipient ID */			
			SMS_SUBMIT outMsg;
			SMS_SUBMIT* pOutMsg = &outMsg;
			length = sizeof(SMS_SUBMIT);
			getSmsByFileName(pMessage->fileName, &length, (char*)pOutMsg);

			for(i=0 ;(pOutMsg->recipient_id[i] != (char)NULL) && (i < ID_MAX_LENGTH); ++i)
				line[i] =  pOutMsg->recipient_id[i];
			for(; i<ID_MAX_LENGTH; ++i)
				line[i] = ' ';

			/* set data */
			dataLength = pOutMsg->data_length;
			pMessageBuffer = pOutMsg->data;
		}
		
		/* display sender/recipient ID */
		display_set_row(0,true,line,SCREEN_WIDTH);

		/* print full message lines */
		unsigned int lineIndex, fullLines = dataLength/SCREEN_WIDTH;
		for(i = 0,lineIndex=2 ; i < fullLines ; ++i,++lineIndex)
		{
			display_set_row(lineIndex,false,pMessageBuffer,SCREEN_WIDTH);
			pMessageBuffer+=SCREEN_WIDTH;
		}

		/* display last line, pad with spaces */
		unsigned int remainingChars =  dataLength %  SCREEN_WIDTH;
		if (remainingChars > 0)
		{
			memset(line,' ',SCREEN_WIDTH);
			memcpy(line,pMessageBuffer,remainingChars);
			display_set_row(lineIndex++,false,line,SCREEN_WIDTH);
		}

		/* display empty lines, then botton row */
		for(;lineIndex<SCREEN_HEIGHT-1;++lineIndex)
			display_set_row(lineIndex,false,BLANK_LINE,SCREEN_WIDTH);
		display_set_row(SCREEN_HEIGHT-1,true,MESSAGE_DISPLAY_SCREEN_BOTTOM,SCREEN_WIDTH);

		/* clear the refresh flag */
		refreshDisplayFlag = false;
	}

	display_flush();
}

/* the main function for the keyPad thread */
void inputPanelThreadMainFunction(unsigned long int v)
{
    unsigned long int actualFlag;

    while(true)
    {
            /* wait for the flags to wake up the thread */
            tx_event_flags_get(&inputPanelEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);

            /* handle the button that was pressed */
            handleButtonPressed(pressedButton);
    }
}

/* call back when the continuous button press timer expires */
void continuousButtonPressTimerCB(unsigned long int v)
{
    continuousButtonPressFlag = false;
}

/* call back when a button is pressed */
void buttonPressedCB(button b)
{
	/* save the button and notify the input panel thread */
    pressedButton = b;
    tx_event_flags_set(&inputPanelEventFlags,1,TX_OR);
}

/* handle a button press by current screen */
void handleButtonPressed(const button b)
{
    if (tx_mutex_get(&smsMutex,TX_WAIT_FOREVER) != TX_SUCCESS)
        return;
    
    switch (currentScreen)
    {
    case MESSAGE_DISPLAY_SCREEN:
        handleDisplayScreen(b);
        break;
    case MESSAGE_EDIT_SCREEN:
        handleEditScreen(b);
        break;
    case MESSAGE_LISTING_SCREEN:
        handleListingScreen(b);
        break;
    case MESSAGE_NUMBER_SCREEN:
        handleNumberScreen(b);
        break;
    }

	/* udpate fields */
    lastPressedButton = b;
    continuousButtonPressFlag = true;

    /* stop the timer, update it and reactivate it */
    if(tx_timer_deactivate(&continuousButtonPressTimer) == TX_SUCCESS)
		if(tx_timer_change(&continuousButtonPressTimer, PANEL_TIMER_DURATION,0) == TX_SUCCESS)
			tx_timer_activate(&continuousButtonPressTimer);

    tx_mutex_put(&smsMutex);
}

/* handle a button press on the listing screen */
void handleListingScreen(button b)
{
    pSmsNode_t pSelectedSms = selectedSms;
    pSmsNode_t pFirstSms = firstSmsOnScreen;

    switch (b)
    {
    case BUTTON_2: /* up */
		if (pSelectedSms == NULL)
			return;
        /* if first row selected and there are more messages, replace the top with the previous
         * for future rendering */
        if (selectedLineIndex==0 && getSmsCount() > getScreenHeight(MESSAGE_LISTING_SCREEN))
            firstSmsOnScreen = pFirstSms->pPrev;
        selectedSms = pSelectedSms->pPrev;
        break;
    case BUTTON_8: /* down */
		if (pSelectedSms == NULL)
			return;
        /* if last row is selected and there are more messages, replace the top SMS with the next
         * for future rendering */
        if(((selectedLineIndex+1 == getSmsCount()) || (selectedLineIndex+1==(SCREEN_HEIGHT-1))) && 
        	(getSmsCount() > getScreenHeight(MESSAGE_LISTING_SCREEN)))
            firstSmsOnScreen = pFirstSms->pNext;
        selectedSms = pSelectedSms->pNext;
        break;
    case BUTTON_STAR: /* new message */
        /* if there is no room for another message, abort */
        if (getSmsCount() == MAX_NUM_SMS)
            break;
        /* reset inEditSms */
        SMS_SUBMIT* inEditSms = &currentEditSms;
		inEditSms->data_length = 0;
		memset(inEditSms->recipient_id ,0, ID_MAX_LENGTH);
		memset(inEditSms->device_id ,0, ID_MAX_LENGTH);
		/* go to message edit screen */
        currentScreen = MESSAGE_EDIT_SCREEN;
        setRefreshDisplay();
        break;
    case BUTTON_NUMBER_SIGN: /* delete message */
		if (pSelectedSms == NULL)
			return;
		/* delete the selected SMS and refresh display */
        deleteSms(pFirstSms,pSelectedSms);
        setRefreshDisplay();
        break;
    case BUTTON_OK: /* display message */
		if (pSelectedSms == NULL)
			return;
		/* go to message display screen */
        currentScreen = MESSAGE_DISPLAY_SCREEN;
        setRefreshDisplay();
        break;
    }

    refreshDisplay();
}

/* handle a button press on the edit screen */
void handleEditScreen(button b)
{
	/* increment the typing counter past the first continuous press */
    if(b != lastPressedButton || !continuousButtonPressFlag)
        inContinuousEditNum = 0;
    else
        inContinuousEditNum++;
    
    SMS_SUBMIT* inEditSms = &currentEditSms;
    switch (b)
    {
    case BUTTON_OK: /* continue to the number screen */
        inEditRecipientIdLen = 0;
        memset(inEditSms->recipient_id ,0, ID_MAX_LENGTH);
        currentScreen = MESSAGE_NUMBER_SCREEN;
        setRefreshDisplay();
        refreshDisplay();
        break;
    case BUTTON_STAR: /* cancel */
        currentScreen = MESSAGE_LISTING_SCREEN;
        setRefreshDisplay();
        refreshDisplay();
        break;
    case BUTTON_NUMBER_SIGN: /* delete the last char */
        if(inEditSms->data_length > 0)
        {
                inEditSms->data_length--;
                refreshDisplay();
        }
        break;
    case BUTTON_1:
        handleAddCharToEditSms(gButton1);
        break;
    case BUTTON_2:
        handleAddCharToEditSms(gButton2);
        break;
    case BUTTON_3:
        handleAddCharToEditSms(gButton3);
        break;
    case BUTTON_4:
        handleAddCharToEditSms(gButton4);
        break;
    case BUTTON_5:
        handleAddCharToEditSms(gButton5);
        break;
    case BUTTON_6:
        handleAddCharToEditSms(gButton6);
        break;
    case BUTTON_7:
        handleAddCharToEditSms(gButton7);
        break;
    case BUTTON_8:
        handleAddCharToEditSms(gButton8);
        break;
    case BUTTON_9:
        handleAddCharToEditSms(gButton9);
        break;
    case BUTTON_0:
        handleAddCharToEditSms(gButton0);
        break;
    }
}

/* handle a button press on the message number screen */
void handleNumberScreen(button b)
{
    SMS_SUBMIT* inEditSms = &currentEditSms;

    switch (b)
    {
    case BUTTON_OK: /* send the edited sms */
        if (inEditRecipientIdLen == 0)
            return;
        /* send the message, go back to listing screen and refresh */
        sendEditSms();
        currentScreen = MESSAGE_LISTING_SCREEN;
        setRefreshDisplay();
        refreshDisplay();
        return;
    case BUTTON_STAR: /* cancel */
    	/* go back to listing screen and refresh */
        currentScreen = MESSAGE_LISTING_SCREEN;
        setRefreshDisplay();
        refreshDisplay();
        return;
    case BUTTON_NUMBER_SIGN: /* delete the last digit */
        if (inEditRecipientIdLen == 0)
                return;
        /* decrease the length, erase the digit and refresh */
        inEditRecipientIdLen--;
        inEditSms->recipient_id[inEditRecipientIdLen] = 0;
        refreshDisplay();
        return;
    }
    
    /* if the field is full, replace the last digit */
    if(inEditRecipientIdLen == ID_MAX_LENGTH)
        inEditSms->recipient_id[inEditRecipientIdLen-1] = getNumberFromButton(b);
    else
    {
        inEditSms->recipient_id[inEditRecipientIdLen] = getNumberFromButton(b);
        inEditRecipientIdLen++;
    }

    refreshDisplay();
}

/* handle a button press on the display screen */
void handleDisplayScreen(button b)
{
    switch (b)
    {
    case BUTTON_STAR: /* back: go back to the listing screen */
        currentScreen = MESSAGE_LISTING_SCREEN;
        break;
    case BUTTON_NUMBER_SIGN: /* delete the displayed sms */
    	/* delete from the database and go back to the listing screen */
        currentScreen = MESSAGE_LISTING_SCREEN;
        pSmsNode_t pSelectedSms = selectedSms;
        pSmsNode_t pFirstSms = firstSmsOnScreen;
        deleteSms(pFirstSms, pSelectedSms);
        break;
    }
    setRefreshDisplay();
    refreshDisplay();
}

/* the main function for the networkReceive thread */
void nwReceiveThreadMainFun(unsigned long int v)
{
    unsigned long int actualFlag;
    while(true)
    {
        /* wait for a flag wake up the thread */
        tx_event_flags_get(&nwReceiveEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);

        /* handle the packet */
        handlePacket();
        tx_timer_activate(&periodicSendTimer);
    }
}

/* packet received */
void rxDoneCB(unsigned char buffer[], unsigned int size, unsigned int length)
{
    /* stop the periodic timer to avoid receiving the same SMS several times */
    tx_timer_deactivate(&periodicSendTimer);

    /* copy the packet to the buffer and notify nwReceive thread */
    memcpy(gPacketReceivedBuffer,buffer,length);
    tx_event_flags_set(&nwReceiveEventFlags,1,TX_OR);
}

/* dropped receiving packet */
void rxPacketDropCB(packet_dropped_reason_t)
{
}

/* handle a received packet */
void handlePacket()
{
	/* try to parse into a submit ack */
	SMS_SUBMIT_ACK submitAck;
    EMBSYS_STATUS status = embsys_parse_submit_ack((char*)gPacketReceivedBuffer,&submitAck);
    if(status == SUCCESS) 
    	return;

    /* try to parse into an incoming message sms */
    SMS_DELIVER deliverSms;
    status = embsys_parse_deliver((char*)gPacketReceivedBuffer,&deliverSms);
    if(status != SUCCESS) 
    	return;

    /* if it is incoming, add it to the linked list */
    if(tx_mutex_get(&smsMutex,TX_WAIT_FOREVER) != TX_SUCCESS)
        return;
    addSmsToDatabase(&deliverSms,INCOMING_MESSAGE);
    
    /* if there is only one sms, update the relevant fields */
    if (getSmsCount() == 1)
    {
            firstSmsOnScreen = smsDatabase.head;
            selectedSms = smsDatabase.head;
    }
    tx_mutex_put(&smsMutex);

    /* probe ack: set sender ID and signal the network send thread */
    memcpy(smsProbe.sender_id,deliverSms.sender_id,ID_MAX_LENGTH);
    memcpy(smsProbe.timestamp,deliverSms.timestamp,TIMESTAMP_MAX_LENGTH);
    tx_event_flags_set(&nwSendEventFlags,SEND_PROBE_ACK,TX_OR);

    /* if the screen is on the message listing screen, refresh the display */
    if(currentScreen == MESSAGE_LISTING_SCREEN)
    {
            setRefreshDisplay();
            refreshDisplay();
    }
}

/* packet transmission done */
void txDoneCB(const unsigned char *buffer, unsigned int size)
{
}
/* dropped transmission packet */
void txPacketDropCB(transmit_error_reason_t, unsigned char *buffer, unsigned int size, unsigned int length )
{
}

/* the main function for the networkSend thread */
void nwSendThreadMainFun(unsigned long int v)
{
    unsigned long int actualFlag;

    while(true)
    {
            /* wait for a flag wake up the thread */
            tx_event_flags_get(&nwSendEventFlags,SEND_PROBE|SEND_PROBE_ACK,TX_OR_CLEAR,&actualFlag,TX_WAIT_FOREVER);

			/* send a probe/ack */
            unsigned int bufLen = 0;
            embsys_fill_probe(smsProbeBuffer,&smsProbe,actualFlag & SEND_PROBE_ACK,&bufLen);
            network_send_packet((unsigned char*)smsProbeBuffer,bufLen,bufLen);
    }
}

/* send the edited sms */
void sendEditSms()
{
	unsigned int dataLen;
	SMS_SUBMIT* smsToSend = (SMS_SUBMIT*)&currentEditSms;

	/* set device id, submit, then send */
    memset(smsToSend->device_id, 0, ID_MAX_LENGTH);
    memcpy(smsToSend->device_id, DEVICE_ID, strlen(DEVICE_ID));
    embsys_fill_submit(smsMsgBuffer,smsToSend,&dataLen);
    network_send_packet((unsigned char*)smsMsgBuffer,dataLen,dataLen);
    
    if(tx_mutex_get(&smsMutex,TX_WAIT_FOREVER) != TX_SUCCESS)
    	return;

    /* add the sms to the database and update the fields */
    addSmsToDatabase(smsToSend, OUTGOING_MESSAGE);
    
    /* if there is only one sms, update the relevant fields */
    if (getSmsCount() == 1)
    {
            firstSmsOnScreen = smsDatabase.head;
            selectedSms = smsDatabase.head;
    }
    
    tx_mutex_put(&smsMutex);
}

/* delete an sms from the database and remove it from the screen */
void deleteSms(const pSmsNode_t pFirstSms,const pSmsNode_t pSelectedSms)
{
    deleteSmsFromScreen(pFirstSms,pSelectedSms);
    removeSmsFromDatabase(pSelectedSms);
}

/* get the number of SMS in the database */
unsigned int getSmsCount()
{
	return smsDatabase.size;
}

/* get the value mapped to the given button */
char getNumberFromButton(button b)
{
    int counter = 0;
    while (b != 0)
    {
            b = b >> 1;
            counter++;
    }
    return '0' + (counter-1)%11;
}

/* set the refresh flag which indicates whether the view should refresh the whole screen */
void setRefreshDisplay()
{
	refreshDisplayFlag = true;
}

/* get the avilable height of the given screen (taken out all reserved lines) */
int getScreenHeight(const screen_type screen)
{
	switch(screen)
	{
		case MESSAGE_NUMBER_SCREEN:
			return SCREEN_HEIGHT - 1;
		case MESSAGE_DISPLAY_SCREEN:
			return SCREEN_HEIGHT - 3;
		case MESSAGE_LISTING_SCREEN:
		case MESSAGE_EDIT_SCREEN:
		default:
			return SCREEN_HEIGHT;
	}
}

/* get an sms from the database and fill the line */
void setMessageListingLineInfo(pSmsNode_t pSms,int serialNumber,char* pLine)
{
	*pLine++ = decToHex(serialNumber/10);
	*pLine++ = decToHex(serialNumber%10);
	*pLine++ = ' ';

	if (pSms->type == INCOMING_MESSAGE)
	{
		for(int i = 0 ; (pSms->title[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
			*pLine++ = pSms->title[i];
		for(;i<ID_MAX_LENGTH ; ++i)
			*pLine++ = ' ';
		*pLine++ = 'I';
	}
	else /* OUTGOING_MESSAGE */
	{
		for(int i = 0 ; (pSms->title[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
			*pLine++ = pSms->title[i];
		for(;i<ID_MAX_LENGTH ; ++i)
			*pLine++ = ' ';
		*pLine++ = 'O';
	}
}

/* delete the selected sms from the screen */
void deleteSmsFromScreen(const pSmsNode_t pFirstSms,const pSmsNode_t pSelectedSms)
{
    if (getSmsCount() == 0)
    	return;

    if (selectedLineIndex==0)
    {
        /* if this is the first only SMS, set selected to NULL */
        if (getSmsCount() == 1)
        {
            //set the first on screen snd the selected to null
            firstSmsOnScreen = NULL;
            selectedSms = NULL;
        }
        else /* deleted SMS is first but not only, so just get to the next */
            firstSmsOnScreen = pFirstSms->pNext;
    }

    selectedSms = pSelectedSms->pNext;
}

/* handles the added char to the edited sms */
void handleAddCharToEditSms(const char buttonX[])
{
    SMS_SUBMIT* inEditSms = &currentEditSms;
    if(inContinuousEditNum > 0 || inEditSms->data_length == DATA_MAX_LENGTH)
    {
        /* if max length reached, replace the last char */
        inEditSms->data[inEditSms->data_length-1] = buttonX[inContinuousEditNum % strlen(buttonX)];
    }
    else
    {
        /* add char to the end of the sms */
        inEditSms->data[inEditSms->data_length] = buttonX[0];
        inEditSms->data_length++;
    }

    refreshDisplay();
}

/* get the index of an sms */
int getSmsSerialNumber(const pSmsNode_t pSms)
{
	if(!pSms)
		return 0;
	
    pSmsNode_t pNode = smsDatabase.head;
    for(int serialNum = 0; serialNum < getSmsCount(); ++serialNum, pNode = pNode->pNext)
    	if(pNode == pSms) 
    		return serialNum;

    /* sms pointer was not found */
    return -1;        
}

/* make the database's linked list cyclic by having the head and tail point to each other */
void updateBufferCyclic()
{
    smsDatabase.head->pPrev = smsDatabase.tail;
    smsDatabase.tail->pNext = smsDatabase.head;
}

/******** PERSISTENCE-RELATED FUNCTIONS *************/

/* appends a new SMS node to the database */
void createAndAddSmsLinkNodeToDB(char* fileName, const message_type type, char* data)
{
	/* create an SMS and allocate a block for it */
	pSmsNode_t pNewSms;
	tx_block_allocate(&smsLinkedListPool, (VOID**)&pNewSms,TX_NO_WAIT);

	/* fill its fields and add it to the database */
	fillSmsNodeFields(fileName, type, pNewSms, data);
	addSmsToLinkedList(pNewSms);
}

/* fill the database fron flash using the persistence module */
void fillLinkedList(unsigned fileCount)
{
	FS_STATUS fsStatus = SUCCESS;
	unsigned i;
	unsigned smsSize;
	char data[SMS_BLOCK_SIZE];
	char pFileName[SMS_FILE_NAME_LENGTH];
	unsigned fileNameLength;
	for ( i = 0 ; i < fileCount ; ++i )
	{
		/* reset the variables */
		smsSize = SMS_BLOCK_SIZE;
		fileNameLength = SMS_FILE_NAME_LENGTH;
		memset(pFileName,0,SMS_FILE_NAME_LENGTH);
		memset(data,0,SMS_BLOCK_SIZE);

		/* fetch the file name */
		fsStatus = embsys_persistence_get_filename_by_index(i, &fileNameLength,(char*) pFileName);
		if(fsStatus != SUCCESS) 
			continue;

		/* read the file */
		fsStatus = embsys_persistence_read((char*)pFileName,&smsSize,(char*)data);
		if(fsStatus != SUCCESS) 
			continue;

		/* add the SMS to the database */
		if(smsSize == sizeof(SMS_DELIVER))
			createAndAddSmsLinkNodeToDB((char*)pFileName, INCOMING_MESSAGE, (char*)data);
		else if(smsSize == sizeof(SMS_SUBMIT))
			createAndAddSmsLinkNodeToDB((char*)pFileName, OUTGOING_MESSAGE, (char*)data);
		else
			continue;
			
		unsigned int fileNumber = *(unsigned int*)pFileName;
		fileNameCounter = _max(fileNameCounter,fileNumber);
	}
}

/* fetch an SMS by its filename */
void getSmsByFileName(const unsigned int fileName, unsigned* smsSize, char* data)
{
	char pFileName[SMS_FILE_NAME_LENGTH] = {0};
	*((unsigned int*)pFileName) = fileName;
	embsys_persistence_read((char*)pFileName, smsSize, data);
}

/* find the next available filename by counting the SMSs */
unsigned int findFileName()
{
	char pFileName[SMS_FILE_NAME_LENGTH];
	unsigned int tempDataLength, fileNumber;
	char tempData[1];
	FS_STATUS status;
	do
	{
		/* reset the variables */
		tempDataLength = 1;
		memset(pFileName,0,SMS_FILE_NAME_LENGTH);

		/* get the next file name */
		fileNumber = fileNameCounter;
		*(unsigned int*)pFileName = fileNameCounter;
		
		/* check if the name is available */
		status = embsys_persistence_read((char*)pFileName, &tempDataLength, (char*)tempData);
		fileNameCounter = ++fileNameCounter;
	}while(status != FILE_NOT_FOUND);
	return fileNumber;
}

/* store an SMS to flash */
void writeSmsToFileSystem(const message_type type, pSmsNode_t pNewSms, char* data)
{
	if(pNewSms == NULL || data == NULL)
		return;
	char pFileName[SMS_FILE_NAME_LENGTH] = {0};
	*(unsigned int*)pFileName = pNewSms->fileName;
	switch(type)
	{
	case INCOMING_MESSAGE:
		embsys_persistence_write((char*)pFileName, sizeof(SMS_DELIVER), data);
		break;
	case OUTGOING_MESSAGE:
		embsys_persistence_write((char*)pFileName, sizeof(SMS_SUBMIT), data);
	}
}

/* add an sms to the database */
void addSmsToLinkedList(pSmsNode_t pNewSms)
{
	/* if the database is empty or the SMS needs to be inserted at the tail, do that */
	if(smsDatabase.size == 0 || smsDatabase.tail->fileName < pNewSms->fileName)
	{
		addSmsToLinkedListEnd(pNewSms);
		return;
	}

	/* if there is only one node, insert the second node in to the list */
	if(smsDatabase.size == 1)
	{
		smsDatabase.head = pNewSms;
		smsDatabase.head->pNext = smsDatabase.tail;
		smsDatabase.tail->pPrev = smsDatabase.head;

	}
	/* if the SMS needs to be inserted at the beginning of the list, update the head */
	else if(smsDatabase.head->fileName > pNewSms->fileName)
	{
		pNewSms->pNext = smsDatabase.head;
		smsDatabase.head->pPrev = pNewSms;
		smsDatabase.head = pNewSms;
	}
	/* if the SMS needs to be inserted inside the list, update the tail, iterate until the relevant node and update the links */
	else
	{
		pSmsNode_t iterator;
		for(iterator = smsDatabase.head ; iterator->fileName < pNewSms->fileName ; iterator = iterator->pNext);
		
		pSmsNode_t iteratorPrev = iterator->pPrev;
		iterator->pPrev = pNewSms;
		pNewSms->pNext = iterator;
		iteratorPrev->pNext = pNewSms;
		pNewSms->pPrev = iteratorPrev;
	}
	
	/* update the database as a cyclic list and increment the database size */
	updateBufferCyclic();
	smsDatabase.size++;
}

/* add the SMS to the end of the linked list database */
void addSmsToLinkedListEnd(pSmsNode_t pNewSms)
{
	/*  the database is empty, setup as a cyclic linked list */
	if(smsDatabase.size == 0)
	{
		smsDatabase.head = pNewSms;
		smsDatabase.tail = pNewSms;
	}
	/* if it has only one node, set the second message */
	else if(smsDatabase.size == 1)
	{
		smsDatabase.tail = pNewSms;
		smsDatabase.head->pNext = smsDatabase.tail;
		smsDatabase.tail->pPrev = smsDatabase.head;
	}
	/* otherwise, just set as tail and update */
	else
	{
		smsDatabase.tail->pNext = pNewSms;
		pNewSms->pPrev = smsDatabase.tail;
		smsDatabase.tail = pNewSms;
	}

	/* update the linked list to be cyclic and increment database size */
	updateBufferCyclic();
	smsDatabase.size++;
}

/* fill the SMS node fields */
void fillSmsNodeFields(char* fileName, const message_type type, pSmsNode_t pNewSms,char* data)
{
	if(pNewSms == NULL || data == NULL)
		return;
	
	/* set the title to be displayed on screen with sender or recipient ID */
	int i;
	memset(pNewSms->title,0,sizeof(ID_MAX_LENGTH));
	switch(type)
	{
	case INCOMING_MESSAGE:
		SMS_DELIVER* pInSms = (SMS_DELIVER*)data;
		for(i = 0 ; (pInSms->sender_id[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
			pNewSms->title[i] = pInSms->sender_id[i];
		break;
	case OUTGOING_MESSAGE:
		SMS_SUBMIT* pOutSms = (SMS_SUBMIT*)data;
		for(i = 0 ; (pOutSms->recipient_id[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
			pNewSms->title[i] = pOutSms->recipient_id[i];
	}

	/* set the node fields relevant to the linked list */
	pNewSms->pNext = NULL;
	pNewSms->pPrev = smsDatabase.tail;
	pNewSms->type = type;

	/* fill the file name */
	char* pFileName = (char*)&pNewSms->fileName;
	for(i = 0; i < SMS_FILE_NAME_LENGTH-1 ; ++i)
		*pFileName++ = *fileName++;
}

/* add an SMS to the database, parsing ans filling its fields */
void addSmsToDatabase(void* pSms,const message_type type)
{
	/* create the new SMS and allocate a block in the pool for it */
	pSmsNode_t pNewSms;
	tx_block_allocate(&smsLinkedListPool, (VOID**)&pNewSms,TX_NO_WAIT);
	
	/* find a file name for it and fill the fields */
	char fileName[SMS_FILE_NAME_LENGTH] = {0};
	*(unsigned int*)fileName = findFileName();
	fillSmsNodeFields(fileName, type, pNewSms,(char*)pSms);

	/* commit the sms to the persistence module */
	writeSmsToFileSystem(type,pNewSms,(char*)pSms);
	addSmsToLinkedListEnd(pNewSms);
}

/* remove an SMS from the flash and from the database */
void removeSmsFromDatabase(const pSmsNode_t pSms)
{	
    if(getSmsCount() == 0)
        return;
	
	/* first, remove the SMS from flash */
	char pFileName[SMS_FILE_NAME_LENGTH] = {0};
	*(unsigned int*)pFileName = pSms->fileName;
	embsys_persistence_erase((char*)pFileName);

    /* if it is the only node, we get an empty list */
    if(getSmsCount() == 1)
    {
		smsDatabase.head = NULL;
		smsDatabase.tail = NULL;
    }
    /* if we remove the head, we must fix it */
    else if(pSms == smsDatabase.head)
    {
        smsDatabase.head = pSms->pNext;
        smsDatabase.head->pPrev = NULL;
        updateBufferCyclic();
    }
    /* if we remove the tail, we must fix it */
    else if(pSms == smsDatabase.tail)
    {
        smsDatabase.tail = pSms->pPrev;
        smsDatabase.tail->pNext = NULL;
        updateBufferCyclic();
    }
    else
    {
        /* general case: unlink the node */
        pSmsNode_t pPrevNode = pSms->pPrev;
        pPrevNode->pNext = pSms->pNext;
        pSms->pNext->pPrev = pPrevNode;
    }
    
    /* release the link's memory block to the sms linked list pool and decrement database size */
    tx_block_release(pSms);
    smsDatabase.size--;
}
