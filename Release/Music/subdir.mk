################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Music/mp3_callback.c \
../Music/mus_data.c \
../Music/player.c \
../Music/wav_callback.c 

OBJS += \
./Music/mp3_callback.o \
./Music/mus_data.o \
./Music/player.o \
./Music/wav_callback.o 

C_DEPS += \
./Music/mp3_callback.d \
./Music/mus_data.d \
./Music/player.d \
./Music/wav_callback.d 


# Each subdirectory must supply rules for building sources it contributes
Music/%.o Music/%.su: ../Music/%.c Music/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32F401xC -DUSE_FULL_LL_DRIVER -DHSE_VALUE=25000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DEXTERNAL_CLOCK_VALUE=12288000 -DHSI_VALUE=16000000 -DLSI_VALUE=32000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=1 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/SamaraPRO/STM32Cube/Repository/stm32f401ccu6_TicTakToe_touch_audio_display/Display" -I"C:/Users/SamaraPRO/STM32Cube/Repository/stm32f401ccu6_TicTakToe_touch_audio_display/XPT2046" -I"C:/Users/SamaraPRO/STM32Cube/Repository/stm32f401ccu6_TicTakToe_touch_audio_display/PCM5102" -I"C:/Users/SamaraPRO/STM32Cube/Repository/stm32f401ccu6_TicTakToe_touch_audio_display/MP3Helix/include" -I"C:/Users/SamaraPRO/STM32Cube/Repository/stm32f401ccu6_TicTakToe_touch_audio_display/Music" -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Music

clean-Music:
	-$(RM) ./Music/mp3_callback.d ./Music/mp3_callback.o ./Music/mp3_callback.su ./Music/mus_data.d ./Music/mus_data.o ./Music/mus_data.su ./Music/player.d ./Music/player.o ./Music/player.su ./Music/wav_callback.d ./Music/wav_callback.o ./Music/wav_callback.su

.PHONY: clean-Music

