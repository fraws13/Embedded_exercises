#include <stdint.h>
#include "gpio.h"
#include "button.h"

int8_t button_init(button_t* button, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, button_state_t init_state)
{

	int8_t res = BUTTON_ERR;

	if(button)
	{
		button->GPIO_Pin = GPIO_Pin;		// Pin number
		button->GPIOx = GPIOx;				// Port number
		button->delay = BUTTON_INIT_DELAY;	// Minimum time interval with no state variations
		button->last_tick = 0;				//
		button->state = init_state;
		button->last_state = init_state;
		res = BUTTON_OK;
	}

	return res;
}

/**
 * Imposta il ritardo di lettura del pulsante
 * @param button: puntatore alla struct button
 * @param read_delay: ritardo di lettura
 * @return: BUTTON_OK se impostazioneRiuscita, BUTTON_ERR altrimenti
 */
int8_t button_set_delay(button_t* button, uint32_t read_delay)
{
	int8_t res = BUTTON_ERR; //inizializza la variabile res a BUTTON_ERR, ovvero ad errore
	//se il puntatore alla struct button e valido, allora imposta il ritardo di lettura del pulsante
	if(button)
	{
		//assegna il ritardo di lettura del pulsante alla struct button
		button->delay = read_delay;
		//imposta res a BUTTON_OK, ovvero aRiuscito
		res = BUTTON_OK;
	}

	return res;
}

int8_t button_get_delay(button_t* button, uint32_t* read_delay)
{
	int8_t res = BUTTON_ERR;

	if(button && read_delay)
	{
		*read_delay = button->delay;
		res = BUTTON_OK;
	}

	return res;
}

int8_t button_read(button_t* button, button_state_t* state)
{

	int8_t res = BUTTON_ERR;
	//
	button_state_t pin_state = 0; //andrà a salvarsi il valore grezzo del pulsante
	uint32_t elapsed_tick = -1;
	//4294967295 è il valore massimo di un uint32_t, ovvero 2^32-1
	uint32_t current_tick = -1;

	if(button && state)
	{
		//calcola il tempo trascorso dall'ultima lettura del pulsante
		current_tick = HAL_GetTick(); //prendo il tempo attuale
		elapsed_tick = current_tick - button->last_tick;
		//differenza tra il tempo attuale e il tempo dell'ultima lettura del pulsante
		//ovvero da quando il segnale è stabile

		pin_state = HAL_GPIO_ReadPin(button->GPIOx, button->GPIO_Pin);
		//legge il valore del pulsante, alto o basso

		/*

		*/
		if(pin_state != button->last_state || pin_state == button->state)
		{
			button->last_tick = current_tick;
		}
		//se il tempo trascorso dall'ultima lettura del pulsante è maggiore del ritardo del pulsante,
		if(elapsed_tick > button->delay)
		{
			//se il valore grezzo del pulsante è diverso dal valore attuale, allora aggiorna il valore attuale
			if(pin_state != button->state)
			{
				//assegna il valore grezzo del pulsante al valore attuale
				button->state = pin_state;
			}
		}
		//assegna il valore grezzo del pulsante al valore attuale
		button->last_state = pin_state;
		//assegna il valore attuale del pulsante al valore di ritorno
		*state = button->state;
		//imposta res a BUTTON_OK
		res = BUTTON_OK;
	}

	return res;
}

inline int8_t button_get_port(const button_t* button, GPIO_TypeDef** port)
{
	int8_t res = BUTTON_ERR;

	if(button)
	{
		*port = button->GPIOx;
		res = BUTTON_OK;
	}

	return res;
}

inline int8_t button_get_pin_number(const button_t* button, uint16_t* pin)
{
	int8_t res = BUTTON_ERR;

	if(button)
	{
		*pin = button->GPIO_Pin;
		res = BUTTON_OK;
	}

	return res;
}
