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

#include "mp3_callback.h"
#include <stdlib.h>

static int mp3_process_decode(Player *player);
static void mp3_mono_to_dual_channel(uint16_t *outbuf, int samples);

PlayerStartPlayStatus mp3_start_play_callback (Player *player)
{
	AudioConfig *audioCFG = &player->audioCFG;
	Player_client_data *client_data = (Player_client_data*)player->client_data;
	Player_client_mp3 *decoder = (Player_client_mp3 *)client_data->client_data_decoder;
	file_in_mem *inbuffer = client_data->inbuffer;
	decoder->mp3Decoder = MP3InitDecoder();
	if (!decoder->mp3Decoder) return PLAYER_START_ERROR;
	inbuffer->cur_ptr = inbuffer->buff;
	inbuffer->data_left = inbuffer->len;
	//задаем характеристики вручную
	audioCFG->channels = 1;
	audioCFG->samples = MP3_SAMPLES;
	return PLAYER_START_OK;
}

PlayerUpdateStatus mp3_update_play_callback (Player *player)
{
	if (mp3_process_decode(player) == 0) return PLAYER_UPDATE_OK;
	return PLAYER_UPDATE_ERROR;
}

PlayerStopStatus mp3_stop_callback (Player *player)
{
	Player_client_data *client_data = (Player_client_data*)player->client_data;
	Player_client_mp3 *decoder = (Player_client_mp3 *)client_data->client_data_decoder;
	MP3FreeDecoder(decoder->mp3Decoder);
	return PLAYER_STOP_OK;
}

static int mp3_process_decode(Player *player)
{
	Player_client_data *client_data = (Player_client_data*)player->client_data;
	Player_client_mp3 *decoder = (Player_client_mp3 *)client_data->client_data_decoder;
	AudioConfig *audioCFG = &player->audioCFG;
	file_in_mem *inbuffer = client_data->inbuffer;
	int nOffset, nDecodeRes;
	while (inbuffer->data_left > 0)
	{
		nOffset = MP3FindSyncWord(inbuffer->cur_ptr, inbuffer->data_left);
		if (nOffset < 0) break;
		inbuffer->cur_ptr += nOffset;
		inbuffer->data_left -= nOffset;
		nDecodeRes = MP3Decode(decoder->mp3Decoder, &inbuffer->cur_ptr, (int*)&inbuffer->data_left, (short*)player->outbuf, 0);
		if (nDecodeRes == ERR_MP3_NONE)
		{
			//если только 1 моно канал, то модифицируем его в два моно канала (dual mono)
			if (audioCFG->channels == 1)
				mp3_mono_to_dual_channel((uint16_t*)player->outbuf, audioCFG->samples);
			return 0;
		}
		if (nDecodeRes==ERR_MP3_MAINDATA_UNDERFLOW || nDecodeRes==ERR_MP3_FREE_BITRATE_SYNC) continue;
		break;
	}
	return 1;
}

//преобразование 1 моно канала в 2 моно канала (dual mono)
//увеличивается размер выходного буфера в 2 раза
static void mp3_mono_to_dual_channel(uint16_t *outbuf, int samples)
{
	for (int i = 1; i <= samples; i++)
	{
		outbuf[2 * (samples - i)] = outbuf[samples - i];
		outbuf[2 * (samples - i) + 1] = outbuf[samples - i];
	}
}
