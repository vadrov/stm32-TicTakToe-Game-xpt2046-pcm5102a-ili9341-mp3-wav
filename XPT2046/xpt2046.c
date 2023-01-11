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
#include "display.h"

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
   			__DSB();
   		}
     }
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
			__DSB();
		}
	}
}

//расчет коэффициентов для преобразования координат тачскрина в дисплейные координаты
static void CoefCalc(tPoint *p_d, tPoint *p_t, tCoef *coef, uint8_t all_points)
{
	uint64_t a = 0, b = 0, c = 0, d = 0, e = 0, X1 = 0, X2 = 0, X3 = 0, Y1 = 0, Y2 = 0, Y3 = 0;
	for(uint8_t i = 0; i < all_points; i++)	{
		a += p_t[i].x * p_t[i].x;
		b += p_t[i].y * p_t[i].y;
		c += p_t[i].x * p_t[i].y;
		d += p_t[i].x;
		e += p_t[i].y;
		X1 += p_t[i].x * p_d[i].x;
		X2 += p_t[i].y * p_d[i].x;
		X3 += p_d[i].x;
		Y1 += p_t[i].x * p_d[i].y;
		Y2 += p_t[i].y * p_d[i].y;
		Y3 += p_d[i].y;
	}
	coef->D = all_points * (a * b - c * c) + 2 * c *  d * e - a * e * e - b * d * d;
	coef->Dx1 = all_points * (X1 * b - X2 * c) + e * (X2 * d - X1 * e) + X3 * (c * e - b * d);
	coef->Dx2 = all_points * (X2 * a - X1 * c) + d * (X1 * e - X2 * d) + X3 * (c * d - a * e);
	coef->Dx3 = X3 * (a * b - c * c) + X1 * (c * e - b * d) + X2 * (c * d - a * e);
	coef->Dy1 = all_points * (Y1 * b - Y2 * c) + e * (Y2 * d - Y1 * e) + Y3 * (c * e - b * d);
	coef->Dy2 = all_points * (Y2 * a - Y1 * c) + d * (Y1 * e - Y2 * d) + Y3 * (c * d - a * e);
	coef->Dy3 = Y3 * (a * b - c * c) + Y1 * (c * e - b * d) + Y2 * (c * d -a * e);
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

//получение координат тачскрина
//координаты не получены когда на выходе 0
uint8_t XPT2046_GetTouch(XPT2046_Handler *t)
{
	XPT2046_ConnectionData *cnt_data = &t->cnt_data;
	SPI_TypeDef *spi = cnt_data->spi;
	if ((spi->CR1 & SPI_CR1_SPE) ||
		(cnt_data->irq_port->IDR & cnt_data->irq_pin)) return 0; //выходим, если spi включено/занято или тач неактивен
	uint16_t cmd[4] = {XPT2046_Y, XPT2046_NOP, XPT2046_X, XPT2046_NOP};
	uint16_t buff[4];
	uint32_t avg_x = 0, avg_y = 0;
	//параметры spi
	spi->CR1 &= ~ (SPI_CR1_BIDIMODE |  	//режим работы с однонаправленной передачей по линиям данных
				   SPI_CR1_RXONLY |   	//full duplex — передача и прием
				   SPI_CR1_CRCEN | 		//аппаратный расчет CRC выключен
				   SPI_CR1_DFF );		//8 бит spi
	spi->CR1 |= SPI_CR1_DFF; //16 бит spi
	uint32_t speed = spi->CR1 & SPI_CR1_BR_Msk; 		//запоминаем скорость передачи spi
	spi->CR1 &= ~SPI_CR1_BR_Msk; 						//установим скорость spi,
	spi->CR1 |= (uint32_t)((cnt_data->speed & 7)<<(3U));//на которой работает touch
	spi->CR1 |= SPI_CR1_SPE; // SPI включаем
	//подключаем контроллер тача к spi (низкий уровень на T_CS)
	cnt_data->cs_port->BSRR = (uint32_t)cnt_data->cs_pin << 16U;
	//получение усредненных координат за заданное количество замеров
	uint8_t repeat = 9, f1 = 0, res = 0, f_1  = 1;
	while (repeat--)
	{
		for (int i = 0; i < 4; i++)
		{
			//в процессе преобразования АЦП тач перестал быть активен?
			if (cnt_data->irq_port->IDR & cnt_data->irq_pin)
			{
				f1 = 1;
				break;
			}
			spi->DR = cmd[i];	//передаем команду
			while (!(spi->SR & SPI_SR_RXNE)) { __NOP(); }   //ждем окончания приема
			buff[i] = spi->DR; //сохраняем ответ
		}
		if (f1) break;
		if (!f_1) {
			avg_y += (buff[1] >> 3) & 0xfff;
			avg_x += (buff[3] >> 3) & 0xfff;
		}
		f_1 = 0;
	}
	if (!f1)
	{
		t->point.x = avg_x >> 3;
		t->point.y = avg_y >> 3;
		if (!t->fl_first_point) {
			t->fl_first_point = 1;
			t->first_click_point = t->point;
		}
		res = 1;
	}
	while (spi->SR & SPI_SR_BSY) { __NOP(); }   //ждем, когда spi освободится
	//отключаем контроллер тача от spi (высокий уровень на T_CS)
	cnt_data->cs_port->BSRR = cnt_data->cs_pin;
	spi->CR1 &= ~SPI_CR1_SPE;		//SPI выключаем
	spi->CR1 &= ~SPI_CR1_BR_Msk; 	//восстанавливаем прежнюю
	spi->CR1 |= speed; 				//скорость spi
	return res;
}

//калибровка тачскрина по 5 точкам
uint8_t XPT2046_CalibrateTouch(XPT2046_Handler *t, LCD_Handler *lcd)
{
	//массивы для хранения данных о 5 точках калибровки
	tPoint p_display[5], p_touch[5];
	//координаты 5 точек калибровки (4 - края + 1 - центр дисплея)
	uint16_t pos_xy[] = {0, 0, 0, lcd->Height - 10, lcd->Width - 10, 0, lcd->Width - 10, lcd->Height - 10, lcd->Width/2 - 5, lcd->Height/2 - 5};
	uint16_t *pos_x = pos_xy, *pos_y = pos_xy + 1;
	uint8_t i  = 0, res = 0;
	LCD_Fill(lcd, COLOR_WHITE);
	//разрешаем обновление координат тачскрина в прерывании
	t->fl_interrupt = 1;
	//цикл по 5 точкам калибровки
	while (i < 5)
	{
		//рисуем квадрат - точку калибровки
		LCD_DrawFilledRectangle(lcd, *pos_x, *pos_y, *pos_x + 9, *pos_y + 9, COLOR_RED);
		while (1) {
			while (!t->click) { //ждем касания тачскрина
				__NOP();
			}
			while (t->click) { //пока есть касание тачскрина
				if (!t->fl_interrupt) {
					(void)XPT2046_GetTouch(t);
				}
				__NOP();
			}
			if (t->last_click_time > 3) break; //минимальная длительность касания
		}
		//запоминаем дисплейные координаты - центр калибровочного квадрата
		p_display[i].x = *pos_x + 5;
		p_display[i].y = *pos_y + 5;
		//запоминаем координаты тачскрина, соответствующие дисплейным
		p_touch[i] = t->point;
		//очищаем экран
		LCD_Fill(lcd, COLOR_WHITE);
		//пауза 1300 миллисекунд
		LL_mDelay(1300);
		//переходим к следующей точке калибровки
		i++;
		pos_x += 2;
		pos_y += 2;
	}
	//расчитываем коэффициенты для перехода от координат тачскрина в дисплейные координаты
	CoefCalc(p_display, p_touch, &t->coef, 5);
	return res;
}
