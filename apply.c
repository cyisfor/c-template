#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h> // NULL
#include <sys/stat.h>
#include <string.h> // memmem etc
#include <unistd.h> // close

void assertp(const char* file, int line, const char* code, int test) {
	if(!test) {
		error(23,errno,"Assert fail %s:%d (%s)",file,line,code);
	}
}

#define assert(test) assertp(__FILE__,__LINE__,#test,(int)(test))

#define PUT(str, len) fwrite(str,1,len,stdout)
#define PUTLIT(lit) PUT(lit,sizeof(lit)-1)

int main(int argc, char *argv[])
{
	struct stat info;
	// must redirect a file from stdin to here.
	assert(0==stat(0, &info));
	char* buf = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, 0, 0);
	assert(buf != MAP_FAILED);
	close(0);
	off_t offset = 0;
	while(offset < info.st_size) {
		char* start = memmem(buf+offset,info.st_size-offset,"$(",2);
		if(start == NULL) {
			// nothing more, output to the end
			PUT(buf+offset,info.st_size-offset);
			break;
		}
		// output up to before the dollar
		PUT(buf+offset,start-buf);
		offset += start-buf+2;
		char* end = memmem(buf+offset,info.st_size-offset,")",1);
		if(end == NULL) {
			// output the dollar and stuff, to the end, no more piddies
			PUTLIT("$(");
			PUT(buf+offset,info.st_size-offset);
			break;
		}
		{
			ssize_t amt = end-start;
			char* name = alloca(amt+1);
			memcpy(name,start,amt);
			name[amt] = '\0';

			const char* value = getenv(name);
			if(NULL==value) {
				// output to the )
				PUT(buf+offset,end-buf+1);
			} else {
				PUT(value,strlen(value));
			}
		}
		// past the )
		offset += end-buf+1;
	}
	return 0;
}
