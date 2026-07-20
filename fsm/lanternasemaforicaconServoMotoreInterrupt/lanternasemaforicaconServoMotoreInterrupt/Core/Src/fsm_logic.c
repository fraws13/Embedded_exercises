#include "fsm_logic.h"
#define SERVO_duty_0_DEG 50
#define SERVO_duty_180_DEG 120
#define DEFAULT_BUTTON_DELAY (3000) //3000ms di ritardo per la lettura del pulsante
#define LEDV_PERIOD (10000)
#define LEDG_PERIOD (15000)
#define LEDR_PERIOD (10000)
#define TOGGLE_PERIOD (1000)
/**
 * Enumeration machine's states
 */
typedef enum fsm_state_enum{
	INACTIVE=0,
	VERDE_ON=1,
	GIALLO_ON=2,
	ROSSO_ON=3,

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
	button_state_t button; //stato corrente del pulsante
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
	fsm_state_t state;
	timer_t* timer;
	timer_t* servo_timer;
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
static int8_t FSM_inactive();
static int8_t FSM_verde_on();
static int8_t FSM_giallo_on();
static int8_t FSM_rosso_on();


/*
 * Private function to update the current status and the output
 */


/*
 * Public init function
 */
int8_t FSM_init(led_t* led_rosso, led_t* led_giallo, led_t* led_verde, button_t* button, timer_t* timer, timer_t* servo_timer, uint32_t cycle_duration)
{
	if(!led_rosso || !led_giallo || !led_verde || !button || !timer || !servo_timer){
		return FSM_ERR;
	}
	fsm.led_rosso = led_rosso;
	fsm.led_giallo = led_giallo;
	fsm.led_verde = led_verde;
	fsm.button = button;
	fsm.timer = timer;
	fsm.servo_timer = servo_timer;
	fsm.cycle_duration = cycle_duration;
	fsm.timer_is_elapsed = 0;
	fsm.state = INACTIVE;

	if((button_set_delay(fsm.button, DEFAULT_BUTTON_DELAY) != BUTTON_OK) || (led_set_toggle_period(fsm.led_giallo, TOGGLE_PERIOD)!=LED_OK)){
		return FSM_ERR;
	}
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

	if(fsm.cycle_duration > cycle_runtime)
	{
		HAL_Delay(fsm.cycle_duration - cycle_runtime);
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

static int8_t FSM_read_inputs(){
	int8_t res = FSM_ERR;
	//leggo lo stato del pulsante e lo salvo nel ciclo precedente
	fsm.in.button_last=fsm.in.button;
	//
	if(button_read_pressed(fsm.button,&fsm.in.button)!=BUTTON_OK){
		res=FSM_ERR;
	}
	button_reset_pressed(fsm.button);
	//aggiorno il flag per vedere se il timer è scaduto
	fsm.timer_is_elapsed=timer_is_elapsed(fsm.timer);
	res=FSM_OK;

	return res;
}

static int8_t FSM_update_state(){
	int8_t res = FSM_ERR;


	switch(fsm.state)
	{
		case INACTIVE:
			if(FSM_inactive()==FSM_OK){
				res=FSM_OK;
			}
			break;
		case VERDE_ON:
			if (FSM_verde_on() == FSM_OK) {
				res = FSM_OK;
			}
			break;
		case GIALLO_ON:
			if (FSM_giallo_on() == FSM_OK) {
				res = FSM_OK;
			}
			break;
		case ROSSO_ON:
			if (FSM_rosso_on() == FSM_OK) {
				res = FSM_OK;
			}
			break;
		default:
			res = FSM_ERR;
			break;
	}
	return res;
}

static int8_t FSM_inactive(){
	int8_t res=FSM_ERR;
	//vedo se il bottone è stato premuto e non è premuto in maniera continua
	if(fsm.in.button == PRESSED){
		//inattivo->led_verde
		if((led_off(fsm.led_giallo)==LED_OK)
		&&(led_on(fsm.led_verde)==LED_OK)
		&&(timer_reset(fsm.timer)==TIMER_OK)
		&&(timer_start(fsm.timer)==TIMER_OK)
		&&(timer_set_period_ms(fsm.timer, LEDV_PERIOD)==TIMER_OK)
		&&(timer_set_duty_x10(fsm.servo_timer, TIM_CHANNEL_1, SERVO_duty_180_DEG)==TIMER_OK)){
			fsm.state=VERDE_ON;
			res=FSM_OK;
		}

	}else{
		//rimango nello stato inattivo
		if((led_off(fsm.led_rosso)==LED_OK)
		//non serve led_on(fsm.led_giallo) perché led_toggle lo accende
		&&(led_toggle(fsm.led_giallo)==LED_OK)){
			res=FSM_OK;
		}

	}
	return res;
}

static int8_t FSM_verde_on(){
	int8_t res = FSM_ERR;
	//se il pulsante è premuto per + di 3 secondi
	//inactive
	if(fsm.in.button == PRESSED){
		if(led_off(fsm.led_verde) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		//il timer del verde è scaduto
		if(fsm.timer_is_elapsed == 1){
			if( (led_off(fsm.led_verde) == LED_OK)
			 && (led_on(fsm.led_giallo) == LED_OK)
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, LEDG_PERIOD) == TIMER_OK)
			 && (timer_start(fsm.timer) == TIMER_OK) ){
				//accendo giallo e faccio partire timer
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
	if(fsm.in.button == PRESSED){
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
			 && (timer_set_period_ms(fsm.timer, LEDR_PERIOD) == TIMER_OK)
			 && (timer_start(fsm.timer) == TIMER_OK)
			 &&(timer_set_duty_x10(fsm.servo_timer, TIM_CHANNEL_1, SERVO_duty_0_DEG)==TIMER_OK)){
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
	if(fsm.in.button == PRESSED){
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
			 && (timer_set_period_ms(fsm.timer, LEDV_PERIOD) == TIMER_OK)
			 && (timer_set_duty_x10(fsm.servo_timer, TIM_CHANNEL_1, SERVO_duty_180_DEG)==TIMER_OK)
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *handler){
	timer_period_elapsed(fsm.timer, handler);

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == BUTTON_Pin){
		button_pressed_handler(fsm.button, BUTTON_Pin);
	}
}
