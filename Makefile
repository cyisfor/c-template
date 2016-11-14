all: libtemplate.a

COMPILE=$(CC) $(MAKESUX) $(CFLAGS) -o $@ $(filter %.c,$^) $(filter %.o,$^) $(LDFLAGS)

CFLAGS+=-g -Ibuild

build/%.o: %.c | build
	$(COMPILE)

build/apply_template: MAKESUX=-DDO_MAIN

build/%.o: MAKESUX=-c

build/apply_template: apply.c build/myassert/module.o | build
	$(COMPILE)

libtemplate.a: build/apply.o build/string_array.o build/myassert/module.o
	ar csr $@ $^

build/myassert/module.o: | build/myassert

build/string_array.c: build/apply_template array.template.c | build
	ELEMENT_TYPE='string' ./build/apply_template array.template.c > $@.temp
	mv $@.temp $@

build/string_array.h: build/apply_template array.template.h | build
	HEADER='typedef const char* string;' \
	ELEMENT_TYPE='string' ./build/apply_template array.template.h > $@.temp
	mv $@.temp $@

build/string_array.o: build/string_array.c build/string_array.h
	$(COMPILE)

build/apply.o: build/string_array.h

build build/myassert:
	mkdir -p $@

clean:
	rm -rf libtemplate.a build
