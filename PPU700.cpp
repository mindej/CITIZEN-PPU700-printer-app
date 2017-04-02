//============================================================================
// Name        : PPU700.c
// Author      : Mindaugas Jarminas
// Version     :
// Copyright   : Mindaugas Jaraminas
// Description : PPU700 implementation in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <string.h>

static void endPrint(int fd);
static void printBuffer(int fd) {
	unsigned char tmp[] = { 0x00, 0x1B, 0x40 };
	write(fd, tmp, sizeof(tmp));
}
static void barCode(int fd, int higth, int with, char Align, char * code) {
	char _barCode[] = { 0x1D, 'h', 30, 0x1D, 'w', 2, 0x1B, 'a', '0', 0x1D, 'H',
			2, 0x1D, 'k', 4 };
	printBuffer(fd);
	_barCode[2] = higth;
	_barCode[5] = with;
	_barCode[8] = Align;
	write(fd, _barCode, sizeof(_barCode));
	dprintf(fd, "%s\0", code);
	printBuffer(fd);
}

static int initPort(int fd) {

	//http://www.easysw.com/~mike/serial/serial.html

	struct termios options;
	// Get the current options for the port...
	tcgetattr(fd, &options);
	// Set the baud rates to 9600...
	cfsetispeed(&options, B19200);
	cfsetospeed(&options, B19200);
	// Enable the receiver and set local mode...
	options.c_cflag |= (CLOCAL | CREAD);

	//No parity (8N1):

	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	options.c_cflag |= CRTSCTS; /* flow control */

	options.c_oflag = 0; // Raw output_file


	tcflush(fd, TCIFLUSH);

	// Set the new options for the port...
	tcsetattr(fd, TCSANOW, &options);
	return 1; // TODO padaryti normalu kaidu tikrinima
}
static void help() {
	puts("Pagalba:");
	puts("   -h arba --help bus rodomas sis informacinis pranesimas\n");
	puts("   -d arba --device irenginio porto nustatymia");
	puts("          PVZ: -d /dev/ttyS0\n");
	puts("   -f arba --file cekio tekstinis failas");
	puts("          PVZ: -f ckeio_testas.txt\n");
	puts("   Kad programa viektu reikia nurodyti devaisa ir faila");
	exit(EXIT_SUCCESS);
}
int main(int argc, char *argv[]) {
	char *dev = NULL;
	char *file = NULL;
	if (argc == 1) {
		puts("CITIZEN PPU700 printeris");
		puts(
				"       Nenurodete jokiu parametru: meginkite -h arba --help kad gautumete");
		puts("       daugiau informacijos");
		exit(EXIT_SUCCESS);
	}
	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-h") == 0) | ((strcmp(argv[i], "--help") == 0))) {
			puts("CITIZEN PPU700 printeris");
			help();
		}
		if ((strcmp(argv[i], "-d") == 0) | ((strcmp(argv[i], "--device") == 0))) {
			dev = argv[i + 1];
		}
		if ((strcmp(argv[i], "-f") == 0) | ((strcmp(argv[i], "--file") == 0))) {
			file = argv[i + 1];
		}
	}
	if (dev == NULL | file == NULL) {
		puts("CITIZEN PPU700 printer");
		puts("   Input params are needed");
		help();
	}
	//	exit(0);
	int ret;
	int serialFD = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (serialFD == -1) {
		puts("Can not oppen serial port");
		return EXIT_FAILURE;
	}
	initPort(serialFD);
	char status[] = {0x1B, 0x76};
	char fontas[] = { 0x1B, 0x4D, '0' };
	char fontoDydis[] = { 0x1D, 0x21, 0x0 };
	char bold[] = { 0x1B, 0x47, 0x00 };
	char underline[] = { 0x1B, 0x2D, '0' };
	char aligning[] = { 0x1B, 0x61, '0' };
	char characterCodeTable[] = { 0x1B, 0x74, 0x00 };

	char beep[] = { 0x1B, 0x1E, 0x00 };

	/* chek status*/
	unsigned char retStatus = 0xFF;
	write(serialFD, status, sizeof(status));
	usleep(500000);
	ret = read(serialFD, &retStatus, 1);
	if(ret != 1){
		fprintf(stdout, "Read Error ret = %d, 0x%02X\n", ret, retStatus);
		exit(EXIT_FAILURE);
	}
	if (retStatus != 0){
		fprintf(stdout, "Printer status error: 0x%02X\n", retStatus);
		switch (retStatus) {
			case 2:
				puts("Error receipt is in printer ");
				break;
			default:
				exit(EXIT_FAILURE);
				break;
		}
	}

	write(serialFD, beep, sizeof(beep)); //beep

	/* INIT */
	unsigned char init[] = { 0x1B, '$', 0, 0 };
	write(serialFD, init, sizeof(init));

	FILE * pFile;
	char mystring[1000];
	pFile = fopen(file, "r");
	if (pFile == NULL)
		perror("Error opening file");

	int line = 0;

	while (!feof(pFile)) {
		fgets(mystring, sizeof(mystring), pFile);
		line++;
		// cia turetu buti jieskojimas specialiu cimboliu
		int len = strlen(mystring);
		for (int i = 0; i < len; i++) {
			if (mystring[i] == '<') {
				switch (mystring[i + 1]) {
				case 'f': // fonto kietimas
					char size;
					ret = sscanf(&mystring[i + 3], "%c", &size);
					if (ret < 1) {
						fprintf(stderr, "Font syntax error at %d line\n",
								line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}

					fontas[2] = size;
					write(serialFD, fontas, sizeof(fontas));
					i += 4;
					break;
				case 's':
					int horizontalMag;
					int verticalMag;
					unsigned char ff;
					ret = sscanf(&mystring[i + 3], "%d %d", &horizontalMag,
							&verticalMag);
					if (ret < 2) {
						fprintf(stderr,
								"Font size error at %d line\n",
								line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}
					ff = ((horizontalMag << 4) & 0xF0) | (verticalMag & 0x0F);
					fontoDydis[2] = ff;
					write(serialFD, fontoDydis, sizeof(fontoDydis));
					i += 6;
					break;
				case 'b':
					int boldas;
					ret = sscanf(&mystring[i + 3], "%d", &boldas);
					if (ret < 1) {
						fprintf(stderr, "Bold syntax error at %d line\n",
								line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}
					bold[2] = boldas;
					write(serialFD, bold, sizeof(bold));
					i += 4;
					break;
				case 'u':
					char und;
					ret = sscanf(&mystring[i + 3], "%c", &und);
					if (ret < 1) {
						fprintf(stderr,
								"Underline syntax error at %d late\n",
								line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}
					underline[2] = und;
					write(serialFD, underline, sizeof(underline));
					i += 4;
					break;
				case 'a':
					char alig;
					ret = sscanf(&mystring[i + 3], "%c", &alig);
					if (ret < 1) {
						fprintf(stderr,
								"Aligment  syntax error at %d file\n",
								line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}
					aligning[2] = alig;
					write(serialFD, aligning, sizeof(aligning));
					i += 4;
					break;
				case 'h':
					int higth;
					int with;
					//					char alig;
					char code[128];
					ret = sscanf(&mystring[i + 3], "%d %d %c>%s", &higth,
							&with, &alig, &code);
					if (ret < 4) {
						fprintf(stderr,
								"Barcode syntax error in %d line\n", line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}
					barCode(serialFD, higth, with, alig, code);
					i += 128;
					break;
				case 'c':
					int ck;
					ret = sscanf(&mystring[i + 3], "%d", &ck);
					if (ret < 1) {
						fprintf(stderr,
								"Selecting the character code table syntax error at %d line\n", line);
						endPrint(serialFD);
						exit(EXIT_FAILURE);
					}
					characterCodeTable[2] = ck;
					write(serialFD,characterCodeTable, sizeof(characterCodeTable));
					i += 3;
					for( ;;){
						if (mystring[i] != '>'){
							i ++;
						} else {
							i ++;
							break;
						}
					}
					break;
				case '<':
					dprintf(serialFD, "<");
					i++;
					break;
				default:
					fprintf(stderr, "Function not implemented at line:%d\n",
							line);

					break;
				}
			} else {
				write(serialFD, &mystring[i], 1);
			}

		}
//		fprintf(stdout, "%s", mystring);
		//		dprintf(serialFD, "%s", mystring);
	}
	fclose(pFile);

	endPrint(serialFD);
	return EXIT_SUCCESS;
}
void endPrint(int fd) {

	dprintf(fd, "\n\n\n\n");
	unsigned char beep[] = { 0x1B, 0x1E, 0x00 };
	unsigned char cut[] = { 0x1B, 0x40, 0x1B, 0x69, };
	write(fd, cut, sizeof(cut)); // cut

	write(fd, beep, sizeof(beep)); //beep

	close(fd);
}
