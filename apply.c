#define _GNU_SOURCE
#include "myassert/assert.h"

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h> // NULL
#include <sys/stat.h>
#include <string.h> // memmem etc
#include <unistd.h> // close


#define PUT(str, len) fwrite(str,1,len,dest); fflush(dest);
#define PUTLIT(lit) PUT(lit,sizeof(lit)-1)

void apply_template(FILE* dest, int source) {
	struct stat info;
	// must redirect a file from stdin to here.
	assert(0==fstat(source, &info));
	char* buf = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, 0, 0);
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

			const char* value = getenv(name);
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
	return 0;
}
