#include "rc.h"
#include <shared.h>

int get_lanports_status(void)
{
	int r = 0;
	FILE *f;
	char s[128], a[16];

	if ((f = popen("/usr/sbin/robocfg showports", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if ((sscanf(s, "Port 1: %s", a) == 1) ||
			    (sscanf(s, "Port 2: %s", a) == 1) ||
			    (sscanf(s, "Port 3: %s", a) == 1) ||
			    (sscanf(s, "Port 4: %s", a) == 1)) {
				if (strncmp(a, "DOWN", 4)) {
					r++;
				}
			}
		}
		pclose(f);
	}

	return r;
}

int blink_br_main(int argc, char *argv[])
{
	int model;

	/* Fork new process, run in the background (daemon) */
	if (fork() != 0) return 0;
	setsid();
	signal(SIGCHLD, chld_reap);

	/* get Router model */
	model = get_model();

	while(1) {
		if (model == MODEL_WS880 ||
		    model == MODEL_RTN18U) {
			if (get_lanports_status()) {
				led(LED_BRIDGE, LED_ON);
			}
			else {
				led(LED_BRIDGE, LED_OFF);
			}
		}
		/* sleep 3 sec before check again */
		sleep(3);
	}
}
