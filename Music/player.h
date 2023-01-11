/*
 *  Author: VadRov
 *  Copyright (C) 2019, VadRov, all right reserved.
 *
 *  ПЛЕЕРы
 *  Сильно урезано. Встроено только два кодека: WAV и MP3
 *  Кодекам "скармливать" чистые данные. Контейнеры (riff, id3, mp4) с тегами в этой урезанной версии не поддерживаются.
 *  Ограничения на входной поток: все муз. данные плееров могут храниться только во флеш памяти МК.
 *  Осуществляется микширование выходных данных нескольких плееров в один выходной PCM буфер.
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

#ifndef PLAYER_H_
#define PLAYER_H_

#include "main.h"
#include "mus_data.h"

#define MP3_SAMPLES	1152 //Количество сэмплов в mp3 фрейме (определяется из параметров кодека)

extern uint32_t *mix_buffer; //Выходной PCM буфер

//Статусы для функции инициализации плейера
typedef enum {
	PLAYER_INIT_OK = 0,
	PLAYER_INIT_ERROR
} PlayerInitStatus;

//Статусы для функции старта воспроизведения
typedef enum {
	PLAYER_START_OK = 0,
	PLAYER_START_ERROR
} PlayerStartPlayStatus;

//Статусы для функции обновления воспроизведения
typedef enum {
	PLAYER_UPDATE_OK = 0,
	PLAYER_UPDATE_END_OF_FILE,
	PLAYER_UPDATE_ERROR
} PlayerUpdateStatus;

//Статусы для функции остановки воспроизведения
typedef enum {
	PLAYER_STOP_OK = 0,
	PLAYER_STOP_ERROR
}PlayerStopStatus;

//Статусы текущего состояния плеера
typedef enum {
	PLAYER_INIT = 0,
	PLAYER_OPEN_FILE,
	PLAYER_READ_HEADER,
	PLAYER_START_PLAY,
	PLAYER_UPDATE_PLAY,
	PLAYER_STOP
} PlayerStatus;

typedef enum {
	PLAY_FILE_OK,
	PLAY_FILE_ALLOC_ERROR,
	PLAY_FILE_INIT_ERROR,
	PLAY_FILE_OPEN_ERROR,
	PLAY_FILE_HEADER_ERROR,
	PLAY_FILE_PICTURE_ERROR,
	PLAY_FILE_START_ERROR,
	PLAY_FILE_DAC_ERROR,
	PLAY_FILE_UPDATE_ERROR,
	PLAY_FILE_UNKNOW_FORMAT
} PlayerPlayFileStatus;

struct Player;

//Определение типов для функций "коллбэков"
typedef PlayerStartPlayStatus (*PlayerStartPlayCallback)(struct Player *player);
typedef PlayerUpdateStatus (*PlayerUpdateCallback)(struct Player *player);
typedef PlayerStopStatus (*PlayerStopCallback)(struct Player *player);

//Определение типа file_in_mem для файлов, хранящихся во флеш памяти микроконтроллера
typedef struct {
	uint8_t *buff;			//Указатель на стартовое местоположение
	uint8_t *cur_ptr;		//Указатель на текущий адрес данных - текущая позиция чтения
	uint32_t len;			//Длина данных
	uint32_t data_left;		//Сколько данных еще не прочитано из файла
} file_in_mem;

//Определение типа Player_client_data с клиентскими данными
typedef struct {
	file_in_mem *inbuffer;		//Входной поток
	void *client_data_decoder;  //Указатель на данные для конкретного пользовательского декодера
} Player_client_data;

//Структура с параметрами аудио
typedef struct {
uint32_t channels;		//Количество каналов (моно, стерео)
uint32_t bitpersample;	//Разрядность данных (8 бит, 16 бит, 24 бит, 32 бит)
uint32_t samplerate;	//Частота дискретизиции
uint32_t samples;		//Количество сэмплов во фрейме/кадре
} AudioConfig;

//Определение типа Player
typedef struct Player {
	AudioConfig audioCFG;						//Параметры аудио
	uint32_t *outbuf;							//Указатель на выходной PCM буфер. Данные в буфере 32-битные (word),
												//располагаются поканально: LLRR (по 2 байта на канал)
	PlayerStartPlayCallback start_callback;		//Указатель на функцию запуска воспроизведения
	PlayerUpdateCallback update_callback;		//Указатель на функцию обновления данных для воспроизведения
	PlayerStopCallback stop_callback;			//Указатель на функцию остановки воспроизведения
	PlayerStatus status;						//Текущий статус плеера (что сделал, т.е. последняя из выполненных операций:
												//инициализация, чтение заголовка, открытие файла, старт, обновление, стоп)
	void *client_data;							//Указатель на клиентские данные
	void *next;									//Указатель на следующий плеер
	void *prev;									//Указатель на предыдущий плеер
	int repeat;									//Определяет количество повторов воспроизведения для плеера
} Player;

extern Player *players; //Список плееров

//Инициализирует плеер для конкретного типа воспроизводимого файла
PlayerInitStatus PlayerInit(Player *player, PlayerStartPlayCallback start_callback,	PlayerUpdateCallback update_callback,
							PlayerStopCallback stop_callback, void *client_data);

//Стартует воспроизведение
PlayerStartPlayStatus PlayerStartPlay(Player *player);

//Обновляет данные для воспроизведения (декодирует фрейм/кадр)
PlayerUpdateStatus PlayerUpdate(Player *player);

//Останавливает воспроизведение
PlayerStopStatus PlayerStop(Player *player);

//Добавляет плеер
int PlayerAdd (File_data *file_data, int repeat);

//Обновляет данные всех плееров (декодирует фрейм/кадр каждого из плееров в списке)
int PlayersUpdate (int offset);

//Удаляет плеер
void PlayerDel(Player *player);

//Удаляет все плееры
void PlayerDelAll(void);

#endif /* PLAYER_H_ */
