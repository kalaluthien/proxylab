#include <stdio.h>

/* global variables */

static unsigned int debug_count;
static unsigned int debug_indent;
static char debug_flag;

/* debug functions */

/* 디버그 기능을 켠다. */
void d_on(void)
{
  debug_flag = 1;
}

/* 디버그 기능을 끈다. */
void d_off(void)
{
  debug_flag = 0;
}

/* 보기 펀하도록 indent를 출력한다. */
static void printi(void)
{
  int i;
  for (i = 1; i < debug_indent; i++)
    printf(" | ");
  printf(" ");
}

/* 함수의 맨 앞에 사용. */
void d_call(const char * str)
{
  if (debug_flag)
  {
    debug_indent++;
    printi();
    printf("\e[1;32m[%s called...]\e[0m\n", str);
  }
}

/* 함수가 반환되기 직전에 사용. */
void d_ret(const char * str)
{
  if (debug_flag)
  {
    printi();
    printf("\e[1;32m[%s returning...]\e[0m\n", str);
    debug_indent--;
  }
}

/* "DEBUG POINT"와 deb() 호출 횟수를 출력한다. */
void d_num(void)
{
  if (debug_flag)
  {
    printi();
    printf("* DEBUG POINT .......... (%d)\n", ++debug_count);
  }
}

/* deb() 호출 횟수를 출력한다. */
int d_cnt(void)
{
  return debug_count;
}

/* 입력받은 문자열을 '*'과 함께 출력한다. */
void d_msg(const char * str)
{
  if (debug_flag)
  {
    printi();
    printf("* \e[1;31m%s\n\e[0m", str);
  }
}

/* 입력받은 문자열을 '@'와 함께 출력한다. */
void d_alt(const char * str)
{
  if (debug_flag)
  {
    printi();
    printf("@ \e[1;33m%s\n\e[0m", str);
  }
}

/* 입력받은 문자열과 정수값을 출력한다. */
void d_prt(const char * str, long long num)
{
  if (debug_flag)
  {
    printi();
    printf("* \e[1;31m%s: \e[0m%lld\n", str, num);
  }
}

/* 입력받은 문자열과 포인터값을 출력한다. */
void d_prtp(const char * str, void * ptr)
{
  if (debug_flag)
  {
    printi();
    printf("* \e[1;31m%s: \e[0m%p\n", str, ptr);
  }
}

/* 입력받은 문자열 둘을 출력한다. */
void d_str(const char * str1, const char * str2)
{
  if (debug_flag)
  {
    printi();
    printf("* \e[1;31m%s: \e[0m%s\n", str1, str2);
  }
}

/* 'c'를 입력할 때까지 Busy waiting을 수행한다. */
void d_brk(void)
{
  if (debug_flag)
  {
    char c;

    printi();
    printf("* BREAK POINT\n");
    printi();
    printf("* ");
    while ((c = getchar()) != 'c') ;
  }
}
