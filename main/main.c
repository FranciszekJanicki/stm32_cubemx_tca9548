#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "stdio.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "tca9548.h"
#include "usart.h"
#include <string.h>

typedef struct {
    I2C_HandleTypeDef* i2c_handle;
    uint8_t i2c_address;
} tca9548_bus_user_t;

tca9548_err_t tca9548_gpio_write_pin(void* user, uint32_t pin, bool state)
{
    GPIO_TypeDef* gpio = (GPIO_TypeDef*)user;

    HAL_GPIO_WritePin(gpio, (uint16_t)pin, (GPIO_PinState)state);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_transmit_data(void* user, uint8_t const* data, size_t data_size)
{
    tca9548_bus_user_t* bus_user = (tca9548_bus_user_t*)user;

    HAL_I2C_Master_Transmit(bus_user->i2c_handle, bus_user->i2c_address << 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_receive_data(void* user, uint8_t* data, size_t data_size)
{
    tca9548_bus_user_t* bus_user = (tca9548_bus_user_t*)user;

    HAL_I2C_Master_Receive(bus_user->i2c_handle, bus_user->i2c_address << 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_slave_transmit_data(void* user,
                                              uint16_t slave_address,
                                              uint8_t const* data,
                                              size_t data_size)
{
    tca9548_bus_user_t* bus_user = (tca9548_bus_user_t*)user;

    HAL_I2C_Master_Transmit(bus_user->i2c_handle, slave_address << 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_slave_receive_data(void* user,
                                             uint16_t slave_address,
                                             uint8_t* data,
                                             size_t data_size)
{
    tca9548_bus_user_t* bus_user = (tca9548_bus_user_t*)user;

    HAL_I2C_Master_Receive(bus_user->i2c_handle, slave_address << 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C3_Init();
    MX_I2C2_Init();
    MX_USART2_UART_Init();

    tca9548_t tca9548 = {};

    tca9548_err_t err = tca9548_initialize(
        &tca9548,
        &(tca9548_config_t){.reset_pin = 1 << 2},
        &(tca9548_interface_t){
            .gpio_user = GPIOC,
            .gpio_write_pin = tca9548_gpio_write_pin,
            .bus_user = &(tca9548_bus_user_t){.i2c_address = TCA9548_SLAVE_ADDRESS_A2L_A1L_A0L,
                                              .i2c_handle = &hi2c3},
            .bus_transmit_data = tca9548_bus_transmit_data});

    uint8_t data[100];

    HAL_I2C_Slave_Receive(&hi2c2, data, sizeof(data), 1000);

    for (uint16_t i = 1U; i < (1U << 7U); ++i) {
        HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(&hi2c3, i << 1U, 10, 100);
        printf("Device ready status: %d, HAL_I2C_GetError: 0x%lx\n\r",
               result,
               HAL_I2C_GetError(&hi2c3));
    }

    while (1) {
        memset(data, 0, sizeof(data));
        snprintf((char*)data, sizeof(data), "%s", "Hello world!");

        HAL_StatusTypeDef slave_err = HAL_I2C_Slave_Receive(&hi2c2, data, sizeof(data), 1000);

        tca9548_err_t master_err = tca9548_transmit_channel_data(&tca9548,
                                                                 TCA9548_CHANNEL_0,
                                                                 0x08,
                                                                 data,
                                                                 strlen((char*)data));
        if (slave_err != HAL_OK) {
            printf("Slave error: %d\n\r", slave_err);
        } else {
            printf("Slave data: %s\n\r", (char*)data);
        }

        if (master_err != TCA9548_ERR_OK) {
            printf("Master error: %d\n\r", master_err);
        } else {
            printf("Master data: %s\n\r", (char*)data);
        }

        HAL_Delay(100);
    }
}