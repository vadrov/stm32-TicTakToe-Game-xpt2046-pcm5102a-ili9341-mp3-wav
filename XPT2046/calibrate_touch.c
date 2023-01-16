/*
 *  Copyright (C) 2019, VadRov, all right reserved.
 *
 *  Калибровка тачскрина по 5 точкам
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

#include "calibrate_touch.h"

/*
 * Расчет коэффициентов для преобразования координат тачскрина в дисплейные координаты
 */
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

/* Калибровка тачскрина по 5 точкам */
void XPT2046_CalibrateTouch(XPT2046_Handler *t, LCD_Handler *lcd)
{
	tPoint p_display[5], p_touch[5]; //Массивы для хранения данных о 5 точках калибровки
	//Таблица с дисплейными координатами 5 точек калибровки (4 - края + 1 - центр)
	uint16_t pos_xy[] = {0, 0, 0, lcd->Height - 10, lcd->Width - 10, 0, lcd->Width - 10, lcd->Height - 10, lcd->Width/2 - 5, lcd->Height/2 - 5};
	uint16_t *pos_x = pos_xy, *pos_y = pos_xy + 1; //Указатели на координаты в таблице
	uint8_t i  = 0;
	t->fl_interrupt = 1; //Разрешаем обновление координат тачскрина в прерывании
	LCD_Fill(lcd, COLOR_WHITE); //Очищаем дисплей
	while (i < 5) { //Цикл по 5 точкам калибровки
		//Рисуем закрашенный квадрат - точку калибровки
		LCD_DrawFilledRectangle(lcd, *pos_x, *pos_y, *pos_x + 9, *pos_y + 9, COLOR_RED);
		while (1) { //Цикл опроса тачскрина
			while (!t->click) { //Ждем касания тачскрина
				__NOP();
			}
			while (t->click) { //Пока есть касание тачскрина
				if (!t->fl_interrupt) { //Если запрещен опрос тачскрина в прерывании,
					(void)XPT2046_GetTouch(t); //то опрашиваем тачскрин здесь, в цикле
				}
				__NOP();
			}
			if (t->last_click_time > 3) { //Ограничиваем минимальную длительность касания, для фильтрации случайных касаний
				break; //Прерываем цикл опроса тачскрина
			}
		}
		//Запоминаем дисплейные координаты - центр калибровочного квадрата
		p_display[i].x = *pos_x + 5;
		p_display[i].y = *pos_y + 5;
		//Запоминаем координаты тачскрина, соответствующие дисплейным
		p_touch[i] = t->point;
		i++; //Переходим к следующей точке калибровки
		pos_x += 2; //Перемещаем указатели по таблице
		pos_y += 2;	//на следующие координаты
		LCD_Fill(lcd, COLOR_WHITE); //Очищаем дисплей
		LL_mDelay(1300); //Ждем 1.3 секунды
	}
	//Расчитываем коэффициенты для перехода от координат тачскрина в дисплейные координаты
	CoefCalc(p_display, p_touch, &t->coef, 5);
}
