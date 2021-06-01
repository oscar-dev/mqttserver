#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

int logger(const char* stexto, ...)
{
    va_list args;
    char* logfile=getenv("LOG_FILE");
    FILE* file=stdout;
    time_t t = time(NULL);

    if( logfile ) {
        file=fopen(logfile, "a");
	if( !file ) file = stdout;
    }

    va_start(args, stexto);

    struct tm* now = localtime(&t);

    fprintf(file, "%02d-%02d-%04d %02d:%02d:%02d - ", now->tm_mday, 
                                                        (now->tm_mon+1),
						       	(now->tm_year+1900),
						       	now->tm_hour,
						       	now->tm_min,
						       	now->tm_sec);

    vfprintf(file, stexto, args);

    fprintf(file, "\n");

    va_end(args);

    fflush(file);

    if( file != stdout) fclose(file);

    return 0;
}
