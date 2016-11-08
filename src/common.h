#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#ifdef DEDI
#include <dlfcn.h>
#else
#include <SDL.h>
#endif

#include "littlesms.h"

