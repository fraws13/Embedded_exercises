#include "fsm_logic.h"
#include "usart.h" //per la comunicazione seriale
#include "string.h" //per la gestione delle stringhe
#include "ds3231_for_stm32_hal.h"
#define DEFAULT_BUTTON_DELAY		(100) //delay del bottone per evitare il bounce
#define INACTIVE_BLINK_PERIOD		(1000) //periodo del toogle per il led giallo
#define GREEN_DURATION				(10) //durata secondi del verde
#define YELLOW_DURATION				(15) //durata secondi del giallo
#define RED_DURATION				(10) //durata del rosso
/**
 * Enumeration machine's states
 */
typedef enum fsm_state_enum{
	INACTIVE=0,
	GREEN=1,
	YELLOW=2,
	RED=3
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
	button_state_t button; //Stable value of the button at current step
    button_state_t button_last; //Stable value of the button at previous step
	uint8_t timer_elapsed; //Used to avoid the first timer interrupt
	uint8_t alarm_state; //State of the alarm
}FSM_input_t;


/**
 * FSM Main Structure
 * It is composed by the input and outputs as well as
 * the current status of the machine and the current input reads
 */
typedef struct FSM_s
{
	//FSM input devices
	button_t *button;
	timer_t *timer;
	uint8_t alarm_state;

	//FSM input state
	FSM_input_t in;

	//FSM output devices
	led_t *green;
	led_t *yellow;
	led_t *red;
	
	//UART TX handling
	uint8_t uart_tx_completed; //Flag to indicate that the UART TX is completed(1)

	//FSM current status
	fsm_state_t state;


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
 static int8_t FSM_inactive();
 static int8_t FSM_green();
 static int8_t FSM_yellow();
 static int8_t FSM_red();
 static void printMessage(const char* msg);


/*
 * Public init function
 */
int8_t FSM_init(button_t *button, timer_t *timer, led_t *green, led_t *yellow, led_t *red)
{
	if(!button || !timer || !green || !yellow || !red){
		return FSM_ERR;
	}
	//inizializzo la macchina a stati
	fsm.state = INACTIVE;
	fsm.button = button;
	fsm.timer = timer;
	fsm.green = green;
	fsm.yellow = yellow;
	fsm.red = red;
	fsm.uart_tx_completed = 1;
	fsm.alarm_state = 0;

	//setto il delay del bottone
	if (button_set_delay(fsm.button, DEFAULT_BUTTON_DELAY) != BUTTON_OK)
        return FSM_ERR;
	//setto il periodo del toogle del led giallo
    if (led_set_toggle_period(fsm.yellow, INACTIVE_BLINK_PERIOD) != LED_OK)
        return FSM_ERR;
		printMessage("Stato iniziale: Giallo lampeggiante\r\n");
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
		//il canale dell'uart è libero
		fsm.uart_tx_completed=0; //cambio il flag per occupare il canale
		HAL_UART_Transmit_IT(&hlpuart1,(uint8_t*) msg , strlen(msg));

	}
}


static int8_t FSM_read_inputs(){
	int8_t res = FSM_ERR;

	//verifco se il bottone è stato premuto in maniera stabile
	if (button_read(fsm.button, &fsm.in.button) != BUTTON_OK)
	{
		return FSM_ERR;
	}


	//registo lo stato dell'allarm nello stato corrente
	fsm.in.alarm_state = fsm.alarm_state;

	//resetto lo stato dell'allarm della struttura principale
	fsm.alarm_state = 0;

	//controllo se il timer è scaduto, se è scaduto setto il flag a 1 al timer del ciclo corrente
	fsm.in.timer_elapsed = timer_is_elapsed(fsm.timer);
	return FSM_OK;

}

static int8_t FSM_update_state(){
	int8_t res = FSM_ERR;


	switch(fsm.state)
	{
		case INACTIVE:
			return FSM_inactive();

		case GREEN:
			return FSM_green();

		case YELLOW:
			return FSM_yellow();

		case RED:
			return FSM_red();

		default:
			res = FSM_ERR;
			break;
	}
	return res;
}

static int8_t FSM_inactive()
{
	//lampeggio il led giallo
	if (led_toggle(fsm.yellow) != LED_OK){
		return FSM_ERR;
	}
	// nessun allarme
    if (fsm.in.alarm_state == 0){
        return FSM_OK;
    }
	//allarme -> va a verde
	if (fsm.in.alarm_state == 1) {

		if((led_off(fsm.yellow) != LED_OK || led_on(fsm.green) != LED_OK || led_off(fsm.red) != LED_OK)){
			return FSM_ERR;
		}
		

		 // imposta il timer e lo avvia
		 if (timer_set_period(fsm.timer, GREEN_DURATION) != TIMER_OK){
		 return FSM_ERR;
		}
		 //avvio il timer
		if (timer_start(fsm.timer) != TIMER_OK){
			 return FSM_ERR;
		}
		printMessage("Semaforo Verde\r\n");
		fsm.state = GREEN;
		return FSM_OK;

	}
    // vai a verde
    if (led_on(fsm.green) != LED_OK || led_off(fsm.yellow) != LED_OK){
        return FSM_ERR;
    }
    return FSM_OK;
}

static int8_t FSM_green() {

	//allarme-> va inattivo
	if (fsm.in.alarm_state == 1) {

		if (led_off(fsm.green) != LED_OK || led_toggle(fsm.yellow) != LED_OK
		|| led_off(fsm.red) != LED_OK)
			return FSM_ERR;

		fsm.state = INACTIVE;

		printMessage("Stato iniziale: Giallo lampeggiante\r\n");

		return FSM_OK;

	}
	// scade timer -> va a giallo
	if (fsm.in.timer_elapsed) {

		if (led_on(fsm.yellow) != LED_OK || led_off(fsm.green) != LED_OK){
			return FSM_ERR;
		}

		fsm.state = YELLOW;

		printMessage("Semaforo Giallo\r\n");

		//Stop and reset the timer
		if (timer_reset(fsm.timer) != TIMER_OK) {
			return FSM_ERR;
		}

		//Starting the timer period
		if (timer_set_period(fsm.timer, YELLOW_DURATION) != TIMER_OK) {
			return FSM_ERR;
		}

		//Starting the timer
		if (timer_start(fsm.timer) != TIMER_OK) {
			return FSM_ERR;
		}

	}
	return FSM_OK;
}

static int8_t FSM_yellow() {


	//allarme -> va inattivo
		if (fsm.in.alarm_state == 1) {

			if (led_off(fsm.green) != LED_OK ||
					led_toggle(fsm.yellow) != LED_OK ||
					led_off(fsm.red) != LED_OK)

				return FSM_ERR;

			fsm.state = INACTIVE;

			printMessage("Stato iniziale: Giallo lampeggiante\r\n");

			return FSM_OK;

		}

	if (fsm.in.timer_elapsed) {

		if (led_on(fsm.red) != LED_OK || led_off(fsm.yellow) != LED_OK) {

			return FSM_ERR;
		}
			fsm.state = RED;

			printMessage("Semaforo Rosso\r\n");

		//Stop and reset the timer
		if (timer_reset(fsm.timer) != TIMER_OK)
			return FSM_ERR;

		//Starting the timer period
		if (timer_set_period(fsm.timer, RED_DURATION) != TIMER_OK)
			return FSM_ERR;

		//Starting the timer
		if (timer_start(fsm.timer) != TIMER_OK)
			return FSM_ERR;
	}
	return FSM_OK;
}
static int8_t FSM_red() {
	//allarme -> va inattivo
	if (fsm.in.alarm_state == 1) {
		if (led_off(fsm.green) != LED_OK ||
			led_toggle(fsm.yellow) != LED_OK ||
			led_off(fsm.red) != LED_OK)
			return FSM_ERR;

		fsm.state = INACTIVE;
		printMessage("Stato iniziale: Giallo lampeggiante\r\n");
		return FSM_OK;
	}

	if (fsm.in.timer_elapsed) {

		if (led_on(fsm.green) != LED_OK || led_off(fsm.red) != LED_OK) {

			return FSM_ERR;
		}

		fsm.state = GREEN;

		printMessage("Semaforo Verde");


		//Stop and reset the timer
		if (timer_reset(fsm.timer) != TIMER_OK) {
			return FSM_ERR;
		}

		//Starting the timer period
		if (timer_set_period(fsm.timer, GREEN_DURATION) != TIMER_OK) {
			return FSM_ERR;
		}

		//Starting the timer
		if (timer_start(fsm.timer) != TIMER_OK) {
			return FSM_ERR;
		}

	}
	return FSM_OK;
}


//********************************************************************************
//******	CALLBACKS (if needed)
//**************************************************************************
//questa funzione viene chiamata quando il messaggio è stato trasmesso
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	//verifico se è l'uart del semaforo
	if(huart->Instance == LPUART1) {
		//libero il canale dell'uart
		fsm.uart_tx_completed = 1;
	}
}
//questa funzione viene chiamata quando il timer è scaduto
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *handler) {
	//chiamo la funzione per gestire il timer
	//blocco il timer e setto il flag di scaduto a 1
	timer_period_elapsed(fsm.timer, handler);
}

//questa funzione viene chiamata quando il pin dell'allarm è stato attivato
/**
quando il DS3231 genera una allarme abbassa il pin SQW, si riceve l interrupt
e alzo il flag dell allarme
*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if( GPIO_Pin == ALLARM_INT_Pin) {
		//setto il flag di allarm a 1
		fsm.alarm_state = 1;
		//pulisco il flag sqw di allarm, per far si che non venga generato un altro interrupt
		DS3231_ClearAlarm1Flag(); //venerdi 10:45 attiva il semaforo
		DS3231_ClearAlarm2Flag(); //venerdi 10:48 disattiva il semaforo
	}
}