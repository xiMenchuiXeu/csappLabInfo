#include "sio.h"

/*************************************************************
 * The Sio (Signal-safe I/O) package - simple reentrant output
 * functions that are safe for signal handlers.
 *************************************************************/

/*begin sio func define*/

/*private sio func only used in this file*/
static void sio_reverse(char s[])/*reverse a string*/
{
    int c, i, j;
    for(i = 0, j = strlen(s) - 1; i < j; i++, j--){
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

static void sio_ltoa(long v, char s[], int b)/*convert long to base b string*/
{
    int c, i = 0;
    do{
        s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
    }while((v /= b) > 0);
    s[i]= '\0';
    sio_reverse(s);
}

static size_t sio_strlen(char s[])/*return length of string*/
{
    int i = 0;
    while(s[i] != '\0')++i;
    return i;
}
/*end private sio func*/

/*public func prototype*/
inline ssize_t sio_puts(char s[])/*put string*/
{
    return  write(STDOUT_FILENO, s, sio_strlen(s)); /*write safe in signal handle; only*/
}
inline ssize_t sio_putl(long v)/*put long*/
{
    char s[128];
    sio_ltoa(v,s,10);
    return sio_puts(s);
}
inline void sio_error(char s[])/*put error message and exit*/
{
    sio_puts(s);
    _exit(1);
}

/*public sio func define end*/

/*wrappers for the sio routines*/
ssize_t Sio_putl(long v){
    ssize_t n;
    if((n = sio_putl(v)) < 0)sio_error("Sio_putl error");
    return n;
}
ssize_t Sio_puts(char s[])
{
    ssize_t n;

    if ((n = sio_puts(s)) < 0) sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[]) { sio_error(s); }
