#ifndef MAIN_LED_DRIVER_H_
#define MAIN_LED_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

_Bool fade_in;
_Bool initial_fade_done;
int LED_R;
int LED_G;
int LED_B;

void flash_led();
void init_led();
void init_turn_off();
void turn_off_esp();
void start_candle_mode();
void breathing();

#ifdef __cplusplus
}
#endif

#endif /* MAIN_LED_DRIVER_H_ */
