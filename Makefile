PROJECT=FireflyIce
SERIES=EFM32LG
DEVICE=EFM32LG330F256

include makefile.conf

SRC_DIR=src
EM_CMSIS_DIR=../energymicro
EM_USB_DIR=../energymicro-usb/usb
EM_USB_SRC_DIR=$(EM_USB_DIR)/src
EM_LIB_DIR=$(EM_CMSIS_DIR)/emlib
EM_LIB_SRC_DIR=$(EM_LIB_DIR)/src
EM_DEVICE_SRC_DIR=$(EM_CMSIS_DIR)/Device/EnergyMicro/$(SERIES)/Source

CINCLUDES=\
-I$(SRC_DIR) \
-I$(EM_USB_DIR)/inc \
-I$(EM_CMSIS_DIR)/emlib/inc \
-I$(EM_CMSIS_DIR)/Device/EnergyMicro/$(SERIES)/Include \
-I$(EM_CMSIS_DIR)/CMSIS/Include

VPATH := $(SRC_DIR):$(EM_USB_SRC_DIR):$(EM_LIB_SRC_DIR):$(EM_DEVICE_SRC_DIR)

SOURCES=\
$(SRC_DIR)/fd_adc.c \
$(SRC_DIR)/fd_at24c512c.c \
$(SRC_DIR)/fd_binary.c \
$(SRC_DIR)/fd_bluetooth.c \
$(SRC_DIR)/fd_control.c \
$(SRC_DIR)/fd_crc.c \
$(SRC_DIR)/fd_detour.c \
$(SRC_DIR)/fd_event.c \
$(SRC_DIR)/fd_i2c1.c \
$(SRC_DIR)/fd_lis3dh.c \
$(SRC_DIR)/fd_log.c \
$(SRC_DIR)/fd_mag3110.c \
$(SRC_DIR)/fd_nrf8001.c \
$(SRC_DIR)/fd_nrf8001_callbacks.c \
$(SRC_DIR)/fd_nrf8001_commands.c \
$(SRC_DIR)/fd_nrf8001_dispatch.c \
$(SRC_DIR)/fd_processor.c \
$(SRC_DIR)/fd_rtc.c \
$(SRC_DIR)/fd_spi0.c \
$(SRC_DIR)/fd_spi1.c \
$(SRC_DIR)/fd_storage.c \
$(SRC_DIR)/fd_sync.c \
$(SRC_DIR)/fd_system.c \
$(SRC_DIR)/fd_tca6507.c \
$(SRC_DIR)/fd_usb.c \
$(SRC_DIR)/main.c \
$(EM_USB_SRC_DIR)/em_usbd.c \
$(EM_USB_SRC_DIR)/em_usbdch9.c \
$(EM_USB_SRC_DIR)/em_usbdep.c \
$(EM_USB_SRC_DIR)/em_usbdint.c \
$(EM_USB_SRC_DIR)/em_usbh.c \
$(EM_USB_SRC_DIR)/em_usbhal.c \
$(EM_USB_SRC_DIR)/em_usbhep.c \
$(EM_USB_SRC_DIR)/em_usbhint.c \
$(EM_USB_SRC_DIR)/em_usbtimer.c \
$(EM_LIB_SRC_DIR)/em_acmp.c \
$(EM_LIB_SRC_DIR)/em_adc.c \
$(EM_LIB_SRC_DIR)/em_aes.c \
$(EM_LIB_SRC_DIR)/em_assert.c \
$(EM_LIB_SRC_DIR)/em_burtc.c \
$(EM_LIB_SRC_DIR)/em_cmu.c \
$(EM_LIB_SRC_DIR)/em_dac.c \
$(EM_LIB_SRC_DIR)/em_dbg.c \
$(EM_LIB_SRC_DIR)/em_dma.c \
$(EM_LIB_SRC_DIR)/em_ebi.c \
$(EM_LIB_SRC_DIR)/em_emu.c \
$(EM_LIB_SRC_DIR)/em_gpio.c \
$(EM_LIB_SRC_DIR)/em_i2c.c \
$(EM_LIB_SRC_DIR)/em_int.c \
$(EM_LIB_SRC_DIR)/em_lcd.c \
$(EM_LIB_SRC_DIR)/em_lesense.c \
$(EM_LIB_SRC_DIR)/em_letimer.c \
$(EM_LIB_SRC_DIR)/em_leuart.c \
$(EM_LIB_SRC_DIR)/em_mpu.c \
$(EM_LIB_SRC_DIR)/em_msc.c \
$(EM_LIB_SRC_DIR)/em_opamp.c \
$(EM_LIB_SRC_DIR)/em_pcnt.c \
$(EM_LIB_SRC_DIR)/em_prs.c \
$(EM_LIB_SRC_DIR)/em_rmu.c \
$(EM_LIB_SRC_DIR)/em_rtc.c \
$(EM_LIB_SRC_DIR)/em_system.c \
$(EM_LIB_SRC_DIR)/em_timer.c \
$(EM_LIB_SRC_DIR)/em_usart.c \
$(EM_LIB_SRC_DIR)/em_vcmp.c \
$(EM_LIB_SRC_DIR)/em_wdog.c \
$(EM_DEVICE_SRC_DIR)/system_efm32lg.c

OBJECTS := $(patsubst %.c, $(ObjDir)/%.o, $(notdir $(SOURCES)))

all:: $(BinDir)/$(PROJECT).elf

$(BinDir)/$(PROJECT).elf: $(OBJECTS) $(STARTUP)
	@echo building output ...
	$(CC) $(LFLAGS) -o $@ $(OBJECTS) $(STARTUP) -lm

$(ObjDir)/%.o : %.c
	@echo creating $@ ...
	$(CC) $(CFLAGS) -D$(DEVICE) $(CINCLUDES) -c -o $@ $<

.s.o :
	$(AS) $(ASFLAGS) -o $@ $< > $(basename $@).lst

clean: 
	rm -f $(BinDir)/*.elf $(BinDir)/*.map $(ObjDir)/*.o
