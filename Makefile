.PHONY: all stat-loc clean

V       ?= @
E_ENCODE = $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE = $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')
T_BASE   = target

T_CC_FLAGS_OPT ?= -O0 -g
T_CC_FLAGS     ?= ${T_CC_FLAGS_OPT} -Wall

SRCFILES:= $(shell find . '(' '!' -regex '\./_.*' ')' -and '(' -iname "*.c" ')' | sed -e 's!\./!!g')
SRCFILES:= $(filter-out simple_interrupter.c, ${SRCFILES})
OBJFILES:= $(addprefix ${T_BASE}/,$(addsuffix .o,$(foreach FILE,${SRCFILES},$(call E_ENCODE,${FILE}))))
DEPFILES:= $(OBJFILES:.o=.d)

all: ${T_BASE}/comp

-include ${DEPFILES}

${T_BASE}/%.d:
	${V}${CC} $(call E_DECODE,$*) -o $@ -MM $(T_CC_FLAGS) -MT $(@:.d=.o)

${T_BASE}/%.o: ${T_BASE}/%.d
	@echo CC $(call E_DECODE,$*)
	${V}${CC} $(call E_DECODE,$*) -o $@ ${T_CC_FLAGS} -c

${T_BASE}/comp: simple_interrupter.c ${T_BASE}/see.a
	@echo LD $@
	${V}${CC} $^ -o $@ ${T_CC_FLAGS}

${T_BASE}/see.a: ${OBJFILES}
	@echo AR $@
	${V}ar r $@ $^

stat-loc:
	${V}wc ${SRCFILES} -l

clean:
	-${V}rm -f target/*
