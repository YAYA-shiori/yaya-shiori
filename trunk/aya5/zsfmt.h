/*
zsfmt.h �������C�u�����E�w�b�_�t�@�C��
coded by isaku@pb4.so-net.ne.jp
*/

#ifndef __zsfmt_h
#define __zsfmt_h

/*
�A���S���Y����
http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html
�� SFMT ���g�����B�g�p���@�� zmtrand �ƑS�������B
SSE2 �́AVC++ 2003,2005 �݂̂ɑΉ����Ă���B
Intel �R���p�C���� SSE2 ���g���ꍇ�͏C�����K�v�B
��������������������������������������������������������������������
�ʏ�}�N��   �����n��     ����
��������������������������������������������������������������������
             state_t        �����n��̗̈�錾�p�̌^
InitMt()     InitMt_r()     �����̎�r�ɂ�鏉����
InitMtEx()   InitMtEx_r()   �����k�̔z��j�ɂ�鏉����
NextMt()     NextMt_r()     32�r�b�g�����Ȃ������̗���
NextUnif()   NextUnif_r()   �O�ȏ�P�����̗���(��Ƃ��}�N��)
NextUnifEx() NextUnifEx_r() �O�ȏ�P�����̗���(53bit���x)
NextInt()    NextInt_r()    �O�ȏ�m�����̐�������(��Ƃ��}�N��)
NextIntEx()  NextIntEx_r()  �ۂߌ덷�̂Ȃ��O�ȏ�m�����̐�������
��������������������������������������������������������������������

 InitMt()�ANextUnif() �� _r �̕t���Ȃ��֐��͒ʏ�̎g����������}�N��
 �ł���B

 �΂��� InitMt_r() NextUnif_r() ���̓}���`�X���b�h�ȃv���O�����╡��
 �n��̗�����K�v�Ƃ����K�̓V�~�����[�V�����p�̊֐��ł���B �����
 �͑������� state_t �ɂ��m�ۂ��ꂽ�̈��v������B �قȂ�̈��
 �g���΁A�݂��̌Ăяo������Ɨ������n��𓾂邱�Ƃ��ł���B

 ��InitMt()�AInitMtEx() ���Ăяo������ NextMt() �����Ăяo���Ă��A
   �����I�ɗ����̏�Ԃ�����������B

 ��NextMt_r() ���Ŏw�肷��̈���A�X�^�b�N�̈��O�ɏ���������Ȃ�
   malloc() �̈�Ȃǂɂ���Ƃ��́A�ŏ��ɕK�� InitMt_r() ��
   InitMtEx_r() �Ŗ����I�ɏ��������邱�ƁB�b�{�{����Ȃ�A
   �����I�ɏ��������Ȃ��Ă��A�K���Ȏ�ŏ����������B
��������������������������������������������������������������������
���e���z�ɏ]������
�u�b����ɂ��ŐV�A���S���Y�����T�v1991 �Z�p�]�_�� ���Q�l�ɂ���
��������������������������������������������������������������������
�ʏ�}�N��      �����n��          ����
��������������������������������������������������������������������
NextChisq()     NextChisq_r()     ���R�x�˂̃J�C�Q�敪�z       p. 27
NextGamma()     NextGamma_r()     �p�����[�^���̃K���}���z     p. 31
NextGeometric() NextGeometric_r() �m���o�̊􉽕��z             p. 34
NextTriangle()  NextTriangle_r()  �O�p���z                     p. 89
NextExp()       NextExp_r()       ���ςP�̎w�����z             p.106
NextNormal()    NextNormal_r()    �W�����K���z(�ő�6.660437��) p.133
NextUnitVect()  NextUnitVect_r()  �m�����̃����_���P�ʃx�N�g�� p.185
NextBinomial()  NextBinomial_r()  �p�����[�^�m,�o�̂Q�����z    p.203
NextBinormal()  NextBinormal_r()  ���֌W���q�̂Q�ϗʐ��K���z   p.211
NextBeta()      NextBeta_r()      �p�����[�^�`,�a�̃x�[�^���z  p.257
NextPower()     NextPower_r()     �p�����[�^�m�̗ݏ敪�z       p.305
NextLogistic()  NextLogistic_r()  ���W�X�e�B�b�N���z           p.313
NextCauchy()    NextCauchy_r()    �R�[�V�[���z                 p.331
NextFDist()     NextFDist_r()     ���R�x�`,�a�̂e���z          p.344
NextPoisson()   NextPoisson_r()   ���σɂ̃|�A�\�����z         p.412
NextTDist()     NextTDist_r()     ���R�x�m�̂����z             p.430
NextWeibull()   NextWeibull_r()   �p�����[�^���̃��C�u�����z   p.431
��������������������������������������������������������������������
���p�����[�^�̕ύX
���̒���ɁA

     #define MEXP    607L
     #define MEXP  19937L
     #define MEXP 132049L

�̂����ꂩ�̃}�N�����`���邱�Ƃɂ���āA�p�����[�^��I�ׂ�B
char*IdString; �́A�p�����[�^�[�����ʂ��镶������L�^���Ă���
��������������������������������������������������������������������
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
    unsigned      index;       /* �C���f�b�N�X */
    long          range;       /* NextIntEx �őO��͈̔� */
    unsigned long base;        /* NextIntEx �őO��̊�l */
    int           shift;       /* NextIntEx �őO��̃V�t�g�� */
    int           sse2;        /* SSE2 ���g�p�\�ł���� 1 */
    int           normal_sw;   /* NextNormal �Ŏc��������Ă��� */
    double        normal_save; /* NextNormal �̎c��̒l */
    union {
#ifdef SSE2
        __m128i m[(int)(MEXP/128)+1];
#endif
        unsigned long x[((int)(MEXP/128)+1)*4]; /* ��ԃe�[�u�� */
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