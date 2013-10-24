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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <bsdconv.h>

#define IBUFLEN 1024

static bsdconv_counter_t process(FILE *, FILE *);
static double evaluate(const char *, struct bsdconv_instance *ins, char *ib, size_t len);
static void usage(void);
static void finish(int r);

struct codec {
	char *name;
	struct bsdconv_instance *evl;
	struct bsdconv_instance *ins;
	char *conv;
	double score;
};

static struct codec codecs[9];
static char outenc;
static size_t bufsiz;
static char verbose;

int main(int argc, char *argv[]){
	int ch;
	int i;
	bsdconv_counter_t e;
	char inplace=0;
	char *tmp;
	int fd;
	FILE *fi, *fo;

	bufsiz=8192;
	outenc ='8';

	codecs[0].name="UTF-8";
	codecs[0].evl=bsdconv_create("utf-8:score:count:null");
	codecs[0].conv="utf-8:nobom:utf-8";
	codecs[0].ins=NULL;

	codecs[1].name="Big5";
	codecs[1].evl=bsdconv_create("big5:score:count:null");
	codecs[1].conv="big5:utf-8";
	codecs[1].ins=NULL;

	codecs[2].name="GBK";
	codecs[2].evl=bsdconv_create("gbk:score:count:null");
	codecs[2].conv="gbk:utf-8";
	codecs[2].ins=NULL;

	codecs[3].name="CCCII";
	codecs[3].evl=bsdconv_create("cccii:score:count:null");
	codecs[3].conv="cccii:utf-8";
	codecs[3].ins=NULL;

	codecs[4].name="UTF-16LE";
	codecs[4].evl=bsdconv_create("utf-16le:score:count:null");
	codecs[4].conv="utf-16le:nobom:utf-8";
	codecs[4].ins=NULL;

	codecs[5].name="UTF-16BE";
	codecs[5].evl=bsdconv_create("utf-16be:score:count:null");
	codecs[5].conv="utf-16be:nobom:utf-8";
	codecs[5].ins=NULL;

	codecs[6].name="UTF-32LE";
	codecs[6].evl=bsdconv_create("utf-32le:score:count:null");
	codecs[6].conv="utf-32le:nobom:utf-8";
	codecs[6].ins=NULL;

	codecs[7].name="UTF-32BE";
	codecs[7].evl=bsdconv_create("utf-32be:score:count:null");
	codecs[7].conv="utf-32be:nobom:utf-8";
	codecs[7].ins=NULL;

	codecs[8].name="GB18030";
	codecs[8].evl=bsdconv_create("gb18030:score:count:null");
	codecs[8].conv="gb18030:utf-8";
	codecs[8].ins=NULL;

	while ((ch = getopt(argc, argv, "ifbugs:v")) != -1)
		switch(ch) {
		case 'i':
			inplace=1;
			break;
		case 'f':
			inplace=2;
			break;
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
		case 'v':
			verbose=1;
			break;
		case '?':
		default:
			usage();
		}

#ifdef WIN32
	setmode(STDIN_FILENO, O_BINARY);
	setmode(STDOUT_FILENO, O_BINARY);
#endif

	if(optind<argc){
		for(;optind<argc;optind++){
			fi=fopen(argv[optind],"rb");
			if(fi==NULL){
				fprintf(stderr, "Failed opening file %s.\n", argv[optind]);
				continue;
			}
			if(inplace==0){
				process(fi, stdout);
				fclose(fi);
			}else{
				tmp=malloc(strlen(argv[optind])+8);
				strcpy(tmp, argv[optind]);
				strcat(tmp, ".XXXXXX");
				if((fd=mkstemp(tmp))==-1){
					free(tmp);
					fprintf(stderr, "Failed creating temp file.\n");
					fclose(fi);
					continue;
				}
				fo=fdopen(fd, "wb");
				if(!fo){
					fprintf(stderr, "Unable to open output file for %s.\n", argv[optind]);
					fclose(fi);
					continue;
				}
#ifndef WIN32
				struct stat stat;
				fstat(fileno(fi), &stat);
				fchown(fileno(fo), stat.st_uid, stat.st_gid);
				fchmod(fileno(fo), stat.st_mode);
#endif
				e=process(fi, fo);
				fclose(fi);
				fclose(fo);
				if(e==0 || inplace==2){
					unlink(argv[optind]);
					rename(tmp,argv[optind]);
				}else{
					fprintf(stderr, "Skipping %s (%zu error(s))\n", argv[optind], e);
					unlink(tmp);
				}
				free(tmp);
			}
		}
	}else{
		process(stdin, stdout);
	}

	finish(0);

	return 0;
}

static bsdconv_counter_t process(FILE *fi, FILE *fo){
	char *conv;
	struct bsdconv_instance *ins;
	int i, max, max_i;
	char *ib;
	size_t len;
	bsdconv_counter_t *e;
	bsdconv_counter_t r=0;

	ib=malloc(bufsiz);
	len=fread(ib, 1, bufsiz, fi);

	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		codecs[i].score=evaluate(codecs[i].name, codecs[i].evl, ib, len);
	}
	max=codecs[0].score;
	max_i=0;
	for(i=1;i<sizeof(codecs)/sizeof(struct codec);++i){
		if(codecs[i].score>max){
			max_i=i;
			max=codecs[i].score;
		}
	}

	if(codecs[max_i].ins){
		ins=codecs[max_i].ins;
	}else{
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
		codecs[max_i].ins=ins=bsdconv_create(conv);
		bsdconv_free(conv);
	}
	bsdconv_counter_reset(ins, NULL);
	bsdconv_init(ins);
	ins->input.data=ib;
	ins->input.flags|=F_FREE;
	ins->input.next=NULL;
	ins->input.len=len;
	ins->output_mode=BSDCONV_FILE;
	ins->output.data=fo;
	bsdconv(ins);
	do{
		ib=malloc(IBUFLEN);
		ins->input.data=ib;
		ins->input.flags|=F_FREE;
		ins->input.next=NULL;
		if((ins->input.len=fread(ib, 1, IBUFLEN, fi))==0){
			ins->flush=1;
		}
		ins->output_mode=BSDCONV_FILE;
		ins->output.data=fo;
		bsdconv(ins);
	}while(ins->flush==0);
	e=bsdconv_counter(ins, "IERR");
	r+=*e;
	e=bsdconv_counter(ins, "OERR");
	r+=*e;
	return r;
}

static double evaluate(const char *name, struct bsdconv_instance *ins, char *ib, size_t len){
	bsdconv_counter_t *_ierr=bsdconv_counter(ins, "IERR");
	bsdconv_counter_t *_score=bsdconv_counter(ins, "SCORE");
	bsdconv_counter_t *_count=bsdconv_counter(ins, "COUNT");
	bsdconv_counter_reset(ins, NULL);
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
	double wv=(score - ierr*10)/count;
	if(verbose){
		fprintf(stderr, "%s: %.2lf\n", name, wv);
		fprintf(stderr, "\tIERR: %.2lf\n", ierr);
		fprintf(stderr, "\tSCORE: %.2lf\n", score);
		fprintf(stderr, "\tCOUNT: %.2lf\n", count);
		fprintf(stderr, "\n");
	}

	return wv;
}

static void usage(void){
	(void)fprintf(stderr,
	    "usage: chiconv [-bug] [-i bufsiz]\n"
	    "\t -i\tSave in-place if no error\n"
	    "\t -f\tSave in-place regardless of errors (implies -i)\n"
	    "\t -b\tOutput Big5\n"
	    "\t -u\tOutput Big5 with UAO exntension\n"
	    "\t -g\tOutput GBK\n"
	    "\t -s\tBuffer size used for encoding detection, default=8192\n"
	    "\t -v\tVerbose\n"
	);
	finish(1);
}

static void finish(int r){
	int i;
	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		bsdconv_destroy(codecs[i].evl);
		if(codecs[i].ins)
			bsdconv_destroy(codecs[i].ins);
	}
	exit(r);

}
