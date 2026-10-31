#include "dht11.h"
#include <string.h>

TIM_HandleTypeDef *timer;
extern UART_HandleTypeDef huart2 ;
uint8_t dht_stage ;   /* 0 = ok, others = where it failed */

static void DHT11_Setoutput(void);
static void DHT11_Setinput(void);
static uint8_t DHT11_Readpin(GPIO_PinState state);
static void DHT11_Delay_us(uint16_t us);

void DHT11_Init(TIM_HandleTypeDef *htim)
{
    timer = htim;
    HAL_TIM_Base_Start(timer);

    DHT11_Setoutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);   /* release the data line */
    HAL_Delay(1000);                                          /* sensor power-up time */
}

static void DHT11_Setoutput(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = DHT11_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;              /* was .Mode by mistake */
    HAL_GPIO_Init(DHT11_PORT, & g);
}

static void DHT11_Setinput(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin  = DHT11_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;                       /* pull-up works in input mode */
    HAL_GPIO_Init(DHT11_PORT, &g);
}

static void DHT11_Delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(timer, 0);
    while (__HAL_TIM_GET_COUNTER(timer) < us);
}

/* waits until the pin becomes 'state'. returns 1 if OK, 0 on timeout (100us) */
static uint8_t DHT11_Readpin(GPIO_PinState state)
{
    uint16_t start = (uint16_t)__HAL_TIM_GET_COUNTER(timer);

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != state)
    {
        uint16_t now = (uint16_t)__HAL_TIM_GET_COUNTER(timer);
        if ((uint16_t)(now - start) > 100u)
        {
            return 0;
        }
    }
    return 1;
}

/* data = array of 5 bytes. returns 1 on success, 0 on failure */
uint8_t DHT11_Read(uint8_t *data)
{
	if ((data == NULL) || (timer == NULL))
	{
	    const char err[] = "Data empty or timer null\r\n";
	    HAL_UART_Transmit(&huart2, (uint8_t *)err, strlen(err), 100);

	    dht_stage = 1;  /* null pointer check fails */

	    return 0;
	}


    /* start signal */
    DHT11_Setoutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);  /* pull low for not less than 18ms */
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);    /* release the data line */
    DHT11_Setinput();

    /* sensor response: low, high, low */
    if (!DHT11_Readpin(GPIO_PIN_RESET))
    	{dht_stage = 2;  /* sensor never pulled line low after start signal */
    	return 0; }

    if (!DHT11_Readpin(GPIO_PIN_SET))

	    dht_stage = 3;  /* sensor low pulse didn't go high */

    	return 0;
    if (!DHT11_Readpin(GPIO_PIN_RESET))
	    dht_stage = 4;  /* sensor high pulse didn't go low */

    	return 0;

    /* read 40 bits */
    for (int i = 0; i < 40; i++)
    {
        if (!DHT11_Readpin(GPIO_PIN_SET))
    	    dht_stage = 5;  /* a data bit failed (waiting for high */
        	return 0;
        DHT11_Delay_us(40);                                    /* 26us pulse = 0, 70us pulse = 1 */

        data[i / 8] <<= 1;
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
        {
            data[i / 8] |= 1;
        }

        if (!DHT11_Readpin(GPIO_PIN_RESET))
    	    dht_stage = 6;  /* checksum mismatch */
        	return 0;          /* wait for pulse to end */
    }

    /* checksum */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
	    dht_stage = 7;  /* checksum mismatch */
    	return 0;

    return 1;
}
