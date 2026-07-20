#include "fsm_logic.h"
#include "string.h"
#include "usart.h"
#define SERVO_duty_0_DEG 50
#define SERVO_duty_180_DEG 120
#define DEFAULT_BUTTON_DELAY (3000) //3000ms di ritardo per la lettura del pulsante
#define DEFAULT_BUTTON_CONFIG_DELAY (2000) //2000ms di ritardo per la lettura del pulsante di configurazione
#define LEDV_PERIOD (10000)
#define LEDG_PERIOD (15000)
#define LEDR_PERIOD (10000)
#define TOGGLE_PERIOD (1000)
#define CODE_LENGTH (3) /* due cifre obbligatorie (es. 01, 15) + il terminatore '\0' */
/**
 * Enumeration machine's states
 */
typedef enum fsm_state_enum{
	INACTIVE=0,
	VERDE_ON=1,
	GIALLO_ON=2,
	ROSSO_ON=3,
	CONFIG=4,

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

	button_state_t button_config; //stato corrente del pulsante di configurazione
	button_state_t button_config_last; //stato del pulsante di configurazione al ciclo precedente

	uint8_t timer_is_elapsed; //flag per indicare se il timer è scaduto

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

	button_t* button_config; //input del pulsante di configurazione

	// FSM Outputs devices
	led_t* led_rosso;
	led_t* led_giallo;
	led_t* led_verde;
	fsm_state_t state;
	timer_t* timer;
	timer_t* servo_timer;
	
	uint8_t rx_buffer[CODE_LENGTH]; //buffer per la ricezione del codice di configurazione
	uint8_t rx_index; //indice del buffer per la ricezione del codice di configurazione
	uint8_t rx_byte; //byte ricevuto

	uint8_t uart_tx_completed; //flag per indicare se il messaggio è stato inviato
	volatile uint8_t buffer_ready;  // flag grezzo (come alarm_state)
	uint8_t led_config;
	uint8_t period_correct;
	uint8_t prompt_sent;

	
	uint32_t cycle_duration;



	uint16_t period_verde_ms;
	uint16_t period_giallo_ms;
	uint16_t period_rosso_ms;

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
static int8_t FSM_config();
static int8_t FSM_verde_config();
static int8_t FSM_giallo_config();
static int8_t FSM_rosso_config();


/*
 * Private function to update the current status and the output
 */


/*
 * Public init function
 */
int8_t FSM_init(led_t* led_rosso, led_t* led_giallo, led_t* led_verde, button_t* button, button_t* button_config, timer_t* timer, timer_t* servo_timer, uint32_t cycle_duration)
{
	if(!led_rosso || !led_giallo || !led_verde || !button || !button_config || !timer || !servo_timer){
		return FSM_ERR;
	}
	fsm.led_rosso = led_rosso;
	fsm.led_giallo = led_giallo;
	fsm.led_verde = led_verde;
	fsm.button = button;
	fsm.button_config = button_config;
	fsm.timer = timer;
	fsm.servo_timer = servo_timer;
	fsm.cycle_duration = cycle_duration;
	fsm.state = INACTIVE;

	fsm.period_verde_ms = LEDV_PERIOD;
	fsm.period_giallo_ms = LEDG_PERIOD;
	fsm.period_rosso_ms = LEDR_PERIOD;

	fsm.in.button_config = RELEASED;
	fsm.in.button_config_last = RELEASED;
	fsm.in.button = RELEASED;
	fsm.in.button_last = RELEASED;
	fsm.in.timer_is_elapsed = 0;
	fsm.uart_tx_completed = 1; //posso avviare la trasmissione
	fsm.buffer_ready = 0; //non ho ricevuto 2 cifre
	fsm.rx_index = 0;
	fsm.led_config = 0; //indica quale led sto configurando: 0: verde, 1: giallo, 2: rosso, 3: completato
	fsm.prompt_sent = 0;

	if((button_set_delay(fsm.button, DEFAULT_BUTTON_DELAY) != BUTTON_OK) || (led_set_toggle_period(fsm.led_giallo, TOGGLE_PERIOD)!=LED_OK)){
		return FSM_ERR;
	}
	if((button_set_delay(fsm.button_config, DEFAULT_BUTTON_CONFIG_DELAY) != BUTTON_OK)){
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
//funzione per inviare un messaggio uart
static void print_message(const char* message){
	//verifico se posso avviare la trasmissione
	if(fsm.uart_tx_completed){
		//blocco la trasmissione
		fsm.uart_tx_completed = 0;
		//avvio la trasmissione del messaggio
		HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)message, strlen(message));
	}
}

//funzione per leggere gli input che servono alla macchina ad ogni ciclo
static int8_t FSM_read_inputs(){
	int8_t res = FSM_ERR;

	//sto nel nuovo ciclo, quindi aggiorno il valore del pulsante al ciclo precedente
	fsm.in.button_last=fsm.in.button;
	//leggo lo stato del pulsante e lo salvo nello stato corrente
	if(button_read(fsm.button,&fsm.in.button)!=BUTTON_OK){
		res=FSM_ERR;
	}
	//aggiorno il flag per vedere se il timer è scaduto
	fsm.in.timer_is_elapsed=timer_is_elapsed(fsm.timer);

	//sto nel nuovo ciclo, quindi aggiorno il valore del pulsante al ciclo precedente
	fsm.in.button_config_last=fsm.in.button_config;
	//leggo lo stato del pulsante di configurazione e lo salvo nello stato corrente
	if(button_read(fsm.button_config,&fsm.in.button_config)!=BUTTON_OK){
		res=FSM_ERR;
	}
	//se tutto è andato a buon fine, ritorno FSM_OK
	res = FSM_OK;
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
		case CONFIG:
			if (FSM_config() == FSM_OK) {
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
	//dopo aver aggiorano gli stati in read_input, vedo se il pulsante è stato premuto
	//e se precedentemente era stato rilasciato, lo stato passa a pressed solo dopo che è passato il tempo di lettura minimo
	if(fsm.in.button == PRESSED && fsm.in.button_last == RELEASED){
		//inattivo->led_verde
		if((led_off(fsm.led_giallo)==LED_OK)
		&&(led_on(fsm.led_verde)==LED_OK)
		&&(timer_reset(fsm.timer)==TIMER_OK)
		&&(timer_start(fsm.timer)==TIMER_OK)
		&&(timer_set_period_ms(fsm.timer, fsm.period_verde_ms)==TIMER_OK)
		&&(timer_set_duty_x10(fsm.servo_timer, TIM_CHANNEL_1, SERVO_duty_180_DEG)==TIMER_OK)){
			fsm.state=VERDE_ON;
			res=FSM_OK;
		}

	//verifico se il pulsante di configurazione è stato premuto
	}else if(fsm.in.button_config == PRESSED && fsm.in.button_config_last == RELEASED){
		fsm.state = CONFIG;
		//ripristino i valori di default per la configurazione
		fsm.led_config = 0;
		//ripristino l'indice del buffer per la ricezione del codice di configurazione
		fsm.rx_index = 0;
		//ripristino il flag per indicare se ho ricevuto 2 cifre
		fsm.buffer_ready = 0;
		//ripristino il flag per indicare se il valore è corretto
		fsm.period_correct = 0;
		//ripristino il flag per indicare se il messaggio di prompt è stato inviato
		fsm.prompt_sent = 0;
		if(!fsm.prompt_sent){
			print_message("Configurazione iniziata\r\n");
			fsm.prompt_sent = 0;
		}
		res = FSM_OK;
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

static uint16_t FSM_parse_seconds(void)
{
	return atoi((char*)fsm.rx_buffer);
}

static int8_t FSM_config(){
	//verifico se il pulsante di configurazione è stato premuto
	//e se precedentemente era stato rilasciato, lo stato passa a pressed solo dopo che è passato il tempo di lettura minimo
	if (fsm.in.button_config == PRESSED && fsm.in.button_config_last == RELEASED) {
		//verifico che ho salvato tutti i valori di configurazione
		if(fsm.led_config == 3){
			//ripristino lo stato inattivo
			fsm.state = INACTIVE;
			//ripristino il flag per indicare se il messaggio di prompt è stato inviato
			fsm.prompt_sent = 0;
			if(!fsm.prompt_sent){
				//invio il messaggio di configurazione terminata
				print_message("Configurazione terminata\r\n");
				fsm.prompt_sent = 1;
			}
			return FSM_OK;
		}
		
	}
	//verifico quale led sto configurando e chiamo la funzione di configurazione corrispondente
	switch(fsm.led_config){
		case 0:
			return FSM_verde_config();
		case 1:
			return FSM_giallo_config();
		case 2:
			return FSM_rosso_config();
		case 3:
			/* periodi impostati: resto in CONFIG fino a CONF */
			if(!fsm.prompt_sent){
				print_message("Configurazione completata. Premi CONF per uscire.\r\n");
				fsm.prompt_sent = 1;
			}
			return FSM_OK;
		default:
			return FSM_ERR;
	}
}


static int8_t FSM_verde_config(){
	if(!fsm.prompt_sent){
		print_message("-> Digita tempo accensione VERDE (2 cifre): ");
		HAL_UART_Receive_IT(&hlpuart1, &fsm.rx_byte, 1);
		fsm.prompt_sent = 1;
	}
	if(fsm.buffer_ready){
		if(fsm.period_correct){
			fsm.buffer_ready = 0;
			fsm.period_verde_ms = FSM_parse_seconds() * 1000;
			fsm.period_correct = 0;
			fsm.led_config = 1;
			fsm.prompt_sent = 0;
			fsm.rx_index = 0;
		}else{
			fsm.prompt_sent = 0;
			fsm.rx_index = 0;
			print_message("\r\nValore non valido\r\n");
		}
	}
	return FSM_OK;
}

static int8_t FSM_giallo_config(){
	if(!fsm.prompt_sent){
		print_message("-> Digita tempo accensione GIALLO (2 cifre): ");
		HAL_UART_Receive_IT(&hlpuart1, &fsm.rx_byte, 1);
		fsm.prompt_sent = 1;
	}

	if(fsm.buffer_ready){
		fsm.buffer_ready = 0;
		if(fsm.period_correct){
			fsm.period_giallo_ms = FSM_parse_seconds() * 1000;
			fsm.period_correct = 0;
			fsm.led_config = 2;
			fsm.prompt_sent = 0;
			fsm.rx_index = 0;
		}else{
			fsm.prompt_sent = 0;
			fsm.rx_index = 0;
			print_message("\r\nValore non valido\r\n");
		}
	}

	return FSM_OK;
}

static int8_t FSM_rosso_config(){
	if(!fsm.prompt_sent){
		print_message("-> Digita tempo accensione ROSSO (2 cifre): ");
		HAL_UART_Receive_IT(&hlpuart1, &fsm.rx_byte, 1);
		fsm.prompt_sent = 1;
	}

	if(fsm.buffer_ready){
		fsm.buffer_ready = 0;
		if(fsm.period_correct){
			fsm.period_rosso_ms = FSM_parse_seconds() * 1000;
			fsm.period_correct = 0;
			fsm.led_config = 3;
			fsm.prompt_sent = 0;
			fsm.rx_index = 0;
		}else{
			fsm.prompt_sent = 0;
			fsm.rx_index = 0;
			print_message("\r\nValore non valido\r\n");
		}
	}

	return FSM_OK;
}

static int8_t FSM_verde_on(){
	int8_t res = FSM_ERR;
	//se il pulsante è premuto per + di 3 secondi
	//inactive
	if(fsm.in.button == PRESSED && fsm.in.button_last == RELEASED){
		if(led_off(fsm.led_verde) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		//il timer del verde è scaduto
		if(fsm.in.timer_is_elapsed == 1){
			if( (led_off(fsm.led_verde) == LED_OK)
			 && (led_on(fsm.led_giallo) == LED_OK)
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, fsm.period_giallo_ms) == TIMER_OK)
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
	if(fsm.in.button == PRESSED && fsm.in.button_last == RELEASED){
		if(led_off(fsm.led_giallo) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		if(fsm.in.timer_is_elapsed == 1){
			if( (led_off(fsm.led_giallo) == LED_OK)
			 && (led_on(fsm.led_rosso) == LED_OK)
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, fsm.period_rosso_ms) == TIMER_OK)
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
	if(fsm.in.button == PRESSED && fsm.in.button_last == RELEASED){
		if(led_off(fsm.led_rosso) == LED_OK){
			timer_stop(fsm.timer);
			fsm.state = INACTIVE;
			res = FSM_OK;
		}
	}else{
		if(fsm.in.timer_is_elapsed == 1){
			if( (led_off(fsm.led_rosso) == LED_OK)
			 && (led_off(fsm.led_giallo) == LED_OK)
			 && (led_on(fsm.led_verde) == LED_OK)
			 && (timer_reset(fsm.timer) == TIMER_OK)
			 && (timer_set_period_ms(fsm.timer, fsm.period_verde_ms) == TIMER_OK)
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
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == &hlpuart1){
		fsm.uart_tx_completed = 1;
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart != &hlpuart1){
		return;
	}

	/* Invio: ignora (bastano 2 cifre senza \r/\n) */
	if(fsm.rx_byte == '\r' || fsm.rx_byte == '\n'){
		//se ho ricevuto un carattere di fine stringa, avvio la ricezione del prossimo carattere
		HAL_UART_Receive_IT(&hlpuart1, &fsm.rx_byte, 1);
		return;
	}
	//verifico se il carattere è un numero
	if(fsm.rx_byte >= '0' && fsm.rx_byte <= '9'){
		//se ho ricevuto un carattere numerico, lo salvo nel buffer
		if(fsm.rx_index < CODE_LENGTH){
			//salvo il carattere nel buffer
			fsm.rx_buffer[fsm.rx_index++] = fsm.rx_byte;
		}
		if(fsm.rx_index == CODE_LENGTH - 1){
			//se ho ricevuto 2 cifre, imposto il flag per indicare se il valore è corretto
			//imposto il terminatore della stringa
			fsm.rx_buffer[fsm.rx_index++] = '\0';
			fsm.period_correct = 1;
			fsm.buffer_ready = 1;
			//ripristino l'indice del buffer per la ricezione del prossimo carattere
			fsm.rx_index = 0;
			
		}
	}else{
		/* carattere non numerico → riproponi inserimento */
		fsm.period_correct = 0;
		fsm.buffer_ready = 1;
		fsm.rx_index = 0;
	}

	HAL_UART_Receive_IT(&hlpuart1, &fsm.rx_byte, 1);
}
