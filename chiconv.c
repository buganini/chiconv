#include <stdio.h>
#include <bsdconv.h>

#define IBUFLEN 1024

double evaluate(struct bsdconv_instance *ins, char *ib, size_t len, double coeff);
int execute(struct bsdconv_instance *ins, int fd, size_t len);

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
	char *ib=malloc(bufsiz);

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

double evaluate(struct bsdconv_instance *ins, char *ib, size_t len, double coeff){
	bsdconv_init(ins);
	ins->input.data=ib;
	ins->input.flags=0;
	ins->input.len=len;
	ins->output_mode=BSDCONV_NULL;
	bsdconv(ins);
	return ins->score * coeff - ins->ierr * 10 - ins->oerr;
}
