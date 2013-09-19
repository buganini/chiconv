/*
 * Copyright (c) 2012-2013 Kuan-Chung Chiu <buganini@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <bsdconv.h>

#define IBUFLEN 1024

static double evaluate(struct bsdconv_instance *ins, char *ib, size_t len);
static void usage(void);
static void finish(int r);

struct codec {
	struct bsdconv_instance *evl;
	char *conv;
	double score;
	double coeff;
};

struct codec codecs[9];

int main(int argc, char *argv[]){
	char *conv;
	struct bsdconv_instance *ins;
	size_t bufsiz=8192;
	int i, max, max_i;
	size_t len;
	char *ib;
	char outenc='8';
	int ch;

	codecs[0].evl=bsdconv_create("utf-8:score:count:null");
	codecs[0].conv="utf-8:nobom:utf-8";

	codecs[1].evl=bsdconv_create("big5:score:count:null");
	codecs[1].conv="big5:utf-8";

	codecs[2].evl=bsdconv_create("gbk:score:count:null");
	codecs[2].conv="gbk:utf-8";

	codecs[3].evl=bsdconv_create("cccii:score:count:null");
	codecs[3].conv="cccii:utf-8";

	codecs[4].evl=bsdconv_create("utf-16le:score:count:null");
	codecs[4].conv="utf-16le:nobom:utf-8";

	codecs[5].evl=bsdconv_create("utf-16be:score:count:null");
	codecs[5].conv="utf-16be:nobom:utf-8";

	codecs[6].evl=bsdconv_create("utf-32le:score:count:null");
	codecs[6].conv="utf-32le:nobom:utf-8";

	codecs[7].evl=bsdconv_create("utf-32be:score:count:null");
	codecs[7].conv="utf-32be:nobom:utf-8";

	codecs[8].evl=bsdconv_create("gb18030:score:count:null");
	codecs[8].conv="gb18030:utf-8";

	while ((ch = getopt(argc, argv, "bugs:")) != -1)
		switch(ch) {
		case 'b':
			outenc='b';
			break;
		case 'u':
			outenc='u';
			break;
		case 'g':
			outenc='g';
			break;
		case 's':
			if(sscanf(optarg, "%d", &i)!=1)
				usage();
			bufsiz=i;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	ib=malloc(bufsiz);
	len=fread(ib, 1, bufsiz, stdin);

	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		codecs[i].score=evaluate(codecs[i].evl, ib, len);
	}
	max=codecs[0].score;
	max_i=0;
	for(i=1;i<sizeof(codecs)/sizeof(struct codec);++i){
		if(codecs[i].score>max){
			max_i=i;
			max=codecs[i].score;
		}
	}

	conv=codecs[max_i].conv;

	switch(outenc){
		case 'b':
			conv=bsdconv_replace_phase(conv, "_CP950,CP950-TRANS,ASCII", TO, 1);
			break;
		case 'u':
			conv=bsdconv_replace_phase(conv, "_CP950,_UAO250,CP950-TRANS,ASCII", TO, 1);
			break;
		case 'g':
			conv=bsdconv_replace_phase(conv, "_GBK,CP936-TRANS,ASCII", TO, 1);
			break;
		default:
			conv=strdup(conv);
			break;
	}
	ins=bsdconv_create(conv);
	bsdconv_free(conv);
	bsdconv_init(ins);
	ins->input.data=ib;
	ins->input.flags|=F_FREE;
	ins->input.next=NULL;
	ins->input.len=len;
	ins->output_mode=BSDCONV_FILE;
	ins->output.data=stdout;
	bsdconv(ins);
	do{
		ib=malloc(IBUFLEN);
		ins->input.data=ib;
		ins->input.flags|=F_FREE;
		ins->input.next=NULL;
		if((ins->input.len=fread(ib, 1, IBUFLEN, stdin))==0){
			ins->flush=1;
		}
		ins->output_mode=BSDCONV_FILE;
		ins->output.data=stdout;
		bsdconv(ins);
	}while(ins->flush==0);
	bsdconv_destroy(ins);

	finish(0);

	return 0;
}

static double evaluate(struct bsdconv_instance *ins, char *ib, size_t len){
	bsdconv_counter_t *_ierr=bsdconv_counter(ins, "IERR");
	bsdconv_counter_t *_score=bsdconv_counter(ins, "SCORE");
	bsdconv_counter_t *_count=bsdconv_counter(ins, "COUNT");
	bsdconv_init(ins);
	ins->input.data=ib;
	ins->input.flags=0;
	ins->input.next=NULL;
	ins->input.len=len;
	ins->output_mode=BSDCONV_NULL;
	bsdconv(ins);
	double ierr=(double)(*_ierr);
	double score=(double)(*_score);
	double count=(double)(*_count);
	return (score - ierr*10)/count;
}

static void usage(void){
	(void)fprintf(stderr,
	    "usage: chiconv [-bug] [-i bufsiz]\n"
	    "\t -b\tOutput Big5\n"
	    "\t -u\tOutput Big5 with UAO exntension\n"
	    "\t -g\tOutput GBK\n"
	    "\t -s\tbuffer size used for encoding detection, default=8192\n"
	);
	finish(1);
}

static void finish(int r){
	int i;
	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		bsdconv_destroy(codecs[i].evl);
	}
	exit(r);

}
