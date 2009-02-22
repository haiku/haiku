/*
 * dpms CLI tool
 * (c) François Revol, revol@free.fr
 */

#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Accelerant.h>
#include <Screen.h>

int usage(char *prog)
{
	printf("%s on|standby|suspend|off|caps|state\n", prog);
	printf("on|standby|suspend|off\tsets corresponding state\n");
	printf("caps\tprints capabilities\n");
	printf("state\tprints the current state\n");
	return 1;
}

int main(int argc, char **argv)
{
	BApplication app("application/x-vnd.Haiku.dpms");
	BScreen bs;
	if (argc < 2)
		return usage(argv[0]);
	if (!strcmp(argv[1], "on"))
		bs.SetDPMS(B_DPMS_ON);
	else if (!strcmp(argv[1], "standby"))
		bs.SetDPMS(B_DPMS_STAND_BY);
	else if (!strcmp(argv[1], "suspend"))
		bs.SetDPMS(B_DPMS_SUSPEND);
	else if (!strcmp(argv[1], "off"))
		bs.SetDPMS(B_DPMS_OFF);
	else if (!strcmp(argv[1], "caps")) {
		uint32 caps = bs.DPMSCapabilites(); // nice typo...
		printf("dpms capabilities: %s%s%s%s\n", (caps & B_DPMS_ON)?("on"):(""),
							(caps & B_DPMS_STAND_BY)?(", standby"):(""),
							(caps & B_DPMS_SUSPEND)?(", suspend"):(""),
							(caps & B_DPMS_OFF)?(", off"):(""));
	} else if (!strcmp(argv[1], "state")) {
		uint32 st = bs.DPMSState();
		printf("%s\n", (st & B_DPMS_ON)?("on"):
				((st & B_DPMS_STAND_BY)?("standby"):
				 ((st & B_DPMS_SUSPEND)?("suspend"):
				  ((st & B_DPMS_OFF)?("off"):("?")))));
	} else
		return usage(argv[0]);
	return 0;
}
