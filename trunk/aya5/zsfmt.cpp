/*
zsfmt.c 乱数ライブラリ本体
coded by isaku@pb4.so-net.ne.jp
*/
#include <math.h>
#include <limits.h>

#include "zsfmt.h"

#define N128 ((int)(MEXP/128)+1)
#define N32 (N128*4)
#define LAG (N32>=623?11:N32>=68?7:N32>=39?5:3)
#define MID ((N32-LAG)/2)
#define MLG (MID+LAG)
#define SL3 (SL2*8)
#define SL4 (32-SL3)
#define SR3 (SR2*8)
#define SR4 (32-SR3)

#if LONG_MAX == 0x7fffffffL
#define MSK32(X)
#define AND32(X) (X)
#else
#define MSK32(X) X&=0xffffffffUL
#define AND32(X) ((X)&0xffffffffUL)
#endif

/*┌────────────────┐
　│マクロで渡されるデフォルトの領域│
　└────────────────┘*/
state_t DefaultMt;

/*┌──────────┐
　│パラメータ識別文字列│
　└──────────┘*/
char*IdString=IDSTR;

/*┌────────────────┐
  │SSE2テクノロジサポートのチェック│
  └────────────────┘*/
#ifdef SSE2
#ifdef _MSC_VER
int IsSSE2SFMT(void)
{
    _asm {
        push    edx
        pushfd
        pop     eax
        xor     eax, 00200000h
        push    eax
        popfd
        pushfd
        pop     edx
        cmp     eax, edx
        jnz     no_sse2
        mov     eax, 1
        cpuid
        and     edx, 04000000h
        jnz     yes_sse2
    no_sse2:
        xor     eax,eax
        jmp     exit_sse2
    yes_sse2:
        mov     eax,1
    exit_sse2:
        pop     edx
    }
}
#else
int IsSSE2SFMT(void) { return 1; }
#endif
#endif

/*┌─────────┐
　│初期化の下請け関数│
　└─────────┘*/
static void period_certification(state_p mt)
{
    static unsigned long par[4]={PARITY1,PARITY2,PARITY3,PARITY4};
    int inner=0,i,j; unsigned long work;

    mt->index=N32; mt->range=0; mt->normal_sw=0;
#ifdef SSE2
    mt->sse2=IsSSE2SFMT();
#else
    mt->sse2=0;
#endif
    for (i=0;i<4;i++) {
        work=mt->u.x[i]&par[i];
        for (j=0;j<32;j++,work>>=1) inner^=(int)work&1;
    }
    if (inner==1) return;
    for (i=0;i<4;i++) for (j=0,work=1;j<32;j++,work<<=1)
     if (work&par[i]) { mt->u.x[i]^=work; return; }
}

/*┌──────┐
　│領域を初期化│
　└──────┘*/
void InitMt_r(state_p mt,unsigned long s)
{
    unsigned p;

    MSK32(s);
    for (mt->u.x[0]=s,p=1;p<N32;p++)
        mt->u.x[p]=s=AND32(1812433253UL*(s^(s>>30))+p);
    period_certification(mt);
}

/*┌─────────┐
　│配列を使って初期化│
　└─────────┘*/
void InitMtEx_r(state_p mt,unsigned long*init_key,unsigned key_len)
{
  unsigned long r,*x=mt->u.x; unsigned i,j,c;

    for (i=0;i<N32;i++) x[i]=0x8b8b8b8bUL;
    if (key_len+1>N32) c=key_len+1; else c=N32;
    r=x[0]^x[MID]^x[N32-1];
    r=AND32((r^(r>>27))*1664525UL);
    x[MID]+=r; MSK32(x[MID]); r+=key_len; MSK32(r);
    x[MLG]+=r; MSK32(x[MLG]); x[0]=r; c--;
    for (i=1,j=0;j<c&&j<key_len;j++) {
        r=x[i]^x[(i+MID)%N32]^x[(i+N32-1)%N32];
        r=AND32((r^(r>>27))*1664525UL); x[(i+MID)%N32]+=r;
        MSK32(x[(i+MID)%N32]); r+=init_key[j]+i; MSK32(r);
        x[(i+MLG)%N32]+=r; MSK32(x[(i+MID+LAG)%N32]);
        x[i]=r; i=(i+1)%N32;
    }
    for (;j<c;j++) {
        r=x[i]^x[(i+MID)%N32]^x[(i+N32-1)%N32];
        r=AND32((r^(r>>27))*1664525UL); x[(i+MID)%N32]+=r;
        MSK32(x[(i+MID)%N32]); r+=i; MSK32(r);
        x[(i+MLG)%N32]+=r; MSK32(x[(i+MLG)%N32]);
        x[i]=r; i=(i+1)%N32;
    }
    for (j=0;j<N32;j++) {
        r=AND32(x[i]+x[(i+MID)%N32]+x[(i+N32-1)%N32]);
        r=AND32((r^(r>>27))*1566083941UL); x[(i+MID)%N32]^=r;
        r-=i; MSK32(r); x[(i+MLG)%N32]^=r; x[i]=r;
        i=(i+1)%N32;
    }
    period_certification(mt);
}

/*┌──────────┐
　│基本関数の下請け関数│
　└──────────┘*/
static void gen_rand_all(unsigned long*a)
{
    unsigned long*b=a+POS1*4,*c=a+N32-8,*d=a+N32-4,
                 *e=a+N32,x0,x1,x2,x3;
    do {
        x0=a[0];
        x1=a[1];
        x2=a[2];
        x3=a[3];

        x0^= a[0]<<SL3;
        x1^=(a[1]<<SL3)|(a[0]>>SL4);
        x2^=(a[2]<<SL3)|(a[1]>>SL4);
        x3^=(a[3]<<SL3)|(a[2]>>SL4);

        x0^=(b[0]>>SR1)&MSK1;
        x1^=(b[1]>>SR1)&MSK2;
        x2^=(b[2]>>SR1)&MSK3;
        x3^=(b[3]>>SR1)&MSK4;

        x0^=(c[0]>>SR3)|(c[1]<<SR4);
        x1^=(c[1]>>SR3)|(c[2]<<SR4);
        x2^=(c[2]>>SR3)|(c[3]<<SR4);
        x3^= c[3]>>SR3;

        x0^=d[0]<<SL1;
        x1^=d[1]<<SL1;
        x2^=d[2]<<SL1;
        x3^=d[3]<<SL1;

        a[0]=AND32(x0);
        a[1]=AND32(x1);
        a[2]=AND32(x2);
        a[3]=AND32(x3);

        c=d; d=a; a+=4; b+=4;
        if (b==e) b-=N32;
    } while (a!=e);
}

#ifdef SSE2
#ifdef __GNUC__
__inline__
#endif
static void gen_rand_all_sse2(__m128i*t)
{
    int i; __m128i c,d,v,x,y,z,mask;

    mask=_mm_set_epi32(MSK4,MSK3,MSK2,MSK1);
    c=_mm_load_si128(&t[N128-2]);
    d=_mm_load_si128(&t[N128-1]);
    for (i=0;i<N128-POS1;i++) {
        x=_mm_load_si128(&t[i]);
        y=_mm_srli_epi32(t[i+POS1],SR1);
        z=_mm_srli_si128(c,SR2);
        v=_mm_slli_epi32(d,SL1);
        z=_mm_xor_si128(z,x);
        z=_mm_xor_si128(z,v);
        x=_mm_slli_si128(x,SL2);
        y=_mm_and_si128(y,mask);
        z=_mm_xor_si128(z,x);
        z=_mm_xor_si128(z,y);
        _mm_store_si128(&t[i],z);
        c=d; d=z;
    }
    for (;i<N128;i++) {
        x=_mm_load_si128(&t[i]);
        y=_mm_srli_epi32(t[i+POS1-N128],SR1);
        z=_mm_srli_si128(c,SR2);
        v=_mm_slli_epi32(d,SL1);
        z=_mm_xor_si128(z,x);
        z=_mm_xor_si128(z,v);
        x=_mm_slli_si128(x,SL2);
        y=_mm_and_si128(y,mask);
        z=_mm_xor_si128(z,x);
        z=_mm_xor_si128(z,y);
        _mm_store_si128(&t[i],z);
        c=d; d=z;
    }
}
#endif

/*┌───────────────────────┐
　│32ビット符号なし長整数乱数、他の乱数の基本関数│
　└───────────────────────┘*/
#ifdef __GNUC__
__inline__
#endif
unsigned long NextMt_r(state_p mt)
{
    static volatile unsigned long seed=1;

    if (mt->index==0)
    { if (mt==DefaultMt) InitMt_r(mt,1); else InitMt_r(mt,++seed); }
    if (mt->index==N32) {
#ifdef SSE2
        if (mt->sse2) gen_rand_all_sse2(mt->u.m); else
#endif
        gen_rand_all(mt->u.x);
        mt->index=0;
    }
    return mt->u.x[mt->index++];
}

/*┌──────────────────────┐
　│０以上１未満の浮動小数点一様乱数(53bit精度) │
　└──────────────────────┘*/
double NextUnifEx_r(state_p mt)
{
    unsigned long x=NextMt_r(mt)>>5,y=NextMt_r(mt)>>6;
    return(x*67108864.0+y)*(1.0/9007199254740992.0);
}

/*┌───────────────────────────┐
　│丸め誤差のない０以上range未満の整数乱数　　　　　　　 │
　│　　　　　　　　　　　　　　　　　　　　　　　　　　　│
　│rangeが一定ならNextInt_r()よりも３倍高速　　　　　　　│
　│for (i=0;i<10000;i++)　　　　　　　　　　 　　　　　　│
　│{ x[i]=NextIntEx(100); y[i]=NextIntEx(200); } 　　　　│
　│とするよりも　　　　　　　　　　　　　　　　　　　　　│
　│for (i=0;i<10000;i++) x[i]=NextIntEx(100);　　　　　　│
　│for (i=0;i<10000;i++) y[i]=NextIntEx(200);　　　　　　│
　│としたほうが５倍高速になる。また、複数系列を使って　　│
　│for (i=0;i<10000;i++)　　　　　　　　　　 　　　　　　│
　│{ x[i]=NextIntEx_r(a,100); y[i]=NextIntEx_r(b,200); } │
　│としても５倍高速になる　　　　　　　　　　　　　　　　│
　└───────────────────────────┘*/
long NextIntEx_r(state_p mt,long range)
{
    unsigned long y,base,remain; int shift;

    if (range<=0) return 0;
    if (range!=mt->range) {
        mt->base=mt->range=range;
        for (mt->shift=0;mt->base<=(1UL<<30);mt->shift++)
            mt->base<<=1;
    }
    for (;;) {
        y=NextMt_r(mt)>>1;
        if (y<mt->base) return(long)(y>>mt->shift);
        base=mt->base; shift=mt->shift; y-=base;
        remain=(1UL<<31)-base;
        for (;remain>=(unsigned long)range;remain-=base) {
            for (;base>remain;base>>=1) shift--;
            if (y<base) return(long)(y>>shift); else y-=base;
        }
    }
}

/*┌──────────────┐
　│自由度νのカイ２乗分布 p.27 │
　└──────────────┘*/
double NextChisq_r(state_p mt,double n)
{
    return 2*NextGamma_r(mt,0.5*n);
}

/*┌───────────────┐
　│パラメータａのガンマ分布 p.31 │
　└───────────────┘*/
double NextGamma_r(state_p mt,double a)
{
    double t,u,x,y;
    if (a>1) {
        t=sqrt(2*a-1);
        do {
            do {
                do {
                    x=1-NextUnif_r(mt);
                    y=2*NextUnif_r(mt)-1;
                } while (x*x+y*y>1);
                y/=x; x=t*y+a-1;
            } while (x<=0);
            u=(a-1)*log(x/(a-1))-t*y;
        } while (u<-50||NextUnif_r(mt)>(1+y*y)*exp(u));
    } else {
        t=2.718281828459045235/(a+2.718281828459045235);
        do {
            if (NextUnif_r(mt)<t) {
                x=pow(NextUnif_r(mt),1/a); y=exp(-x);
            } else {
                x=1-log(1-NextUnif_r(mt)); y=pow(x,a-1);
            }
        } while (NextUnif_r(mt)>=y);
    }
    return x;
}

/*┌───────────┐
　│確率Ｐの幾何分布 p.34 │
　└───────────┘*/
int NextGeometric_r(state_p mt,double p)
{ return(int)ceil(log(1.0-NextUnif_r(mt))/log(1-p)); }

/*┌───────┐
　│三角分布 p.89 │
　└───────┘*/
double NextTriangle_r(state_p mt)
{ double a=NextUnif_r(mt),b=NextUnif_r(mt); return a-b; }

/*┌───────────┐
　│平均１の指数分布 p.106│
　└───────────┘*/
double NextExp_r(state_p mt)
{ return-log(1-NextUnif_r(mt)); }

/*┌─────────────────┐
　│標準正規分布(最大6.660437σ) p.133│
　└─────────────────┘*/
double NextNormal_r(state_p mt)
{
    if (mt->normal_sw==0) {
        double t=sqrt(-2*log(1.0-NextUnif_r(mt)));
        double u=3.141592653589793*2*NextUnif_r(mt);
        mt->normal_save=t*sin(u); mt->normal_sw=1; return t*cos(u);
    }else{ mt->normal_sw=0; return mt->normal_save; }
}

/*┌─────────────────┐
　│Ｎ次元のランダム単位ベクトル p.185│
　└─────────────────┘*/
void NextUnitVect_r(state_p mt,double*v,int n)
{
    int i; double r=0;
    for (i=0;i<n;i++) { v[i]=NextNormal_r(mt); r+=v[i]*v[i]; }
    r=sqrt(r);
    for (i=0;i<n;i++) v[i]/=r;
}

/*┌────────────────┐
　│パラメータＮ,Ｐの２項分布 p.203 │
　└────────────────┘*/
int NextBinomial_r(state_p mt,int n,double p)
{
    int i,r=0;
    for (i=0;i<n;i++) if (NextUnif_r(mt)<p) r++;
    return r;
}

/*┌────────────────┐
　│相関係数Ｒの２変量正規分布 p.211│
　└────────────────┘*/
void NextBinormal_r(state_p mt,double r,double*x,double*y)
{
    double r1,r2,s;
    do {
        r1=2*NextUnif_r(mt)-1;
        r2=2*NextUnif_r(mt)-1;
        s=r1*r1+r2*r2;
    } while (s>1||s==0);
    s= -log(s)/s; r1=sqrt((1+r)*s)*r1;
    r2=sqrt((1-r)*s)*r2; *x=r1+r2; *y=r1-r2;
}

/*┌─────────────────┐
　│パラメータＡ,Ｂのベータ分布 p.257 │
　└─────────────────┘*/
double NextBeta_r(state_p mt,double a,double b)
{
    double temp=NextGamma_r(mt,a);
    return temp/(temp+NextGamma_r(mt,b));
}

/*┌──────────────┐
　│パラメータＮの累乗分布 p.305│
　└──────────────┘*/
double NextPower_r(state_p mt,double n)
{ return pow(NextUnif_r(mt),1.0/(n+1)); }

/*┌────────────┐
　│ロジスティック分布 p.313│
　└────────────┘*/
double NextLogistic_r(state_p mt)
{
    double r;
    do r=NextUnif_r(mt); while (r==0);
    return log(r/(1-r));
}

/*┌─────────┐
　│コーシー分布 p.331│
　└─────────┘*/
double NextCauchy_r(state_p mt)
{
    double x,y;
    do { x=1-NextUnif_r(mt); y=2*NextUnif_r(mt)-1; }
    while (x*x+y*y>1);
    return y/x;
}

/*┌─────────────┐
　│自由度Ａ,ＢのＦ分布 p.344 │
　└─────────────┘*/
double NextFDist_r(state_p mt,double n1,double n2)
{
    double nc1=NextChisq_r(mt,n1),nc2=NextChisq_r(mt,n2);
    return (nc1*n2)/(nc2*n1);
}

/*┌─────────────┐
　│平均λのポアソン分布 p.412│
　└─────────────┘*/
int NextPoisson_r(state_p mt,double lambda)
{
    int k; lambda=exp(lambda)*NextUnif_r(mt);
    for (k=0;lambda>1;k++) lambda*=NextUnif_r(mt);
    return k;
}

/*┌───────────┐
　│自由度Ｎのｔ分布 p.428│
　└───────────┘*/
double NextTDist_r(state_p mt,double n)
{
    double a,b,c;
    if (n<=2) {
        do a=NextChisq_r(mt,n); while (a==0);
        return NextNormal_r(mt)/sqrt(a/n);
    }
    do {
        a=NextNormal_r(mt); b=a*a/(n-2);
        c=log(1-NextUnif_r(mt))/(1-0.5*n);
    } while (exp(-b-c)>1-b);
    return a/sqrt((1-2.0/n)*(1-b));
}

/*┌────────────────┐
　│パラメータαのワイブル分布 p.431│
　└────────────────┘*/
double NextWeibull_r(state_p mt,double alpha)
{ return pow(-log(1-NextUnif_r(mt)),1/alpha); }