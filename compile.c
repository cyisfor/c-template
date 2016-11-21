#define _GNU_SOURCE
#include "apply.h"

#include "myassert/assert.h"

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h> // NULL
#include <sys/stat.h>
#include <string.h> // memmem etc
#include <unistd.h> // close

#define PUT(str, len) write(dest,str,len)
#define PUTLIT(lit) PUT(lit,sizeof(lit)-1)

#ifndef DO_MAIN
#include <stdarg.h>

#include "string_array.h"
#endif

// apply_template(dest,source,"key","value","key2","value2",NULL);

struct item {
	enum { LITERAL, VARIABLE, FUNCTION, IF, LOOP, INCLUDE } type;
	uv_buf_t d;
};

struct closure {
	enum { STEP, FORK, END } type;
	void* data;
	void cleanup_data(void*);
};

/*
$VARIABLE (not IF, WHEN, LOOP, INCLUDE)
$(VARIABLE)
$(VARIABLE:type) type = i32, u32, i16, u16, f32, f64, b, c, etc
-> type VARIABLE in the struct, and printf(...) in the generator compilers
$(VARIABLE:length) length = [0-9]+ fixed length string
-> char VARIABLE[length] in the struct, length hard-coded in the generator compilers
-> instead of write(lit) write(buf) write(lit), it's memcpy(buflitwithspace,buf,length) write(buflitwithspace)
$(VARIABLE:type:length)
$(VARIABLE)
-> uv_buf_t VARIABLE (.base, .len) in the struct
$IF(CONDITION) { ... }
-> bool CONDITION in the struct, conditional code in the generator compilers
-> (careful about conditional continuations, skip unused stages)
$INCLUDE(FILE[:PREFIX])
-> include file inline within template, optional prefix to all variables within (recursive)
$LOOP(CONDITION) { ... }
-> void (*CONDITION)(struct*, closure loop, closure done) in the struct
-> set struct variables, then call loop to loop, call done when done.
$(FUNCTION arg1 arg2 arg3) 
-> void (*FUNCTION)(struct*, closure done, uv_buf_t arg1, arg2, arg3)
-> set struct variables, then call done
 */

#ifdef JUST_AN_EXAMPLE

/* LITERAL $(VARIABLE1) LITERAL  */

struct params;

struct end_closure {
	void* data;
	void (*end)(struct params*, void* data);
};

struct params {
	uv_buf_t VARIABLE1;
	struct end_closure FINALLY;
};

void all_done(uv_write_t* req, int status) {
	assert(status >= 0);
	struct params* this = *((struct params**) &req[1]);
	free(this->VARIABLE1.base);
	this->FINALLY.end(this, this->FINALLY.data);
	if(this->FINALLY.cleanup_data)
		this->FINALLY.cleanup_data(this->FINALLY.data);
	free(this);
	free(req);
}

void output_to_stream(struct params* params, uv_stream_t* dest) {
	uv_buf_t bufs[3] = {
		{LITERAL1,LEN},
		params.VARIABLE1,
		{LITERAL2,LEN},
	};
	void* data = malloc(sizeof(uv_write_t) + sizeof(struct params*));
	uv_write_t* writing = (uv_write_t*) data;
	writing->data = dest;
	struct params** save = (struct params**)(data + sizeof(uv_write_t));
	*save = params;
	uv_write(writing, dest, bufs, 3, all_done);
}

#endif

#ifdef EXAMPLE_FUNCTION

/* LITERAL1 $(FUNCTION1 2 3 4) LITERAL2 */
 
struct params;

struct end_closure {
	struct closure base;
	void (*end)(struct params*, void* data);
};


struct continue_closure {
	struct closure base;
	void (*next)(void*);
}

struct params {
	struct function1_closure {
		struct closure base;
		void (*step)(struct params*, void* data, uv_buf_t arguments, struct continue_closure* next);
	} FUNCTION1;
	struct end_closure FINALLY;
};

// 1 uv_write_t past start
#define THIS ((struct params**) &writing[1])

void all_done(uv_write_t* writing, int status) {
	assert(status >= 0);
	struct params* this = *(THIS);
	this->FINALLY.end(this, this->FINALLY.base.data);
	if(this->FINALLY.cleanup_data)
		this->FINALLY.cleanup_data(this->FINALLY.base.data);
	free(this);
	free(writing); // only had a malloc'd **pointer to this
}

void called_function1(void* data) {
	static uv_buf_t bufs[1] = {
		{LITERAL2,LEN},
	};

	if(this->FUNCTION1.base.cleanup_data)
		this->FUNCTION1.base.cleanup_data(this->FINALLY.base.data);

	uv_write_t* writing = (uv_write_t*) data;

	uv_write(writing, dest, bufs, 1, all_done);
}

void call_function1(uv_write_t* writing, int status) {
	assert(status >= 0);
	struct params* this = *(THIS);
	struct continue_closure* after = ((struct continue_closure*) &(THIS)[1]);
	// 1 struct params** past this
	after->base.data = writing;
	after->next = called_function1;
	const uv_buf_t arguments = { LITARGUMENTS, sizeof(LITARGUMENTS) };
	this->FUNCTION1.step(this, this->FUNCTION1.data, arguments, after);
#undef THIS
}

void output_to_stream(struct params* params, uv_stream_t* dest) {
	static uv_buf_t bufs[1] = {
		{LITERAL1,LEN},
	};
	void* data = malloc(sizeof(uv_write_t) + sizeof(struct params*));
	uv_write_t* writing = (uv_write_t*) data;
	writing->data = dest;
	struct params** save = (struct params**)(data + sizeof(uv_write_t)
																					 + sizeof(struct end_closure) // after function1
		);
	*save = params;
	uv_write(writing, dest, bufs, 1, call_function1);
}




#endif
		

void apply_template(int dest, int source) {
	struct stat info;
	// may redirect a file from stdin to here.
	assert(0==fstat(source, &info));
	char* buf = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, source, 0);
	assert(buf != MAP_FAILED);
	close(source);
	off_t offset = 0;
	while(offset < info.st_size) {
		char* start = memmem(buf+offset,info.st_size-offset,"$(",2);
		if(start == NULL) {
			// nothing more, output to the end
			PUT(buf+offset,info.st_size-offset);
			break;
		}
		// output up to before the dollar
		PUT(buf+offset,start-(buf+offset));
		offset = start-buf+2;
		char* end = memmem(buf+offset,info.st_size-offset,")",1);
		if(end == NULL) {
			// output the dollar and stuff, to the end, no more piddies
			PUTLIT("$(");
			PUT(buf+offset,info.st_size-offset);
			break;
		}
		{
			ssize_t amt = end-start-2;
			char* name = alloca(amt+1);
			memcpy(name,start+2,amt);
			name[amt] = '\0';

			// environment overrides the passed defaults
			const char* value = getenv(name);
#ifndef DO_MAIN
			if(NULL==value) {
				int i;
				for(i=0;i<args.length;i+=2) {
					if(0==strcmp(args.items[i], name)) {
						value = args.items[i+1];
					}
				}
			}
#endif
			if(NULL==value) {
				// don't forget the $(
				// output to the )
				PUT(buf+offset-2,end-start+1);
			} else {
				PUT(value,strlen(value));
			}
		}
		// past the )
		offset = end-buf+1;
	}
	close(dest);
	munmap(buf,info.st_size);
}

#ifdef DO_MAIN
// ugh... bootstrapping
#include <fcntl.h>
int main(int argc, char** argv) {
	assert(argc == 2);
	int source = open(argv[1],O_RDONLY);
	assert(source >= 0);
	apply_template(1,source);

	return 0;
}
#endif
