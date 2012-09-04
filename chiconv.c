#include <stdio.h>
#include <unistd.h>
#include <bsdconv.h>

#define IBUFLEN 1024

static double evaluate(struct bsdconv_instance *ins, char *ib, size_t len, double coeff);
static void usage(void);
static void finish(int r);

struct codec {
	struct bsdconv_instance *evl;
	struct bsdconv_instance *ins;
	double score;
	double coeff;
};

struct codec codecs[3];

int main(int argc, char *argv[]){
	struct bsdconv_instance *ins;
	size_t bufsiz=8192;
	int i, max, max_i;
	size_t len;
	char *ib;
	char outenc='u';
	int ch;

	codecs[0].evl=bsdconv_create("utf-8:score:null");
	codecs[0].ins=bsdconv_create("utf-8:utf-8");
	codecs[0].score=0;
	codecs[0].coeff=2.5;

	codecs[1].evl=bsdconv_create("big5:score:null");
	codecs[1].ins=bsdconv_create("big5:utf-8");
	codecs[1].score=0;
	codecs[1].coeff=1.5;

	codecs[2].evl=bsdconv_create("gbk:score:null");
	codecs[2].ins=bsdconv_create("gbk:utf-8");
	codecs[2].score=0;
	codecs[2].coeff=1.5;

	while ((ch = getopt(argc, argv, "bgs:")) != -1)
		switch(ch) {
		case 'b':
			outenc='b';
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
		codecs[i].score=evaluate(codecs[i].evl, ib, len, codecs[i].coeff);
	}
	max=codecs[0].score;
	max_i=0;
	for(i=1;i<sizeof(codecs)/sizeof(struct codec);++i){
		if(codecs[i].score>max){
			max_i=i;
			max=codecs[i].score;
		}
	}

	ins=codecs[max_i].ins;

	switch(outenc){
		case 'b':
			bsdconv_replace_phase(ins, "_CP950,CP950_TRANS,ASCII", TO, 1);
			break;
		case 'g':
			bsdconv_replace_phase(ins, "_GBK,CP936_TRANS,ASCII", TO, 1);
			break;
	}

	bsdconv_init(ins);
	ins->input.data=ib;
	ins->input.flags|=F_FREE;
	ins->input.len=len;
	ins->output_mode=BSDCONV_FILE;
	ins->output.data=stdout;
	bsdconv(ins);
	do{
		ib=malloc(IBUFLEN);
		ins->input.data=ib;
		ins->input.flags|=F_FREE;
		if((ins->input.len=fread(ib, 1, IBUFLEN, stdin))==0){
			ins->flush=1;
		}
		ins->output_mode=BSDCONV_FILE;
		ins->output.data=stdout;
		bsdconv(ins);
	}while(ins->flush==0);

	finish(0);

	return 0;
}

static double evaluate(struct bsdconv_instance *ins, char *ib, size_t len, double coeff){
	bsdconv_init(ins);
	ins->input.data=ib;
	ins->input.flags=0;
	ins->input.len=len;
	ins->output_mode=BSDCONV_NULL;
	bsdconv(ins);
	return ins->score * coeff * (len/16384) - ins->ierr * 10 - ins->oerr;
}

static void usage(void){
	(void)fprintf(stderr,
	    "usage: chiconv [-bg] [-i bufsiz]\n"
	    "\t -b\tOutput Big5\n"
	    "\t -g\tOutput GBK\n"
	    "\t -s\tbuffer size, default=8192\n"
	);
	finish(1);
}

static void finish(int r){
	int i;
	for(i=0;i<sizeof(codecs)/sizeof(struct codec);++i){
		bsdconv_destroy(codecs[i].evl);
		bsdconv_destroy(codecs[i].ins);
	}
	exit(r);

}
