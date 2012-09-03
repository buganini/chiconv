#include <stdio.h>
#include <unistd.h>
#include <bsdconv.h>

#define IBUFLEN 1024

static double evaluate(struct bsdconv_instance *ins, char *ib, size_t len, double coeff);
static void usage(void);

struct codec {
	struct bsdconv_instance *evl;
	struct bsdconv_instance *ins;
	double score;
	double coeff;
};

int main(int argc, char *argv[]){
	struct bsdconv_instance *ins;
	size_t bufsiz=8192;
	struct codec codecs[]={
		{
			.evl=bsdconv_create("utf-8:score:null"),
			.ins=bsdconv_create("utf-8:utf-8"),
			.score=0,
			.coeff=2.5
		},
		{
			.evl=bsdconv_create("big5:score:null"),
			.ins=bsdconv_create("big5:utf-8"),
			.score=0,
			.coeff=1.5
		},
		{
			.evl=bsdconv_create("gbk:score:null"),
			.ins=bsdconv_create("gbk:utf-8"),
			.score=0,
			.coeff=1.5
		}
	};
	int i, max, max_i;
	size_t len;
	char *ib;
	char outenc;
	int ch;

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

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: chiconv [-bg] [-i bufsiz]\n"
	    "\t -b\tOutput Big5\n"
	    "\t -g\tOutput GBK\n"
	    "\t -s\tbuffer size, default=8192\n"
	);
	exit(1);
}
