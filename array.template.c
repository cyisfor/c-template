#include "$(ELEMENT_TYPE)_array.h"

// va_list can't be rewinded (because C is retarded)

void $(ELEMENT_TYPE)_array_push($(ELEMENT_TYPE)_array* self) {
	if(self->length == self->space) {
		self->space += 0x100;
		self->items = realloc(self->items,
														sizeof(*self->items)*self->space);
	}
	++self->length; // we shall set this item
}

void $(ELEMENT_TYPE)_array_done_pushing($(ELEMENT_TYPE)_array* self) {
	self->space = self->length;
	self->items = realloc(self->items,
													sizeof(*self->items)*self->space);
}

void va_list_to_$(ELEMENT_TYPE)_arrayv
($(ELEMENT_TYPE)_array* self, va_list args) {
	size_t space = 0;
	for(;;) {
#ifdef BY_VALUE
		$(ELEMENT_TYPE)* value = va_arg(args, $(ELEMENT_TYPE)*);
		if(value == NULL) break;
		$(ELEMENT_TYPE)_array_push(self);
		memcpy(&self->items[self->length-1],value,sizeof($(ELEMENT_TYPE)));
#else
		$(ELEMENT_TYPE) value = va_arg(args,$(ELEMENT_TYPE));
		if(value == NULL) break;
		$(ELEMENT_TYPE)_array_push(self);
		self->items[self->length-1] = value;
#endif
	}
	$(ELEMENT_TYPE)_array_done_pushing(self);
}

// this function will ONLY work if your function has exactly 1
// non-variadic parameter.
void va_list_to_$(ELEMENT_TYPE)s
		 ($(ELEMENT_TYPE)_array* self, ...) {
	va_list args;
	va_start(args,self);
	va_list_to_$(ELEMENT_TYPE)_arrayv(self,args);
	va_end(args);
}

// define this somewhere
void $(ELEMENT_TYPE)_free($(ELEMENT_TYPE));

void $(ELEMENT_TYPE)_array_clear($(ELEMENT_TYPE)_array* self) {
	int i = 0;
	for(;i<self->length;++i) {
		$(ELEMENT_TYPE)_free(self->items[i]);
	}
	free(self->items);
	self->length = 0;
	self->space = 0;
	self->items = NULL; // just in case
}
