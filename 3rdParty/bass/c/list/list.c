/*
	WASAPI device list example
	Copyright (c) 2010-2014 Un4seen Developments Ltd.
*/

#include <stdio.h>
#include "basswasapi.h"

void main()
{
	BASS_WASAPI_DEVICEINFO di;
	int a;
	for (a=0;BASS_WASAPI_GetDeviceInfo(a,&di);a++) {
		printf("dev %d: %s\n\tid: %s\n\ttype: ",a,di.name,di.id);
		switch (di.type) {
			case BASS_WASAPI_TYPE_NETWORKDEVICE:
				printf("Remote Network");
				break;
			case BASS_WASAPI_TYPE_SPEAKERS:
				printf("Speakers");
				break;
			case BASS_WASAPI_TYPE_LINELEVEL:
				printf("Line");
				break;
			case BASS_WASAPI_TYPE_HEADPHONES:
				printf("Headphones");
				break;
			case BASS_WASAPI_TYPE_MICROPHONE: 
				printf("Microphone");
				break;
			case BASS_WASAPI_TYPE_HEADSET:
				printf("Headset");
				break;
			case BASS_WASAPI_TYPE_HANDSET:
				printf("Handset");
				break;
			case BASS_WASAPI_TYPE_DIGITAL:
				printf("Digital");
				break;
			case BASS_WASAPI_TYPE_SPDIF:
				printf("SPDIF");
				break;
			case BASS_WASAPI_TYPE_HDMI:
				printf("HDMI");
				break;
			default:
				printf("Undefined (%d)",di.type);
		}
		printf("\n\tflags:");
		if (di.flags&BASS_DEVICE_LOOPBACK) printf(" loopback");
		if (di.flags&BASS_DEVICE_INPUT) printf(" input");
		if (di.flags&BASS_DEVICE_ENABLED) printf(" enabled");
		if (di.flags&BASS_DEVICE_DEFAULT) printf(" default");
		if (di.flags&BASS_DEVICE_UNPLUGGED) printf(" unplugged");
		if (di.flags&BASS_DEVICE_DISABLED) printf(" disabled");
		printf(" (%d)\n",di.flags);
	}
}
