/*

	wetbulbpi.h

*/
#include <math.h>
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
	defines
*/
#define true 1
#define false 0

/*
	constants
*/

/*
	typedefs
*/
typedef unsigned char boolean;

/*
	structs
*/
struct config_t
{
	char th_sensor[6];
	char thb_sensor[6];
	boolean snow_flag;
	boolean write_log;
	char log_file_name[FILENAME_MAX];
	uint16_t sleep_seconds;
};

/*
	function prototypes
*/
double calcwetbulb (double Ctemp, double MBpressure, double rh);
uint32_t get_seconds_since_midnight (void);
void writelog (char *logfilename, char *process_name, char *message);
void display_usage(char *myname);
int get_configuration(struct config_t *config, char *path);