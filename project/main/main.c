#include <led_driver.h>
#include "aws_connection.h"

void app_main(void)
{
	aws_connect();
	init_led();
	flash_led();
	init_turn_off();
	turn_off_esp();
	start_candle_mode();
	breathing();
}
