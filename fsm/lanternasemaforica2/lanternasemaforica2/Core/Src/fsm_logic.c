#include "fsm_logic.h"

#define DEFAULT_BUTTON_DELAY (3000) //3000ms di ritardo per la lettura del pulsante
/**
 * Enumeration machine's states
 */
typedef enum fsm_state_enum{
	INACTIVE = 0, //il semaforo è spento
	VERDE_ON = 1, //il verde è acceso
	GIALLO_ON = 2, //il giallo è acceso
	ROSSO_ON = 3, //il rosso è acceso

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
	button_state_t button; //input del pulsante
	button_state_t button_last; //stato del pulsante al ciclo precedente


}FSM_input_t;


/**
 * FSM Main Structure
 * It is composed by the input and outputs as well as
 * the current status of the machine and the current input reads
 */
typedef struct FSM_s
{
	button_t* button; //input del pulsante
	FSM_input_t in; //stato degli input al ciclo corrente

	// FSM Outputs devices
	led_t* led_rosso;
	led_t* led_giallo;
	led_t* led_verde;

	// FSM current status
	fsm_state_t state;

	// FSM timer device
	timer_t* timer;
	uint32_t cycle_duration;
	uint8_t timer_is_elapsed; //flag per indicare se il timer è scaduto

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
 static int8_t FSM_verde_on();
 static int8_t FSM_giallo_on();
 static int8_t FSM_rosso_on();


/*
 * Public init function
 */
int8_t FSM_init(led_t* led_rosso, led_t* led_giallo, led_t* led_verde, button_t* button, timer_t* timer, uint32_t cycle_duration)
{
	int8_t res = FSM_ERR;
	if(led_rosso && led_giallo && led_verde && button && timer){
		fsm.state = INACTIVE;
		fsm.led_rosso = led_rosso;
		fsm.led_giallo = led_giallo;
		fsm.led_verde = led_verde;
		fsm.button = button;
		fsm.timer = timer;
		fsm.cycle_duration = cycle_duration;
		res = FSM_OK;
	}
	if(button_set_delay(fsm.button, DEFAULT_BUTTON_DELAY) != BUTTON_OK || (led_set_toggle_period(led_giallo, 1000) != LED_OK)){
		res = FSM_ERR;
	}
	else{
		res = FSM_OK;
	}
	return res;
	
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

//GESTIONE DEL PULSANTE IN POLLING, AD OGNI LETTURA SI AGGIORNA IL VALORE PRECEDENTE DEL PULSANTE
static int8_t FSM_read_inputs(){
	int8_t res = FSM_ERR;
	fsm.in.button_last = fsm.in.button;
	if(button_read(fsm.button, &fsm.in.button) == BUTTON_OK){
		res = FSM_OK;
	}
	fsm.timer_is_elapsed = timer_is_elapsed(fsm.timer);

	return res;
}

static int8_t FSM_update_state(){
	int8_t res = FSM_ERR;


	switch(fsm.state)
	{
		case INACTIVE:
			if(FSM_inactive() == FSM_OK){
				res = FSM_OK;
			}
			break;
		case VERDE_ON:
			if(FSM_verde_on() == FSM_OK){
				res = FSM_OK;
			}
			break;
		case GIALLO_ON:
			if(FSM_giallo_on() == FSM_OK){
				res = FSM_OK;
			}
			break;
		case ROSSO_ON:
			if(FSM_rosso_on() == FSM_OK){
				res = FSM_OK;
			}
			break;
		default:
			res = FSM_ERR;
			break;
	}
	return res;
}
//stato dove il sistema è inattivo o passa da inattivo a verde acceso
static int8_t FSM_inactive(){
	int8_t res = FSM_ERR;
	//se il pulsante è premuto per + di 3 secondi
	if( (fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		//spengo il giallo e accendo il verde e avvio il timer per 10 secondi
		if((led_off(fsm.led_giallo) == LED_OK) 
		&& (led_on(fsm.led_verde) == LED_OK)
	    &&(timer_reset(fsm.timer) == TIMER_OK)
	    &&(timer_start(fsm.timer) == TIMER_OK)
		&&(timer_set_period_ms(fsm.timer, 10000) == TIMER_OK)){
			//passo allo stato VERDE_ON
			fsm.state = VERDE_ON;
			res = FSM_OK;
		}
	//altrimenti rimaniamo in stato inattivo
	}else{
		//spengo il rosso e il verde e accendo il giallo
		if((led_off(fsm.led_rosso) == LED_OK) 
	    && (led_off(fsm.led_verde) == LED_OK) 
	   //avvio il toggle del giallo
        && (led_toggle(fsm.led_giallo) == LED_OK)){
			res = FSM_OK;
		}
	}
	return res;
}
static int8_t FSM_verde_on(){

	int8_t res = FSM_ERR;
	//se il pulsante è premuto per + di 3 secondi
	if((fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button)){
		if(led_off(fsm.led_verde) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		if(fsm.timer_is_elapsed == 1){
			if( (led_off(fsm.led_verde) == LED_OK)
			 && (led_on(fsm.led_giallo) == LED_OK)
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, 15000) == TIMER_OK)
			 && (timer_start(fsm.timer) == TIMER_OK) ){
				fsm.state = GIALLO_ON;
				res = FSM_OK;
			}
		}
		else{
			res = FSM_OK;
		}
	}
	
	
	return res;
}

static int8_t FSM_giallo_on(){
	int8_t res = FSM_ERR;
	if((fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button)){
		if(led_off(fsm.led_giallo) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		if(fsm.timer_is_elapsed == 1){
			if( (led_off(fsm.led_giallo) == LED_OK)
			 && (led_on(fsm.led_rosso) == LED_OK)
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, 10000) == TIMER_OK)
			 && (timer_start(fsm.timer) == TIMER_OK) ){
				fsm.state = ROSSO_ON;
				res = FSM_OK;
			}
		}else{
			res = FSM_OK;
		}
	}

	
	return res;
}

static int8_t FSM_rosso_on(){
	int8_t res = FSM_ERR;
	if((fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button)){
		if(led_off(fsm.led_rosso) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		if(fsm.timer_is_elapsed == 1){
			if( (led_off(fsm.led_rosso) == LED_OK)
			 && (led_off(fsm.led_giallo) == LED_OK)
			 && (led_on(fsm.led_verde) == LED_OK) 
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, 5000) == TIMER_OK)
			 && (timer_start(fsm.timer) == TIMER_OK) ){
				fsm.state = VERDE_ON;
				res = FSM_OK;
			}
		}else{
			res = FSM_OK;
		}
	}
	return res;
}

//********************************************************************************
//******	CALLBACKS (if needed)
//**************************************************************************
//quando il timer scade, ovvero quando il counter del timer arriva al valore del array del timer, viene chiamata questa funzione
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *handeler)
{
	//verifico se il timer passato come parametro è quello del fsm
	if(handler == fsm.timer){
		//chiamo la funzione timer_period_elapsed per aggiornare il flag del timer
		timer_period_elapsed(fsm.timer, handler);
	}
}