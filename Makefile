WARN := -Wall -Werror -pedantic
CFLAGS := $(WARN) -Iclang/include
LDFLAGS := -Lclang/lib -lclang -Wl,-rpath,'@loader_path/clang/lib'

.DEFAULT_GOAL := structgen

structgen: structgen.c
	cc $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: run
run: structgen
	@./structgen
