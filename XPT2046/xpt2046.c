/*
 *  Author: VadRov
 *  Copyright (C) 2019, VadRov, all right reserved.
 *
 *  Драйвер для работы с тачскрином на базе контроллера XPT2046 (HR2046 и т.п.)
 *
 *  Допускается свободное распространение без целей коммерческого использования.
 *  При коммерческом использовании необходимо согласование с автором.
 *  Распространятся по типу "как есть", то есть использование осуществляете на свой страх и риск.
 *  Автор не предоставляет никаких гарантий.
 *
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 *
 */

#include <string.h>
#include "xpt2046.h"

XPT2046_Handler *touch = 0;

//обработчик прерывания EXTI по спадающему фронту сигнала на линии T_IRQ
//(добавить в соответствующий обработчик EXTIx_IRQHandler - см. файл stm32f4xx_it.c)
void XPT2046_EXTICallback(XPT2046_Handler *t)
{
    if (t) {
   		//на линии T_IRQ низкий уровень?
   		if (!(t->cnt_data.irq_port->IDR & t->cnt_data.irq_pin)) {
   			//инициализация счетчика длительности текущего касания тачскрина
   			t->click_time_counter = 0;
   			t->fl_first_point = 0;
   			t->fl_timer = 1;
   			t->timer_counter = t->timer_update_period;
   			//запрещаем внешнее прерывание для канала exti
   			NVIC_ClearPendingIRQ(t->cnt_data.exti_irq);
   			NVIC_DisableIRQ(t->cnt_data.exti_irq);
   		}
     }
    __DSB();
}

//обработчик прерывания системного таймера
//(добавить в обработчик SysTick_Handler - см. файл stm32f4xx_it.c)
void XPT2046_TIMCallback(XPT2046_Handler *t)
{
	if (t) {
		if (t->fl_timer) {
			t->timer_counter--;
			if (t->timer_counter) return;
			t->timer_counter = t->timer_update_period;
			if (t->fl_wait_exti) {
				t->fl_timer = 0;
				t->fl_wait_exti = 0;
				//разрешаем внешнее прерывание для канала exti
				NVIC_EnableIRQ(t->cnt_data.exti_irq);
				__DSB();
				return;
			}
			if (t->cnt_data.irq_port->IDR & t->cnt_data.irq_pin) { //на входе T_IRQ высокий уровень?
				t->click = 0; //нет касания тачскрина
				t->last_click_time = t->click_time_counter; //длительность последнего касания
				tPoint delta_p1, delta_p2;
				XPT2046_ConvertPoint(&delta_p1, &t->first_click_point, &t->coef);
				XPT2046_ConvertPoint(&delta_p2, &t->point, &t->coef);
				t->delta_p.x = delta_p2.x - delta_p1.x;
				t->delta_p.y = delta_p2.y - delta_p1.y;
				t->fl_wait_exti = 1;
				t->timer_counter = t->timer_update_period;
				__DSB();
				return;
			}
			//разрешено обновление координат тачскрина в прерывании?
			if (t->fl_interrupt) {
				//обновляем координаты тачскрина
				(void)XPT2046_GetTouch(t);
			}
			//есть касание тачскрина
			t->click = 1;
			//увеличиваем длительность текущего касания тачскрина
			t->click_time_counter++;
		}
	}
	__DSB();
}

//преобразование координат тачскрина в дисплейные/экранные координаты
void XPT2046_ConvertPoint(tPoint *p_d, tPoint *p_t, tCoef *coef)
{
	p_d->x = (int)((p_t->x * coef->Dx1 + p_t->y * coef->Dx2 + coef->Dx3) / coef->D);
	p_d->y = (int)((p_t->x * coef->Dy1 + p_t->y * coef->Dy2 + coef->Dy3) / coef->D);
}

//инициализация обработчика тачскрина
void XPT2046_InitTouch(XPT2046_Handler *t, uint32_t timer_update_period, XPT2046_ConnectionData *cnt_data)
{
	memset(t, 0, sizeof(XPT2046_Handler));
	t->timer_update_period = timer_update_period;
	t->cnt_data = *cnt_data;
	touch = t;
}

typedef int (*error_callback)(void *data); //коллбэк обработчика ошибок в процедуре измерения напряжения

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} Data_pin;

typedef struct {
	uint16_t *command;
	int		 *adc;
	uint8_t  n;
	uint8_t  repeat;
} Data_adc;

static int Read_IRQ_Pin(void *param) //функция обработки ошибок при получении данных с тачскрина
{
	Data_pin *irq_pin = (Data_pin *)param;
	if (irq_pin->port->IDR & irq_pin->pin) return 1;
	return 0;
}

/*
 * Измерение напряжения АЦП xpt2046
 * Параметр data определяет на каком входе, с какими параметрами и сколько раз делать измерения
 * для усреднения, а также куда сохранять результаты измерений
 * Возвращает 0 при успешном завершении
 * Результаты измерений формируются в виде 12 битного значения измеряемого напряжения
 */
static int get_xptADC(SPI_TypeDef *spi, Data_adc *data, error_callback func, void *func_param)
{
	int j;
	uint8_t f_first = 1;									//Установим флаг первого измерения
	uint8_t repeat = data->repeat;							//Количество повторов измерений
	uint16_t respond;										//Переменная для хранения ответа от контроллера
	while (repeat--) {										//Повторяем заданное количество измерений параметров
		for (j = 0; j < data->n; j++) {						//Идем по массиву управляющих полуслов
			if (func && func_param) {						//Проверяем, задана ли функция обработки ошибок при измерениях и ее параметры
				if (func(func_param)) { return 1; }			//Если функция и параметры заданы, то вызываем эту функцию
			}												//Измерения прерываем, если функция возвратит значение отличное от 0.
			spi->DR = data->command[j];						//Отправляем управляющее полуслово контроллеру
			while (!(spi->SR & SPI_SR_RXNE)) { __NOP(); }   //Ждем окончания приема ответа от XPT2046
			respond = spi->DR; 								//Получаем ответ
			if (j & 1) {									//Нас интересуют нечетные ответы
				if (!f_first) {								//Мы пропускаем первое измерение, а результаты остальных
					data->adc[j >> 1] += (respond >> 3) & 0xfff; //накапливаем/суммируем в соответствующих переменных параметров
				}
				else {
					data->adc[j >> 1] = 0;					//Инициализируем переменные параметров в случае первого измерения,
															//которое мы пропускаем
				}
			}
		}
		f_first = 0;										//Сбрасываем флаг первого измерения
	}
	if (data->repeat > 1) {									//Если количество повторов измерений больше 1
		for (j = 0; j < data->n >> 1; j++) {				//То вычисляем среднее для каждого измеренного параметра
			data->adc[j] = data->adc[j] / (data->repeat - 1);
		}
	}
	return 0;
}

/* Подключение к контроллеру с сохранением скорости spi */
static uint32_t connect_on(XPT2046_ConnectionData *cnt_data)
{
	SPI_TypeDef *spi = cnt_data->spi;
	//Параметры spi
	spi->CR1 &= ~ (SPI_CR1_BIDIMODE |  	//Режим работы с однонаправленной передачей по линиям данных
				   SPI_CR1_RXONLY |   	//full duplex — передача и прием
				   SPI_CR1_CRCEN | 		//Аппаратный расчет CRC выключен
				   SPI_CR1_DFF );		//8 бит spi
	spi->CR1 |= SPI_CR1_DFF; //16 бит spi
	uint32_t speed = spi->CR1 & SPI_CR1_BR_Msk; 		//Запоминаем скорость передачи spi
	spi->CR1 &= ~SPI_CR1_BR_Msk; 						//Установим скорость spi,
	spi->CR1 |= (uint32_t)((cnt_data->speed & 7)<<(3U));//на которой работает touch
	spi->CR1 |= SPI_CR1_SPE; //spi включаем
	cnt_data->cs_port->BSRR = (uint32_t)cnt_data->cs_pin << 16U; //Подключаем XPT2046 к spi (низкий уровень на T_CS)
	return speed;
}

/* Отключение от контроллера с восстановлением скорости spi */
static void connect_off(XPT2046_ConnectionData *cnt_data, uint32_t speed)
{
	SPI_TypeDef *spi = cnt_data->spi;
	while (spi->SR & SPI_SR_BSY) { __NOP(); }   //Ждем, когда spi освободится
	cnt_data->cs_port->BSRR = cnt_data->cs_pin; //Отключаем XPT2046 от spi (высокий уровень на T_CS)
	spi->CR1 &= ~SPI_CR1_SPE;		//spi выключаем
	spi->CR1 &= ~SPI_CR1_BR_Msk; 	//Восстанавливаем прежнюю
	spi->CR1 |= speed; 				//скорость spi
}

/*
 * Получение координат тачскрина
 * Возвращает 0 при успешном завершении, а при ошибке возвращает:
 * 		1 - если тачскрин неактивен (нет касания )
 * 		2 - если spi занято
 */
uint8_t XPT2046_GetTouch(XPT2046_Handler *t)
{
	XPT2046_ConnectionData *cnt_data = &t->cnt_data;
	SPI_TypeDef *spi = cnt_data->spi;
	if (spi->CR1 & SPI_CR1_SPE) {  	//Выходим с ошибкой, если spi включено (а, значит, занято)
		return 2;
	}
	if (cnt_data->irq_port->IDR & cnt_data->irq_pin) { //Выходим с ошибкой, если тач неактивен (сигнал T_IRQ = 1)
		return 1;
	}
	uint32_t speed = connect_on(cnt_data);
	uint16_t cmd[4] = {XPT2046_Y, XPT2046_NOP, XPT2046_X, XPT2046_NOP}; //Управляющая строка с командами XPT2046
	int adc[2]; //Буфер для приема ответов от XPT2046
	Data_adc data;
	data.command = cmd;
	data.adc = adc;
	data.n = 4;
	data.repeat = 9;
	Data_pin pin_irq;
	pin_irq.port = cnt_data->irq_port;
	pin_irq.pin = cnt_data->irq_pin;
	int f_error = get_xptADC(spi, &data, Read_IRQ_Pin, (void*)&pin_irq);
	if (!f_error) { //Если нет ошибки при измерениях, то:
		t->point.x = adc[1]; //Получаем напряжения, соотвестсвующие
		t->point.y = adc[0]; //точке касания тачскрина
		if (!t->fl_first_point) { //Если точка была первой точкой касания,
			t->fl_first_point = 1; //то запоминаем ее
			t->first_click_point = t->point; //и устанавливаем соответствующий флаг
		}
	}
	connect_off(cnt_data, speed);
	return f_error;
}
