#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#ifndef RS232_IF_H
#define RS232_IF_H

int rs232_setbaud(int fd, int baud);
int rs232_init(char *dev, int baud, int parity, int time);
int rs232_close(int fd);
int rs232_flush(int fd);
int rs232_read(int fd, uint8_t * d, int len);
int rs232_send(int fd, uint8_t * d, int len);
int rs232_set_rts(int fd, int rts);

#endif
