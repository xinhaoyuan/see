.PHONY: all stat-loc clean

V       ?= @
E_ENCODE = $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE = $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')
T_BASE   = target

SEE_OM  ?= naive-scan-sweep

T_CC_FLAGS  ?= -O0 -g -Ilib -I_om_${SEE_OM}

SRCFILES:= $(shell find . '(' '!' -regex '\./_.*' ')' -and '(' -iname "*.c" ')' | sed -e 's!\./!!g') \
			$(shell find _om_${SEE_OM} -iname "*.c" | sed -e 's!\./!!g')
OBJFILES:= $(addprefix ${T_BASE}/,$(addsuffix .o,$(foreach FILE,${SRCFILES},$(call E_ENCODE,${FILE}))))
DEPFILES:= $(OBJFILES:.o=.d)

all: ${T_BASE}/comp

-include ${DEPFILES}

${T_BASE}/%.d:
	${V}${CC} $(call E_DECODE,$*) -o$@ -MM $(T_CC_FLAGS) -MT $(@:.d=.o)

${T_BASE}/%.o: ${T_BASE}/%.d
	@echo CC $(call E_DECODE,$*)
	${V}${CC} $(call E_DECODE,$*) -o$@ ${T_CC_FLAGS} -c

${T_BASE}/comp: ${OBJFILES}
	@echo LD $@
	${V}${CC} $^ -o$@ ${T_CC_FLAGS}

stat-loc:
	${V}wc ${SRCFILES} -l

clean:
	-${V}rm -f target/*
