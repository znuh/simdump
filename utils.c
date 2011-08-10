#include "utils.h"

char *str_append(char *dst, char *src)
{

	while (*src) {
		*dst = *src;
		dst++;
		src++;
	}

	return dst;
}

char *hex_append(char *dst, uint8_t * src, int len)
{

	while (len--) {
		sprintf(dst, "%02x", *src);
		src++;
		dst += 2;
	}

	return dst;
}

void skip_char(char sc, char **buf)
{
	char *ptr = *buf;

	while (*ptr == sc)
		ptr++;

	*buf = ptr;
}

void find_char(char fc, char **buf)
{
	char *ptr = *buf;

	while (*ptr != fc)
		ptr++;

	*buf = ptr;
}

void hexdump(FILE * fl, uint8_t * buf, int len, int space)
{
	char format[] = "%02x ";

	if (!space)
		format[4] = 0;

	while (len--) {
		fprintf(fl, format, *buf);
		buf++;
	}

	fflush(fl);
}

int hex2bin(char *hex, uint8_t * bin)
{
	int res = 0;

	while (*hex) {
		char *n;

		*bin = strtoul(hex, &n, 16);

		bin++;
		res++;

		while (*n == ' ')
			n++;

		hex = n;
	}

	return res;
}

char *parse_hex(char *buf, uint8_t * dst, int len)
{
	char tmp[3] = { 0, 0, 0 };

	while (len--) {
		skip_char(' ', &buf);
		tmp[0] = buf[0];
		tmp[1] = buf[1];
		*dst = strtoul(tmp, NULL, 16);
		buf += 2;
		dst++;
	}
	return buf;
}

void dump_hex(uint8_t * src, int len)
{
	while (len--) {
		printf("%02x ", *src);
		src++;
	}
	printf("\n");
}

void dump_bytes_to_file(char *fname, uint8_t *d, int len) {
	FILE *fl=fopen(fname,"w");
	int i;

	for(i=0; i<len; i++)
		fprintf(fl, "%c",d[i]);
	fclose(fl);
}
