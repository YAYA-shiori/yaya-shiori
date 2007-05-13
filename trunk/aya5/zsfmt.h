/*
zsfmt.h 乱数ライブラリ・ヘッダファイル
coded by isaku@pb4.so-net.ne.jp
*/

#ifndef __zsfmt_h
#define __zsfmt_h

/*
アルゴリズムは
http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html
の SFMT を使った。使用方法は zmtrand と全く同じ。
SSE2 は、VC++ 2003,2005 のみに対応している。
Intel コンパイラで SSE2 を使う場合は修正が必要。
──────────────────────────────────
通常マクロ   複数系列     説明
──────────────────────────────────
             state_t        複数系列の領域宣言用の型
InitMt()     InitMt_r()     整数の種Ｓによる初期化
InitMtEx()   InitMtEx_r()   長さＬの配列Ｋによる初期化
NextMt()     NextMt_r()     32ビット符号なし整数の乱数
NextUnif()   NextUnif_r()   ０以上１未満の乱数(二つともマクロ)
NextUnifEx() NextUnifEx_r() ０以上１未満の乱数(53bit精度)
NextInt()    NextInt_r()    ０以上Ｎ未満の整数乱数(二つともマクロ)
NextIntEx()  NextIntEx_r()  丸め誤差のない０以上Ｎ未満の整数乱数
──────────────────────────────────

 InitMt()、NextUnif() 等 _r の付かない関数は通常の使い方をするマクロ
 である。

 対して InitMt_r() NextUnif_r() 等はマルチスレッドなプログラムや複数
 系列の乱数を必要とする大規模シミュレーション用の関数である。 これら
 は第一引数で state_t により確保された領域を要求する。 異なる領域を
 使えば、互いの呼び出しから独立した系列を得ることかできる。

 ※InitMt()、InitMtEx() を呼び出さずに NextMt() 等を呼び出しても、
   自動的に乱数の状態を初期化する。

 ※NextMt_r() 等で指定する領域を、スタック領域や０に初期化されない
   malloc() 領域などにするときは、最初に必ず InitMt_r() か
   InitMtEx_r() で明示的に初期化すること。Ｃ＋＋言語なら、
   明示的に初期化しなくても、適当な種で初期化される。
──────────────────────────────────
●各分布に従う乱数
「Ｃ言語による最新アルゴリズム事典」1991 技術評論社 を参考にした
──────────────────────────────────
通常マクロ      複数系列          説明
──────────────────────────────────
NextChisq()     NextChisq_r()     自由度νのカイ２乗分布       p. 27
NextGamma()     NextGamma_r()     パラメータａのガンマ分布     p. 31
NextGeometric() NextGeometric_r() 確率Ｐの幾何分布             p. 34
NextTriangle()  NextTriangle_r()  三角分布                     p. 89
NextExp()       NextExp_r()       平均１の指数分布             p.106
NextNormal()    NextNormal_r()    標準正規分布(最大6.660437σ) p.133
NextUnitVect()  NextUnitVect_r()  Ｎ次元のランダム単位ベクトル p.185
NextBinomial()  NextBinomial_r()  パラメータＮ,Ｐの２項分布    p.203
NextBinormal()  NextBinormal_r()  相関係数Ｒの２変量正規分布   p.211
NextBeta()      NextBeta_r()      パラメータＡ,Ｂのベータ分布  p.257
NextPower()     NextPower_r()     パラメータＮの累乗分布       p.305
NextLogistic()  NextLogistic_r()  ロジスティック分布           p.313
NextCauchy()    NextCauchy_r()    コーシー分布                 p.331
NextFDist()     NextFDist_r()     自由度Ａ,ＢのＦ分布          p.344
NextPoisson()   NextPoisson_r()   平均λのポアソン分布         p.412
NextTDist()     NextTDist_r()     自由度Ｎのｔ分布             p.430
NextWeibull()   NextWeibull_r()   パラメータαのワイブル分布   p.431
──────────────────────────────────
●パラメータの変更
この直後に、

     #define MEXP    607L
     #define MEXP  19937L
     #define MEXP 132049L

のいずれかのマクロを定義することによって、パラメータを選べる。
char*IdString; は、パラメーターを識別する文字列を記録している
──────────────────────────────────
*/
#define MEXP  19937L

#if MEXP == 607L

#define POS1    2
#define SL1     15
#define SL2     3
#define SR1     13
#define SR2     3
#define MSK1    0xfdff37ffUL
#define MSK2    0xef7f3f7dUL
#define MSK3    0xff777b7dUL
#define MSK4    0x7ff7fb2fUL
#define PARITY1 0x00000001UL
#define PARITY2 0x00000000UL
#define PARITY3 0x00000000UL
#define PARITY4 0x5986f054UL
#define IDSTR \
"SFMT-607:2-15-3-13-3:fdff37ff-ef7f3f7d-ff777b7d-7ff7fb2f"

#elif MEXP == 19937L

#define POS1    122
#define SL1     18
#define SL2     1
#define SR1     11
#define SR2     1
#define MSK1    0xdfffffefUL
#define MSK2    0xddfecb7fUL
#define MSK3    0xbffaffffUL
#define MSK4    0xbffffff6UL
#define PARITY1 0x00000001UL
#define PARITY2 0x00000000UL
#define PARITY3 0x00000000UL
#define PARITY4 0x13c9e684UL
#define IDSTR \
"SFMT-19937:122-18-1-11-1:dfffffef-ddfecb7f-bffaffff-bffffff6"

#elif MEXP == 132049L

#define POS1    110
#define SL1     19
#define SL2     1
#define SR1     21
#define SR2     1
#define MSK1    0xffffbb5fUL
#define MSK2    0xfb6ebf95UL
#define MSK3    0xfffefffaUL
#define MSK4    0xcff77fffUL
#define PARITY1 0x00000001UL
#define PARITY2 0x00000000UL
#define PARITY3 0xcb520000UL
#define PARITY4 0xc7e91c7dUL
#define IDSTR \
"SFMT-132049:110-19-1-21-1:ffffbb5f-fb6ebf95-fffefffa-cff77fff"

#endif

#if _MSC_VER >= 1310 && LONG_MAX == 0x7fffffffL && !defined(SSE2)
#define SSE2
#endif

#ifdef SSE2
#include <emmintrin.h>
#endif

typedef struct _state_t {
    unsigned      index;       /* インデックス */
    long          range;       /* NextIntEx で前回の範囲 */
    unsigned long base;        /* NextIntEx で前回の基準値 */
    int           shift;       /* NextIntEx で前回のシフト数 */
    int           sse2;        /* SSE2 が使用可能であれば 1 */
    int           normal_sw;   /* NextNormal で残りを持っている */
    double        normal_save; /* NextNormal の残りの値 */
    union {
#ifdef SSE2
        __m128i m[(int)(MEXP/128)+1];
#endif
        unsigned long x[((int)(MEXP/128)+1)*4]; /* 状態テーブル */
    } u;
#ifdef __cplusplus
    _state_t() { index=0; }
#endif
}state_t[1];

typedef struct _state_t*state_p;
extern state_t DefaultMt;
extern char*IdString;

extern unsigned long NextMt_r(state_p);
extern void   InitMt_r    (state_p,unsigned long);
extern void   InitMtEx_r  (state_p,unsigned long*,unsigned);
extern double NextUnif_r  (state_p);
extern double NextUnifEx_r(state_p);
extern long   NextIntEx_r (state_p,long);

extern double NextChisq_r    (state_p,double);
extern double NextGamma_r    (state_p,double);
extern int    NextGeometric_r(state_p,double);
extern double NextTriangle_r (state_p);
extern double NextExp_r      (state_p);
extern double NextNormal_r   (state_p);
extern void   NextUnitVect_r (state_p,double*,int);
extern int    NextBinomial_r (state_p,int,double);
extern void   NextBinormal_r (state_p,double,double*,double*);
extern double NextBeta_r     (state_p,double,double);
extern double NextPower_r    (state_p,double);
extern double NextLogistic_r (state_p);
extern double NextCauchy_r   (state_p);
extern double NextFDist_r    (state_p,double,double);
extern int    NextPoisson_r  (state_p,double);
extern double NextTDist_r    (state_p,double);
extern double NextWeibull_r  (state_p,double);

#define InitMt(S)      InitMt_r    (DefaultMt,S)
#define InitMtEx(K,L)  InitMtEx_r  (DefaultMt,K,L)
#define NextMt()       NextMt_r    (DefaultMt)
#define NextUnif_r(M)  (NextMt_r(M)*(1.0/4294967296.0))
#define NextUnif()     NextUnif_r  (DefaultMt)
#define NextUnifEx()   NextUnifEx_r(DefaultMt)
#define NextInt_r(M,N) ((long)((N)*(1.0/4294967296.0)*NextMt_r(M)))
#define NextInt(N)     NextInt_r   (DefaultMt,N)
#define NextIntEx(N)   NextIntEx_r (DefaultMt,N)

#define NextChisq(N)        NextChisq_r    (DefaultMt,N)
#define NextGamma(A)        NextGamma_r    (DefaultMt,A)
#define NextGeometric(P)    NextGeometric_r(DefaultMt,P)
#define NextTriangle()      NextTriangle_r (DefaultMt)
#define NextExp()           NextExp_r      (DefaultMt)
#define NextNormal()        NextNormal_r   (DefaultMt)
#define NextUnitVect(V,N)   NextUnitVect_r (DefaultMt,V,N)
#define NextBinomial(N,P)   NextBinomial_r (DefaultMt,N,P)
#define NextBinormal(R,X,Y) NextBinormal_r (DefaultMt,R,X,Y)
#define NextBeta(A,B)       NextBeta_r     (DefaultMt,A,B)
#define NextPower(N)        NextPower_r    (DefaultMt,N)
#define NextLogistic()      NextLogistic_r (DefaultMt)
#define NextCauchy()        NextCauchy_r   (DefaultMt)
#define NextFDist(A,B)      NextFDist_r    (DefaultMt,A,B)
#define NextPoisson(L)      NextPoisson_r  (DefaultMt,L)
#define NextTDist(N)        NextTDist_r    (DefaultMt,N)
#define NextWeibull(A)      NextWeibull_r  (DefaultMt,A)

#endif