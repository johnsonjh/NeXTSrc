/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <string.h>

#undef strcoll
size_t strcoll(char *to, size_t maxsize, const char *from) {
      size_t len;

      len = strlen(from);
      if (maxsize <= len) {
	    return 0;
      } else {
	    memcpy(to, from, len+1);
	    return len;
      }
      
}
