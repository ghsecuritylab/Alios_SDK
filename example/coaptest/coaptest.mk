NAME := coaptest

$(NAME)_SOURCES     := coaptest.c
GLOBAL_DEFINES      += ALIOT_DEBUG

CONFIG_COAP_DTLS_SUPPORT := y
#CONFIG_COAP_ONLINE := y

ifeq ($(CONFIG_COAP_DTLS_SUPPORT), y)
$(NAME)_DEFINES += COAP_DTLS_SUPPORT
endif
ifeq ($(CONFIG_COAP_ONLINE), y)
$(NAME)_DEFINES += COAP_ONLINE
endif


$(NAME)_COMPONENTS  := cli
#ifeq ($(LWIP),1)
#$(NAME)_COMPONENTS  += protocols.net
#endif

ifeq ($(findstring mk3060, $(BUILD_STRING)), mk3060)
$(NAME)_DEFINES += PLATFORM_MK3060
endif

$(NAME)_CFLAGS += \
    -Wno-unused-function \
    -Wno-implicit-function-declaration \
    -Wno-unused-function \
#    -Werror

$(NAME)_COMPONENTS  += connectivity.coap
