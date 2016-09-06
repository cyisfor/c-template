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

void apply_template(int dest, int source
#ifndef DO_MAIN
										, va_list varg
#endif
										) {
#ifndef DO_MAIN
	string_array args = {};
	va_list_to_string_arrayv(&args,varg);
#endif
	struct stat info;
	// must redirect a file from stdin to here.
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
