#ifndef DSTRUCTS_H
#define DSTRUCTS_H

typedef struct df_info_s {
	uint16_t rfu;
	uint16_t free_mem;
	uint16_t fid;
	uint8_t fid_type;
	//
	uint8_t rfu2[5];
	uint8_t len;
	// GSM specific data
	uint8_t file_chars;
	uint8_t df_entries;
	uint8_t ef_entries;
	uint8_t admin_codes;
	uint8_t rfu3;
	uint8_t chv1_status;
	uint8_t unblock_chv1;
	uint8_t chv2_status;
	uint8_t unblock_chv2;
	uint8_t rfu4;
	uint8_t rfu5[11];
} __attribute__((packed)) df_info_t;

typedef struct ef_info_s {
	uint16_t rfu;
	uint16_t file_size;
	uint16_t fid;
	uint8_t fid_type;
	//
	uint8_t inc_allowed;
	uint8_t access_cond[3];
	uint8_t file_status;
	uint8_t ext_length;
	uint8_t ef_struct;
	uint8_t rec_len;
	// ...
} __attribute__((packed)) ef_info_t;

typedef struct node_s {
	uint16_t fid;
	struct node_s *parent;
	struct node_s *next;
	struct node_s *sub_df;
	struct node_s *sub_ef;
	struct node_s *ref;
	int df_cnt;
	int ef_cnt;
	int is_ef;
	union {
		df_info_t df_info;
		ef_info_t ef_info;
		char name[32];
	} u;
} node_t;

#endif
