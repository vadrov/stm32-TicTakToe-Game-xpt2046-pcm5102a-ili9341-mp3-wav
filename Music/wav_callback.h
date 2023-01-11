/*
 *  Author: VadRov
 *  Copyright (C) 2019, VadRov, all right reserved.
 *
 *  WAV декодер (Урезан. Только для входного буфера во флеш МК. Контейнеры не поддерживаются. "Скармливать" чистые данные)
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

#ifndef WAVCALLBACK_H_
#define WAVCALLBACK_H_

#include "player.h"

typedef struct {
	void *w;
} Player_client_wav;

//Декодирование WAV потока из файлового буфера в буфер вывода DAC (учитывается количество каналов: 1 (моно) или 2 (стерео) и разрядность: 8, 16, 24, 32 бит (с плавающей точкой)
//audioCGF - конфигурация аудио
//inbuf - двойной указатель на входной файловый буфер
//bytesLeft - указатель на счетчик, который показывает, сколько байт осталось обработать во входном буфере
//outbuf - указатель на выходной PCM буфер (формат хранения данных LRLRLRLRLRLRLR....LR, где L и R - 16-битные данные (half word))
//samples - число выходных сэмплов
//Возвращает количество сэмплов, оставшихся необработанными. При успешном завершении это 0
uint32_t WAV_Decoder(AudioConfig *audioCGF, uint8_t **inbuf, int *bytesLeft, uint16_t *outbuf, uint32_t samples);

PlayerStartPlayStatus wav_start_play_callback (Player *player);
PlayerUpdateStatus wav_update_play_callback (Player *player);
PlayerStopStatus wav_stop_callback (Player *player);

#endif /* WAVCALLBACK_H_ */
