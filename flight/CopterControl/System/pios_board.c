/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Defines board specific static initializers for hardware for the OpenPilot board.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <pios.h>
#include <pios_spi_priv.h>
#include <pios_usart_priv.h>
#include <pios_com_priv.h>
#include <pios_i2c_priv.h>
#include <openpilot.h>
#include <uavobjectsinit.h>

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
void PIOS_Board_Init(void) {

	/* Delay system */
	PIOS_DELAY_Init();	
	
	/* SPI Init */
	PIOS_SPI_Init();
	PIOS_Flash_W25X_Init();

#if defined(PIOS_INCLUDE_SPEKTRUM)
	/* SPEKTRUM init must come before comms */
	PIOS_SPEKTRUM_Init();
#endif
	/* Initialize UAVObject libraries */
	EventDispatcherInitialize();
	UAVObjInitialize();
	UAVObjectsInitializeAll();

	/* Initialize the alarms library */
	AlarmsInitialize();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Initialize the PiOS library */
	PIOS_COM_Init();

	/* Remap AFIO pin */
	GPIO_PinRemapConfig( GPIO_Remap_SWJ_NoJTRST, ENABLE);
	PIOS_Servo_Init();
	
	PIOS_ADC_Init();
	PIOS_GPIO_Init();

#if defined(PIOS_INCLUDE_PWM)
	PIOS_PWM_Init();
#endif
#if defined(PIOS_INCLUDE_PPM)
	PIOS_PPM_Init();
#endif
#if defined(PIOS_INCLUDE_USB_HID)
	PIOS_USB_HID_Init(0);
#endif
	PIOS_I2C_Init();
	PIOS_IAP_Init();
	PIOS_WDG_Init();
}

/* Flash/Accel Interface
 * 
 * NOTE: Leave this declared as const data so that it ends up in the 
 * .rodata section (ie. Flash) rather than in the .bss section (RAM).
 */
void PIOS_SPI_ahrs_irq_handler(void);
void DMA1_Channel4_IRQHandler() __attribute__ ((alias ("PIOS_SPI_ahrs_irq_handler")));
void DMA1_Channel5_IRQHandler() __attribute__ ((alias ("PIOS_SPI_ahrs_irq_handler")));
const struct pios_spi_cfg pios_spi_flash_accel_cfg = {
  .regs   = SPI2,
  .init   = {
    .SPI_Mode              = SPI_Mode_Master,
    .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
    .SPI_DataSize          = SPI_DataSize_8b,
    .SPI_NSS               = SPI_NSS_Soft,
    .SPI_FirstBit          = SPI_FirstBit_MSB,
    .SPI_CRCPolynomial     = 7,
    .SPI_CPOL              = SPI_CPOL_High,
    .SPI_CPHA              = SPI_CPHA_2Edge,
    .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8, 
  },
  .use_crc = FALSE,
  .dma = {
    .ahb_clk  = RCC_AHBPeriph_DMA1,
    
    .irq = {
      .handler = PIOS_SPI_ahrs_irq_handler,
      .flags   = (DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 | DMA1_FLAG_GL4),
      .init    = {
	.NVIC_IRQChannel                   = DMA1_Channel4_IRQn,
	.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
	.NVIC_IRQChannelSubPriority        = 0,
	.NVIC_IRQChannelCmd                = ENABLE,
      },
    },

    .rx = {
      .channel = DMA1_Channel4,
      .init    = {
	.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR),
	.DMA_DIR                = DMA_DIR_PeripheralSRC,
	.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
	.DMA_MemoryInc          = DMA_MemoryInc_Enable,
	.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
	.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
	.DMA_Mode               = DMA_Mode_Normal,
	.DMA_Priority           = DMA_Priority_High,
	.DMA_M2M                = DMA_M2M_Disable,
      },
    },
    .tx = {
      .channel = DMA1_Channel5,
      .init    = {
	.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR),
	.DMA_DIR                = DMA_DIR_PeripheralDST,
	.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
	.DMA_MemoryInc          = DMA_MemoryInc_Enable,
	.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
	.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
	.DMA_Mode               = DMA_Mode_Normal,
	.DMA_Priority           = DMA_Priority_High,
	.DMA_M2M                = DMA_M2M_Disable,
      },
    },
  },
  .ssel = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_12,
      .GPIO_Speed = GPIO_Speed_10MHz,
      .GPIO_Mode  = GPIO_Mode_Out_PP,
    },
  },
  .sclk = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_13,
      .GPIO_Speed = GPIO_Speed_10MHz,
      .GPIO_Mode  = GPIO_Mode_AF_PP,
    },
  },
  .miso = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_14,
      .GPIO_Speed = GPIO_Speed_10MHz,
      .GPIO_Mode  = GPIO_Mode_IN_FLOATING,
    },
  },
  .mosi = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_15,
      .GPIO_Speed = GPIO_Speed_10MHz,
      .GPIO_Mode  = GPIO_Mode_AF_PP,
    },
  },
};

/*
 * Board specific number of devices.
 */
struct pios_spi_dev pios_spi_devs[] = {
  {
    .cfg = &pios_spi_flash_accel_cfg,
  },
};

uint8_t pios_spi_num_devices = NELEMENTS(pios_spi_devs);

void PIOS_SPI_ahrs_irq_handler(void)
{
  /* Call into the generic code to handle the IRQ for this specific device */
//  PIOS_SPI_IRQ_Handler(PIOS_OPAHRS_SPI);
}

/*
 * ADC system
 */
#include "pios_adc_priv.h"
extern void PIOS_ADC_handler(void);
void DMA1_Channel1_IRQHandler() __attribute__ ((alias("PIOS_ADC_handler")));
// Remap the ADC DMA handler to this one
const struct pios_adc_cfg pios_adc_cfg = {
	.dma = {
		.ahb_clk  = RCC_AHBPeriph_DMA1,
		.irq = {
			.handler = PIOS_ADC_DMA_Handler,
			.flags   = (DMA1_FLAG_TC1 | DMA1_FLAG_TE1 | DMA1_FLAG_HT1 | DMA1_FLAG_GL1),
			.init    = {
				.NVIC_IRQChannel                   = DMA1_Channel1_IRQn,
				.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
				.NVIC_IRQChannelSubPriority        = 0,
				.NVIC_IRQChannelCmd                = ENABLE,
			},
		},
		.rx = {
			.channel = DMA1_Channel1,
			.init    = {
				.DMA_PeripheralBaseAddr = (uint32_t) & ADC1->DR,
				.DMA_DIR                = DMA_DIR_PeripheralSRC,
				.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
				.DMA_MemoryInc          = DMA_MemoryInc_Enable,
				.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,
				.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
				.DMA_Mode               = DMA_Mode_Circular,
				.DMA_Priority           = DMA_Priority_High,
				.DMA_M2M                = DMA_M2M_Disable,
			},
		}
	}, 
	.half_flag = DMA1_IT_HT1,
	.full_flag = DMA1_IT_TC1,
};

struct pios_adc_dev pios_adc_devs[] = {
	{
		.cfg = &pios_adc_cfg,
		.callback_function = NULL,
	},
};

uint8_t pios_adc_num_devices = NELEMENTS(pios_adc_devs);

void PIOS_ADC_handler() {
	PIOS_ADC_DMA_Handler();
}


/*
 * Telemetry USART
 */
void PIOS_USART_telem_irq_handler(void);
void USART1_IRQHandler() __attribute__ ((alias ("PIOS_USART_telem_irq_handler")));
const struct pios_usart_cfg pios_usart_telem_cfg = {
  .regs  = USART1,
  .init = {
    #if defined (PIOS_COM_TELEM_BAUDRATE)
        .USART_BaudRate        = PIOS_COM_TELEM_BAUDRATE,
    #else
        .USART_BaudRate        = 57600,
    #endif
    .USART_WordLength          = USART_WordLength_8b,
    .USART_Parity              = USART_Parity_No,
    .USART_StopBits            = USART_StopBits_1,
    .USART_HardwareFlowControl = USART_HardwareFlowControl_None,
    .USART_Mode                = USART_Mode_Rx | USART_Mode_Tx,
  },
  .irq = {
    .handler = PIOS_USART_telem_irq_handler,
    .init    = {
      .NVIC_IRQChannel                   = USART1_IRQn,
      .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
      .NVIC_IRQChannelSubPriority        = 0,
      .NVIC_IRQChannelCmd                = ENABLE,
    },
  },
  .rx   = {
    .gpio = GPIOA,
    .init = {
      .GPIO_Pin   = GPIO_Pin_10,
      .GPIO_Speed = GPIO_Speed_2MHz,
      .GPIO_Mode  = GPIO_Mode_IPU,
    },
  },
  .tx   = {
    .gpio = GPIOA,
    .init = {
      .GPIO_Pin   = GPIO_Pin_9,
      .GPIO_Speed = GPIO_Speed_2MHz,
      .GPIO_Mode  = GPIO_Mode_AF_PP,
    },
  },
};

#ifdef PIOS_COM_GPS
/*
 * GPS USART
 */
void PIOS_USART_gps_irq_handler(void);
void USART3_IRQHandler() __attribute__ ((alias ("PIOS_USART_gps_irq_handler")));
const struct pios_usart_cfg pios_usart_gps_cfg = {
  .regs = USART3,
  .init = {
    #if defined (PIOS_COM_GPS_BAUDRATE)
        .USART_BaudRate        = PIOS_COM_GPS_BAUDRATE,
    #else
        .USART_BaudRate        = 57600,
    #endif
    .USART_WordLength          = USART_WordLength_8b,
    .USART_Parity              = USART_Parity_No,
    .USART_StopBits            = USART_StopBits_1,
    .USART_HardwareFlowControl = USART_HardwareFlowControl_None,
    .USART_Mode                = USART_Mode_Rx | USART_Mode_Tx,
  },
  .irq = {
    .handler = PIOS_USART_gps_irq_handler,
    .init    = {
      .NVIC_IRQChannel                   = USART3_IRQn,
      .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
      .NVIC_IRQChannelSubPriority        = 0,
      .NVIC_IRQChannelCmd                = ENABLE,
    },
  },
  .rx   = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_11,
      .GPIO_Speed = GPIO_Speed_2MHz,
      .GPIO_Mode  = GPIO_Mode_IPU,
    },
  },
  .tx   = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_10,
      .GPIO_Speed = GPIO_Speed_2MHz,
      .GPIO_Mode  = GPIO_Mode_AF_PP,
    },
  },
};
#endif

#ifdef PIOS_COM_SPEKTRUM
/*
 * SPEKTRUM USART
 */
#include <pios_spektrum_priv.h>
void PIOS_USART_spektrum_irq_handler(void);
void USART3_IRQHandler() __attribute__ ((alias ("PIOS_USART_spektrum_irq_handler")));
void TIM6_IRQHandler();
void TIM6_IRQHandler() __attribute__ ((alias ("PIOS_TIM6_irq_handler")));
const struct pios_spektrum_cfg pios_spektrum_cfg = {
	.pios_usart_spektrum_cfg = {
		  .regs = USART3,
		  .init = {
			#if defined (PIOS_COM_SPEKTRUM_BAUDRATE)
				.USART_BaudRate        = PIOS_COM_SPEKTRUM_BAUDRATE,
			#else
				.USART_BaudRate        = 115200,
			#endif
			.USART_WordLength          = USART_WordLength_8b,
			.USART_Parity              = USART_Parity_No,
			.USART_StopBits            = USART_StopBits_1,
			.USART_HardwareFlowControl = USART_HardwareFlowControl_None,
			.USART_Mode                = USART_Mode_Rx,
		  },
		  .irq = {
			.handler = PIOS_USART_spektrum_irq_handler,
			.init    = {
			  .NVIC_IRQChannel                   = USART3_IRQn,
			  .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
			  .NVIC_IRQChannelSubPriority        = 0,
			  .NVIC_IRQChannelCmd                = ENABLE,
			},
		  },
		  .rx   = {
			.gpio = GPIOB,
			.init = {
			  .GPIO_Pin   = GPIO_Pin_11,
			  .GPIO_Speed = GPIO_Speed_2MHz,
			  .GPIO_Mode  = GPIO_Mode_IPU,
			},
		  },
		  .tx   = {
			.gpio = GPIOB,
			.init = {
			  .GPIO_Pin   = GPIO_Pin_10,
			  .GPIO_Speed = GPIO_Speed_2MHz,
			  .GPIO_Mode  = GPIO_Mode_IN_FLOATING,
			},
		  },
	},
	.tim_base_init = {
		.TIM_Prescaler = (PIOS_MASTER_CLOCK / 1000000) - 1,	/* For 1 uS accuracy */
		.TIM_ClockDivision = TIM_CKD_DIV1,
		.TIM_CounterMode = TIM_CounterMode_Up,
		.TIM_Period = ((1000000 / 60) - 1), //60hz
		.TIM_RepetitionCounter = 0x0000,
	},
	.gpio_init = { //used for bind feature
		.GPIO_Mode = GPIO_Mode_Out_PP,
		.GPIO_Speed = GPIO_Speed_2MHz,
	},
	.remap = 0,
	.irq = {
		.handler = TIM6_IRQHandler,
		.init    = {
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.timer = TIM6,
	.port = GPIOB,
	.ccr = TIM_IT_Update,
	.pin = GPIO_Pin_11,
};

void PIOS_TIM6_irq_handler()
{
	PIOS_SPEKTRUM_irq_handler();
}
#endif

/*
 * Board specific number of devices.
 */
struct pios_usart_dev pios_usart_devs[] = {
#define PIOS_USART_TELEM  0
  {
    .cfg = &pios_usart_telem_cfg,
  },
#ifdef PIOS_COM_GPS
#define PIOS_USART_GPS    1
  {
    .cfg = &pios_usart_gps_cfg,
  },
#endif
#ifdef PIOS_COM_SPEKTRUM
#define PIOS_USART_AUX    2
  {
    .cfg = &pios_spektrum_cfg.pios_usart_spektrum_cfg,
  },
#endif
};

uint8_t pios_usart_num_devices = NELEMENTS(pios_usart_devs);

void PIOS_USART_telem_irq_handler(void)
{
  PIOS_USART_IRQ_Handler(PIOS_USART_TELEM);
}

#ifdef PIOS_COM_GPS
void PIOS_USART_gps_irq_handler(void)
{
  PIOS_USART_IRQ_Handler(PIOS_USART_GPS);
}
#endif

#ifdef PIOS_COM_SPEKTRUM
void PIOS_USART_spektrum_irq_handler(void)
{
	SPEKTRUM_IRQHandler();
}
#endif

/*
 * COM devices
 */

/*
 * Board specific number of devices.
 */
extern const struct pios_com_driver pios_usart_com_driver;
extern const struct pios_com_driver pios_usb_com_driver;

struct pios_com_dev pios_com_devs[] = {
  {
    .id     = PIOS_USART_TELEM,
    .driver = &pios_usart_com_driver,
  },
#ifdef PIOS_COM_GPS
  {
    .id     = PIOS_USART_GPS,
    .driver = &pios_usart_com_driver,
  },
#endif
#if defined(PIOS_INCLUDE_USB_HID)
  {
    .id     = 0,
    .driver = &pios_usb_com_driver,
  },
#endif
#ifdef PIOS_COM_AUX
  {
    .id     = PIOS_USART_AUX,
    .driver = &pios_usart_com_driver,
  },
#endif
#ifdef PIOS_COM_SPEKTRUM
  {
    .id     = PIOS_USART_AUX,
    .driver = &pios_usart_com_driver,
  },
#endif
};

const uint8_t pios_com_num_devices = NELEMENTS(pios_com_devs);


/* 
 * Servo outputs 
 */
#include <pios_servo_priv.h>
const struct pios_servo_channel pios_servo_channels[] = {
	{
		.timer = TIM4,
		.port = GPIOB,
		.channel = TIM_Channel_4,
		.pin = GPIO_Pin_9,
	}, 
	{
		.timer = TIM4,
		.port = GPIOB,
		.channel = TIM_Channel_3,
		.pin = GPIO_Pin_8,
	}, 
	{
		.timer = TIM4,
		.port = GPIOB,
		.channel = TIM_Channel_2,
		.pin = GPIO_Pin_7,
	}, 
	{
		.timer = TIM1,
		.port = GPIOA,
		.channel = TIM_Channel_1,
		.pin = GPIO_Pin_8,
	}, 
	{ /* needs to remap to alternative function */
		.timer = TIM3,
		.port = GPIOB,
		.channel = TIM_Channel_1,
		.pin = GPIO_Pin_4,
	},  	
	{
		.timer = TIM2,
		.port = GPIOA,
		.channel = TIM_Channel_3,
		.pin = GPIO_Pin_2,
	}, 		
};

const struct pios_servo_cfg pios_servo_cfg = {
	.tim_base_init = {
		.TIM_Prescaler = (PIOS_MASTER_CLOCK / 1000000) - 1,
		.TIM_ClockDivision = TIM_CKD_DIV1,
		.TIM_CounterMode = TIM_CounterMode_Up,
		.TIM_Period = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
		.TIM_RepetitionCounter = 0x0000,
	},
	.tim_oc_init = {
		.TIM_OCMode = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse = PIOS_SERVOS_INITIAL_POSITION,		
		.TIM_OCPolarity = TIM_OCPolarity_High,
		.TIM_OCNPolarity = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.gpio_init = {
		.GPIO_Mode = GPIO_Mode_AF_PP,
		.GPIO_Speed = GPIO_Speed_2MHz,
	},
	.remap = GPIO_PartialRemap_TIM3,
	.channels = pios_servo_channels,
	.num_channels = NELEMENTS(pios_servo_channels),
};


/* 
 * PWM Inputs 
 */
#if defined(PIOS_INCLUDE_PWM)
#include <pios_pwm_priv.h>
const struct pios_pwm_channel pios_pwm_channels[] = {
	{
		.timer = TIM4,
		.port = GPIOB,
		.ccr = TIM_IT_CC1,
		.channel = TIM_Channel_1,
		.pin = GPIO_Pin_6,
	}, 
	{
		.timer = TIM3,
		.port = GPIOB,
		.ccr = TIM_IT_CC2,
		.channel = TIM_Channel_2,
		.pin = GPIO_Pin_5,
	}, 
	{
		.timer = TIM3,
		.port = GPIOB,
		.ccr = TIM_IT_CC3,
		.channel = TIM_Channel_3,
		.pin = GPIO_Pin_0
	}, 
	{
		.timer = TIM3,
		.port = GPIOB,
		.ccr = TIM_IT_CC4,
		.channel = TIM_Channel_4,
		.pin = GPIO_Pin_1,
	}, 
	{ 
		.timer = TIM2,
		.port = GPIOA,
		.ccr = TIM_IT_CC1,
		.channel = TIM_Channel_1,
		.pin = GPIO_Pin_0,
	},  	
	{
		.timer = TIM2,
		.port = GPIOA,
		.ccr = TIM_IT_CC2,
		.channel = TIM_Channel_2,
		.pin = GPIO_Pin_1,
	}, 		
};

void TIM2_IRQHandler();
void TIM3_IRQHandler();
void TIM4_IRQHandler();
void TIM2_IRQHandler() __attribute__ ((alias ("PIOS_TIM2_irq_handler")));
void TIM3_IRQHandler() __attribute__ ((alias ("PIOS_TIM3_irq_handler")));
void TIM4_IRQHandler() __attribute__ ((alias ("PIOS_TIM4_irq_handler")));
const struct pios_pwm_cfg pios_pwm_cfg = {
	.tim_base_init = {
		.TIM_Prescaler = (PIOS_MASTER_CLOCK / 1000000) - 1,
		.TIM_ClockDivision = TIM_CKD_DIV1,
		.TIM_CounterMode = TIM_CounterMode_Up,
		.TIM_Period = 0xFFFF,
		.TIM_RepetitionCounter = 0x0000,
	},
	.tim_ic_init = {
		.TIM_ICPolarity = TIM_ICPolarity_Rising,
		.TIM_ICSelection = TIM_ICSelection_DirectTI,
		.TIM_ICPrescaler = TIM_ICPSC_DIV1,
		.TIM_ICFilter = 0x0,		
	},
	.gpio_init = {
		.GPIO_Mode = GPIO_Mode_IPD,
		.GPIO_Speed = GPIO_Speed_2MHz,
	},
	.remap = 0,
	.irq = {
		.handler = TIM2_IRQHandler,
		.init    = {
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.channels = pios_pwm_channels,
	.num_channels = NELEMENTS(pios_pwm_channels),
};
void PIOS_TIM2_irq_handler()
{
	PIOS_PWM_irq_handler(TIM2);
}
void PIOS_TIM3_irq_handler()
{
	PIOS_PWM_irq_handler(TIM3);
}
void PIOS_TIM4_irq_handler()
{
	PIOS_PWM_irq_handler(TIM4);
}
#endif

/*
 * I2C Adapters
 */

void PIOS_I2C_main_adapter_ev_irq_handler(void);
void PIOS_I2C_main_adapter_er_irq_handler(void);
void I2C2_EV_IRQHandler() __attribute__ ((alias ("PIOS_I2C_main_adapter_ev_irq_handler")));
void I2C2_ER_IRQHandler() __attribute__ ((alias ("PIOS_I2C_main_adapter_er_irq_handler")));

const struct pios_i2c_adapter_cfg pios_i2c_main_adapter_cfg = {
  .regs = I2C2,
  .init = {
    .I2C_Mode                = I2C_Mode_I2C,
    .I2C_OwnAddress1         = 0,
    .I2C_Ack                 = I2C_Ack_Enable,
    .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit,
    .I2C_DutyCycle           = I2C_DutyCycle_2,
    .I2C_ClockSpeed          = 400000,	/* bits/s */
  },
  .transfer_timeout_ms = 50,
  .scl = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_10,
      .GPIO_Speed = GPIO_Speed_10MHz,
      .GPIO_Mode  = GPIO_Mode_AF_OD,
    },
  },
  .sda = {
    .gpio = GPIOB,
    .init = {
      .GPIO_Pin   = GPIO_Pin_11,
      .GPIO_Speed = GPIO_Speed_10MHz,
      .GPIO_Mode  = GPIO_Mode_AF_OD,
    },
  },
  .event = {
    .handler = PIOS_I2C_main_adapter_ev_irq_handler,
    .flags   = 0,		/* FIXME: check this */
    .init = {
      .NVIC_IRQChannel                   = I2C2_EV_IRQn,
      .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
      .NVIC_IRQChannelSubPriority        = 0,
      .NVIC_IRQChannelCmd                = ENABLE,
    },
  },
  .error = {
    .handler = PIOS_I2C_main_adapter_er_irq_handler,
    .flags   = 0,		/* FIXME: check this */
    .init = {
      .NVIC_IRQChannel                   = I2C2_ER_IRQn,
      .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
      .NVIC_IRQChannelSubPriority        = 0,
      .NVIC_IRQChannelCmd                = ENABLE,
    },
  },
};

/*
 * Board specific number of devices.
 */
struct pios_i2c_adapter pios_i2c_adapters[] = {
  {
    .cfg = &pios_i2c_main_adapter_cfg,
  },
};

uint8_t pios_i2c_num_adapters = NELEMENTS(pios_i2c_adapters);

void PIOS_I2C_main_adapter_ev_irq_handler(void)
{
  /* Call into the generic code to handle the IRQ for this specific device */
  PIOS_I2C_EV_IRQ_Handler(PIOS_I2C_MAIN_ADAPTER);
}

void PIOS_I2C_main_adapter_er_irq_handler(void)
{
  /* Call into the generic code to handle the IRQ for this specific device */
  PIOS_I2C_ER_IRQ_Handler(PIOS_I2C_MAIN_ADAPTER);
}

/**
 * @}
 */
