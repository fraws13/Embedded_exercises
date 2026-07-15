#include "fsm_logic.h"

/**
 * Enumeration machine's states
 */
#define DEFAULT_BUTTON_DELAY		(200) //200ms di ritardo per la lettura del pulsante

typedef enum fsm_state_enum{
	//stati della macchina a stati finiti
	//TO DO:
	LED_OFF	    = 0, //tutti i led sono spenti, STATO 0
	ROSSO_ON	= 1, //il rosso è acceso, STATO 1
	GIALLO_ON	= 2, //il giallo è acceso, STATO 2
	VERDE_ON	= 3, //il verde è acceso, STATO 3

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
	button_state_t button;	 		//Stable value of the button at current step
	button_state_t button_last;		//Stable value of the button at previous step

}FSM_input_t;


/**
 * FSM Main Structure
 * It is composed by the input and outputs as well as
 * the current status of the machine and the current input reads
 */
typedef struct FSM_s
{
	// FSM Inputs devices
	button_t* button;

	// FSM Inputs state
	FSM_input_t in;

	// FSM Outputs devices
	led_t* led_rosso;
	led_t* led_giallo;
	led_t* led_verde;

	// FSM current status
	fsm_state_t state;

	uint32_t cycle_duration;

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
 static int8_t FSM_led_off();
 static int8_t FSM_led_rosso();
 static int8_t FSM_led_giallo();
 static int8_t FSM_led_verde();


/*
 * Public init function
 */
int8_t FSM_init(led_t* led_rosso, led_t* led_giallo, led_t* led_verde, button_t* button, uint32_t cycle_duration){
	int8_t res = FSM_ERR;
	//se i puntatori alle struct led e button sono validi, allora inizializza il FSM

	if(led_rosso && led_giallo && led_verde && button){
		//imposto lo stato iniziale della macchina a stati finiti
		fsm.state = LED_OFF;
		//assegno il puntatore alla struct led_rosso al FSM
		fsm.led_rosso = led_rosso;
		//assegno il puntatore alla struct led_giallo al FSM
		fsm.led_giallo = led_giallo;
		//assegno il puntatore alla struct led_verde al FSM
		fsm.led_verde = led_verde;
		//assegno il puntatore alla struct button al FSM
		fsm.button = button;
		//assegno il ciclo di durata al FSM
		fsm.cycle_duration = cycle_duration;
		//setto il ritardo di lettura del pulsante a 200ms alla struct button
		if(button_set_delay(fsm.button, DEFAULT_BUTTON_DELAY) != BUTTON_OK){
			res = FSM_ERR;
		}
		else{
			res = FSM_OK;
		}
	}
	return res;
	//restituisco il risultato della funzione init

}


/*
 * Public step function
 * Esegue un passo del FSM
 * @return: FSM_OK se passoRiuscito, FSM_ERR altrimenti
 */
int8_t FSM_step(){
	int8_t res = FSM_ERR;
	uint32_t cycle_start = 0; //tempo di inizio del ciclo
	uint32_t cycle_runtime = 0; //tempo di esecuzione del ciclo

	cycle_start = HAL_GetTick();

	if(FSM_read_inputs() == FSM_OK){
		res = FSM_OK;
	}
	//aggiorna lo stato della macchina a stati finiti se il passo è riuscito e l'aggiornamento dello stato è riuscito
	if( (res == FSM_OK) && (FSM_update_state() != FSM_OK) ){
		res = FSM_ERR;
	}
	//calcolo il tempo di esecuzione del ciclo
	cycle_runtime = HAL_GetTick() - cycle_start;

	//se il tempo di esecuzione del ciclo è minore del ciclo di durata, allora aspetto il tempo rimanente
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
	fsm.in.button_last = fsm.in.button;
	//leggo il valore del pulsante e lo salvo nella struct fsm.in.button
	if(button_read(fsm.button, &fsm.in.button) == BUTTON_OK){
		res = FSM_OK;
	}


	return res;
}

static int8_t FSM_update_state(){
	int8_t res = FSM_ERR;


	switch(fsm.state)
	{
		//tutti i led sono spenti, accendo il rosso
		case LED_OFF:
			if(FSM_led_off() == FSM_OK){
				res = FSM_OK;
			}
			break;

		//il rosso è acceso, lo spegno e accendo il giallo
		case ROSSO_ON:
			if(FSM_led_rosso() == FSM_OK){
				res = FSM_OK;
			}
			break;
		//il giallo è acceso, lo spegno e accendo il verde
		case GIALLO_ON:
			if(FSM_led_giallo() == FSM_OK){
				res = FSM_OK;
			}
			break;
		//il verde è acceso, lo spegno, tutti i led sono spenti
		case VERDE_ON:
			if(FSM_led_verde() == FSM_OK){
				res = FSM_OK;
			}
			break;
		default:
			res = FSM_ERR;
			break;
	}
	return res;
}

/**
 * Esegue lo stato di accensione del led rosso
 * @return: FSM_OK se statoRiuscito, FSM_ERR altrimenti
 */
 static int8_t FSM_led_off(){
	//inizializza la variabile res a FSM_ERR, ovvero ad errore
	int8_t res = FSM_ERR;
	//se il valore attuale del pulsante è alto e il valore attuale del pulsante è diverso dal valore attuale, allora accende la lampada
	if( (fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		//accende il led rosso
		if((led_on(fsm.led_rosso) == LED_OK) && (led_off(fsm.led_giallo) == LED_OK) && (led_off(fsm.led_verde) == LED_OK)){
			fsm.state = ROSSO_ON;
			res = FSM_OK;
		}else{
			res = FSM_ERR;
		}
	}else
		res = FSM_OK;

	return res;
}

static int8_t FSM_led_rosso(){
	//inizializza la variabile res a FSM_ERR, ovvero ad errore
	int8_t res = FSM_ERR;
	//se il valore attuale del pulsante è alto e il valore attuale del pulsante è diverso dal valore attuale, allora accende il led rosso
	if( (fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		//accende il led rosso
		if((led_on(fsm.led_giallo) == LED_OK) && (led_off(fsm.led_rosso) == LED_OK) && (led_off(fsm.led_verde) == LED_OK)){
			fsm.state = GIALLO_ON;
			res = FSM_OK;
		}else{
			res = FSM_ERR;
		}
	}else
		res = FSM_OK;

	return res;
}

static int8_t FSM_led_giallo(){
	//inizializza la variabile res a FSM_ERR, ovvero ad errore
	int8_t res = FSM_ERR;
	//se il valore attuale del pulsante è alto e il valore attuale del pulsante è diverso dal valore attuale, allora accende il led giallo
	if( (fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		//accende il led giallo
		if((led_on(fsm.led_verde) == LED_OK) && (led_off(fsm.led_rosso) == LED_OK) && (led_off(fsm.led_giallo) == LED_OK)){
			fsm.state = VERDE_ON;
			res = FSM_OK;
		}else{
			res = FSM_ERR;
		}
	}else
		res = FSM_OK;

	return res;
}

static int8_t FSM_led_verde(){
	//inizializza la variabile res a FSM_ERR, ovvero ad errore
	int8_t res = FSM_ERR;
	//se il valore attuale del pulsante è alto e il valore attuale del pulsante è diverso dal valore attuale, allora accende il led verde
	if( (fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		//accende il led verde
		if((led_off(fsm.led_rosso) == LED_OK) && (led_off(fsm.led_giallo) == LED_OK) && (led_off(fsm.led_verde) == LED_OK)){
			fsm.state = LED_OFF;
			res = FSM_OK;
		}else{
			res = FSM_ERR;
		}
	}else
		res = FSM_OK;

	return res;
}
//********************************************************************************
//******	CALLBACKS (if needed)
//**************************************************************************
