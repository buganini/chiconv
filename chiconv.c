/*
 * Copyright (c) 2012-2014 Kuan-Chung Chiu <buganini@gmail.com>
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
static void usage(void);
static void finish(int r);

struct codec {
	char *name;
	struct bsdconv_instance *evl;
	struct bsdconv_instance *ins;
	char *evl_conv, *conv;
	double wv;
	char up;
};

static struct codec codecs[15];
static char outenc;
static size_t bufsiz;
static char verbose;
static char linebreak;

int main(int argc, char *argv[]){
	int ch;
	int i = 0;
	bsdconv_counter_t e;
	char inplace = 0;
	char *tmp;
	int fd;
	FILE *fi, *fo;
	char enable_hkscs = 0;
	int hkscs[] = {2, 3, 4};

	linebreak=0;
	bufsiz=8192;
	outenc ='8';

	codecs[i].name="UTF-8";
	codecs[i].evl_conv="utf-8:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="utf-8:nobom:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5 (UAO)";
	codecs[i].evl_conv="big5:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="big5:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5 (HKSCS 2004)";
	codecs[i].evl_conv="hkscs2004:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="hkscs2004:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5 (HKSCS 2001)";
	codecs[i].evl_conv="hkscs2001:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="hkscs2001:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5 (HKSCS 1999)";
	codecs[i].evl_conv="hkscs1999:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="hkscs1999:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5-2003";
	codecs[i].evl_conv="big5-2003:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="big5-2003:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5E";
	codecs[i].evl_conv="big5e:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="big5e:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="Big5-ETEN";
	codecs[i].evl_conv="big5-eten:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="big5-eten:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="GBK";
	codecs[i].evl_conv="gbk:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="gbk:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="CCCII";
	codecs[i].evl_conv="cccii:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="cccii:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="UTF-16LE";
	codecs[i].evl_conv="utf-16le:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="utf-16le:nobom:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="UTF-16BE";
	codecs[i].evl_conv="utf-16be:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="utf-16be:nobom:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="UTF-32LE";
	codecs[i].evl_conv="utf-32le:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="utf-32le:nobom:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="UTF-32BE";
	codecs[i].evl_conv="utf-32be:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="utf-32be:nobom:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	codecs[i].name="GB18030";
	codecs[i].evl_conv="gb18030:score#with=cjk:count:zh-bonus:zhtw:zh-bonus-phrase:null";
	codecs[i].conv="gb18030:utf-8";
	codecs[i].ins=NULL;
	i+=1;

	while ((ch = getopt(argc, argv, "ifbugks:vwmx")) != -1)
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
		case 'k':
			enable_hkscs=1;
			break;
		case 's':
			if(sscanf(optarg, "%d", &i)!=1)
				usage();
			bufsiz=i;
			break;
		case 'v':
			verbose=1;
			break;
		case 'w':
		case 'm':
		case 'x':
			linebreak=ch;
			break;
		case '?':
		default:
			usage();
		}

#ifdef WIN32
	setmode(STDIN_FILENO, O_BINARY);
	setmode(STDOUT_FILENO, O_BINARY);
#endif

	if(!enable_hkscs){
		for(i=0;i<sizeof(hkscs)/sizeof(hkscs[0]);i+=1){
			codecs[hkscs[i]].evl_conv=NULL;
		}
	}
	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		if(codecs[i].evl_conv==NULL)
			continue;
		codecs[i].evl = bsdconv_create(codecs[i].evl_conv);
		if(codecs[i].evl == NULL){
			char *e = bsdconv_error();
			fprintf(stderr, "WARNING: Skipping %s: %s\n", codecs[i].name, e);
			bsdconv_free(e);
			continue;
		}
	}

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
	int i, max_i=-1;
	char *ib, *ctmp;
	size_t len;
	FILE *tmp;
	int candidates = sizeof(codecs)/sizeof(struct codec);
	bsdconv_counter_t *e;
	bsdconv_counter_t r=0;
	ib=malloc(bufsiz);

	tmp=tmpfile();

	for(i=0;i<sizeof(codecs)/sizeof(struct codec);i+=1){
		if(codecs[i].evl == NULL){
			codecs[i].up = 0;
			continue;
		}
		ins = codecs[i].evl;
		bsdconv_counter_reset(ins, NULL);
		bsdconv_init(ins);
		codecs[i].up = 1;
	}

	int rnd=0;
	int flush=0;
	while(candidates > 1 && !flush){
		rnd += 1;
		if(verbose){
			fprintf(stderr, "Round %d\n================================\n", rnd);
		}
		len = fread(ib, 1, bufsiz, fi);
		if(tmp != NULL)
			fwrite(ib, len, 1, tmp);
		if(feof(fi))
			flush = 1;
		for(i = 0;i < sizeof(codecs)/sizeof(struct codec);++i){
			if(codecs[i].up!=1 || codecs[i].evl_conv==NULL)
				continue;
			ins=codecs[i].evl;
			bsdconv_counter_t *_ierr=bsdconv_counter(ins, "IERR");
			bsdconv_counter_t *_score=bsdconv_counter(ins, "SCORE");
			bsdconv_counter_t *_count=bsdconv_counter(ins, "COUNT");
			ins->input.data=ib;
			ins->input.flags=0;
			ins->input.next=NULL;
			ins->input.len=len;
			ins->flush=flush;
			ins->output_mode=BSDCONV_NULL;
			bsdconv(ins);
			double ierr=(double)(*_ierr);
			double score=(double)(*_score);
			double count=(double)(*_count);
			codecs[i].wv=(score - ierr*(count*0.01))/count;
			if(verbose){
				fprintf(stderr, "%s: %.6lf\n", codecs[i].name, codecs[i].wv);
				fprintf(stderr, "\tIERR: %.0lf\n", ierr);
				fprintf(stderr, "\tSCORE: %.0lf\n", score);
				fprintf(stderr, "\tCOUNT: %.0lf\n", count);
				fprintf(stderr, "\n");
			}
		}
		for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
			if(max_i==-1){
				max_i=i;
				continue;
			}
			if(codecs[i].up!=1)
				continue;
			if(codecs[i].wv > codecs[max_i].wv){
				codecs[max_i].up=0;
				max_i=i;
				candidates-=1;
			}else if(codecs[i].wv < codecs[max_i].wv){
				codecs[i].up=0;
				candidates-=1;
			}
		}
		if(tmp==NULL){
			fprintf(stderr, "WARNING: Early finished because of temporary file creation failure\n");
			break;
		}
	}

	if(verbose){
		fprintf(stderr, "Detected encoding: %s\n", codecs[max_i].name);
	}
	if(codecs[max_i].ins){
		ins=codecs[max_i].ins;
	}else{
		conv=codecs[max_i].conv;
		switch(outenc){
			case 'b':
				conv=bsdconv_replace_phase(conv, "_CP950,CP950-TRANS,ASCII", TO, -1);
				break;
			case 'u':
				conv=bsdconv_replace_phase(conv, "_CP950,_UAO250,CP950-TRANS,ASCII", TO, -1);
				break;
			case 'g':
				conv=bsdconv_replace_phase(conv, "_GBK,CP936-TRANS,ASCII", TO, -1);
				break;
			default:
				conv=strdup(conv);
				break;
		}
		switch(linebreak){
			case 'w':
				ctmp=conv;
				conv=bsdconv_insert_phase(conv, "WIN", INTER, -1);
				bsdconv_free(ctmp);
				break;
			case 'm':
				ctmp=conv;
				conv=bsdconv_insert_phase(conv, "MAC", INTER, -1);
				bsdconv_free(ctmp);
				break;
			case 'x':
				ctmp=conv;
				conv=bsdconv_insert_phase(conv, "UNIX", INTER, -1);
				bsdconv_free(ctmp);
				break;
		}
		codecs[max_i].ins=ins=bsdconv_create(conv);
		bsdconv_free(conv);
	}
	bsdconv_counter_reset(ins, NULL);
	bsdconv_init(ins);

	if(tmp==NULL){
		ins->input.data=ib;
		ins->input.len=len;
		ins->input.flags=F_FREE;
		ins->input.next=NULL;
		ins->output_mode=BSDCONV_FILE;
		ins->output.data=fo;
		bsdconv(ins);
	}else{
		free(ib);
		fseek(tmp, 0L, SEEK_SET);
		do{
			ib=malloc(IBUFLEN);
			ins->input.len=fread(ib, 1, IBUFLEN, tmp);
			ins->input.data=ib;
			ins->input.flags=F_FREE;
			ins->input.next=NULL;
			ins->output_mode=BSDCONV_FILE;
			ins->output.data=fo;
			bsdconv(ins);
		}while(!feof(tmp));
		fclose(tmp);
	}
	do{
		ib=malloc(IBUFLEN);
		ins->input.data=ib;
		ins->input.flags=F_FREE;
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

static void usage(void){
	(void)fprintf(stderr,
		"usage: chiconv [-bug] [-i bufsiz]\n"
		"\t -i\tSave in-place if no error\n"
		"\t -f\tSave in-place regardless of errors (implies -i)\n"
		"\t -b\tOutput Big5\n"
		"\t -u\tOutput Big5 with UAO exntension\n"
		"\t -g\tOutput GBK\n"
		"\t -k\tEnable HKSCS\n"
		"\t -s\tBuffer size used for encoding2 detection, default=8192\n"
		"\t -v\tVerbose\n"
		"\t -w\tUse Windows linebreak\n"
		"\t -m\tUse Mac linebreak\n"
		"\t -x\tUse Unix linebreak\n"
	);
	finish(1);
}

static void finish(int r){
	int i;
	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		if(codecs[i].evl)
			bsdconv_destroy(codecs[i].evl);
		if(codecs[i].ins)
			bsdconv_destroy(codecs[i].ins);
	}
	exit(r);

}
