/*
 * Author: VadRov
 * Copyright (C) 2022, VadRov, all right reserved.
 *
 * Музыкальные данные (композиции и эффекты)
 *
 * Использованы фрагменты произведений (на основе лицензии CC BY 3.0):
 * Трек         : Constance
 * Композитор   : Kevin MacLeod
 * Жанр         : Soundtrack
 * Дата записи  : 2011
 *
 * Трек			: Delay Rock
 * Композитор	: Kevin MacLeod
 * Жанр			: Rock
 * Дата записи	: 2005
 */

#ifndef MUS_DATA_H_
#define MUS_DATA_H_

#include "main.h"

//Тип входных данных
typedef enum {
	FILE_TYPE_MP3,	//Данные в mp3
	FILE_TYPE_WAV	//Данные в wav
} FILE_TYPE;

//Структура описывает входящие данные плееров
typedef struct {
	const uint8_t *data; //Указатель на блок данных
	const uint32_t len;	 //Длина блока данных
	FILE_TYPE type;		 //Тип данных (mp3, wav)
} File_data;

extern File_data Mp3_intro, Mp3_tema, Mp3_final;
extern File_data WAV_you_move, WAV_drum;

#endif /* MUS_DATA_H_ */
