#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <endian.h>
#include <time.h>
#include <assert.h>

#include "terminal.h"
#include "utils.h"

#include "dstructs.h"

#include "card.h"

char ef_struct[][16] = {"transp", "linear", "??????", "cyclic"};

int class_byte = 0xa0;

#define SIM_MF	0x3f00

void indent(int i, FILE *fl) {
	while(i--)
		putc(' ',fl);
}

void dump_records(FILE *log, int level, int size, int entries) {
	int rec, res;
	
	for(rec=1;rec<=256;rec++) {
		uint8_t rbuf[256];
		res = read_record(rbuf, size);
		// TODO: save to node struct!
		if(res > 0) {
			indent(level+1, log);
			fprintf(log, "%3d: ",rec);
			hexdump(log, rbuf, res, 1);
			fprintf(log, "\n");
		}
		if(entries && (rec>=entries))
			break;
	}
}

void dump_binary(FILE *log, int level, int size) {
	uint8_t *buf=malloc(65536);
	uint8_t *ptr;
	int res, ofs=0;
	assert(buf);

	res = read_binary(buf, size);

	for(ptr=buf;res>0;ofs+=16, ptr+=16) {
		int csize = res > 16 ? 16 : res;
		indent(level+1, log);
		fprintf(log, "%04x: ", ofs);
		hexdump(log, ptr, csize, 1);
		fprintf(log, "\n");
		res -= csize;
	}
	
	free(buf);
}

int is_ef(node_t *n) {
	return n->is_ef;
}

void append_node(node_t *parent, node_t *new) {
	node_t *next = is_ef(new) ? parent->sub_ef : parent->sub_df;
	node_t *last=NULL;

	for(; next; last=next, next=next->next) {
		assert(next->fid != new->fid);
		if(next->fid > new->fid)
			break;
	}
	
	new->next = next;
	if(last)
		last->next = new;
	else if(is_ef(new))
		parent->sub_ef = new;
	else
		parent->sub_df = new;

	if(!parent)
		return;
	
	// update DF/EF counter of parent
	if(is_ef(new))
		parent->ef_cnt++;
	else
		parent->df_cnt++;
}

node_t *create_node(node_t *parent, uint16_t fid, uint8_t *sel, int sel_len, char *name) {
	node_t *new = malloc(sizeof(node_t));

	bzero(new, sizeof(node_t));
	
	new->parent = parent;
	new->fid = fid;
	if(sel) {
		memcpy(&(new->u.ef_info),sel,sel_len); // HACK
		new->is_ef = (new->u.ef_info.fid_type == 4) ? 1 : 0;
	}
	else {
		strcpy(new->u.name, name);
		new->is_ef = 1;
		if((name[0] == 'D') || (name[0] == 'M'))
			new->is_ef = 0;
	}

	/* only MF has no parent */
	assert((parent) || (fid == SIM_MF));

	/* done if MF */
	if(fid == SIM_MF)
		return new;

	append_node(parent, new);

	return new;
}

void fid_info(FILE *log, node_t *n, int level, int content) {
	char buf[] = "???";
	char *name = buf;

	if(n->ref)
		name = n->ref->u.name;
	
	indent(level, log);
	
	if(is_ef(n)) {
		int ef_struct_id = n->u.ef_info.ef_struct;
				
		if(ef_struct_id > 3)
			ef_struct_id = 2;

		fprintf(log, "%04x %-12s (EF) %s status %02x size %5d rec_len %3d\n",
		       n->fid,
		       name,
		       ef_struct[ef_struct_id],
		       n->u.ef_info.file_status,
		       be16toh(n->u.ef_info.file_size),
		       n->u.ef_info.rec_len
		       );
		if(!content)
			return;
		if((ef_struct_id == 1) || (ef_struct_id == 3)) {
			int records = 0;

			// limit for cyclic EFs
			if(ef_struct_id == 3)
				records = be16toh(n->u.ef_info.file_size) / n->u.ef_info.rec_len;
			
			dump_records(log, level, n->u.ef_info.rec_len, records);
		}
		else
			dump_binary(log, level, be16toh(n->u.ef_info.file_size));
	}
	else {
		fprintf(log, "%04x %-12s (%cF) %d DFs %d EFs\n",
		       n->fid,
		       name,
		       n->parent ? 'D' : 'M',
		       n->u.df_info.df_entries,
		       n->u.df_info.ef_entries
		       );
	}
}

int fid_in_list(uint16_t i, node_t *nptr) {
	for(; nptr; nptr = nptr->next) {
		if(nptr->fid == i)
			return 1;
	}
	return 0;
}

#define FID_ENDRANGE	0x7FFF

node_t *lookup_fid(node_t *p, uint16_t fid, int is_ef) {
	node_t *t = NULL;

	if(!p)
		return NULL;
	
	t = is_ef ? p->sub_ef : p->sub_df;

	for(;t;t=t->next) {
		if(t->fid == fid)
			return t;
	}
	
	return NULL;
}

void scan_dir(FILE *fs_log, node_t *n, int level) {
	node_t *nptr;
	uint32_t i;
	int res;

	fid_info(fs_log, n, level, 0);
	if(fs_log != stdout)
		fid_info(stdout, n, level, 0);
	
	level++;
	
	for(i=0x2F00;i<FID_ENDRANGE;i++) {
		uint8_t buf[256];
		
		if((i & 0xf00) != 0xf00)
			i += 0xf00;
		
		/* skip if FID equal to this DF or MF */
		if((i == n->fid) || (i == SIM_MF) || (i == 0x3fff))
			continue;
		
		if(n->parent) {

			/* skip if FID equal to parent of this DF */
			if(i == n->parent->fid)
				continue;

			/* skip if FID equal to any sub-DF of parent DF */
			if(fid_in_list(i, n->parent->sub_df))
				continue;
		}

		if(!(i&0xf)) {
			indent(level,stderr);
			fprintf(stderr,"%04x      \r",i);
		}

		res = select_fid(i, buf, 256);
		
		if(res > 0) {
			node_t *new = create_node(n, i, buf, res, NULL);

			new->ref = lookup_fid(n->ref, i, is_ef(new));

			/* DF selected - reselect parent, scan new DF later */
			if(!is_ef(new)) {
				res = select_fid(n->fid, NULL, 0);
				if(res <= 0)
					fprintf(fs_log, "\nWTF?? %d %04x %04x\n",res,n->fid,i&0xffff);
			}
			else {				
				fid_info(fs_log, new, level, 1);
			}
		} // FID exists
	} // foreach FID

	/* scan DFs now */
	for(nptr=n->sub_df; nptr; nptr=nptr->next) {
		res = select_fid(nptr->fid, NULL, 0);
		if(res<=0)
			fprintf(fs_log, "\nWTF?? %d %04x %04x\n",res,n->fid,nptr->fid);
		else
			scan_dir(fs_log, nptr, level);
		select_fid(n->fid, NULL, 0);
	}
}

node_t *scan_tree(FILE *fs_log, int mf_id, node_t *ref) {
	uint8_t buf[256];
	node_t *root;
	int res;

	res = select_fid(mf_id, buf, 256);
	assert(res > 0);
	
	root = create_node(NULL, mf_id, buf, res, NULL);
	root->ref = ref;

	scan_dir(fs_log, root, 0);

	return root;
}

void dump_aids(void) {
	uint8_t buf[256];
	int res=1, i;

	select_fid(0x2f00, buf, 256);
	
	printf("AIDs:\n");
	for(i=1; i<256; i++) {
		res = read_record(buf, 256);
		if(res>0) {
			printf(" * ");
			hexdump(stdout, buf, res, 1);
			printf("\n");
		}
		else
			break;
	}
}

char *skip(char *p) {
	for(;*p == ' ';p++) {}
	return p;
}

node_t *load_fidlist(char *fn) {
	char buf[128];
	FILE *fl=fopen(fn,"r");
	int res=0;
	node_t *n, *root=NULL, *parent=NULL;
	node_t *n_stack[16];

	assert(fl);

	while(fgets(buf, 128, fl)) {
		char *ptr = skip(buf);
		int level = ptr - buf;
		int fid;
		char fid_name[32];
		
		fid_name[0]=0;
		
		res = sscanf(ptr, "%04X %s",&fid,fid_name);
		if(!res)
			break;

		if(!level)
			parent = NULL;
		else
			parent = n_stack[level-1];
		
		n = create_node(parent, fid, NULL, 0, fid_name);

		if(!root)
			root = n;
		
		if(!is_ef(n))
			n_stack[level] = n;
		
		//printf("%d %x %s\n",res,fid,fid_name);
	}
	
	fclose(fl);

	return root;
}

int main(int argc, char **argv) {
	uint8_t buf[256];
	node_t *ref_tree = NULL;
	FILE *term_log = NULL;
	FILE *fs_log = NULL;
	char txt[128];
	uint32_t now = (uint32_t)time(NULL);
	int fd, res;

	ref_tree = load_fidlist("gsm_fids.txt");

	if(argc<2) {
		printf("usage: %s phoenix:<serial_device> (pin)\n",argv[0]);
		printf("examples: \n");
		printf("  %s phoenix:/dev/ttyUSB0\n",argv[0]);
		printf("  %s phoenix:/dev/ttyUSB0 1234\n",argv[0]);
		return 0;
	}

	sprintf(txt,"%u-log.txt",now);
	term_log = fopen(txt,"w");
	assert(term_log);

	sprintf(txt,"%u-fs.txt",now);
	fs_log = fopen(txt,"w");
	assert(fs_log);
	
	fd = term_init(argv[1], term_log ? 255 : 0, term_log);
	if(!fd) {
		fprintf(stderr,"cannot initialize terminal\n");
		return -5;
	}

	res = term_reset(buf, 256);
	if(res < 1) {
		fprintf(stderr,"no ATR! result: %d\n",res);
		return 23;
	}

	if(term_log) {
		fprintf(term_log, "ATR: ");
		hexdump(term_log, buf, res, 1);
		fprintf(term_log, "\n");
	}

	if(fs_log) {
		fprintf(fs_log, "ATR: ");
		hexdump(fs_log, buf, res, 1);
		fprintf(fs_log, "\n");
	}
	
	printf("ATR: ");
	hexdump(stdout, buf, res, 1);
	printf("\n");

	res = usim_detect();
	assert(res>=0);
	if(res) {
		fprintf(stderr,"probably USIM - aborting\n");
//		goto out;
//		class_byte = 0;
	}

	if(argc>2) {
		res = auth(argv[2]);
		printf("auth result: %d\n",res);
	}

//	dump_aids();
	
	scan_tree(fs_log, SIM_MF, ref_tree);

//out:	
	if(term_log)
		fclose(term_log);

	if(fs_log)
		fclose(fs_log);

	return 0;
}
