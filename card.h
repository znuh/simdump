
#ifndef CARD_H
#define CARD_H

int get_status(uint8_t *buf, int len);
int select_fid(uint16_t fid, uint8_t *resp, int resp_len);
int read_file(uint8_t *buf, int len);
int read_record(uint8_t *buf, int len);
int read_binary(uint8_t *buf, int len);
int auth(char *pin);
int usim_detect(void);

#endif