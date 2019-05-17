
#define SHORTCODE
#define JOBS
#define LIST
#define STAB

#define MINN  3
#define MAXN 99
#define MINK  2
#define MAXK  9
#define MINT  3
#define MAXT  9

#define NOTSET 127
#define COUNTSTEP 100

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __UNIX__
#include <sys/times.h>
#include <time.h>
#include <limits.h>
#else
#include <time.h>
#define DOSTIME
#endif

#ifdef __osf__ /* OSF auf den alphas */
#define time_factor CLK_TCK
#endif

#ifdef __sgi /* Silicon Graphics */
#define time_factor CLK_TCK 
#define SIGNED 
#endif

#ifdef sun /* Sun */
#define time_factor 60
#endif

#ifdef __hpux /* Hewlet Packard */
#define time_factor CLK_TCK
#endif

#ifdef __dec /* DEC */
#define time_factor 60
#endif

#ifdef __ultrix /* DEC */
#define time_factor 60
#endif        /* thanks to Gunnar */

#ifdef time_factor
#define UNIXTIME
#endif

#ifdef DOSTIME
#define time_factor CLK_TCK
#endif

#ifdef SHORTCODE
#define APP "scd"
#else
#define APP "lst"
#endif

typedef int SCHAR;
typedef unsigned long ULONG;
typedef unsigned int UINT;


SCHAR default_mid_max(SCHAR n, SCHAR k,SCHAR t);
SCHAR default_splitlevel(SCHAR n, SCHAR k, SCHAR t);
void ordstart(SCHAR _n, SCHAR _k, SCHAR _t, SCHAR _mid_max,
	      SCHAR _splitlevel, UINT _jobs, UINT _jobnr,
	      FILE* _lstfile, FILE* _autfile, FILE* _ergfile,
	      ULONG _tostore, ULONG _toprint, ULONG _count,
	      SCHAR _storeall, SCHAR _printall, ULONG* _anz);
