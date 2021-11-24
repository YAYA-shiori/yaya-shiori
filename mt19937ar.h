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

#ifndef	MTRANDH
#define	MTRANDH

#ifdef __cplusplus
extern "C" {
#endif

//because this code is based on C code, we cannot use std::int32_t / int64_t.

typedef long std_int32_t;
#if _MSC_VER <= 1200
typedef __int64 std_int64_t;
#else
typedef long long std_int64_t;
#endif //1200

typedef struct MersenneTwister {
	unsigned long state[624]; /* the array for the state vector  */
	int left;
//	int initf;
	unsigned long *next;
} MersenneTwister;

/* initializes mt[N] with a seed */
void init_genrand(MersenneTwister &rs,unsigned long s);

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(MersenneTwister &rs,const unsigned long init_key[],const int key_length);

/* generates a random number on [0,0xffffffff]-interval */
std_int32_t genrand_int32(MersenneTwister &rs);

/* generates a random number on [0,0x7fffffff]-interval */
std_int32_t genrand_int31(MersenneTwister &rs);

// Returns an 64bit long in the range [0,2^64-1]
// Its lowest value is : 0
// Its highest value is: 18446744073709551615
//
std_int64_t genrand_int64(MersenneTwister& rs);

// Returns an 63bit long in the range [0,2^63-1]
// Its lowest value is : 0
// Its highest value is: 9223372036854775807
//
std_int64_t genrand_int63(MersenneTwister& rs);

/* These real versions are due to Isaku Wada, 2002/01/09 added */
/* generates a random number on [0,1]-real-interval */
double genrand_real1(MersenneTwister &rs);

/* generates a random number on [0,1)-real-interval */
double genrand_real2(MersenneTwister &rs);

/* generates a random number on (0,1)-real-interval */
double genrand_real3(MersenneTwister &rs);

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(MersenneTwister &rs);

#ifdef __cplusplus
}
#endif

#endif
