#include "fsm_logic.h"
#include "usart.h" //per la comunicazione seriale
#include "string.h" //per la gestione delle stringhe
#define CODE_LENGTH		(4) //lunghezza del codice + il terminatore

/**
 * Enumeration machine's states
 */
typedef enum fsm_state_enum{
	COLLECT_CODE=0,
	CHECK_CODE=1,
	CORRECT_CODE=2,
	INCORRECT_CODE=3,

} fsm_state_t;

/*
 * The FSM is a Moore machine that is updated at each step cycle
 * The input are read before evaluating state and output changes
 * therefore, we need to store the value from each input device at a given cycle
 *
 * This structure represent the state read from the input devices at each cycle
 * it buffers the state of each input ensuring that is stable for the overall cycle duration
 */
typedef struct FSM_input_s{
	uint8_t buffer_ready;
	uint8_t buffer[CODE_LENGTH];


}FSM_input_t;


/**
 * FSM Main Structure
 * It is composed by the input and outputs as well as
 * the current status of the machine and the current input reads
 */
typedef struct FSM_s
{
	fsm_state_t state;
	led_t* led_verde;
	FSM_input_t in;

	uint8_t real_code[CODE_LENGTH];//codice reale
	uint8_t rx_buffer[CODE_LENGTH]; //codice inserito dall'utente
	uint8_t rx_index; //indice del codice inserito dall'utente
	uint8_t uart_rx_byte;           // byte per HAL_UART_Receive_IT
	uint8_t code_correct; //flag per indicare se il codice inserito dall'utente è corretto
	uint8_t uart_tx_completed; //flag per indicare se il messaggio è stato inviato
	volatile uint8_t buffer_ready;  // flag grezzo (come alarm_state)




} FSM_t;


/*
 * Private machine state
 */
static FSM_t fsm;


/*
 * Private function to read and buffers the inputs
 */
static int8_t FSM_read_inputs();
static int8_t FSM_update_state();


/*
 * Private function to update the current status and the output
 */
static void printMessage(const char* msg);
static int8_t FSM_collect_code(void);
static int8_t FSM_check_code(void);
static int8_t FSM_correct_code(void);
static int8_t FSM_incorrect_code(void);


/*
 * Public init function
 */
int8_t FSM_init(led_t* led_verde, uint32_t cycle_duration)
{
	if(!led_verde){
		return FSM_ERR;
	}

	fsm.state = COLLECT_CODE;
	fsm.led_verde = led_verde;
	fsm.uart_tx_completed=1;
	fsm.rx_index=0;
	fsm.code_correct=0;
	fsm.buffer_ready=0;
	fsm.real_code[0]=1;
	fsm.real_code[1]=2;
	fsm.real_code[2]=3;
	fsm.real_code[3]=4;

	printMessage("Inserisci codice (4 cifre):\r\n");
	//armo la ricezione per essere pronto a ricevere il primo byte
	HAL_UART_Receive_IT(&hlpuart1, &fsm.uart_rx_byte,1);
	return FSM_OK;
}

/*
 * Public step function
 */
int8_t FSM_step(){
	int8_t res = FSM_ERR;
	uint32_t cycle_start = 0;
	uint32_t cycle_runtime = 0;

	cycle_start = HAL_GetTick();

	if(FSM_read_inputs() == FSM_OK){
		res = FSM_OK;
	}

	if( (res == FSM_OK) && (FSM_update_state() != FSM_OK) ){
		res = FSM_ERR;
	}

	cycle_runtime = HAL_GetTick() - cycle_start;

	if(FSM_CYCLE_DURATION > cycle_runtime)
	{
		HAL_Delay(FSM_CYCLE_DURATION - cycle_runtime);
	}
	else
	{
		res = FSM_ERR;
	}

	return res;
}

//********************************************************************************
//******	STATIC FUNCTIONS
//**************************************************************************
static void printMessage(const char* msg){
	if(fsm.uart_tx_completed){
		fsm.uart_tx_completed=0;
		HAL_UART_Transmit_IT(&hlpuart1,(uint8_t*) msg, strlen(msg));
	}
}

static int8_t FSM_read_inputs(){
	int8_t res = FSM_ERR;
	//ad ogni ciclo, leggo lo stato del buffer_ready e negli input della fsm
	fsm.in.buffer_ready = fsm.buffer_ready;
	//resetto il falg per far in modo che la fsm possa leggere nuovamente il buffer_ready
	fsm.buffer_ready = 0;
	//se il buffer è pronto leggo i 4 valori e li memorizzo nel buffer della fsm
	if(fsm.in.buffer_ready){
		for(uint8_t i=0; i<CODE_LENGTH; i++){
			fsm.in.buffer[i] = fsm.rx_buffer[i];
		}
		res = FSM_OK;
	}
	return FSM_OK;
}

static int8_t FSM_update_state(){
	int8_t res = FSM_ERR;


	switch(fsm.state)
	{
		case COLLECT_CODE:
			res = FSM_collect_code();
			break;
		case CHECK_CODE:
			res = FSM_check_code();
			break;
		case CORRECT_CODE:
			res = FSM_correct_code();
			break;
		case INCORRECT_CODE:
			res = FSM_incorrect_code();
			break;
		default:
			res = FSM_ERR;
			break;
	}
	return res;
}

static int8_t FSM_collect_code(void)
{
	//finche non ho ricevuto le 4 cifre, aspetto
	if (!fsm.in.buffer_ready)
		return FSM_OK;   // aspetta le 4 cifre
	//se ho ricevuto le 4 cifre, passo allo stato di check_code
	fsm.state = CHECK_CODE;
	return FSM_OK;
}


static int8_t FSM_check_code(void)
{
	
	fsm.code_correct = 1;
	for (uint8_t i = 0; i < CODE_LENGTH; i++) {
		if (fsm.in.buffer[i] != fsm.real_code[i]) {
			fsm.code_correct = 0;
			break;
		}
	}
	if (fsm.code_correct)
		fsm.state = CORRECT_CODE;
	else
		fsm.state = INCORRECT_CODE;
	return FSM_OK;
}


static int8_t FSM_correct_code(void)
{
	led_on(fsm.led_verde);
	printMessage("Codice corretto!\r\n");
	fsm.state = COLLECT_CODE;
	printMessage("Inserisci codice (4 cifre):\r\n");
	return FSM_OK;
}

static int8_t FSM_incorrect_code(void)
{
	led_off(fsm.led_verde);
	printMessage("Codice errato!\r\n");
	fsm.state = COLLECT_CODE;
	printMessage("Inserisci codice (4 cifre):\r\n");
	return FSM_OK;
}


//********************************************************************************
//******	CALLBACKS (if needed)
//**************************************************************************
//ho ricevuto il byte su uart_rx_byte  correttamente
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == LPUART1) {
		if(fsm.uart_rx_byte>='0' && fsm.uart_rx_byte<='9' && fsm.rx_index<CODE_LENGTH){
			fsm.rx_buffer[fsm.rx_index]=fsm.uart_rx_byte-48;
			fsm.rx_index++;

			/* debug: stampa cifra e posizione a ogni inserimento */
			{
				char dbg[16];
				dbg[0] = 'C'; dbg[1] = 'i'; dbg[2] = 'f'; dbg[3] = 'r';
				dbg[4] = 'a'; dbg[5] = ' '; dbg[6] = fsm.uart_rx_byte;
				dbg[7] = ' '; dbg[8] = '('; dbg[9] = '0' + fsm.rx_index;
				dbg[10] = '/'; dbg[11] = '0' + CODE_LENGTH;
				dbg[12] = ')'; dbg[13] = '\r'; dbg[14] = '\n';
				HAL_UART_Transmit(&hlpuart1, (uint8_t*)dbg, 15, 100);
			}
		}
		if(fsm.rx_index==CODE_LENGTH){
			fsm.buffer_ready=1;
			fsm.rx_index=0;
		}
		HAL_UART_Receive_IT(&hlpuart1, &fsm.uart_rx_byte, 1);

	}
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == LPUART1)
		fsm.uart_tx_completed = 1;
}
