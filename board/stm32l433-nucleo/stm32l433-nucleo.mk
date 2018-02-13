NAME := board_b_l433


$(NAME)_TYPE := kernel
MODULE               := 1062
HOST_ARCH            := Cortex-M4
HOST_MCU_FAMILY      := stm32l4xx
SUPPORT_BINS         := no
HOST_MCU_NAME        := STM32L433RC-Nucleo

GLOBAL_INCLUDES += .

GLOBAL_DEFINES += STDIO_UART=0 CONFIG_NO_TCPIP

CONFIG_SYSINFO_PRODUCT_MODEL := ALI_AOS_433-nucleo
CONFIG_SYSINFO_DEVICE_NAME := 433-nucleo

GLOBAL_CFLAGS += -DSYSINFO_OS_VERSION=\"$(CONFIG_SYSINFO_OS_VERSION)\"
GLOBAL_CFLAGS += -DSYSINFO_PRODUCT_MODEL=\"$(CONFIG_SYSINFO_PRODUCT_MODEL)\"
GLOBAL_CFLAGS += -DSYSINFO_DEVICE_NAME=\"$(CONFIG_SYSINFO_DEVICE_NAME)\"
