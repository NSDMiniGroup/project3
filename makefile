#鲁棒性-各种条件检查
#TODO:
#

COMMENT ?= auto commit

FILESTOTRACK += $(shell find . | egrep "\.h|\.cpp|\.c|README|makefile|Makefile")
# GITFLAGS +=

INC_DIR += ./include
BUILD_DIR ?= ./build

CLTBINARY ?= $(BUILD_DIR)/client
SRVBINARY ?= $(BUILD_DIR)/server

# Compilation flags
CC = g++
INCLUDES = $(addprefix -I, $(INC_DIR)) #-I是可以用相对路径的，只是不能识别`~`的路径
CFLAGS += -Wall -Werror -ggdb3 -std=c++11

# Files to be compiled
SRCS = $(shell find src/ -name "*.cpp" | egrep -v "(s.*r.*v)|(c.*l.*t)")
CLTSRCS += $(SRCS) $(shell find src/client/ -name "*.cpp") \

SRVSRCS += $(SRCS) $(shell find src/server/ -name "*.cpp") \


# Rules
$(CLTBINARY): $(CLTSRCS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(SRVBINARY): $(SRVSRCS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Command 
.PHONY: all client server clean commit 

all: client server

client: $(CLTBINARY)

server: $(SRVBINARY)

clean:
	-rm -rf $(BUILD_DIR)

commit:
	@git add $(FILESTOTRACK)
	-@git status | grep "deleted" | sed 's/deleted://' | xargs git rm
	@echo "$(COMMENT)" | git commit -F - # -F indicate the commit message from a file, and `-` indicate from the standard input 
