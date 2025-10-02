CFLAGS = -Wall -Wextra -pedantic -std=c17 -Wswitch-enum

OBJs = $(patsubst %.c,build/%.o,$(1))

ASSEMBLER_OBJs := $(call OBJs, assembler.c vm.c disk.c)
INTERPRET_OBJs := $(call OBJs, interpret.c vm.c disk.c)

-include $(ASSEMBLER_OBJs:.o=.d)

assembler: $(ASSEMBLER_OBJs)
	$(CC) $(CFLAGS) -o $@ $^

interpreter: $(INTERPRET_OBJs)
	$(CC) $(CFLAGS) -o $@ $^

test: tests.c
	$(CC) $(CFLAGS) -o __testrunner $<
	@./__testrunner
	@rm -f __testrunner

build/:
	mkdir -p build

build/%.o: %.c | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build assembler interpreter examples/*.ins
