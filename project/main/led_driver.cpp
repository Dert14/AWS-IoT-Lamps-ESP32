#include "FastLED.h"
#include "FX.h"
#include "aws_connection.h"
#include "led_driver.h"

#define LED_PIN    15
#define NUM_LEDS   12
CRGB leds[NUM_LEDS];

void initialize_led_task(void* pvParameters)
{
	fade_in = false;
	initial_fade_done = false;
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(0);
    while (1)
    {
        if (!data_recived)
        {
        	if (!fade_in)
        	{
				for (int i = 0; i < 100; i += 2)
				{
					FastLED.setBrightness(i);
					fill_solid(leds, NUM_LEDS, CRGB::Aqua);
					FastLED.show();
					vTaskDelay(pdMS_TO_TICKS(40));
				}
				for (int i = 100; i < 200; i += 2)
				{
					FastLED.setBrightness(i);
					fill_solid(leds, NUM_LEDS, CRGB::Aqua);
					FastLED.show();
					vTaskDelay(pdMS_TO_TICKS(20));
				}
				fade_in = true;
        	}
    	}
        else
    	{
        	for (int i = 200; i > 0; i -= 2)
        	     {
        	        FastLED.setBrightness(i);
        	        fill_solid(leds, NUM_LEDS, CRGB::Aqua);
        	        FastLED.show();
        			vTaskDelay(pdMS_TO_TICKS(20));
        	     }
        	fill_solid(leds, NUM_LEDS, CRGB::Black);
        	FastLED.show();
        	initial_fade_done = true;
            vTaskDelete(NULL);
    	}
    }
}

void flash_led_task(void* pvParameters)
{
	bool done = false;
	LED_R = 0;
	LED_G = 0;
	LED_B = 0;
	while (1)
	{
		FastLED.setBrightness(255);
		if((LED_R != led_states[0] || LED_G != led_states[1] || LED_B != led_states[2]) && led_states[3] == 3)
		{
			done = 0;
		}
		else if(led_states[3] == 3)
		{
			done = 1;
		}
		if (initial_fade_done && !done && led_states[3] == 3)
		{
			for (int i = 0; i < 255; i++)
			{
				if (LED_R < led_states[0])
				{
					LED_R++;
				}
				else if (LED_R > led_states[0])
				{
					LED_R--;
				}
				if (LED_G < led_states[1])
				{
					LED_G++;
				}
				else if (LED_G > led_states[1])
				{
					LED_G--;
				}
				if (LED_B < led_states[2])
				{
					LED_B++;
				}
				else if (LED_B > led_states[2])
				{
					LED_B--;
				}

				for (int i = 0; i < NUM_LEDS; i++)
				{
					leds[i] = CRGB(LED_R, LED_G, LED_B);
				}
				FastLED.show();
				vTaskDelay(pdMS_TO_TICKS(10));
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void turn_off_led_task(void* pvParameters)
{
	gpio_pad_select_gpio(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	while (1)
	{
		FastLED.setBrightness(255);
		if (led_states[3] == 2 && initial_fade_done)
		{
			for (int i = 0; i < 255; i++)
			{
					if (LED_R > 0)
					{
						LED_R--;
					}
					if (LED_G > 0)
					{
						LED_G--;
					}
					if (LED_B > 0)
					{
						LED_B--;
					}

				for (int j = 0; j < NUM_LEDS; j++)
				{
					leds[j] = CRGB(LED_R, LED_G, LED_B);
				}
				FastLED.show();
				vTaskDelay(pdMS_TO_TICKS(10));
				if (LED_R == 0 && LED_G == 0 && LED_B == 0)
				{
					gpio_set_level(GPIO_NUM_2, 1);
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}


void turn_off_esp(void* pvParameters)
{
	while (1)
	{
		if (led_states[3] == 1)
		{
		gpio_set_level(GPIO_NUM_2, 1);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void start_candle_mode_task(void* pvParameters)
{
	while (1)
	{
		int random_value = 0;
		int random_led = 0;
		if (led_states[3] == 4 && initial_fade_done)
		{
			LED_R = 0;
			LED_G = 0;
			LED_B = 0;
			for (int i = 0; i < 12 ; i++)
			{
				leds[i] = CRGB(LED_R,LED_G,LED_B);
			}
			random_value = esp_random();
			if (random_value % 2 == 0)
			{
				LED_R = 255;
				LED_G = 98;
				LED_B = 10;
				for (int i = 0; i < 9 ; i+=3)
				{
					leds[i] = CRGB(LED_R,LED_G,LED_B);
				}
			}
			if (random_value % 3 == 0)
			{
				LED_R = 255;
				LED_G = 98;
				LED_B = 10;
				for (int i = 0; i < 10 ; i+=2)
				{
					leds[i] = CRGB(LED_R,LED_G,LED_B);
				}
			}
			if (random_value % 4 == 0)
			{
				LED_R = 255;
				LED_G = 98;
				LED_B = 10;
				for (int i = 1; i < 10 ; i+=3)
				{
					leds[i] = CRGB(LED_R,LED_G,LED_B);
				}
			}
			if (random_value % 5 == 0)
			{
				LED_R = 255;
				LED_G = 98;
				LED_B = 10;
				for (int i = 1; i < 11 ; i+=2)
				{
					leds[i] = CRGB(LED_R,LED_G,LED_B);
				}
			}
			else
			{
				LED_R = 255;
				LED_G = 98;
				LED_B = 10;
				for (int i = 1; i < 12 ; i++)
				{
					leds[i] = CRGB(LED_R,LED_G,LED_B);
				}
			}

			FastLED.show();
		}
			vTaskDelay(pdMS_TO_TICKS(180));
	}
}

void breathing_task(void* pvParameters)
{
	bool up = 0;
	bool down = 0;
	while (1)
	{
		if (led_states[3] == 5 && initial_fade_done)
		{
			LED_R = 0;
			LED_G = 0;
			if (LED_B < 15)
			{
				up = 1;
				down = 0;
			}
			else if (LED_B > 135)
			{
				up = 0;
				down = 1;
			}
			if(up)
			{
				LED_B++;
			}
			else if(down)
			{
				LED_B--;
			}
			for (int j = 0; j < NUM_LEDS; j++)
			{
				leds[j] = CRGB(LED_R, LED_G, LED_B);
			}
			FastLED.show();
		}
		vTaskDelay(pdMS_TO_TICKS(35));
	}
}

void init_led()
{
    xTaskCreate(initialize_led_task, "init LED Task", 4096, NULL, 5, NULL);
}
void flash_led()
{
    xTaskCreate(flash_led_task, "start LED Task", 4096, NULL, 4, NULL);
}
void init_turn_off()
{
    xTaskCreate(turn_off_led_task, "turn off led_task", 4096, NULL, 4, NULL);
}
void turn_off_esp()
{
    xTaskCreate(turn_off_esp, "turn off esp", 4096, NULL, 4, NULL);
}
void start_candle_mode()
{
    xTaskCreate(start_candle_mode_task, "start candle mode task", 4096, NULL, 4, NULL);
}
void breathing()
{
    xTaskCreate(breathing_task, "breathing task", 4096, NULL, 4, NULL);
}
