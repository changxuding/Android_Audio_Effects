# CXX := g++
CC := gcc


INC_DIR := -I./audio -I./audio_utils -I./bass -I./bundle -I./common/lib -I./common/src -I./eq -I./spectrumanalyzer -I./stereowidening 
SRC_DIR := audio_utils bass bundle eq common/src spectrumanalyzer stereowidening 
TAR_DIR := bin
OBJ_DIR := tmp

$(shell if [ ! -e $(TAR_DIR) ];then mkdir $(TAR_DIR); fi)
$(foreach folder,$(SRC_DIR),$(shell mkdir -p $(OBJ_DIR)/$(folder)))

# CPPFLAGS := -std=c++17 -g -W
# CPPFLAGS += $(INC_DIR) -I./test
# CPPFLAGS +=  -lstdc++ -lbsd
# CPPFLAGS += -DBUILD_FLOAT -DHIGHER_FS -DSUPPORT_MC

CFLAGS := -O2 -g -W 
CFLAGS += -lm
CFLAGS += $(INC_DIR) -I./test
CFLAGS += -DBUILD_FLOAT -DHIGHER_FS -DSUPPORT_MC

SRCS := $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))
OBJS := $(foreach cfile, $(SRCS), $(patsubst %.c, $(OBJ_DIR)/%.o, $(cfile)))
TARGET := $(TAR_DIR)/test_lvm


$(TARGET): $(OBJS) ./test/lvmtest.c
	$(CC) $(CFLAGS) $^ -o $@ 

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"


.PHONY: clean debug
clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(TAR_DIR)
debug:
	@echo srcs=$(SRCS)
	@echo objs=$(OBJS) 
