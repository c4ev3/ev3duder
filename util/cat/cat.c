/** @file 	cat.c
  *	@brief 	cat(1) wrapper for cat'ing to the ev3 LCD
  * @author Ahmad Fatoum
  */
#include "ev3_lcd.h"
#define _GNU_SOURCE
#include "unistd.h"

#define PTEMP_FAILURE_RETRY(expr) ({\
       	void * __r;\
		do { errno = 0;\
		__r = (void*) expr;\
		}while (__r == 0 && errno == EINTR);\
		 __r; })
static int run(const char *cmd);

int main(int argc, char *argv[])
{
	char buffer[80];
	size_t len;
	LcdInit();
	LcdClean();
	LcdPrintf(1, ":)");
	LcdPrintf(1, ":)");
	while (PTEMP_FAILURE_RETRY(fgets(buffer, sizeof buffer, stdin)))
	{
		printf("%s", buffer);
		LcdPrintf(1, "%s", buffer);
	}
	sleep(7);
	LcdPrintf(1, "-----");
	printf("-----");
	
#if 0
	if (argc == 1)
		argv[1] = "ls /";
	run(argv[1]);
#endif
	LcdExit();
	return 0;
}
static int run(const char *cmd)
{
	FILE *pp = popen(cmd,"r");

	if(!pp) {
		LcdPrintf(1, "Error popen \n");
		exit(1);
	}
	char buffer[100];
	int count = 0;
	while(PTEMP_FAILURE_RETRY(fgets(buffer, 100, pp)))
		count += LcdPrintf(1, "%s", buffer);

	int ret = pclose(pp);
	LcdPrintf(1, "\n--- %d characters read. return value = %d. errno=%d\n---\n", count, ret, errno);
	return ret;
}


