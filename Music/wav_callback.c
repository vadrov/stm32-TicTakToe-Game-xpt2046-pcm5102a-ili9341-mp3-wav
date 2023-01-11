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

#include <string.h>
#include "wav_callback.h"

static int wav_process_decode(Player *player);

PlayerStartPlayStatus wav_start_play_callback (Player *player)
{
	AudioConfig *audioCFG = &player->audioCFG;
	Player_client_data *client_data = (Player_client_data*)player->client_data;
	file_in_mem *inbuffer = client_data->inbuffer;
	inbuffer->cur_ptr = inbuffer->buff;
	inbuffer->data_left = inbuffer->len;
	//задаем характеристики вручную
	audioCFG->channels = 1;
	audioCFG->samples = MP3_SAMPLES;
	audioCFG->bitpersample = 8;
	return PLAYER_START_OK;
}

PlayerUpdateStatus wav_update_play_callback (Player *player)
{
	if (wav_process_decode(player) == 0) return PLAYER_UPDATE_OK;
	return PLAYER_UPDATE_ERROR;
}

PlayerStopStatus wav_stop_callback (Player *player)
{
	return PLAYER_STOP_OK;
}

static int wav_process_decode (Player *player)
{
	Player_client_data *client_data = (Player_client_data*)player->client_data;
	AudioConfig *audioCFG = &player->audioCFG;
	file_in_mem *inbuffer = client_data->inbuffer;
	uint32_t samples = audioCFG->samples;
	samples = WAV_Decoder(audioCFG, &inbuffer->cur_ptr, (int*)&inbuffer->data_left, (uint16_t*)player->outbuf, samples);
	if (samples) {  //Если входных данных не хватило на фрейм/кадр, то обнуляем оставшиеся данные выходного PCM буфера
		memset(&player->outbuf[audioCFG->samples - samples], 0, 4 * samples);
		return 1; //Завершаем декодирование с ошибкой
	}
	return 0; //Завершаем декодирование без ошибки
}

/*
 * Декодирование WAV потока во входном буфере в выходной PCM буфер
 * audioCGF - конфигурация аудио (количество каналов: 1 (моно) или 2 (стерео) и разрядность: 8, 16, 24, 32 бит)
 * inbuf - двойной указатель на входной буфер
 * bytesLeft - указатель на счетчик, который показывает, сколько байт осталось обработать во входном буфере
 * outbuf - указатель на выходной PCM буфер (формат хранения данных LRLRLRLRLRLRLR....LR, где L и R - 16-битные данные (half word))
 * samples - число выходных сэмплов
 * Возвращает количество сэмплов, оставшихся необработанными. При успешном завершении это 0
 */
uint32_t WAV_Decoder(AudioConfig *audioCFG, uint8_t **inbuf, int *bytesLeft, uint16_t *outbuf, uint32_t samples)
{
	uint32_t channels = audioCFG->channels;
	uint32_t bitsPerSample = audioCFG->bitpersample;
	uint32_t samples_in_buffer = (*bytesLeft)/(channels*bitsPerSample/8); //На сколько сэмплов хватит данных во входном буфере
	float tmp;
	uint8_t ch;
	while (samples && samples_in_buffer) { //Цикл по количеству сэмплов выходного PCM буфера или сэмплов,
										   //на которые хватит данных во входном буфере (в зависимости от того, что меньше)
		ch = channels;
		while (ch)	{						//Расчет для каждого из каналов
			if (bitsPerSample == 32) { //32 бита плавающий тип
				tmp=(*(float*)(*inbuf))*32767;  //Преообразование диапазона -1.0...+1.0 в -32767.0...+32767.0
#ifdef STM32F4_DAC
				*outbuf = (int16_t)tmp + 32768;	//Перевод в беззнаковый 16 бит
#else
				*outbuf = (int16_t)tmp;
#endif
				*inbuf += 4;
				*bytesLeft -= 4;
			}
			else if (bitsPerSample == 24) { //24 бита
				*inbuf += 1;       			//Отбрасываем младшие биты
#ifdef STM32F4_DAC
				*outbuf = 32768 + (**inbuf) + ((*(*inbuf+1))<<8); //Перевод в беззнаковый 16 бит: 0...65535
#else
				*outbuf = (**inbuf) + ((*(*inbuf+1))<<8);
#endif
				*inbuf += 2;
				*bytesLeft -= 3;
			}
			else if (bitsPerSample == 16) { //16 бит
#ifdef STM32F4_DAC
				*outbuf = 32768 + (**inbuf) + ((*(*inbuf+1))<<8);
#else
				*outbuf = (**inbuf) + ((*(*inbuf+1))<<8);
#endif
				*inbuf += 2;
				*bytesLeft -= 2;
			}
			else if (bitsPerSample == 8) {  //8 бит беззнаковый
				*outbuf = (0x80 - **inbuf)<<8;	//Перевод из 8 бит: 0...255 в 16 бит: 0...65535
				*inbuf += 1;
				*bytesLeft -= 1;
			}
			outbuf++;
			ch--;
		}
		if (channels == 1)	{					//Для моно делаем копию 1 канала
			*outbuf = *(outbuf - 1);			//во втором канале
			outbuf++;
		}
		samples--;					//Уменьнаем количество выходных сэмплов
		samples_in_buffer--;		//Уменьшаем количество сэмплов, на которые хватит данных во входном буфере
	}
	return samples; //Возвращаем количество выходных сэмплов, оставшихся необработанными
}
