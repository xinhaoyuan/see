.PHONY: all 

T_CC_FLAGS       ?= ${T_CC_FLAGS_OPT} -Wall -I src
T_CC_OPT_FLAGS   ?= -O0
T_CC_DEBUG_FLAGS ?= -g

SRCFILES:= $(shell find src '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" ')' | sed -e 's!^\./!!g')

include ${T_BASE}/utl/template.mk

all: ${T_OBJ}/see.a ${T_OBJ}/see-simpintp

-include ${DEPFILES}

${T_OBJ}/see-simpintp: util/demo/simple_interpreter.c util/simple_stream/simple_stream.c ${T_OBJ}/see.a
	@echo LD $@
	${V}${CC} $^ -o $@ ${T_CC_ALL_FLAGS} -I util

${T_OBJ}/see.a: ${OBJFILES}
	@echo AR $@
	${V}ar r $@ $^
