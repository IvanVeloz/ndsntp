# Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
#
# SPDX-License-Identifier: MIT
#
# SPDX-FileContributor: Ivan Veloz, 2024

BLOCKSDS	?= /opt/blocksds/core

# User config

NAME			:= ndsntp
GAME_TITLE		:= ndsntp
GAME_SUBTITLE	:= NTP client for the DS
GAME_AUTHOR		:= Ivan Veloz

# Libraries
# ---------

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9arm7/Makefile
