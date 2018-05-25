ROOTDIR := ../../..
DEBOUNCERS := $(shell ls $(ROOTDIR)/firmware/debounce-*.h | cut -d \/ -f 5 | cut -d \. -f 1 | grep -v debounce-state-machine )
STATE_MACHINES := $(shell ls $(ROOTDIR)/firmware/config/debounce-state-machines/*h |cut -d \/ -f 6,7 |cut -d \. -f 1)

CFLAGS=-Wall -Wextra -O2 -g -DF_CPU=8000000

BROWSER := firefox
TEST_RESULT_HTML=$(abspath generated_test_results.html)

all: clean debouncers state-machines

dirs:
	-mkdir -p debounce-state-machines

state-machines: dirs $(STATE_MACHINES)

debounce-state-machines/%:
	$(CXX) debounce_test.cpp $(CFLAGS) -include ../debounce_test.h \
		-DDEBOUNCE_STATE_MACHINE=\"config/$(@).h\"  \
		-include $(ROOTDIR)/firmware/debounce-state-machine.h  \
		 -o $(@)

debounce-%:
	$(CXX) debounce_test.cpp $(CFLAGS) -include ../debounce_test.h -include $(ROOTDIR)/firmware/$(@).h -o $(@)

debouncers: $(DEBOUNCERS)

clean:
	rm -f $(DEBOUNCERS)
	rm -f $(STATE_MACHINES)
	rm -f $(TEST_RESULT_HTML)

test: clean state-machines debouncers
	perl run_tests.pl > $(TEST_RESULT_HTML)
	@echo -e "\nYou can now open the result file with:\n\n    $(BROWSER) file://$(TEST_RESULT_HTML)\n\n"
