/*
 *  Author: VadRov
 *  Copyright (C) 2019, VadRov, all right reserved.
 *
 *	Драйвер для работы с аудио ЦАП (Audio Stereo DAC) PCM5102
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

#ifndef PCM5102_H_
#define PCM5102_H_

#include "main.h"

//старт DAC DMA
int Start_DAC_DMA (SPI_TypeDef *spi, DMA_TypeDef *dma, uint32_t dma_stream, uint32_t SampleRate, uint32_t DataFormat, uint32_t *outbuf, uint32_t bufsize);
void I2S3_Init(void);

#endif /* PCM5102_H_ */
