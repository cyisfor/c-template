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
	enum { LITERAL, VARIABLE, FUNCTION } type;
	uv_buf_t d;
};

/* LITERAL VARIABLE1 LITERAL VARIABLE2 LITERAL FUNCTION LITERAL VARIABLE1 VARIABLE3 LITERAL ETC ... */
#ifdef JUST_AN_EXAMPLE

struct closure {
	void (*func)(void* data, struct closure continuation);
	void* data;
};


struct params {
	uv_buf_t VARIABLE1;
	uv_buf_t VARIABLE2;
	struct closure FUNCTION;
	uv_buf_t VARIABLE3;
	struct closure FINALLY;
};

void output_to_stream4(uv_write_t* req, int status) {
	assert(status >= 0);
	free(this->VARIABLE3.base);
	struct params* this = *((struct params**) &req[1]);
	free(req);
	struct closure continuation = { NULL, NULL };
	this->FINALLY.func(this->FUNCTION.data, continuation);
	free(this);
}

void output_to_stream3(void* data, struct closure next) {
	uv_write_t* writing = (uv_write_t*)data;
	uv_stream_t* dest = writing->data;
	struct params* this = *((struct params**) data + sizeof(uv_write_t));
	uv_write(writing, dest, &this->VARIABLE3, 1, output_to_stream4);
}

void output_to_stream2(uv_write_t* req, int status) {
	assert(status >= 0);
	free(this->VARIABLE1.base);
	free(this->VARIABLE2.base);
	struct params* this = *((struct params**) &req[1]);
	struct closure continuation = {
		output_to_stream3,
		req
	};
	this->FUNCTION.func(this->FUNCTION.data, continuation);
}

void output_to_stream(struct params* params, uv_stream_t* dest) {
	uv_buf_t bufs[5] = {
		{LITERAL,LEN},
		params.VARIABLE,
		{LITERAL,LEN},
		params.VARIABLE2,
		{LITERAL,LEN},
	};
	void* data = malloc(sizeof(uv_write_t) + sizeof(struct params*));
	uv_write_t* writing = (uv_write_t*) data;
	writing->data = dest;
	struct params** save = (struct params**)(data + sizeof(uv_write_t));
	*save = params;
	uv_write(writing, dest, bufs, 5, output_to_stream2);
}


		

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
