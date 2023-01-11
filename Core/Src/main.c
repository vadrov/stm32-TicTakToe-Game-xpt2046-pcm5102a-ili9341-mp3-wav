/* USER CODE BEGIN Header */
/*
 * Copyright (C) 2022, VadRov, all right reserved.
 *
 * Демонстрация работы с сенсорным экраном (тачскрином) на контроллере XPT2046 (HR2046 и т.п.),
 * дисплеем на базе контроллера ILI9341 (spi) 320х240, аудио ЦАПом PCM5102
 *
 * --------------------------------------- ДЕМО-ИГРА ------------------------------------------
 *	         		       для stm32f4 (stm32f401ccu6) с touch XPT2046
 * 		        				        Крестики-нолики
 *
 * С ИИ на основе алгоритма Минимакс (Minimax)
 *
 * Со звуком и эффектами (программные декодеры mp3 и wav)
 *
 * Использованы фрагменты музыкальных произведений (на основе лицензии CC BY 3.0)
 * композитора Кевина Маклауда:
 * 					Constance,  (с)2011 Kevin MacLeod
 * 					Delay Rock, (с)2005 Kevin MacLeod
 *
 * https://www.youtube.com/@VadRov
 * https://dzen.ru/vadrov
 * https://vk.com/vadrov
 * https://t.me/vadrov_channel
 *---------------------------------------------------------------------------------------------*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include <xpt2046.h>  //Драйвер для сенсора на контроллере XPT2046 (HR2046 и т.п.)
#include "ili9341.h"  //Драйвер для дисплея на контроллере ILI9341
#include "pcm5102.h"  //Драйвер аудио ЦАПа pcm5102
#include "display.h"  //Библиотека управления дисплеями по spi
#include "player.h"   //Библиотека для воспроизведения wav и mp3 файлов, хранящихся во флеш памяти микроконтроллера
#include "mus_data.h" //Музыкальные данные игры (музыка и эффекты)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint32_t millis = 0;	//Cчетчик миллисекунд
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define COMPUTER	1	//ID номер ИИ на поле
#define HUMAN		2	//ID номер человека на поле
#define MAXIMIZE	1	//Определение критерия максимизации
#define MINIMIZE	2	//определение критерия минимизации

/*
 * Сканирует игровое поле field размером field_size. Вызывается после хода игрока gamer.
 * В случае победы игрока gamer возвращает 1.
 * В случае ничьей (параметр gamer при этом, естественно, игнорируется) возвращает 2. (При этом
 * подразумевается, что проверка осуществляется сразу после хода игрока gamer, иначе есть шанс "прозевать" победу противника!)
 * Ничья, разумеется, означает, что свободных клеток нет.
 * В ином случае возвращает 0.
 * В переменных row и col возвращает номер строки, колонки либо диагонали, в которых
 * зафиксирован победный ряд:
 * row!=0, а col==0, то в row - номер горизонтального ряда (1...size);
 * row==0, а col!=0, то в col - номер вертикального ряда (1...size);
 * row==1 и col==1 - диагональный ряд, проходящий через левый верхний и правый нижний углы поля;
 * row==2 и col==2 - диагональный ряд, проходящий через левый нижний и правый верхний углы поля;
 * row==0 и col==0 - нет линии
 * В переменной sub_scores возвращает дополнительные очки за заполнение ряда игроком
 * (горизонтального/вертикального/диагонального) из расчета 1 балл за 1 клетку в формирующемся (сформированном) ряду.
 * (используется, в т.ч. при выборе хода в алгоритме Минимакс). При этом, если в ряду есть клетка противника,
 * то баллы не начисляются (0 очков).
 */
static uint8_t ScanField(uint8_t *field, uint8_t size, uint8_t gamer, uint8_t* row, uint8_t* col, uint8_t *sub_scores)
{
	*row = *col = 0;
	uint8_t i, j, flag, flag_d1 = 0, flag_d2 = 0, f = 0;
	int scores, cell_counter;
	int scores_d1 = 1, cell_d1_counter = size;
	int scores_d2 = 1, cell_d2_counter = size;
	*sub_scores = 0;
	//Поиск по строкам, столбцам и диагоналям
	for (i = 0; i < size; i++) {
		//Строки - горизонтальные ряды
		for (j = 0, flag = 0, scores = 1, cell_counter = size; j < size; j++) {
			if (field[i * size + j] != gamer) {
				flag = 1;
				if (!field[i * size + j]) {	cell_counter--;	}
				else { scores = 0; }
			}
		}
		scores *= cell_counter;
		*sub_scores = scores > *sub_scores ? scores : *sub_scores;
		if (!flag) { *row = i + 1; return 1; } //Есть горизонтальный ряд
		//Столбцы - вертикальные ряды
		for (j = 0, flag = 0, scores = 1, cell_counter = size; j < size; j++) {
			if (field[j * size + i] != gamer) {
				flag = 1;
				if (!field[j * size + i]) { cell_counter--; }
				else { scores = 0; }
			}
		}
		scores *= cell_counter;
		*sub_scores = scores > *sub_scores ? scores : *sub_scores;
		if (!flag) { *col = i + 1; return 1; } //Есть вертикальный ряд
		if (field[i * size + i] != gamer) { //Проверка диагонали 1
			flag_d1 = 1;
			if (!field[i * size + i]) { cell_d1_counter--; }
			else { scores_d1 = 0; }
		}
		if (field[(size - 1 - i) * size + i] != gamer) { //Проверка диагонали 2
			flag_d2 = 1;
			if (!field[(size - 1 - i) * size + i]) { cell_d2_counter--; }
			else { scores_d2 = 0; }
		}
		for (j = 0; j < size; j++) { //Проверка на ничью (наличие свободных клеток)
			if (!field[i * size + j]) f = 1;
		}
	}
	scores_d1 *= cell_d1_counter;
	scores_d2 *= cell_d2_counter;
	*sub_scores = scores_d1 > *sub_scores ? scores_d1 : *sub_scores; //Переопределение
	*sub_scores = scores_d2 > *sub_scores ? scores_d2 : *sub_scores; //максимума
	if (!flag_d1) { *col = 1; *row = 1; return 1; } //Есть диагональный ряд 1
	if (!flag_d2) { *col = 2; *row = 2; return 1; } //Есть диагональный ряд 2
	//Ничья (при условии, что проверка осуществляется после хода игрока gamer!)
	if (!f) return 2;
	return 0;
}

/*
 * Градиент (переход цвета)
 */
static uint32_t GradientColor(uint32_t color1, uint32_t color2, uint8_t a)
{
	//Получаем составляющие (r, g, b) цвета color1
	uint8_t r_col1 = (color1 >> 16) & 0xff;
	uint8_t g_col1 = (color1 >> 8) & 0xff;
	uint8_t b_col1 = color1 & 0xff;

	//Получаем составляющие (r, g, b) цвета color2
	uint8_t r_col2 = (color2 >>16) & 0xff;
	uint8_t g_col2 = (color2 >> 8) & 0xff;
	uint8_t b_col2 = color2 & 0xff;

	//"Смешиваем" составляющие (r, g, b) цветов color1 и color2
	r_col1 -= (a * (r_col1 - r_col2)) / 255;
	g_col1 -= (a * (g_col1 - g_col2)) / 255;
	b_col1 -= (a * (b_col1 - b_col2)) / 255;

	//"Собираем" составляющие
	return (((uint32_t)r_col1) << 16) | (((uint32_t)g_col1) << 8) | ((uint32_t)b_col1);
}

/*
 * Вывод масштабированного символа sym в рамке в позиции (x, y), цветом txcolor, цветом окружения bgcolor, цветом рамки brdcolor
 * p1 и p2 - параметры, определяющие габариты символа (ширина = высоте) в виде дроби (для целочисленных вычислений).
 * Например, ширина экрана 240 пикселей, а нам надо вывести символ шириной 60 пикселей, значит: p1 = 240, а p2 = 4
 * Если взглянуть на определение pixel_size и дальнейшее его использование, то станет понятно,
 * почему ширина символа задается дробью.
 */
static void DrawSym(LCD_Handler *lcd, char sym, uint16_t x, uint16_t y, uint32_t txcolor, uint32_t bgcolor, uint32_t brdcolor, int p1, int p2)
{
	#define pixel_size 	p1 / (p2 * f_h)
	uint32_t color, tmp = 0;
	FontDef *font = &Font_12x20; //Шрифт, из которого будем формировать масштабированный символ
	const uint8_t *b = font->data;
	int i, j, k, f_h = font->height;
	int bytes_per_line = ((font->width-1) >> 3) + 1; //Количество байт данных, приходящихся на горизонтальную линию шрифта
	k = 1 << ((bytes_per_line << 3) - 1); //Битовая маска для побитного сканирования горизонтальной линии шрифта
	k <<= (font->height - font->width) / 2; //Выравниваем битовую маску с учетом разницы высоты и ширины шрифта, приводя символ к "квадратному"
	if (!sym) sym = ' ';	//Если ID равен 0, то символ = пробелу (код 32)
	else if (sym == COMPUTER) sym = 'X'; //Если ID компьютера, то символ 'Х'
	else if (sym == HUMAN)    sym = 'O'; //Если ID человека, то символ 'O'
	sym = sym < font->firstcode || sym > font->lastcode ? 0: sym - font->firstcode; //Вычисляем
	b +=  sym * bytes_per_line * f_h;		//начальный адрес символа в блоке данных
	for (i = 0; i < f_h; i++) {	//Цикл по высоте шрифта
		if (bytes_per_line == 1) { tmp = *((uint8_t*)b); }     //Считываем данные строки шрифта
		else if (bytes_per_line == 2) { tmp = *((uint16_t*)b); }
		else if (bytes_per_line == 3) { tmp = *((uint8_t*)b); tmp += (*((uint16_t*)(b+1))) >> 8; }
		else if (bytes_per_line == 4) { tmp = *((uint32_t*)b); }
		b += bytes_per_line; //Раccчитываем адрес следующей строки символа
		for (j = 0; j < f_h; j++) { //Цикл по ширине символа (символ "квадратный", т.к. уравниваем его высоту и ширину)
			if (!j || j == (f_h - 1) || !i || i == (f_h - 1)) { //Рамку вокруг символа рисуем для 1 и последней
																//его строки, а также для первой и последней
																//его колонки. Цвет пикселя - brdcolor с градиентным переходом
				color = GradientColor(brdcolor, COLOR_WHITE, 255 * (i * f_h + j) / (f_h * f_h));
			}
			else { //символ на заданном фоне
				color = (tmp << j) & k ? txcolor: bgcolor;	//Если текущий бит данных горизонтальной строки шрифта установлен,
															//то цвет пикселя txcolor, в противном случае цвет bgcolor
			}
			LCD_FillWindow(	lcd,  							//Отрисовываем масштабированный "пиксель"
							x + j * pixel_size,
							y + i * pixel_size,
							x + (j + 1) * pixel_size - 1,
							y + (i + 1) * pixel_size - 1,
							color);
		}
	}
}

/*
 * Вывод на дисплей lcd игрового поля field размером field_size
 */
static void ShowGameField(LCD_Handler *lcd, uint8_t *field, uint8_t field_size)
{
	uint32_t txcolor_x = COLOR_RED; 	//Цвет "крестиков"
	uint32_t txcolor_0 = COLOR_WHITE; 	//Цвет "ноликов"
	uint32_t bgcolor = COLOR_BLUE; 		//Цвет фона клетки
	uint32_t brdcolor = COLOR_YELLOW; 	//Цвет рамки вокруг клетки
	uint32_t txcolor_text = COLOR_WHITE;//Цвет текста (если печатаем символы вместо "крестиков" и "ноликов", для заставки)
	uint32_t txcolor = 0;
	uint16_t x0, y0; //Координаты левого верхнего угла клетки на дисплее
	//Габариты поля берем по ширине либо высоте экрана, в зависимости от того, что меньше
	int size = lcd->Height > lcd->Width ? lcd->Width: lcd->Height;
	int i, j;
	uint8_t sym;
	for (i = 0; i < field_size; i++) { 		//Цикл по количеству рядов клеток
		y0 = i * size/field_size;			//Координата Y верхнего левого угла клетки на дисплее
		for (j = 0; j < field_size; j++)  {	//Цикл по количеству столбцов клеток
			x0 = j * size/field_size;		//Координата X верхнего левого угла клетки на дисплее
			sym = field[i * field_size + j]; //ID - содержимое клетки
			if (sym == COMPUTER)   { txcolor = txcolor_x; } //Если ID компьютера, то символ 'Х'
			else if (sym == HUMAN) { txcolor = txcolor_0; } //Если ID человека, то символ 'O'
			else { txcolor = txcolor_text; } //Иначе, имеем дело с текстом и код символа не меняем
			DrawSym(lcd, sym, x0, y0, txcolor, bgcolor, brdcolor, size, field_size); //Выводим символ на дисплей
		}
	}
}

/*
 * Игровая логика. Выбор хода ИИ
 * Возвращает очки выбранного хода. В переменных row и col - позицию (строка, столбец) выбранного хода.
 * На входе: игровое поле field и его размер field_size, ID игрока gamer, критерий алгоритма Минимакс maxmin,
 * указатели на переменные row и col, в которых возвратятся индексы выбранной для хода клетки,
 * а также текущая глубина рекурсии depth
 */
static int GameLogic(uint8_t *field, uint8_t field_size, uint8_t gamer, uint8_t maxmin, uint8_t *row, uint8_t *col, uint8_t depth)
{
	typedef struct { //Объявляем структуру, описывающую ход
		uint8_t i, j;			//Позиция шага на игровом поле
		int score;				//Очки шага (результативность хода)
		uint8_t sub_score;		//Дополнительные очки шага (учитываются в случае равенства очков за основные шаги)
	} GAME_STEP;
	int score, sub_score, m, k;
	uint8_t row1, col1, i, j, f;
	if (!depth) return 0; //Если достигнут предел глубины рекурсии, то выходим, возвратив 0 очков за ход
	int max_step = 0;
	for (m = 0; m < field_size * field_size; m++) {
		if (!field[m]) max_step++;
	}
	if (!max_step) { //Игровых ситуаций нет, т.к. нет свободных ячеек: некуда ходить -> ничья
		*row = *col = field_size; //Возвращаем заведомо некорректные индексы клетки для хода
		return 0;
	}
	GAME_STEP logic_mas[max_step]; //Массив игровых ходов для текущего игрового поля
	uint8_t field_copy[field_size * field_size]; //Массив для копии игрового поля
	//Для каждой свободной клетки игрового поля создадим игровую ситуацию GAME_STEP, которая будет содержать
	//копию текущего игрового поля, но с ходом игрока gamer в эту свободную клетку, индексы этой клетки на поле (параметры хода),
	//основные и дополнительные очки за ход (результативность хода). Считать игровые ситуации будем в переменной k
	for (i = 0, k = 0; i < field_size; i++)	{ //цикл по количеству строк в игровом поле
		for (j = 0; j < field_size; j++) {	  //Цикл по количеству столбцов в игровом поле
			if (!field[i * field_size + j]) { //Если клетка в позиции [i, j] игрового поля свободна:
				for (m = 0; m < field_size * field_size; m++) { //Делаем копию текущего игрового поля
					field_copy[m] = field[m];					//для k-той игровой ситуации
				}
				field_copy[i * field_size + j] = gamer;	//В копию поля для k-той игровой ситуации
				logic_mas[k].i = i;						//делаем ход (записываем переменную gamer) игроком gamer
				logic_mas[k].j = j;						//и запоминаем номера строки и столбца хода.
				logic_mas[k].score = logic_mas[k].sub_score = 0; //Инициализируем очки хода
				row1 = col1 = 0;
				//Сканируем игровое поле k-той игровой ситуации, оценивая результативность предполагаемого хода
				f = ScanField(field_copy, field_size, gamer, &row1, &col1, &logic_mas[k].sub_score);
				if (f) {			//Шаг привел к победе или ничьей?
					if (f == 1) {   //Победа
						logic_mas[k].score = (maxmin == MAXIMIZE) ? 1 : -1; //очки за победу
					}
					else {			//Ничья
						logic_mas[k].score = (maxmin == MAXIMIZE) ? 0 : 0;	//очки за ничью
					}
				}
				else { //Если ход не привел ни к победе, ни к ничьей, то анализируем все возможные ответные ходы противника
					   //на ход игрока gamer. Осуществим, т.н., рекурсивный вызов функции по отношению к противнику.
					   //Проанализируем все возможные ответные ходы противника на наш ход, но критерий алгоритма Минимакс
					   //изменяем на противоположный, а саму функцию вызываем для противника gamer. В качестве поля для анализа
					   //передаем копию поля с предполагаемым шагом игрока gamer
					logic_mas[k].score = GameLogic(field_copy, field_size, (gamer == HUMAN)? COMPUTER: HUMAN, (maxmin == MAXIMIZE)? MINIMIZE : MAXIMIZE, row, col, depth - 1);
				}
				k++; //Увеличиваем счетчик шагов
			}
		}
	}
	//Сравниваем очки итогов игровых ситуаций с учетом критерия maxmin.
	//Выбираем из всех ходов оптимальный с позиций оценки.
	i = j = 0; //Инициализация индекса игрового хода
	score = logic_mas[0].score; //Инициализация экстремума (min/max) очков за ход
	sub_score = logic_mas[0].sub_score; //Инициализация max дополнительных очков за ход
	while (k--) {
		if (maxmin == MAXIMIZE) { //Критерий алгоритма Минимакс - > максимизация шансов на победу игрока
			if (score <= logic_mas[i].score) {//Максимум меньше или равен очкам текущего хода?
				if (score < logic_mas[i].score) { score = logic_mas[i].score; j = i; } //Если меньше, то переназначаем максимум и запоминаем ход
				else { //Если равен, то сравниваем дополнительные очки хода и выбираем тот ход, у которого дополнительные очки больше
					if (sub_score < logic_mas[i].sub_score) { sub_score = logic_mas[i].sub_score; j = i; }
				}
			}
		}
		else { //Критерий алгоритма Минимакс -> минимизация шансов победы противника
			if (score >= logic_mas[i].score) {//Текущий минимум больше или равен?
				if (score > logic_mas[i].score) { score = logic_mas[i].score; j = i; } //Если меньше, то переназначаем минимум и запоминаем ход
				else { //Если равен, то сравниваем дополнительные очки хода и выбираем тот ход, у которого дополнительные очки больше
					if (sub_score < logic_mas[i].sub_score) { sub_score = logic_mas[i].sub_score; j = i; }
				}
			}
		}
		i++;	//переходим к следующему игровому шагу
	}	//Индексы клетки для лучшего хода с учетом результативности:
	*row = logic_mas[j].i;	//строка
	*col = logic_mas[j].j;	//столбец
	return score; 			//Возвращаем очки хода
}

/*
 * Выводит победный ряд игрового поля field размером field_size на дисплей lcd
 * i_start, j_start - индексы стартовой клетки победного ряда
 * i_step, j_step - приращения индексов для каждого последующего шага из field_size шагов (длина ряда всегда = field_size)
 */
static void DrawVictoryLine(LCD_Handler *lcd, uint8_t *field, uint8_t field_size, int i_start, int j_start, int i_step, int j_step)
{
	uint32_t txcolor = 0x1E90FF; //Цвет символов X и O
	uint32_t bgcolor = 0xFF1493; //Цвет фона
	uint32_t brdcolor = 0xCD853F;//Цвет рамки вокруг символов
	int x, y;
	char sym;
	int size = lcd->Height > lcd->Width ? lcd->Width: lcd->Height; //Ширина поля вывода в пикселях
	for (int i = 0; i < field_size; i++) { //Цикл по длине ряда
		x = j_start * size/field_size; //Преобразование индексов поля
		y = i_start * size/field_size; //в координаты дисплея
		sym = field[i_start * field_size + j_start];
		DrawSym(lcd, sym, x, y, txcolor, bgcolor, brdcolor, size, field_size); //Вывод символа на игровое поле
		i_start += i_step; //Расчет индексов следующей клетки победного ряда
		j_start += j_step; //с учетом приращения индексов для шага
	}
}

/*
 * Ищет вариант для случайного хода. В row и col возвращает индексы клетки игрового поля для хода
 * Если свободных клеток нет, то возвращает в row и col значение field_size - размер поля,
 * т.е. недопустимые индексы для хода
 */
static void RandMove(uint8_t *field, uint8_t field_size, uint8_t *row, uint8_t *col)
{
	int i, j;
	//Проверка возможности хода
	for (i = 0, j = 0; i < field_size * field_size; i++) { //Перебираем клетки поля
		if (!field[i]) { j = 1; break; } //Свободная клетка? Да, выставляем флаг и выходим из цикла
	}
	if (j == 0) { //Некуда ходить ?
		*row = *col = field_size; //Ходить некуда. Устанавливаем недопустимые индексы клетки и выходим
		return;
	}
	while (1) { //Крутим цикл до тех пор, пока не найдем вариант для хода:
		*row = (uint8_t)rand();
		*col = (uint8_t)rand();
		if (*row < field_size && *col < field_size) { //Индексы row и col должны быть в пределах размеров поля,
			if (!field[*row * field_size + *col]) return; //а клетка должна быть свободной
		}
	}
}

/*
 * "Растворяет" изображение на дисплее lcd
 * var - определяет вариант "растворения":
 * 0 - "растворяет" изображение "попиксельно" в случайном порядке пикселей;
 * !=0 - на первом шаге изображение стирается через строку, а на втором шаге через столбец.
 * color - "растворяющий" цвет
 */
static void ScreenDissolve(LCD_Handler *lcd, uint8_t var, uint32_t color)
{
	uint16_t x, y;
	if (!var) { //Первый вариант - попиксельно в случайном порядке
		for (int i = 0; i < lcd->Width * lcd->Height; i++ ) { //Цикл по количеству пикселей на дисплее
			x = lcd->Width;
			y = lcd->Height;
			while (x >= lcd->Width) { //Получаем случайную координату x
				x = lcd->Width * ((uint16_t)rand()) / 65535;
			}
			while (y >= lcd->Height) { //Получаем случайную координату y
				y = lcd->Height * ((uint16_t)rand()) / 65535;
			}
			LCD_FillWindow(lcd, x, y, x + 1, y + 1, color); //Рисуем "увеличенный" (2x2) пиксель заданным стирающим цветом color
		}
	}
	else { //Второй вариант - по линиям через строку и через столбец
		for (y = 0; y < lcd->Height; y += 2) { //Цикл по числу строк, с шагом через строку
			LCD_DrawLine(lcd, 0, y, lcd->Width - 1, y, color); //Рисуем горизонтальную линию
			LL_mDelay(1);	//Задержка в 1 мс
		}
		for (x = 0; x < lcd->Width; x += 2) { //Цикл по числу столбцов, с шагом через столбец
			LCD_DrawLine(lcd, x, 0, x, lcd->Height - 1, color); //Рисуем вертикальную линию
			LL_mDelay(1); //Задержка в 1 мс
		}
	}
	LCD_Fill(lcd, color); //Окончательно очищаем экран
}

/*
 * Структура с информацией о строках поля, которые необходимо выделить
 */
typedef struct {
	uint8_t *row_num; //Указатель на массив с номерами строк
	uint8_t all_rows; //Количество строк в массиве
} Menu_Row_Sel;

/*
 * Работа с меню
 * lcd - указатель на обработчик дисплея
 * t - указатель на обработчик тачскрина
 * field - указатель на поле
 * field_size - размерность поля
 * Пункты меню задаются интервалом строк игрового поля:
 * row_0 - стартовый номер строки поля - первый пункт меню (параметр "от")
 * row_1 - кочечный номер строки поля - последний пункт меню (параметр "до")
 * sel - указатель на структуру с информацией о строках поля, подлежащих выделению
 * Возвращает порядковый номер выбранного пункта меню (от 0)
 *
 * В передаваемом поле field заданной размерности field_size содержится построчно информация о меню (наименование,
 * прилашение к действию) и сами пункты меню. Пункты меню в поле задаются номерами строк от row_0 до row_1.
 * Строки поля, которые необходимо выделить задаются структурой sel, содержащей массив строк для выделения
 * и их (строк) количество. Обычно выделяют строки с наименованием меню (например, "Сложность игры")
 * и приглашением к действию (например, "выбирай")
 */
static int SelectMenu(LCD_Handler *lcd, XPT2046_Handler *t, uint8_t *field, uint8_t field_size, int row_0, int row_1, Menu_Row_Sel *sel)
{
	ShowGameField(lcd, field, field_size); //Отображение меню выбора размера игрового поля
	int i = sel->all_rows;
	while (i) { //Выделяем требуемые строки
		DrawVictoryLine(lcd, field, field_size, sel->row_num[sel->all_rows - i], 0, 0, 1);
		i--;
	}
	int size = lcd->Height > lcd->Width ? lcd->Width: lcd->Height; //Размер игрового поля в пикселях
	while(1) { //Ожидание касания тачскрина и обработка результатов
		uint32_t tick = millis, k_tick = 1; //Инициализация счетчика миллисекунд и коэффициента интервала ожидания
		while (!t->click) {  //Ждем касания тачскрина
			if (millis - tick >= k_tick * 10000) {  //Звуковое оповещение игрока первый раз через 10 секунд, а затем с интервалом
				tick = millis;						//через 20, 30, 40 ... 120 секунд игрока о том, что сейчас его ход.
				k_tick++;
				if (k_tick > 12) k_tick = 1;		//Пересчет коэффициента интервала ожидания k_tick
				(void)PlayerAdd(&WAV_you_move, 0); 	//Звуковое сообщение "Твой ход"
			}
			__NOP();
		}
		while (t->click) {					//До тех пор, пока есть касание
			if (!t->fl_interrupt) {			//Если обновление координат тачскрина в прерывании запрещено,
				(void)XPT2046_GetTouch(t);	//то считывает здесь координаты тачскрина
			}
			__NOP();
		}
		if (t->last_click_time > 3) { //Исключаем случайные/короткие касания тача, ограничивая длительность клика
			int i;
			tPoint pos;
			XPT2046_ConvertPoint(&pos, &t->first_click_point, &t->coef); //Конвертируем координаты тачскрина в дисплейные
			i = pos.y / (size / 8); //Рассчитываем номер строки по координате y дисплея
			if (i >= row_0 && i <= row_1) {
				(void)PlayerAdd(&WAV_drum, 0); //Звук хода
				DrawVictoryLine(lcd, field, field_size, i, 0, 0, 1); //Выделяем текст выбранной строки
				LL_mDelay(500); //Пауза 0.5 сек.
				return i - row_0;
			}
		}
	}
}

/*
 * Меню выбора размерности игрового поля
 * Возвращает выбранную размерность игрового поля
 */
static int SelectFieldSize (LCD_Handler *lcd, XPT2046_Handler *t)
{
	//Игровое поле 8x8, на котором изображено меню игры. Коды символов в поле представлены в кодировке cp1251
	const uint8_t field[8*8] = { 0, 208, 224, 231, 236, 229, 240, 0,	// Размер
								 0, 0, 239, 238, 235, 255, 0, 0,		//  поля
								 0, 0, '3', 'x', '3', 0, 0, 0,			//  3x3
								 0, 0, '4', 'x', '4', 0, 0, 0,			//  4x4
								 0, 0, '5', 'x', '5', 0, 0, 0,			//  5x5
								 0, 0, '6', 'x', '6', 0, 0, 0,			//  6x6
								 0, 0, '7', 'x', '7', 0, 0, 0,			//  7x7
								 0, 226, 251, 225, 229, 240, 232, 0 }; // выбери

	uint8_t rows[] = {0, 1, 7}; //Порядковые номера строк поля, которые необходимо выделить
	Menu_Row_Sel sel = { .row_num = rows,
						 .all_rows = 3    };

	return SelectMenu(lcd, t, (uint8_t*)field, 8, 2, 6, &sel) + 3; //Вызываем процедуру работы с меню
}

/*
 * Меню выбора сложности игры
 * Возвращает глубину рекурсии для алгоритма минимакс в зависимости от размера поля field_size
 * и выбранного уровня сложности
 */
static int SelectDifficulty (LCD_Handler *lcd, XPT2046_Handler *t, uint8_t field_size)
{
	//Игровое поле 9x9, на котором изображено меню игры. Коды символов в поле представлены в кодировке cp1251
	const uint8_t field[9*9] = { 209, 235, 238, 230, 237, 238, 241, 242, 252,	//Сложность
								 0, 0, 232, 227, 240, 251, 0, 0, 0,				//  игры
								 0, 0, 0, 0, 0, 0, 0, 0, 0,						//
								 0, 0, 235, 229, 227, 234, 238, 0, 0,			//  легко
								 0, 0, 241, 240, 229, 228, 237, 229, 0,			//  средне
								 0, 0, 241, 235, 238, 230, 237, 238, 0,			//  сложно
								 0, 0, 0, 0, 0, 0, 0, 0, 0,						//
								 0, 0, 0, 0, 0, 0, 0, 0, 0, 					//
								 0, 226, 251, 225, 232, 240, 224, 233, 0 };  	// выбирай

	uint8_t rows[] = {0, 1, 8}; //Порядковые номера строк поля, которые необходимо выделить
	Menu_Row_Sel sel = { .row_num = rows,
						 .all_rows = 3    };

	int dfclt = SelectMenu(lcd, t, (uint8_t*)field, 9, 3, 5, &sel); //Вызываем процедуру работы с меню
	if (!dfclt) return 2; //Уровень сложности "Легко"
	if (dfclt == 1) { 	  //Уровень сложности "Средне"
		if (field_size == 3) return 4;
		return 2;
	}
	//Уровень сложности "Сложно"
	if (field_size == 3) return 6;
	return 4;
}

/*
 * Демка крестики-нолики
 */
void TicTacToe_DemoGame(LCD_Handler *lcd, XPT2046_Handler *t)
{
	uint32_t COLOR_DEFINE = COLOR_YELLOW; //Цвет по умолчанию
	//Игровое поле 8x8, на котором изображена заставка игры. Коды символов в поле представлены в кодировке cp1251
	//(а файлы этого проекта в кодировке UTF-8).
	const uint8_t field_demo[8*8] = { 0, 0, 0, 0, 0, 0, 0, 0,
									  0, 0, 200, 227, 240, 224, 0, 0, 			//  Игра
									  0, 0, 0, 0, 0, 0, 0, 0,
									  202, 208, 197, 209, 210, 200, 202, 200, 	//КРЕСТИКИ
									  0, 205, 206, 203, 200, 202, 200, 0,  		// НОЛИКИ
									  0, 0, 0, 0, 0, 0, 0, 0,
									  0, 0, 0, 0, 0, 0, 0, 0,
									  0, 'V', 'a', 'd', 'R','o', 'v', 0,	};	// VadRov
	uint8_t row, col, row1, col1, sub_score;
	//----------------------------- Строки с игровой информацией ----------------------------------
	const char str0[] = "Tic-Tac-Toe  (c)2022 VadRov"; 	//Название игры и кто запрограммировал
	const char str1[] = "Round: ";					 	//Раунд, количество сыгранных игр и т.п.
	const char str2[] = "You:";						 	//Имя человека-игрока, противника ИИ "Ты"
	const char str3[] = " Stm:";					    //Имя ИИ, противника человека " Stm", с пробелом в первом символе для "склейки"
	const char str_humanwin[] = " You Win! ";			//Надпись: "Ты выиграл!"
	const char str_computerwin[] = " STM32F4 Win! ";	//Надпись: "STM32F4 выиграл!"
	const char str_nowin[] = " No Winner! ";			//Надпись: "Нет победителя!", т.е. ничья
	const char str_music[] = "Music: Kevin MacLeod"; 	//Автор музыки
	const char str_music_lic[] = "license CC BY 3.0";	//Лицензия на использование музыки
	char str_buf[50]; //Временный буфер для "склейки" текстовых строк
	char *str_winner = (char*)str_nowin;	//Указатель для информационных текстовых строк
	int size = lcd->Height > lcd->Width ? lcd->Width: lcd->Height; //Размер игрового поля в пикселях
	tPoint pos; //Координаты тачскрина, приведенные к дисплейным координатам
	int i, j;
	t->fl_interrupt = 1;	//Разрещаем обновление координат тачскрина в прерывании
	uint32_t score_human = 0, score_comp = 0, all_games = 0; 	//Инициализация очков и количества сыгранных игр
	uint8_t random_move; //Счетчик случайных шагов
	for (i = 0; i < millis; i++) {	//Инициализация генератора случайных чисел
		(void)rand();				//millis - системное время в миллисекундах,
	}								//отчитывамое с момента старта микроконтроллера
	uint8_t fl_new_game; //флаг "новая игра"
	uint8_t field_size, max_depth;
	uint8_t *field = 0; //Игровое поле
Game_option:
	PlayerDelAll(); //Останавливаем воспроизведение всех плееров и удаляем их
	LCD_Fill(lcd, COLOR_DEFINE); //Заливаем дисплей цветом по умолчанию
	(void)PlayerAdd(&Mp3_intro, -1); //Включаем воспроизведение интро
	ShowGameField(lcd, (uint8_t*)field_demo, 8); //Отображение заставки (игровое поле 8x8 с надписями)
	LCD_WriteString(lcd, 		//Отображение строки с автором музыки
					(size - Font_12x20.width * strlen(str_music)) / 2,
					size + 22,
					str_music,
					&Font_12x20,
					COLOR_RED,
					COLOR_DEFINE,
					LCD_SYMBOL_PRINT_FAST);
	LCD_WriteString(lcd, 		//Отображение строки с лицензией на использование музыки
					(size - Font_8x13.width * strlen(str_music_lic)) / 2,
					size + 43,
					str_music_lic,
					&Font_8x13,
					COLOR_BLUE,
					COLOR_DEFINE,
					LCD_SYMBOL_PRINT_FAST);
	LL_mDelay(5000); //Задержка 5 секунд
	DrawVictoryLine(lcd, (uint8_t*)field_demo, 8, 3, 0, 0, 1); //Выделяем
	DrawVictoryLine(lcd, (uint8_t*)field_demo, 8, 4, 0, 0, 1); //название игры
	LL_mDelay(2000); //Задержка 2 секунды
	field_size = (uint8_t)SelectFieldSize(lcd, t); //Выбор размера игрового поля
	max_depth = (uint8_t)SelectDifficulty(lcd, t, field_size); //Выбор сложности игры
	if (field) free(field);
	field = (uint8_t*)malloc(field_size * field_size);
	while (1) {
		PlayerDelAll(); //Останавливаем и удаляем все плееры
		(void)PlayerAdd(&Mp3_tema, -1); //Включаем воспроизведение музыкальной темы игры
		ScreenDissolve(lcd, 0, COLOR_DEFINE); //"Растворяем" экран
		all_games++; //Инкремент количества сыгранных игр
		fl_new_game = 0; //Сбрасываем флаг "новая игра"
		memset(field, 0, field_size * field_size); //Инициализация игрового поля
		ShowGameField(lcd, field, field_size); //Вывод на дисплей игрового поля
		//Вывод названия игры, номера раунда и количества очков игроков
		strcpy(str_buf, str1);							//Склеиваем строку с номером раунда игры
		utoa(all_games, &str_buf[strlen(str1)], 10);	//и выводим информацию на дисплей
		LCD_WriteString(lcd,
						(size - Font_12x20.width * strlen(str_buf)) / 2,
						size + 2,
						str_buf,
						&Font_12x20,
						COLOR_RED,
						COLOR_DEFINE,
						LCD_SYMBOL_PRINT_FAST);
		LCD_WriteString(lcd,	//Выводим "иконку" для выхода в меню опций игры
						0,
						size + 2,
						"MENU",
						&Font_12x20,
						COLOR_DEFINE,
						COLOR_RED,
						LCD_SYMBOL_PRINT_FAST);
		strcpy(str_buf, str2);								//Склеиваем строку с информацией
		utoa(score_human, &str_buf[strlen(str_buf)], 10);	//об очках игроков, заработанных в игре
		strcat(str_buf, str3);								//и выводим информацию на дисплей
		utoa(score_comp, &str_buf[strlen(str_buf)], 10);
		LCD_WriteString(lcd,
						(size - Font_12x20.width * strlen(str_buf)) / 2,
						size + 22,
						str_buf,
						&Font_12x20,
						COLOR_BLACK,
						COLOR_DEFINE,
						LCD_SYMBOL_PRINT_FAST);
		LCD_WriteString(lcd, 		//Выводим информацию о названии игры и кто ее запрограммировал
						(size - Font_8x13.width * strlen(str0)) / 2,
						size + 42,
						str0,
						&Font_8x13,
						COLOR_BLUE,
						COLOR_DEFINE,
						LCD_SYMBOL_PRINT_FAST);
		LCD_WriteString(lcd,		//Вывод строки с автором музыки
						(size - Font_8x13.width * strlen(str_music)) / 2,
						size + 55,
						str_music,
						&Font_8x13,
						COLOR_BLUE,
						COLOR_DEFINE,
						LCD_SYMBOL_PRINT_FAST);
		random_move = 1; //Инициализация счетчика случайных ходов ИИ
		while (!fl_new_game)  {	//Крутимся в цикле пока не установится флаг fl_new_game (новая игра)
			/*----------------------------------- ХОД искусственного интеллекта --------------------------------*/
			row = col = row1 = col1 = sub_score = 0;
			if(random_move) { //Случайный ход ИИ ("на дурака", "пальцем в небо" и т.п.), если в счетчике не 0
				RandMove(field, field_size, &row, &col); //Получаем позицию клетки для случайного хода
				random_move--; //Уменьшаем счетчик случайных шагов
			}
			else { //"Обдуманный" ход ИИ с применением алгоритма минимакс
				GameLogic(field, field_size, COMPUTER, MAXIMIZE, &row, &col, max_depth);
			}
			if (row != field_size && col != field_size) { //Если ход возможен (есть свободные клетки),
				if (!field[row * field_size + col]) {	  //то записываем в клетку поля ID ИИ
					field[row * field_size + col] = COMPUTER; //и перересовываем игровое поле
					(void)PlayerAdd(&WAV_drum, 0); //Звук хода
					for (uint16_t a = 0; a <= 255; a += 3) { //Всплывающий X
						DrawSym(lcd,
								COMPUTER,
								col * size / field_size,
								row * size / field_size,
								GradientColor(COLOR_WHITE, COLOR_RED, (uint8_t)a),
								COLOR_BLUE,
								COLOR_YELLOW,
								size,
								field_size);
					}
					ShowGameField(lcd, field, field_size);
				}
			}
			else { //Если же все клетки заняты, т.е. возможности хода нет:
				str_winner = (char*)str_nowin; //Присваиваем "информационному указателю" адрес строки с текстом "ничья"
				fl_new_game = 1; //Устанавливаем флаг "новая игра"
				score_comp += 5; //Начисляем игрокам по 5 балллов за ничью
				score_human += 5;
			}
			if (!fl_new_game) { //При сброшенном флаге "новая игра" проверяем есть ли победа или ничья после хода ИИ
				uint8_t f = ScanField(field, field_size, COMPUTER, &row1, &col1, &sub_score); //f = 1 - победа ИИ, f = 2 - ничья
				if (f) { //После хода ИИ есть либо победа, либо ничья
					fl_new_game = 1; //Устанавливаем флаг "новая игра"
					if (f == 1) { //ИИ победил
						str_winner = (char*)str_computerwin; //Присваиваем "информационному указателю" адрес строки с именем победителя
						score_comp += 10; //Начисляем очки игроку за "победу"
					}
					else { //Ничья (нет свободных клеток после хода ИИ и нет победного ряда)
						str_winner = (char*)str_nowin; //Присваиваем "информационному указателю" адрес строки с надписью "ничья"
						score_comp += 5; //начисляем очки игрокам за ничью
						score_human += 5;
					}
				}
			}
			/*----------------------------------------- ХОД человека ---------------------------------------*/
			while(!fl_new_game) { //При сброшенном флаге "новая игра" крутимся в цикле и опрашиваем тач
				uint32_t tick = millis, k_tick = 1;
				while (!t->click) {  		//Ждем касания тачскрина
					if (millis - tick >= k_tick * 10000) {  //Звуковое оповещение игрока первый раз через 10 секунд, а затем с интервалом
						tick = millis;						//через 20, 30, 40 ... 120 секунд о том, что сейчас его ход.
						k_tick++;							//Пересчет коэффициента
						if (k_tick > 12) k_tick = 1;		//интервала ожидания k_tick
						(void)PlayerAdd(&WAV_you_move, 0);	//Звуковое сообщение "Твой ход"
					}
					__NOP();
				}
				while (t->click) {					//До тех пор, пока есть касание тачскриина
					if (!t->fl_interrupt) {			//Если обновление координат тачскрина в прерывании запрещено,
						(void)XPT2046_GetTouch(t);	//то считывает здесь координаты тачскрина
					}
					__NOP();
				}
				if (t->last_click_time > 3) { //Исключаем случайные/короткие касания тача, ограничивая длительность клика
					XPT2046_ConvertPoint(&pos, &t->first_click_point, &t->coef); //Конвертируем координаты тачскрина в дисплейные
					if (pos.x >= 0 && pos.x <= 47 &&								  //Если был клик по "иконке" "MENU",
						pos.y >= size + 2 && pos.y <= size + 2 + 19) {				  //то переходим в меню опций игры
						goto Game_option;
					}
					i = pos.y / (size / field_size); //Рассчитываем по координатам дисплея
					j = pos.x / (size / field_size); //индексы клетки игрового поля
					//Индексы сравниваем с размером поля и проверяем, свободна ли выбранная клетка игрового поля
					if (i >= 0 && i < field_size && j >= 0 && j < field_size ) {//Если индексы "правильные",
						if (!field[i * field_size + j])  {     					//а клетка свободна, то
							field[i * field_size + j] = HUMAN; 					//записываем в ячейку поля ID человека и прерываем цикл опроса тача
							(void)PlayerAdd(&WAV_drum, 0); //Звук хода
							for (uint16_t a = 0; a <= 255; a += 3) { //Всплывающий O
								DrawSym(lcd,
										HUMAN,
										j * size / field_size,
										i * size / field_size,
										GradientColor(COLOR_LIGHTGREY, COLOR_WHITE, (uint8_t)a),
										COLOR_BLUE,
										COLOR_YELLOW,
										size,
										field_size);
							}
							break;
						}
					}
				}
			}
			if (!fl_new_game) { //При сброшенном флаге "новая игра"
				ShowGameField(lcd, field, field_size); //Перересовываем игровое поле и проверяем, есть ли победа человека после хода.
				if (ScanField(field, field_size, HUMAN, &row1, &col1, &sub_score) == 1) { //Если есть:
					str_winner = (char*)str_humanwin; //Присваиваем "информационному указателю" адрес строки с именем победителя
					fl_new_game = 1; //Устанавливаем флаг "новая игра"
					score_human += 10; //Начисляем очки игроку за "победу"
				}
			}

			/*--------------------------------------------- Оформление итогов раунда -----------------------------------------*/
			if (fl_new_game) {//При установленном флаге "новая игра" в случае победы ИИ или человека:
				PlayerDelAll(); //Останавливаем воспроизведение всех плееров и удаляем их
				(void)PlayerAdd(&Mp3_final, 0); //Включаем воспроизведение финальной композиции
				//Выделяем победный ряд:
				if (row1 && col1) { //по диагоналям \/
					if (row1 == 1 && col1 == 1) { DrawVictoryLine(lcd, field, field_size, 0, 0, 1, 1); }
					else { DrawVictoryLine(lcd, field, field_size, field_size - 1, 0, -1, 1); }
				}
				else if (row1) { //по строкам -
					DrawVictoryLine(lcd, field, field_size, row1 - 1, 0, 0, 1); }
				else if (col1){ //по столбцам |
					DrawVictoryLine(lcd, field, field_size, 0, col1 - 1, 1, 0);	}
					LCD_WriteString(lcd, //Вывод имени победителя или надписи "ничья" (в зависимости от исхода раунда)
									(size - strlen(str_winner) * Font_12x20.width)/2,
									size/2 - Font_12x20.height/2,
									str_winner,
									&Font_12x20,
									COLOR_YELLOW,
									COLOR_BLUE,
									LCD_SYMBOL_PRINT_FAST);
				LL_mDelay(8200); //Задержка в 8.2 секунды перед началом новой игры
			}
		}
	}
}

//Возвращает максимальный размер памяти, который может быть выделен 1 блоком
#define max_allocate_one_block	50000
static uint32_t max_memory()
{
	uint32_t i = max_allocate_one_block;
	uint8_t *mem = 0;
	while (i)
	{
		mem = (uint8_t*)calloc(i, sizeof(uint8_t));
		if (mem)
		{
			free(mem);
			*((uint32_t*)(mem - 4)) = 0;
			mem = 0;
			return i;
		}
		i--;
	}
	return 0;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
/* включаем кэширование инструкций */
#if (INSTRUCTION_CACHE_ENABLE != 0U)
	((FLASH_TypeDef *) ((0x40000000UL + 0x00020000UL) + 0x3C00UL))->ACR |= (0x1UL << (9U));
#endif

/* включаем кэширование данных */
#if (DATA_CACHE_ENABLE != 0U)
	((FLASH_TypeDef *) ((0x40000000UL + 0x00020000UL) + 0x3C00UL))->ACR |= (0x1UL << (10U));
#endif

/* включаем систему предварительной выборки инструкций*/
#if (PREFETCH_ENABLE != 0U)
	((FLASH_TypeDef *) ((0x40000000UL + 0x00020000UL) + 0x3C00UL))->ACR |= (0x1UL << (8U));
#endif
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  //настраиваем системный таймер (прерывания 1000 раз в секунду, т.е. с периодом 1 мс)
  SysTick_Config(SystemCoreClock/1000);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  /* ---------------- Инициализируем I2S на базе SPI3 для работы с аудио ЦАП PCM5102 -------------------*/
  I2S3_Init();
  /*----------------------------------------------------------------------------------------------------*/

  /* -------------------------------------- Настройка дисплея ------------------------------------------*/
  //Данные DMA
  LCD_DMA_TypeDef dma_tx = { .dma    = DMA2,				//Контроллер DMA
    		  	  	  	  	 .stream = LL_DMA_STREAM_3 };	//Поток контроллера  DMA

  //Данные для подсветки
  LCD_BackLight_data bkl_data = { .htim_bk 		   = TIM3,				 //Таймер - для подсветки с PWM (изменение яркости подсветки)
    		  	  	  	  	  	  .channel_htim_bk = LL_TIM_CHANNEL_CH1, //Канал таймера - для подсветки с PWM (изменение яркости подсветки)
								  .blk_port 	   = 0,					 //Порт gpio - подсветка по типу вкл./выкл.
								  .blk_pin 		   = 0,					 //Вывод порта - подсветка по типу вкл./выкл.
								  .bk_percent	   = 80  };				 //Яркость подсветки в %

  //Данные подключения
  LCD_SPI_Connected_data spi_con = { .spi 		 = SPI1,				//Используемый spi
    		  	  	  	  	  	  	 .dma_tx 	 = dma_tx,				//Данные DMA
									 .reset_port = LCD_RESET_GPIO_Port,	//Порт вывода RES
									 .reset_pin  = LCD_RESET_Pin,		//Пин вывода RES
									 .dc_port 	 = LCD_DC_GPIO_Port,	//Порт вывода DC
									 .dc_pin 	 = LCD_DC_Pin,			//Пин вывода DC
									 .cs_port	 = LCD_CS_GPIO_Port,	//Порт вывода CS
									 .cs_pin	 = LCD_CS_Pin         };//Пин вывода CS

#ifndef  LCD_DYNAMIC_MEM
  LCD_Handler lcd1;
#endif
   //Создаем обработчик дисплея на контроллере ILI9341
   LCD = LCD_DisplayAdd( LCD,
#ifndef  LCD_DYNAMIC_MEM
		   	   	   	   	 &lcd1,
#endif
		   	   	   	   	 240,
						 310,
						 ILI9341_CONTROLLER_WIDTH,
						 310,//ILI9341_CONTROLLER_HEIGHT,
						 PAGE_ORIENTATION_PORTRAIT_MIRROR,
						 ILI9341_Init,
						 ILI9341_SetWindow,
						 ILI9341_SleepIn,
						 ILI9341_SleepOut,
						 &spi_con,
						 LCD_DATA_16BIT_BUS,
						 bkl_data				   );

  LCD_Handler *lcd = LCD; //Указатель на первый дисплей в списке
  LCD_Init(lcd);
  LCD_Fill(lcd, COLOR_WHITE);

  (void)max_memory();
  /* --------------------------------------------------------------------------------------------------*/

  /* ------------------------------------ Запускаем "звуковую карту" ----------------------------------*/
  //Выделяем память для выходного PCM буфера
  mix_buffer = (uint32_t*)calloc(3 * MP3_SAMPLES, sizeof(uint32_t));
  //Стартуем i2s кодек PCM51202 на частоте 44100 в формате 16-битных сэмплов
  Start_DAC_DMA(SPI3, DMA1, 5, 44100, LL_I2S_DATAFORMAT_16B_EXTENDED, mix_buffer, 2 * MP3_SAMPLES);
  /* --------------------------------------------------------------------------------------------------*/

  /* ----------------------------------- Настройка тачскрина ------------------------------------------*/
  XPT2046_ConnectionData cnt_touch = { .spi 	 = SPI1,				//Используемый spi
		  	  	  	  	  	  	  	   .speed 	 = 5,					//Скорость spi 0...7 (0 - clk/2, 1 - clk/4, ..., 7 - clk/256)
									   .cs_port  = T_CS_GPIO_Port,		//Порт для управления T_CS
									   .cs_pin 	 = T_CS_Pin,			//Вывод порта для управления T_CS
									   .irq_port = T_IRQ_GPIO_Port,		//Порт для управления T_IRQ
									   .irq_pin  = T_IRQ_Pin,			//Вывод порта для управления T_IRQ
									   .exti_irq = T_IRQ_EXTI_IRQn };   //Канал внешнего прерывания

  //Инициализация обработчика тачскрина
  XPT2046_Handler touch1;
  XPT2046_InitTouch(&touch1, 20, &cnt_touch);
  //Калибровка тачскрина
  (void)XPT2046_CalibrateTouch(&touch1, lcd);
  /*------------------------------------------------------------------------------------------------------*/

  //Демо игра "Крестики-нолики"
 TicTacToe_DemoGame(lcd, &touch1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_HSE_EnableCSS();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 168, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_Init1msTick(84000000);
  LL_SetSystemCoreClock(84000000);
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**SPI1 GPIO Configuration
  PA5   ------> SPI1_SCK
  PA7   ------> SPI1_MOSI
  PB4   ------> SPI1_MISO
  */
  GPIO_InitStruct.Pin = LCD_SCK_Pin|LCD_SDI_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = T_OUT_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
  LL_GPIO_Init(T_OUT_GPIO_Port, &GPIO_InitStruct);

  /* SPI1 DMA Init */

  /* SPI1_TX Init */
  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_3, LL_DMA_CHANNEL_3);

  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_3, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_3, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA2, LL_DMA_STREAM_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_3, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_3, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_3, LL_DMA_PDATAALIGN_HALFWORD);

  LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_3, LL_DMA_MDATAALIGN_HALFWORD);

  LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_3);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_16BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 10;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  TIM_InitStruct.Prescaler = 999;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 139;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM3, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM3);
  LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH1);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = 104;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH1);
  LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM3);
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**TIM3 GPIO Configuration
  PA6   ------> TIM3_CH1
  */
  GPIO_InitStruct.Pin = LCD_LED_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(LCD_LED_GPIO_Port, &GPIO_InitStruct);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* Init with LL driver */
  /* DMA controller clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  NVIC_SetPriority(DMA2_Stream3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
  NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_SetOutputPin(GPIOA, LCD_DC_Pin|LCD_RESET_Pin|LCD_CS_Pin);

  /**/
  LL_GPIO_SetOutputPin(T_CS_GPIO_Port, T_CS_Pin);

  /**/
  GPIO_InitStruct.Pin = LCD_DC_Pin|LCD_RESET_Pin|LCD_CS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = T_CS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(T_CS_GPIO_Port, &GPIO_InitStruct);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE0);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  LL_GPIO_SetPinPull(T_IRQ_GPIO_Port, T_IRQ_Pin, LL_GPIO_PULL_UP);

  /**/
  LL_GPIO_SetPinMode(T_IRQ_GPIO_Port, T_IRQ_Pin, LL_GPIO_MODE_INPUT);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),4, 0));
  NVIC_EnableIRQ(EXTI0_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
