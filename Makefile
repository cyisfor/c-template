all: libtemplate.a

COMPILE=$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

CFLAGS+=-g

build/%.o: %.c | build
	$(COMPILE)

build/%.o: CFLAGS+=-c

build/apply_template: apply.c build/myassert/module.o | build
	DO_MAIN=1 $(COMPILE)

libtemplate.a: build/apply.o build/string_array.o build/myassert/module.o
	ar csr $@ $^

build/myassert/module.o: | build/myassert

build/string_array.o: build/apply_template array_template.c
	HEADER='typedef const char* string;'

build build/myassert:
	mkdir -p $@
