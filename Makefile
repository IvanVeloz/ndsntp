# Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
#
# SPDX-License-Identifier: MIT
#
# SPDX-FileContributor: Antonio Niño Díaz, 2024

BLOCKSDS	?= /opt/blocksds/core

# User config

NAME			:= ndsntp
GAME_TITLE		:= ndsntp
GAME_SUBTITLE	:= NTP client for the DS
GAME_AUTHOR		:= Ivan Veloz

# Libraries
# ---------

SOURCEDIRS	:= source coreSNTP/source
INCLUDEDIRS	:= include coreSNTP/source/include
LIBS		:= -ldswifi9 -lnds9
LIBDIRS		:= $(BLOCKSDS)/libs/dswifi \
			   $(BLOCKSDS)/libs/libnds \

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
