NAME := meshapp
no_with_lwip := 0

$(NAME)_SOURCES     := main.c

$(NAME)_COMPONENTS  += protocols.mesh cli netmgr
GLOBAL_DEFINES      += TAPIF_DEFAULT_OFF DEBUG

LWIP ?=1
ifeq ($(LWIP), 1)
$(NAME)_COMPONENTS  += protocols.net
endif


ifeq ($(ipv6),0)
GLOBAL_DEFINES += LWIP_IPV6=0
endif

ifneq (,$(filter linuxhost,$(COMPONENTS)))
DDA ?= 1
endif

ifneq (,$(filter armhflinux,$(COMPONENTS)))
DDA ?= 1
endif

ifeq ($(DDA),1)
GLOBAL_LDFLAGS += -lreadline -lncurses
$(NAME)_COMPONENTS  += dda
endif
