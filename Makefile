.DEFAULT_GOAL := all
KCONFIG_DIR := config
SRC_DIR := src
TMP_DIR := tmp
TARGET := $(TMP_DIR)/cweb
KCONFIG_INCLUDE_DIR := $(TMP_DIR)/include
KCONFIG_GENERATED_DIR := $(KCONFIG_INCLUDE_DIR)/generated
KCONFIG_CONFIG_DIR := $(KCONFIG_INCLUDE_DIR)/config
KCONFIG_LIST := $(shell find $(KCONFIG_DIR) -type f -name '*.config')
KCONFIG_FILE := $(abspath Kconfig)
KCONFIG_ORIG := $(TMP_DIR)/.config.orig

include $(wildcard $(KCONFIG_CONFIG_DIR)/auto.conf)
include $(wildcard $(TMP_DIR)/*.d)

$(notdir $(KCONFIG_LIST)):
	@ mkdir -p $(KCONFIG_GENERATED_DIR)
	@ mkdir -p $(KCONFIG_CONFIG_DIR)
	@ cd $(TMP_DIR) && KCONFIG_ALLCONFIG=$(abspath $(filter %/$@, $(KCONFIG_LIST))) kconfig-conf --alldefconfig $(KCONFIG_FILE)
	@ cd $(TMP_DIR) && kconfig-conf --silentoldconfig $(KCONFIG_FILE)
	@ echo $@ > $(KCONFIG_ORIG)

ifeq ($(wildcard $(KCONFIG_ORIG)),$(KCONFIG_ORIG))
KCONFIG_NAME := $(shell cat $(KCONFIG_ORIG))
KCONFIG_ALLCONFIG := $(abspath $(filter %/$(KCONFIG_NAME), $(KCONFIG_LIST)))

menuconfig:
	@ mkdir -p $(KCONFIG_GENERATED_DIR)
	@ mkdir -p $(KCONFIG_CONFIG_DIR)
	@ cd $(TMP_DIR) && kconfig-mconf $(KCONFIG_FILE)
	@ cd $(TMP_DIR) && kconfig-conf --silentoldconfig $(KCONFIG_FILE)

update-defconfig:
	@ cd $(TMP_DIR) && kconfig-conf --savedefconfig $(KCONFIG_ALLCONFIG) $(KCONFIG_FILE)

IFLAGS := -I$(SRC_DIR) -I$(KCONFIG_GENERATED_DIR)
CFLAGS := -MD -std=c2x
CFLAGS := $(CFLAGS) -Wall -Wextra -Werror -Wpedantic -fdiagnostics-color=auto -Wfatal-errors # Warning as error
CFLAGS := $(CFLAGS) -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 # Additional warnings
CFLAGS := $(CFLAGS) -fmerge-all-constants # Merge all same constants into one
CFLAGS := $(CFLAGS) -ffunction-sections -fdata-sections -Wl,--gc-sections # Remove unused functions and data
CFLAGS := $(CFLAGS) $(IFLAGS)

CPPCHECK_FLAGS := --enable=all --suppress=missingIncludeSystem
CPPCHECK_FLAGS := $(CPPCHECK_FLAGS) $(IFLAGS)

ifdef CONFIG_BUILD_RELEASE
CFLAGS := $(CFLAGS) -O2 -flto=auto # Enable link-time optimization
endif
ifdef CONFIG_BUILD_DEBUG
CFLAGS := $(CFLAGS) -O0 -g # Enable debugging symbols
CFLAGS := $(CFLAGS) -fsanitize=address,undefined,leak # Enable sanitizers
endif

VPATH := $(SRC_DIR)
VPATH := $(VPATH) $(SRC_DIR)/core
VPATH := $(VPATH) $(SRC_DIR)/core/base
VPATH := $(VPATH) $(SRC_DIR)/core/db
VPATH := $(VPATH) $(SRC_DIR)/core/json
VPATH := $(VPATH) $(SRC_DIR)/core/http
VPATH := $(VPATH) $(SRC_DIR)/core/html
VPATH := $(VPATH) $(SRC_DIR)/core/telebot
VPATH := $(VPATH) $(SRC_DIR)/db
VPATH := $(VPATH) $(SRC_DIR)/bot
VPATH := $(VPATH) $(SRC_DIR)/parser
VPATH := $(VPATH) $(SRC_DIR)/backend

SRC := main.c
SRC := $(SRC) log.c
SRC := $(SRC) str.c
SRC := $(SRC) mem.c
SRC := $(SRC) args.c
SRC := $(SRC) file.c
SRC := $(SRC) config.c
SRC := $(SRC) jsmn.c
SRC := $(SRC) json-parser.c
LIBS := -lev
ifdef CONFIG_HTTP_CLIENT
SRC := $(SRC) http-client.c
LIBS := $(LIBS) -lcurl
endif
ifdef CONFIG_HTTP_SERVER
SRC := $(SRC) http-server.c
SRC := $(SRC) http-parser.c
SRC := $(SRC) html-gen.c
endif
ifdef CONFIG_DB
SRC := $(SRC) db.c
SRC := $(SRC) db-table.c
LIBS := $(LIBS) -llmdb
endif
ifdef CONFIG_HTML_PARSER
SRC := $(SRC) html-parser.c
LIBS := $(LIBS) -lgumbo
endif
ifdef CONFIG_TELEBOT
SRC := $(SRC) telebot.c
SRC := $(SRC) telebot-parser.c
endif
ifdef CONFIG_DB_JOB_TABLE
SRC := $(SRC) job.c
endif
ifdef CONFIG_PARSER_CVBANKAS
SRC := $(SRC) cvbankas.c
SRC := $(SRC) cvbankas-helper.c
endif
ifdef CONFIG_APP_JOBLY
SRC := $(SRC) backend-jobly.c
SRC := $(SRC) bot-jobly.c
endif
ALL_SRC := $(shell find . -name '*.c')
OBJ := $(SRC:%.c=$(TMP_DIR)/%.o)

all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) 

$(TMP_DIR)/%.o: %.c
	@ mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

cpp-check:
	cppcheck $(CPPCHECK_FLAGS) $(ALL_SRC)

clean:
	rm -rf $(TMP_DIR)
endif

