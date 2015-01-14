/*
	wetbulbtemp.c

	Calculate wet-bulb temperature for a meteohub sensor from
	dry-bulb temperature, barometric pressure and relative humdity.
	
	Written 22-March-2013 by Fred T. Trimble (ftt@smtcpa.com)
	
	Adapted from java script code found in the source of the web page 
	on the NOAA web site at:
	
	http://www.srh.noaa.gov/epz/?n=wxcalc_rh

*/

#include <math.h>

double esubs (double Ctemp)
{
	return (6.112 * exp ((17.67 * Ctemp) / (Ctemp + 243.5)));
}

double invertedRH (double Es, double rh)
{
	return (Es * (rh / 100.0));
}

double calcwetbulb (double Ctemp, double MBpressure, double rh)
{
	double Edifference = 1.0;
	double Twguess = 0.0;
	double incr = 10.0;
	int previoussign = 1;
	
	double E2 = invertedRH (esubs (Ctemp), rh);
	
	double Ewguess = 0.0;
	double Eguess = 0.0;
	int cursign = 0;
	
	while (fabs (Edifference) > 0.05)
	{
		Ewguess = esubs (Twguess);
		Eguess = Ewguess - MBpressure * (Ctemp - Twguess) * 0.00066 * (1 + (0.00115 * Twguess));
		Edifference = E2 - Eguess;
		
		if (Edifference == 0.0)
			break;
		else
		{
			if (Edifference < 0.0)
			{
				cursign = -1;
				if (cursign != previoussign)
				{
					previoussign = cursign;
					incr = incr / 10.0;
				}
				else
					incr = incr;
			}
			else
			{
				cursign = 1;
				if (cursign != previoussign)
				{
					previoussign = cursign;
					incr = incr / 10.0;
				}
				else
					incr = incr;
			}
		}
		Twguess = Twguess + incr * previoussign;
	}
	return Twguess;
}