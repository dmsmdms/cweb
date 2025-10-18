.DEFAULT_GOAL := all

KCONFIG_DIR := config
HTML_DIR := html
SRC_DIR := src
TMP_DIR := tmp
TMP_OBJ_DIR := $(TMP_DIR)/obj
TMP_HTML_DIR := $(TMP_DIR)/html
TMP_CSS_DIR := $(TMP_HTML_DIR)/css
TMP_JS_DIR := $(TMP_HTML_DIR)/js
TARGET := $(TMP_DIR)/cweb

KCONFIG_INCLUDE_DIR := $(TMP_DIR)/include
KCONFIG_GENERATED_DIR := $(KCONFIG_INCLUDE_DIR)/generated
KCONFIG_CONFIG_DIR := $(KCONFIG_INCLUDE_DIR)/config
KCONFIG_LIST := $(shell find $(KCONFIG_DIR) -type f -name '*.config')
KCONFIG_FILE := $(abspath Kconfig)
KCONFIG_ORIG := $(TMP_DIR)/.config.orig

include $(wildcard $(KCONFIG_CONFIG_DIR)/auto.conf)
include $(wildcard $(TMP_OBJ_DIR)/*.d)
include $(wildcard $(TMP_JS_DIR)/*.d)

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

CFLAGS := -MD -std=c2x
CFLAGS := $(CFLAGS) -Wall -Wextra -Werror -Wpedantic -fdiagnostics-color=auto -Wfatal-errors # Warning as error
CFLAGS := $(CFLAGS) -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 # Additional warnings
CFLAGS := $(CFLAGS) -fmerge-all-constants # Merge all same constants into one
CFLAGS := $(CFLAGS) -ffunction-sections -fdata-sections -Wl,--gc-sections # Remove unused functions and data
CFLAGS := $(CFLAGS) -I$(SRC_DIR) -I$(KCONFIG_GENERATED_DIR) # Include paths
WASM_FLAGS := $(CFLAGS) --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all # WASM compilation flags

ifdef CONFIG_BUILD_RELEASE
CFLAGS := $(CFLAGS) -O2 -flto=auto # Enable link-time optimization
WASM_FLAGS := $(WASM_FLAGS) -O2 -flto=auto # Enable link-time optimization
endif
ifdef CONFIG_BUILD_DEBUG
CFLAGS := $(CFLAGS) -O0 -g # Enable debugging symbols
CFLAGS := $(CFLAGS) -fsanitize=address,undefined,leak # Enable sanitizers
endif

VPATH := $(SRC_DIR)
VPATH := $(VPATH) $(SRC_DIR)/core
VPATH := $(VPATH) $(SRC_DIR)/core/base
VPATH := $(VPATH) $(SRC_DIR)/core/db
VPATH := $(VPATH) $(SRC_DIR)/core/ws
VPATH := $(VPATH) $(SRC_DIR)/core/json
VPATH := $(VPATH) $(SRC_DIR)/core/http
VPATH := $(VPATH) $(SRC_DIR)/core/html
VPATH := $(VPATH) $(SRC_DIR)/core/telebot
VPATH := $(VPATH) $(SRC_DIR)/db
VPATH := $(VPATH) $(SRC_DIR)/bot
VPATH := $(VPATH) $(SRC_DIR)/parser
VPATH := $(VPATH) $(SRC_DIR)/api
VPATH := $(VPATH) $(SRC_DIR)/wasm
VPATH := $(VPATH) $(HTML_DIR)
VPATH := $(VPATH) $(HTML_DIR)/scss
VPATH := $(VPATH) $(HTML_DIR)/js

SRC := main.c
SRC := $(SRC) log.c
SRC := $(SRC) str.c
SRC := $(SRC) mem.c
SRC := $(SRC) args.c
SRC := $(SRC) file.c
SRC := $(SRC) config.c
SRC := $(SRC) jsmn.c
SRC := $(SRC) json-parser.c
SRC := $(SRC) json-gen.c
LIBS := -lev
ifdef CONFIG_HTTP_CLIENT
SRC := $(SRC) http-client.c
LIBS := $(LIBS) -lcurl
endif
ifdef CONFIG_WS_CLIENT
SRC := $(SRC) ws-client.c
LIBS := $(LIBS) -lwebsockets
endif
ifdef CONFIG_HTTP_SERVER
SRC := $(SRC) http-server.c
SRC := $(SRC) http-parser.c
SRC := $(SRC) http-parser-ext.c
SRC := $(SRC) http-gen.c
SRC := $(SRC) html-gen.c
endif
ifdef CONFIG_LANG
SRC := $(SRC) lang.c
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
SRC := $(SRC) db-job.c
endif
ifdef CONFIG_DB_CRYPTO_TABLE
SRC := $(SRC) db-crypto.c
endif
ifdef CONFIG_PARSER_CVBANKAS
SRC := $(SRC) parser-cvbankas.c
SRC := $(SRC) parser-cvbankas-helper.c
endif
ifdef CONFIG_PARSER_BINANCE
SRC := $(SRC) parser-binance.c
SRC := $(SRC) parser-binance-helper.c
endif
ifdef CONFIG_APP_JOBLY_API
SRC := $(SRC) api-jobly.c
endif
ifdef CONFIG_APP_CRYPTO_API
SRC := $(SRC) api-crypto.c
SRC := $(SRC) api-crypto-parser.c
endif
ifdef CONFIG_APP_JOBLY_BOT
SRC := $(SRC) bot-jobly.c
endif
OBJ := $(SRC:%.c=$(TMP_OBJ_DIR)/%.o)

SCSS_DEPS := $(shell find $(HTML_DIR)/scss -name '*.scss')
ifdef CONFIG_HTML_CRYPTO
HTML := $(HTML) crypto-db.html
SCSS := $(SCSS) crypto-db.scss
JS := $(JS) crypto-db.js
WASM := $(WASM) calc-crypto.c
endif
HTML := $(HTML:%.html=$(TMP_HTML_DIR)/%.html)
CSS := $(SCSS:%.scss=$(TMP_CSS_DIR)/%.css)
JS := $(JS:%.js=$(TMP_JS_DIR)/%.js)
WASM := $(WASM:%.c=$(TMP_JS_DIR)/%.wasm)

all: $(TARGET) $(HTML) $(CSS) $(JS) $(WASM)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) 

$(TMP_OBJ_DIR)/%.o: %.c
	@ mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TMP_HTML_DIR)/%.html: %.html
	@ mkdir -p $(dir $@)
	minify --type html $< -o $@

$(TMP_JS_DIR)/%.js: %.js
	@ mkdir -p $(dir $@)
	minify --type js $< -o $@

$(TMP_JS_DIR)/%.wasm: %.c
	@ mkdir -p $(dir $@)
	clang $(WASM_FLAGS) $< -o $@

$(TMP_CSS_DIR)/%.css: %.scss $(SCSS_DEPS) $(TMP_HTML_DIR)/%.html
	@ mkdir -p $(dir $@)
	sassc $< $@
	purgecss -css $@ -con $(TMP_HTML_DIR)/$*.html -o $@
	minify --type css $@ -o $@

clean:
	rm -rf $(TMP_DIR)
endif

