.PHONY: all 

T_CC_FLAGS       ?= ${T_CC_FLAGS_OPT} -Wall -Isrc
T_CC_OPT_FLAGS   ?= -O0
T_CC_DEBUG_FLAGS ?= -g

SRCFILES:= $(shell find src '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" ')' | sed -e 's!^\./!!g')
SRCFILES:= $(filter-out src/simple_interpreter.c,${SRCFILES})

include ${T_BASE}/utl/template.mk

all: ${T_OBJ}/see.a ${T_OBJ}/see-simpintp

-include ${DEPFILES}

${T_OBJ}/see-simpintp: src/simple_interpreter.c ${T_OBJ}/see.a
	@echo LD $@
	${V}${CC} $^ -o $@ ${T_CC_ALL_FLAGS}

${T_OBJ}/see.a: ${OBJFILES}
	@echo AR $@
	${V}ar r $@ $^
