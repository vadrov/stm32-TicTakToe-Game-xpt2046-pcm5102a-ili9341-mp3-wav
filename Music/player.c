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

#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "wav_callback.h"
#include "mp3_callback.h"

Player *players = 0;
uint32_t *mix_buffer = 0;

#define DISABLE_IRQ		__disable_irq(); __DSB(); __ISB();
#define ENABLE_IRQ		__enable_irq();

PlayerInitStatus PlayerInit (Player *player, PlayerStartPlayCallback start_callback, PlayerUpdateCallback update_callback,
							 PlayerStopCallback stop_callback, void *client_data)
{
	if (player == 0 || start_callback == 0 || update_callback == 0 || stop_callback == 0)
		return PLAYER_INIT_ERROR;
	player->start_callback = start_callback;
	player->update_callback = update_callback;
	player->stop_callback = stop_callback;
	player->client_data = client_data;
	player->status = PLAYER_INIT;
	return PLAYER_INIT_OK;
}

PlayerStartPlayStatus PlayerStartPlay(Player *player)
{
	player->status = PLAYER_START_PLAY;
	return player->start_callback(player);
}

PlayerUpdateStatus PlayerUpdate(Player *player)
{
	player->status = PLAYER_UPDATE_PLAY;
	return player->update_callback(player);
}

PlayerStopStatus PlayerStop(Player *player)
{
	player->status = PLAYER_STOP;
	return player->stop_callback(player);
}

/*
 * Добавляет плеер.
 * repeat - количество повторов воспроизведения.
 * Причем, при repeat = 0 -> 1 повтор, при 1 -> 2 и т.д. При repeat = -1 - бесконечное воспроизведение.
 * file_data - указатель на данные с музыкой/эффектом
 */
int PlayerAdd (File_data *file_data, int repeat)
{
	Player *player = (Player *)calloc(1, sizeof(Player));
	Player_client_data *client_data = (Player_client_data *)calloc(1, sizeof(Player_client_data));
	//Инициализируем входящий поток плеера
	file_in_mem *inbuffer = (file_in_mem *)malloc(sizeof(file_in_mem));
	inbuffer->buff = inbuffer->cur_ptr = (uint8_t*)file_data->data; //Данные музыки/эффекта
	inbuffer->len = file_data->len;	//Длина данных с музыкой/эффектом
	inbuffer->data_left = file_data->len;
	client_data->inbuffer = inbuffer;
	player->repeat = repeat; //Количество повторов для плеера (сколько раз повторить музыку/эффект)
	if (file_data->type == FILE_TYPE_MP3) { //Mp3 декодер - для входных данных в формате mp3
		client_data->client_data_decoder = calloc(1, sizeof(Player_client_mp3));
		(void)PlayerInit(player, mp3_start_play_callback, mp3_update_play_callback, mp3_stop_callback, (void*)client_data);
	}
	else if (file_data->type == FILE_TYPE_WAV) { //WAV декодер - для входных данных в формате wav
		client_data->client_data_decoder = calloc(1, sizeof(Player_client_wav));
		(void)PlayerInit(player, wav_start_play_callback, wav_update_play_callback, wav_stop_callback, (void*)client_data);
	}
	(void)PlayerStartPlay(player); //Стартуем плеер
	//Добавляем плеер в список players
	if (!players) {        //Если список плееров пустой, то инициализируем его указателем на
		players = player;  //только что созданный плеер
	}
	else {								//Если список плееров не пустой,
		Player *prev = players;			//то добавляем в конец списка плееров указатель на только что созданный
		while (prev->next)				//плеер
		{
			prev = (Player*)prev->next;
		}
		player->prev = (void*)prev;		//Вновь созданный плеер ссылается на предыдующий
		prev->next = (void*)player;		//Предыдующий плеер ссылается на вновь созданный
	}
	return 0;
}

/*
 * Останавливает и удаляет заданный плеер
 */
void PlayerDel(Player *player)
{
	if (!player) return;

	//Исключаем плеер из списка players
	void *prev = player->prev;			//Указатель на предыдущий плеер в списке
	void *next = player->next;			//Указатель на следующий плеер в списке

	if (prev) {							//Переназначаем указатели на следующий и
		((Player *)prev)->next = next;	//предыдущий плеееры в списке у следующего и
	}									//предыдущего плееров, исключая тем самым из
	else {								//списка текущий плеер
		players = next;
	}
	if (next) {
		((Player *)next)->prev = prev;
	}
	(void)PlayerStop(player); //Останавливаем текущий плеер
	//Освобождаем память занимаемую плеером
	Player_client_data *client_data = (Player_client_data *)player->client_data;
	if (client_data) {
		 if (client_data->client_data_decoder) free(client_data->client_data_decoder);
		 if (client_data->inbuffer) free(client_data->inbuffer);
		 free(client_data);
	}
	memset(player, 0, sizeof(Player));
	free(player);
}

/*
 * Останавливает и удаляет все плейеры
 */
void PlayerDelAll(void)
{
	DISABLE_IRQ //Запрещаем прерывания для исключения возможных конфликтов.
	Player *player = players, *player1;
	while (player) { //Идем по списку плееров
		player1 = player->next; //Сохраняем указатель на следующий плеер
		PlayerDel(player);		//Останавливаем и удаляем текущий плеер
		player = player1; 		//Восстанавливаем указатель на следующий плеер и
	}					  		//переходим в начало цикла
	players = 0;	//Инициализируем список плееров
	ENABLE_IRQ		//Разрещаем прерывания
}

/*
 * Обновляет данные плееров (декодирует фреймы) и микширует их выходные данные
 * offset - определяет половину выходного PCM буфера,
 * в которую будут микшироваться выходные данные плееров
 */
int PlayersUpdate (int offset)
{
	if (offset == -1) return 1;
	//Очищаем неактивную половину буфера, в которую будем микшировать декодированные данные с плееров
	memset(&mix_buffer[offset * MP3_SAMPLES], 0, 4 * MP3_SAMPLES);
	Player *player = players, *player1;
	uint8_t f = 0; //Флаг первого/единственного плеера в списке
	while (player) { //Идем по списку плееров
		player1 = player->next; //Запоминаем указатель на следующий плеер для случая, если придется удалить текущий
		//Назначаем плееру выходной PCM буфер
		if (!f) { //Если плеер первый/единственный, то выходной буфер - это неактивная часть основного PCM буфера
			player->outbuf = &mix_buffer[offset * MP3_SAMPLES];
		}
		else { //Для остальных плееров - это дополнительный PCM буфер в области основного PCM буфера
			player->outbuf = &mix_buffer[2 * MP3_SAMPLES];
		}
		if (PlayerUpdate(player) != PLAYER_UPDATE_OK) { //Декодируем фрейм/кадр
			if (player->repeat) { //Если повтор воспроизведения доступен, то инициализируем источник входных данных
				file_in_mem *inbuffer = ((Player_client_data*)player->client_data)->inbuffer;
				inbuffer->cur_ptr = inbuffer->buff; 	//Указатель чтения вновь указывает на начало данных
				inbuffer->data_left = inbuffer->len;	//Счетчик оставшихся для чтения данных вновь равен длине данных
				if (player->repeat != -1) { //Если повтор не бесконечен,
					player->repeat--;		//то уменьшаем счетчик повторов
				}
			}
			else { //Если повторили воспроизведение необходимое количество раз,
				PlayerDel(player);  //то останавливаем и удаляем плеер
			}
		}
		else { //Микшируем выходные потоки плееров в пределах фрейма
			if (f) { //Плеер не первый/единственный - микшируем данные
				int16_t *ptr1 = (int16_t*)(&mix_buffer[offset * MP3_SAMPLES]);
				int16_t *ptr2 = (int16_t*)player->outbuf;
				for (int i = 0; i < 2 * MP3_SAMPLES; i++) { //Складываем приведенные к знаковому типу сэмплы
					ptr1[i] = ptr1[i] + ptr2[i];			//Результат накапливаем в неактивной половине PCM буфера
				}
			}
			else { //Плеер первый/единственный - устанавливаем флаг
				f = 1;
			}
		}
		player = player1; //Восстанавливаем указатель на следующий плеер в списке
	}
	return 0;
}
