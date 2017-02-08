#include <stdlib.h> // size_t
#include <stdarg.h>

$(HEADER)

// va_list can't be rewinded (because C is retarded)
typedef struct {
	$(ELEMENT_TYPE)* items;
	size_t length;
	size_t space;
} $(ELEMENT_TYPE)_array;

void $(ELEMENT_TYPE)_array_push($(ELEMENT_TYPE)_array* result);
// ...then assign the result->items[result->length] item

void $(ELEMENT_TYPE)_array_done_pushing($(ELEMENT_TYPE)_array* result); 

void va_list_to_$(ELEMENT_TYPE)_arrayv
($(ELEMENT_TYPE)_array* result, va_list args);

// this function will ONLY work if your function has exactly 1
// non-variadic parameter.
void va_list_to_$(ELEMENT_TYPE)s
		 ($(ELEMENT_TYPE)_array* result, ...);

void $(ELEMENT_TYPE)_array_clear($(ELEMENT_TYPE)_array* self);

#define $(ELEMENT_TYPE)_PUSH(array,thing) { \
		$(ELEMENT_TYPE)_array_push(&array); \
		array.items[array.length-1] = thing; \
	}
