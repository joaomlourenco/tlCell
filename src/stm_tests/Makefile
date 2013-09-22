CTL_DIR=/home/lopeici/CTL/CTLlib

CTL_INCLUDE = $(CTL_DIR)/include
CTL_LIB = $(CTL_DIR)/lib

# criar um makefile que automaticamente verifique quais as versoes
# do CTL estao instaladas e compilar o mesmo programa para as varias
# versoes

LIBS = -L$(CTL_LIB) -L/usr/lib64/ -L/opt/cell/sdk/usr/lib64 -R/opt/cell/sdk/usr/lib64 -lrt -lspe2 -lpthread -lctl-cmt-word-64
CFLAGS = -Wall -g -DTL2 -D_REENTRANT list_stm_tl_spu -I$(CTL_INCLUDE) -I./

BIN_DIR = bin

SRC_RBTREE = g_kv_harness.c g_rb_stm_tl.c
SRC_LIST = g_kv_harness.c g_list_stm_tl.c

all: rbtree list

rbtree:
	ppu-gcc -o  $(BIN_DIR)/rbtree $(CFLAGS) $(SRC_RBTREE) $(LIBS)

list:
	@echo 'Compiling Harness Cell Test.....'
	ppu-gcc -o  $(BIN_DIR)/list $(CFLAGS) $(SRC_LIST) $(LIBS)
	@echo 'Done!!'

clean:
	rm -f $(BIN_DIR)/*
