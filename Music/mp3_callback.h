/*
 *  Author: VadRov
 *  Copyright (C) 2019, VadRov, all right reserved.
 *
 *	MP3 декодер (Урезан. Только для входного буфера во флеш МК. Контейнеры не поддерживаются. "Скармливать" чистые данные)
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

#ifndef MP3_CALLBACK_H_
#define MP3_CALLBACK_H_

#include "player.h"
#include "mp3dec.h"

typedef struct {
	HMP3Decoder mp3Decoder;
	MP3FrameInfo mp3FrameInfo;
} Player_client_mp3;

PlayerStartPlayStatus  mp3_start_play_callback (Player *player);
PlayerUpdateStatus     mp3_update_play_callback (Player *player);
PlayerStopStatus       mp3_stop_callback (Player *player);

#endif /* MP3_CALLBACK_H_ */
