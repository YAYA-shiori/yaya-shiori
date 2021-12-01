/* 
   A C-program for MT19937, with initialization improved 2002/2/10.
   Coded by Takuji Nishimura and Makoto Matsumoto.
   This is a faster version by taking Shawn Cokus's optimization,
   Matthe Bellew's simplification, Isaku Wada's real version.

   Before using, initialize the state by using init_genrand(seed) 
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include "mt19937ar.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Period parameters */  
#define N 624
#define M 397
#undef MATRIX_A
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UMASK 0x80000000UL /* most significant w-r bits */
#define LMASK 0x7fffffffUL /* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

/*static unsigned long state[N]; //the array for the state vector
static int left = 1;
static int initf = 0;
static unsigned long *next;  */

/* initializes state[N] with a seed */
void init_genrand(MersenneTwister &rs,unsigned long s)
{
    int j;
    rs.state[0]= s & 0xffffffffUL;
    for (j=1; j<N; j++) {
        rs.state[j] = (1812433253UL * (rs.state[j-1] ^ (rs.state[j-1] >> 30)) + j); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array rs.state[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        rs.state[j] &= 0xffffffffUL;  /* for >32 bit machines */
    }
    rs.left = 1; /*rs.initf = 1;*/
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(MersenneTwister &rs,const unsigned long init_key[], const int key_length)
{
    int i, j, k;
    init_genrand(rs,19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        rs.state[i] = (rs.state[i] ^ ((rs.state[i-1] ^ (rs.state[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        rs.state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { rs.state[0] = rs.state[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        rs.state[i] = (rs.state[i] ^ ((rs.state[i-1] ^ (rs.state[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        rs.state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { rs.state[0] = rs.state[N-1]; i=1; }
    }

    rs.state[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
    rs.left = 1; /*rs.initf = 1;*/
}

static void next_state(MersenneTwister &rs)
{
    unsigned long *p=rs.state;
    int j;

    /* if init_genrand() has not been called, */
    /* a default initial seed is used         */
    //if (rs.initf==0) init_genrand(rs,5489UL);

    rs.left = N;
    rs.next = rs.state;
    
    for (j=N-M+1; --j; p++) 
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j=M; --j; p++) 
        *p = p[M-N] ^ TWIST(p[0], p[1]);

    *p = p[M-N] ^ TWIST(p[0], rs.state[0]);
}

/* generates a random number on [0,0xffffffff]-interval */
std_int32_t genrand_int32(MersenneTwister &rs)
{
    unsigned long y;

    if (--rs.left == 0) next_state(rs);
    y = *rs.next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
std_int32_t genrand_int31(MersenneTwister &rs)
{
    unsigned long y;

    if (--rs.left == 0) next_state(rs);
    y = *rs.next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (long)(y>>1);
}

/* generates a random number on [0,1]-real-interval */
double genrand_real1(MersenneTwister &rs)
{
    unsigned long y;

    if (--rs.left == 0) next_state(rs);
    y = *rs.next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (double)y * (1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double genrand_real2(MersenneTwister &rs)
{
    unsigned long y;

    if (--rs.left == 0) next_state(rs);
    y = *rs.next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (double)y * (1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double genrand_real3(MersenneTwister &rs)
{
    unsigned long y;

    if (--rs.left == 0) next_state(rs);
    y = *rs.next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return ((double)y + 0.5) * (1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(MersenneTwister &rs) 
{ 
    unsigned long a=genrand_int32(rs)>>5, b=genrand_int32(rs)>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */

#define NN 312
#define MM 156
#undef MATRIX_A
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */


/* The array for the state vector //
static unsigned long long mt[NN]; 
static int mti=NN+1; 
*/

/* initializes mt[NN] with a seed */
void init_genrand64(MersenneTwister64& rs,unsigned long long seed)
{
	rs.mt[0] = seed;
	for (rs.mti=1; rs.mti<NN; rs.mti++) 
		rs.mt[rs.mti] =  (6364136223846793005ULL * (rs.mt[rs.mti-1] ^ (rs.mt[rs.mti-1] >> 62)) + rs.mti);
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
void init_by_array64(MersenneTwister64& rs,const unsigned long long init_key[],
		     unsigned long long key_length)
{
	unsigned long long i, j, k;
	init_genrand64(rs,19650218ULL);
	i=1; j=0;
	k = (NN>key_length ? NN : key_length);
	for (; k; k--) {
		rs.mt[i] = (rs.mt[i] ^ ((rs.mt[i-1] ^ (rs.mt[i-1] >> 62)) * 3935559000370003845ULL))
		  + init_key[j] + j; /* non linear */
		i++; j++;
		if (i>=NN) { rs.mt[0] = rs.mt[NN-1]; i=1; }
		if (j>=key_length) j=0;
	}
	for (k=NN-1; k; k--) {
		rs.mt[i] = (rs.mt[i] ^ ((rs.mt[i-1] ^ (rs.mt[i-1] >> 62)) * 2862933555777941757ULL))
		  - i; /* non linear */
		i++;
		if (i>=NN) { rs.mt[0] = rs.mt[NN-1]; i=1; }
	}

	rs.mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0, 2^64-1]-interval */
unsigned long long genrand64_int64(MersenneTwister64& rs)
{
	int i;
	unsigned long long x;
	static unsigned long long mag01[2]={0ULL, MATRIX_A};

	if (rs.mti >= NN) { /* generate NN words at one time */
		for (i=0;i<NN-MM;i++) {
			x = (rs.mt[i]&UM)|(rs.mt[i+1]&LM);
			rs.mt[i] = rs.mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
		}
		for (;i<NN-1;i++) {
			x = (rs.mt[i]&UM)|(rs.mt[i+1]&LM);
			rs.mt[i] = rs.mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
		}
		x = (rs.mt[NN-1]&UM)|(rs.mt[0]&LM);
		rs.mt[NN-1] = rs.mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

		rs.mti = 0;
	}
  
	x = rs.mt[rs.mti++];

	x ^= (x >> 29) & 0x5555555555555555ULL;
	x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
	x ^= (x << 37) & 0xFFF7EEE000000000ULL;
	x ^= (x >> 43);

	return x;
}

/* generates a random number on [0, 2^63-1]-interval */
long long genrand64_int63(MersenneTwister64& rs)
{
	return (long long)(genrand64_int64(rs) >> 1);
}

/* generates a random number on [0,1]-real-interval */
double genrand64_real1(MersenneTwister64& rs)
{
	return (genrand64_int64(rs) >> 11) * (1.0/9007199254740991.0);
}

/* generates a random number on [0,1)-real-interval */
double genrand64_real2(MersenneTwister64& rs)
{
	return (genrand64_int64(rs) >> 11) * (1.0/9007199254740992.0);
}

/* generates a random number on (0,1)-real-interval */
double genrand64_real3(MersenneTwister64& rs)
{
	return ((genrand64_int64(rs) >> 12) + 0.5) * (1.0/4503599627370496.0);
}

#ifdef __cplusplus
}
#endif
