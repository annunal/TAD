#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>
#include <math.h>

//bool compString(char *s1, char *s2);
int sl(char *a);
char * cltrim(char *a);
char * _mid(char * b,unsigned int n, unsigned int l);
void   cmid(char * b,unsigned int n, unsigned int l, char *a);
void   setmid (char *a, unsigned int n, unsigned int l, char * b);

char * crtrim(char *a);
void   cstring (int ncount,char *a,char *b);
char * replace_str(char *str, char *orig, char *rep);
void replace(char *st, char *orig, char *repl, char * outString);
char * ctrim(char *a);
char * cleanValue1(char * label);
void   copystring ( char *source, char *dest);
char * _AddString(char * s1, char * s2, char * s3 , char * s4 , char * s5 );
void   AddString_s(char * sout, char * s1, char * s2, char * s3 , char * s4 , char * s5 );
