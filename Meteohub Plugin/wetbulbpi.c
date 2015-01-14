/*

wetbulbpi.c

meteohub plug-in weather staion to calculate web-bulb temperature from weather station sensor data.
	
Written:	26-March-2013 by Fred T. Trimble ftt@smtcpa.com

Modified:	27-Dec-2013   by Fred T. Trimble ftt@smtcpa.com Added sleep before main loop to allow datalogger to get
								started after a meteohub reboot.
								
Modified:	20-Sep-2014   by Fred T. Trimble ftt@smtcpa.com Added initial sleep to keep polling in intrval boundry								
	
*/

// includes
#include "wetbulbpi.h"
#define VERSION "0.3"
#define SNOW_WET_BULB 1.2 // treshold temp in Celsius when snow is likely

/*
main program
*/
int main (int argc, char *argv[])
{
	char log_file_name[FILENAME_MAX] = "";
	char config_file_name[FILENAME_MAX] = "";
	strcpy(config_file_name, argv[0]);
	strcat(config_file_name, ".conf");
	static const char *optString = "b:d:h?Lst:";
	uint32_t seconds_since_midnight = 0;

	struct config_t config;
	
	int read_count = 0; // number of values read from pipe
	char mh_url[256] = "'http://127.0.0.1/meteolog.cgi?mode=data&sensor="; // url to meteohub datalogger
	char wget_cl[256]= "wget -q -O /dev/stdout "; // wget commandline & options
	char data[60] = "";
	FILE *wget_stdin;
	char *message_buffer;
	
	double baro = 0.0;
	double temp = 0.0;
	double rel_hum = 0.0;
	double wetbulb_temp = 0.0;
	
	// set default values for command line/config options
	strcpy(config.th_sensor, "th0");
	strcpy(config.thb_sensor, "thb0");
	config.snow_flag = false;
	config.write_log = false;
	strcpy(log_file_name, argv[0]);
	strcat(log_file_name, ".log");
	strcpy(config.log_file_name, log_file_name);
	config.sleep_seconds = 60; // 1 min is default sleep time;

	// get cofig options
	if(!get_configuration(&config, config_file_name))
	{
		fprintf(stderr,"\nno readable .conf file found, using values from command line arguments\n");
	}
	
	// get command line options, will override values read from .conf file
	int opt = 0;	
	while((opt = getopt(argc, argv ,optString)) != -1)
	{
		switch(opt)
		{
		case 'b':
			strcpy(config.thb_sensor, optarg);
			break;
		case 'd':
			strcpy(config.th_sensor, optarg);
			break;
		case 'h':
		case '?':
			display_usage(argv[0]);
			break;
		case 'L':
			config.write_log = true;
			break;
		case 's':
			config.snow_flag = true;
			break;
		case 't':
			config.sleep_seconds = (uint16_t)atoi(optarg);
			break;
		}
	}

	message_buffer = (char *)malloc(sizeof(char) * 256);
	if(message_buffer == NULL)
	{
		fprintf(stderr, "can't allocate dynamic memory for buffers\n");
		exit (EXIT_FAILURE);
	}
	
	// formulate wget meteohub datalogger command line
	strcat(mh_url, config.th_sensor);
	strcat(mh_url, "&sensor=");
	strcat(mh_url, config.thb_sensor);
	strcat(mh_url, "'");
	strcat(wget_cl, mh_url);

	if(config.write_log)
	{
		sprintf(message_buffer, "Using Temp/Humidity sensor: %s, Baro sensor: %s", config.th_sensor, config.thb_sensor);
		writelog (config.log_file_name, argv[0], message_buffer);
		sprintf(message_buffer, "Polling interval (seconds): %d", config.sleep_seconds);
		writelog (config.log_file_name, argv[0], message_buffer);
		sprintf(message_buffer, "Meteohub datalogger commandline: %s", wget_cl);
		writelog (config.log_file_name, argv[0], message_buffer);
	}
	
	seconds_since_midnight = get_seconds_since_midnight();

	if(config.sleep_seconds - (seconds_since_midnight % config.sleep_seconds) > 0)
	{
		sprintf(message_buffer,"Initial sleep: %d", config.sleep_seconds - (seconds_since_midnight % config.sleep_seconds));
		writelog(config.log_file_name, argv[0], message_buffer);
		sleep(config.sleep_seconds - (seconds_since_midnight % config.sleep_seconds)); // start polling on an even boundry of the specified polling interval
	}

	sleep(config.sleep_seconds); // pause to allow meteohub datalogger a chance to get started after a reboot
	
	while(true)
	{
		// open pipe to read sensor values from meteohub datalogger interface via wget
		if((wget_stdin = popen(wget_cl, "r")) != NULL)
		{
			// parse values
			while(fgets(data, sizeof(data), wget_stdin) != NULL) // read one line of results from meteohub datalogger results
			{  
				if(strstr(data, config.th_sensor)) // thx?
					read_count += sscanf(data, "%*s %*s %lf %lf %*f", &temp, &rel_hum); // parse temp and rel humidity from thx
				else
				if(strstr(data, config.thb_sensor)) // thbx?
					read_count += sscanf(data, "%*s %*s %*f %*f %*f %lf %*f", &baro); // parse baro pressure from thbx
			}
			// flush and close pipe
			fflush(wget_stdin);
			pclose(wget_stdin);
			
			// output only if we got everything we needed from the pipe
			if(read_count == 3)
			{
				// calculate & output wet-bulb temp as t0 sensor
				wetbulb_temp = calcwetbulb(temp, baro, rel_hum);
				printf("t0 %1.0lf\n", round(wetbulb_temp * 10.0));
				// output temp, humidity & pressure used to calculate wet-bulb temm to thb0 sensor
				printf("thb0 %1.0lf %1.0lf %1.0lf\n", round(temp * 10.0), round(rel_hum), round(baro * 10.0));
				if(config.snow_flag)
					// output snow possible flag as data0 sensor. Set to 1 for snow, 0 for rain
					printf("data0 %d\n",wetbulb_temp < SNOW_WET_BULB? 100: 0); 
				// reset the data space for next read...
				strcpy(data, "");
				read_count = 0;
			}
			else
			{
				if(config.write_log)
					writelog (config.log_file_name, argv[0], "Error getting values piped from meteohub datalogger");
				exit (EXIT_FAILURE);
			}
			fflush(stdout);
		}
		else
			if(config.write_log)
				writelog (config.log_file_name, argv[0], "Error opening pipe to meteohub datalogger");
		
		seconds_since_midnight = get_seconds_since_midnight();
		sleep(config.sleep_seconds - (seconds_since_midnight % config.sleep_seconds)); // sleep just the right amount to keep on boundry	
		//sleep(config.sleep_seconds);
	}
}

// get seconds since midnight local time
uint32_t get_seconds_since_midnight (void)
{
	time_t t;
	struct tm *localtm;

	t = time(NULL);
	localtm = localtime(&t);

	return localtm->tm_sec + localtm->tm_min * 60 + localtm->tm_hour * 3600;
}
	
// wrire formatted messages to a log file named in logfilename
void writelog (char *logfilename, char *process_name, char *message)
{
	char timestamp[25];
	time_t t;
	struct tm *localtm;
	FILE *stream;

	t = time(NULL);
	localtm = localtime(&t);

	strftime(timestamp, sizeof(timestamp), "%d.%m.%Y %T", localtm);

	stream = fopen(logfilename, "a");
	fprintf(stream, "%s (%s): %s.\n", process_name, timestamp, message);
	fprintf(stderr, "%s (%s): %s.\n", process_name, timestamp, message);
	fclose(stream);
}

void display_usage(char *myname)
{
	fprintf(stderr, "wetbulbpi Version %s - Meteohub Plug-In for Wet-Bulb Temperature Calculation\n", VERSION);
	fprintf(stderr, "Usage: %s [-d temp_sensor] [-b baro_sensor] [-s] [-L] [-t sleep_time]\n", myname);
	fprintf(stderr, "  -d temp_sensor Dry-Bulb Temp+Humidity sensor to read live values from.\n");
	fprintf(stderr, "                 default value is th0\n");
	fprintf(stderr, "  -b baro_sensor Barometric pressure sensor to read live values from.\n");
	fprintf(stderr, "                 default value is thb0\n");
	fprintf(stderr, "  -s             Write 'snow likely' value to data0 sensor\n");	
	fprintf(stderr, "  -L             Write messages to log file.\n");
	fprintf(stderr, "  -t sleep_time  Number of seconds to sleep between polling the Pentametric.\n");
	exit(EXIT_FAILURE);
}
