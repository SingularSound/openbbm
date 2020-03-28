/*
 * button.h
 *
 *  Created on: Jun 18, 2013
 *      Author: omasse
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
**
******************************************************************************/
typedef enum BUTTON_STATE
{
	BUTTON_STATE_RELEASE = 0,
	BUTTON_STATE_PRESS,
	BUTTON_STATE_LONG_PRESS,
	BUTTON_STATE_MULTI_TAP,
	BUTTON_STATE_UNCHANGED
} BUTTON_STATE;

typedef enum BUTTON_EVENT
{
	BUTTON_EVENT_UP_RELEASE,
	BUTTON_EVENT_UP_PRESS,
	BUTTON_EVENT_UP_LONG_PRESS,

	BUTTON_EVENT_DOWN_RELEASE,
	BUTTON_EVENT_DOWN_PRESS,
	BUTTON_EVENT_DOWN_LONG_PRESS,

	BUTTON_EVENT_LEFT_RELEASE,
	BUTTON_EVENT_LEFT_PRESS,
	BUTTON_EVENT_LEFT_LONG_PRESS,

	BUTTON_EVENT_RIGHT_RELEASE,
	BUTTON_EVENT_RIGHT_PRESS,
	BUTTON_EVENT_RIGHT_LONG_PRESS,

	BUTTON_EVENT_TAP_RELEASE,
	BUTTON_EVENT_TAP_PRESS,
	BUTTON_EVENT_TAP_LONG_PRESS,

	BUTTON_EVENT_FOOT_SECONDARY_PRESS,
	BUTTON_EVENT_FOOT_SECONDARY_RELEASE,

	BUTTON_EVENT_FOOT_PRIMARY_PRESS,
	BUTTON_EVENT_FOOT_PRIMARY_RELEASE,

	BUTTON_EVENT_HP_VOL_UP,
	BUTTON_EVENT_HP_VOL_DOWN,
	
	BUTTON_EVENT_VOL_UP,
	BUTTON_EVENT_VOL_DOWN,

	BUTTON_EVENT_PEDAL_PRESS,
	BUTTON_EVENT_PEDAL_RELEASE,
	BUTTON_EVENT_PEDAL_LONG_PRESS,
	BUTTON_EVENT_PEDAL_MULTI_TAP,

	BUTTON_EVENT_ENC_TEMPO_PRESS,
	BUTTON_EVENT_ENC_TEMPO_RELEASE,
    BUTTON_EVENT_ENC_TEMPO_UP,
    BUTTON_EVENT_ENC_TEMPO_FAST_UP,
	BUTTON_EVENT_ENC_DRUM_RELEASE,
    BUTTON_EVENT_ENC_TEMPO_DOWN,
    BUTTON_EVENT_ENC_TEMPO_FAST_DOWN,

	BUTTON_EVENT_ENC_DRUM_PRESS,
    BUTTON_EVENT_ENC_DRUM_UP,
    BUTTON_EVENT_ENC_DRUM_FAST_UP,
    BUTTON_EVENT_ENC_DRUM_DOWN,
    BUTTON_EVENT_ENC_DRUM_FAST_DOWN,

    BUTTON_EVENT_AUDIO_LEFT_CON_IN,
    BUTTON_EVENT_AUDIO_LEFT_CON_OUT,
    BUTTON_EVENT_AUDIO_RIGHT_CON_IN,
    BUTTON_EVENT_AUDIO_RIGHT_CON_OUT,

	BUTTON_EVENT_INACTIVITY,

	BUTTON_EVENT_COUNT
} BUTTON_EVENT;


typedef enum {
     ENCODER_STATE_UP,
     ENCODER_STATE_DOWN,
     ENCODER_STATE_FAST_UP,
     ENCODER_STATE_SUPER_FAST_UP,
     ENCODER_STATE_FAST_DOWN,
     ENCODER_STATE_SUPER_FAST_DOWN
}ENCODER_STATE;



typedef void (*button_eventFuncPtr)(BUTTON_EVENT event, unsigned long long time);

/******************************************************************************
**                     API FUNCTION PROTOTYPES
******************************************************************************/
void button_init(void);
void button_tick(void);
void button_task(void);
void button_clearEventQueue(void);
void button_enableEventCallbacks(void);
void button_enableEventLog(void);
void button_disableEventLog(void);
void button_disableEventsCallbacks(void);
void button_registerListener(button_eventFuncPtr listener);
BUTTON_STATE button_getLeftAudioSwitchStatus(void);
BUTTON_STATE button_getRightAudioSwitchStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H_ */
