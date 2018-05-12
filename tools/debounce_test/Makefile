DEBOUNCERS := $(shell ls ../../firmware/debounce-*.h |cut -d \/ -f 4 |cut -d \. -f 1 |grep -v debounce-state-machine )
STATE_MACHINES := $(shell ls ../../firmware/config/debounce-state-machines/*h |cut -d \/ -f 5,6 |cut -d \. -f 1)


all: clean debouncers state-machines

dirs:
	-mkdir debounce-state-machines

state-machines: dirs $(STATE_MACHINES)

debounce-state-machines/%:
	$(CC) debounce_test.c -DF_CPU=8000000  -include debounce_test.h \
		-DDEBOUNCE_STATE_MACHINE=\"config/$(@).h\"  \
		-include ../../firmware/debounce-state-machine.h  \
		 -o $(@)

debouncers: $(DEBOUNCERS)

debounce-%:
	$(CC) debounce_test.c -DF_CPU=8000000 -include debounce_test.h -include ../../firmware/$(@).h -o $(@)

clean: 
	-rm $(DEBOUNCERS)
	-rm debounce-state-machines/*

test: clean state-machines debouncers
	perl run_tests.pl
