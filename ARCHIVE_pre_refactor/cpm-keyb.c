#include <stdio.h>

/* BDOS function codes */
#define CONSOLE_STATUS 0x0B
#define CONSOLE_INPUT 0x06

void main() {
    int ch;

    while (1) {

        /* Check console status */
        if (bdos(CONSOLE_STATUS, 0) != 0) {

            /* Character available - read it */
            ch = bdos(CONSOLE_INPUT, 0xFF);

            if (ch == 'q' || ch == 'Q') {
		break;

            } else if (ch>32) {

		printf("Key pressed: %c\n", ch);

            } else {

	        /* No key available - continue with other tasks */
        	printf(".");
	   }
        } else {
	    printf("Waiting for a key ...\n");
	}
    }

    return;
}
