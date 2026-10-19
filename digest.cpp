//
// AYA version 5
//
// ダイジェスト(MD5 / SHA-1 / SHA-2 / SHA-3 / Adler-32 / 各種 CRC)の計算
//

#include <string.h>

#include "digest.h"

namespace {

typedef digest_u32	u32;
typedef digest_u64	u64;

enum {
	F_CRC,
	F_ADLER32,
	F_MD5,
	F_SHA1,
	F_SHA256,		// SHA-224 / SHA-256
	F_SHA512,		// SHA-384 / SHA-512
	F_SHA3
};

enum {
	CRC_REFLECT	= 1,	// 入力・出力のビット順を反転する(下位ビットから処理する)
	CRC_LE_OUT	= 2		// 結果を下位バイトから先に出す(従来の crc32 の出力順)
};

} // namespace

// アルゴリズムの定義
struct CDigest::Algo
{
	const char		*names;		// 正規化した(小文字の英数字だけの)名前を空白で区切って並べる。先頭が正式名
	unsigned char	family;
	unsigned char	size;		// ダイジェスト値のバイト数
	unsigned char	block;		// 1ブロックのバイト数(SHA-3 は rate)
	unsigned char	variant;	// SHA-2 の 224 / 384 は 1
	unsigned char	width;		// CRC のビット幅
	unsigned char	flags;		// CRC_*
	u32				poly[2];	// CRC の多項式・初期値・最終 XOR(上位32bit, 下位32bit の順)
	u32				init[2];
	u32				xorout[2];
};

namespace {

inline u32 RotateLeft(u32 x, unsigned int n)
{
	return (x << n) | (x >> (32 - n));
}

inline u32 RotateRight(u32 x, unsigned int n)
{
	return (x >> n) | (x << (32 - n));
}

inline u64 RotateRight64(u64 x, unsigned int n)
{
	return (x >> n) | (x << (64 - n));
}

inline u64 RotateLeft64(u64 x, unsigned int n)
{
	return n ? ((x << n) | (x >> (64 - n))) : x;
}

inline u64 Pair(const u32 *p)
{
	return (static_cast<u64>(p[0]) << 32) | static_cast<u64>(p[1]);
}

inline u64 Pair(u32 hi, u32 lo)
{
	return (static_cast<u64>(hi) << 32) | static_cast<u64>(lo);
}

// ビット幅 width の値のビット順を反転する
u64 Reflect(u64 v, unsigned int width)
{
	u64 r = 0;
	for ( unsigned int i = 0 ; i < width ; ++i ) {
		if ( (v >> i) & 1 ) {
			r |= static_cast<u64>(1) << (width - 1 - i);
		}
	}
	return r;
}

u64 WidthMask(unsigned int width)
{
	return (width >= 64) ? ~static_cast<u64>(0) : ((static_cast<u64>(1) << width) - 1);
}

//----- アルゴリズム表 -----------------------------------------------------------

#define CRC_ROW(names, bytes, width, flags, poly_hi, poly_lo, init_hi, init_lo, xor_hi, xor_lo) \
	{ names, F_CRC, bytes, 0, 0, width, flags, { poly_hi, poly_lo }, { init_hi, init_lo }, { xor_hi, xor_lo } }

#define PLAIN_ROW(names, family, bytes, block, variant) \
	{ names, family, bytes, block, variant, 0, 0, { 0, 0 }, { 0, 0 }, { 0, 0 } }

// 各 CRC の check 値(入力 "123456789")はカタログ(Rocksoft model)のとおり。tests でも確かめている
const CDigest::Algo s_algos[] = {
	PLAIN_ROW("md5",		F_MD5,		16,	 64, 0),
	PLAIN_ROW("sha1",		F_SHA1,		20,	 64, 0),
	PLAIN_ROW("sha224",		F_SHA256,	28,	 64, 1),
	PLAIN_ROW("sha256",		F_SHA256,	32,	 64, 0),
	PLAIN_ROW("sha384",		F_SHA512,	48,	128, 1),
	PLAIN_ROW("sha512",		F_SHA512,	64,	128, 0),
	PLAIN_ROW("sha3224",	F_SHA3,		28,	144, 0),
	PLAIN_ROW("sha3256",	F_SHA3,		32,	136, 0),
	PLAIN_ROW("sha3384",	F_SHA3,		48,	104, 0),
	PLAIN_ROW("sha3512",	F_SHA3,		64,	 72, 0),
	PLAIN_ROW("adler32",	F_ADLER32,	 4,	  0, 0),
	// CRC-32 は従来どおり下位バイトから先に出す。標準的な表記(例: "123456789" → CBF43926)は crc32b
	CRC_ROW("crc32",					4, 32, CRC_REFLECT | CRC_LE_OUT, 0, 0x04C11DB7u, 0, 0xFFFFFFFFu, 0, 0xFFFFFFFFu),
	CRC_ROW("crc32b crc32isohdlc",		4, 32, CRC_REFLECT,              0, 0x04C11DB7u, 0, 0xFFFFFFFFu, 0, 0xFFFFFFFFu),
	CRC_ROW("crc32c crc32castagnoli",	4, 32, CRC_REFLECT,              0, 0x1EDC6F41u, 0, 0xFFFFFFFFu, 0, 0xFFFFFFFFu),
	CRC_ROW("crc32bzip2",				4, 32, 0,                        0, 0x04C11DB7u, 0, 0xFFFFFFFFu, 0, 0xFFFFFFFFu),
	CRC_ROW("crc32mpeg2",				4, 32, 0,                        0, 0x04C11DB7u, 0, 0xFFFFFFFFu, 0, 0),
	CRC_ROW("crc32jamcrc jamcrc",		4, 32, CRC_REFLECT,              0, 0x04C11DB7u, 0, 0xFFFFFFFFu, 0, 0),
	CRC_ROW("crc16arc crc16 crc16ibm",	2, 16, CRC_REFLECT,              0, 0x8005u,     0, 0,           0, 0),
	CRC_ROW("crc16ccittfalse crc16ibm3740", 2, 16, 0,                    0, 0x1021u,     0, 0xFFFFu,     0, 0),
	CRC_ROW("crc16xmodem crc16zmodem",	2, 16, 0,                        0, 0x1021u,     0, 0,           0, 0),
	CRC_ROW("crc16kermit crc16ccitttrue", 2, 16, CRC_REFLECT,            0, 0x1021u,     0, 0,           0, 0),
	CRC_ROW("crc16modbus",				2, 16, CRC_REFLECT,              0, 0x8005u,     0, 0xFFFFu,     0, 0),
	CRC_ROW("crc16x25 crc16ibmsdlc",	2, 16, CRC_REFLECT,              0, 0x1021u,     0, 0xFFFFu,     0, 0xFFFFu),
	CRC_ROW("crc8 crc8smbus",			1,  8, 0,                        0, 0x07u,       0, 0,           0, 0),
	CRC_ROW("crc8maxim crc8dow",		1,  8, CRC_REFLECT,              0, 0x31u,       0, 0,           0, 0),
	CRC_ROW("crc64xz crc64",			8, 64, CRC_REFLECT,              0x42F0E1EBu, 0xA9EA3693u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu),
	CRC_ROW("crc64ecma182",				8, 64, 0,                        0x42F0E1EBu, 0xA9EA3693u, 0, 0,           0, 0)
};

const size_t s_algo_count = sizeof(s_algos) / sizeof(s_algos[0]);

//----- CRC の表 -------------------------------------------------------------------

// CRC のアルゴリズムごとに256要素の表を持つ。静的初期化で作る
// (CRC の定義を足して MAX_CRC_ALGOS を超えると表が足りなくなる。tests の全アルゴリズム検査で気付ける)
struct CRCTables
{
	enum { MAX_CRC_ALGOS = 32 };

	u64		table[MAX_CRC_ALGOS][256];
	int		slot[sizeof(s_algos) / sizeof(s_algos[0])];

	CRCTables()
	{
		int used = 0;
		for ( size_t a = 0 ; a < s_algo_count ; ++a ) {
			slot[a] = -1;
			const CDigest::Algo &algo = s_algos[a];
			if ( algo.family != F_CRC || used >= MAX_CRC_ALGOS ) {
				continue;
			}
			slot[a] = used;
			u64 *tab = table[used];
			++used;

			const unsigned int width = algo.width;
			const u64 mask = WidthMask(width);
			const u64 poly = Pair(algo.poly);

			if ( algo.flags & CRC_REFLECT ) {
				const u64 rpoly = Reflect(poly, width);
				for ( u32 n = 0 ; n < 256 ; ++n ) {
					u64 c = n;
					for ( int k = 0 ; k < 8 ; ++k ) {
						c = (c & 1) ? ((c >> 1) ^ rpoly) : (c >> 1);
					}
					tab[n] = c;
				}
			}
			else {
				const u64 top = static_cast<u64>(1) << (width - 1);
				for ( u32 n = 0 ; n < 256 ; ++n ) {
					u64 c = static_cast<u64>(n) << (width - 8);
					for ( int k = 0 ; k < 8 ; ++k ) {
						c = (c & top) ? ((c << 1) ^ poly) : (c << 1);
					}
					tab[n] = c & mask;
				}
			}
		}
	}
};

const CRCTables	s_crc_tables;

//----- 定数表 ---------------------------------------------------------------------

// MD5: K[i] = floor(2^32 * abs(sin(i + 1)))
const u32 s_md5_k[64] = {
	0xD76AA478u, 0xE8C7B756u, 0x242070DBu, 0xC1BDCEEEu,
	0xF57C0FAFu, 0x4787C62Au, 0xA8304613u, 0xFD469501u,
	0x698098D8u, 0x8B44F7AFu, 0xFFFF5BB1u, 0x895CD7BEu,
	0x6B901122u, 0xFD987193u, 0xA679438Eu, 0x49B40821u,
	0xF61E2562u, 0xC040B340u, 0x265E5A51u, 0xE9B6C7AAu,
	0xD62F105Du, 0x02441453u, 0xD8A1E681u, 0xE7D3FBC8u,
	0x21E1CDE6u, 0xC33707D6u, 0xF4D50D87u, 0x455A14EDu,
	0xA9E3E905u, 0xFCEFA3F8u, 0x676F02D9u, 0x8D2A4C8Au,
	0xFFFA3942u, 0x8771F681u, 0x6D9D6122u, 0xFDE5380Cu,
	0xA4BEEA44u, 0x4BDECFA9u, 0xF6BB4B60u, 0xBEBFBC70u,
	0x289B7EC6u, 0xEAA127FAu, 0xD4EF3085u, 0x04881D05u,
	0xD9D4D039u, 0xE6DB99E5u, 0x1FA27CF8u, 0xC4AC5665u,
	0xF4292244u, 0x432AFF97u, 0xAB9423A7u, 0xFC93A039u,
	0x655B59C3u, 0x8F0CCC92u, 0xFFEFF47Du, 0x85845DD1u,
	0x6FA87E4Fu, 0xFE2CE6E0u, 0xA3014314u, 0x4E0811A1u,
	0xF7537E82u, 0xBD3AF235u, 0x2AD7D2BBu, 0xEB86D391u
};

// MD5: 4ラウンドそれぞれの、ステップ(i & 3)ごとの回転量
const unsigned int s_md5_shift[4][4] = {
	{  7, 12, 17, 22 },
	{  5,  9, 14, 20 },
	{  4, 11, 16, 23 },
	{  6, 10, 15, 21 }
};

// SHA-256: 先頭64個の素数の立方根の小数部の上位32bit
const u32 s_sha256_k[64] = {
	0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u,
	0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u,
	0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u,
	0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u,
	0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu,
	0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
	0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u,
	0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u,
	0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u,
	0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
	0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u,
	0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
	0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u,
	0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u,
	0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u,
	0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u
};

// SHA-256: 先頭8個の素数の平方根の小数部の上位32bit
const u32 s_sha256_iv[8] = {
	0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
	0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u
};

// SHA-224: 9～16個目の素数の平方根の小数部の、32bit目から64bit目
const u32 s_sha224_iv[8] = {
	0xC1059ED8u, 0x367CD507u, 0x3070DD17u, 0xF70E5939u,
	0xFFC00B31u, 0x68581511u, 0x64F98FA7u, 0xBEFA4FA4u
};

// SHA-512: 先頭80個の素数の立方根の小数部の上位64bit(上位32bit, 下位32bit の順)
const u32 s_sha512_k[80][2] = {
	{ 0x428A2F98u, 0xD728AE22u }, { 0x71374491u, 0x23EF65CDu },
	{ 0xB5C0FBCFu, 0xEC4D3B2Fu }, { 0xE9B5DBA5u, 0x8189DBBCu },
	{ 0x3956C25Bu, 0xF348B538u }, { 0x59F111F1u, 0xB605D019u },
	{ 0x923F82A4u, 0xAF194F9Bu }, { 0xAB1C5ED5u, 0xDA6D8118u },
	{ 0xD807AA98u, 0xA3030242u }, { 0x12835B01u, 0x45706FBEu },
	{ 0x243185BEu, 0x4EE4B28Cu }, { 0x550C7DC3u, 0xD5FFB4E2u },
	{ 0x72BE5D74u, 0xF27B896Fu }, { 0x80DEB1FEu, 0x3B1696B1u },
	{ 0x9BDC06A7u, 0x25C71235u }, { 0xC19BF174u, 0xCF692694u },
	{ 0xE49B69C1u, 0x9EF14AD2u }, { 0xEFBE4786u, 0x384F25E3u },
	{ 0x0FC19DC6u, 0x8B8CD5B5u }, { 0x240CA1CCu, 0x77AC9C65u },
	{ 0x2DE92C6Fu, 0x592B0275u }, { 0x4A7484AAu, 0x6EA6E483u },
	{ 0x5CB0A9DCu, 0xBD41FBD4u }, { 0x76F988DAu, 0x831153B5u },
	{ 0x983E5152u, 0xEE66DFABu }, { 0xA831C66Du, 0x2DB43210u },
	{ 0xB00327C8u, 0x98FB213Fu }, { 0xBF597FC7u, 0xBEEF0EE4u },
	{ 0xC6E00BF3u, 0x3DA88FC2u }, { 0xD5A79147u, 0x930AA725u },
	{ 0x06CA6351u, 0xE003826Fu }, { 0x14292967u, 0x0A0E6E70u },
	{ 0x27B70A85u, 0x46D22FFCu }, { 0x2E1B2138u, 0x5C26C926u },
	{ 0x4D2C6DFCu, 0x5AC42AEDu }, { 0x53380D13u, 0x9D95B3DFu },
	{ 0x650A7354u, 0x8BAF63DEu }, { 0x766A0ABBu, 0x3C77B2A8u },
	{ 0x81C2C92Eu, 0x47EDAEE6u }, { 0x92722C85u, 0x1482353Bu },
	{ 0xA2BFE8A1u, 0x4CF10364u }, { 0xA81A664Bu, 0xBC423001u },
	{ 0xC24B8B70u, 0xD0F89791u }, { 0xC76C51A3u, 0x0654BE30u },
	{ 0xD192E819u, 0xD6EF5218u }, { 0xD6990624u, 0x5565A910u },
	{ 0xF40E3585u, 0x5771202Au }, { 0x106AA070u, 0x32BBD1B8u },
	{ 0x19A4C116u, 0xB8D2D0C8u }, { 0x1E376C08u, 0x5141AB53u },
	{ 0x2748774Cu, 0xDF8EEB99u }, { 0x34B0BCB5u, 0xE19B48A8u },
	{ 0x391C0CB3u, 0xC5C95A63u }, { 0x4ED8AA4Au, 0xE3418ACBu },
	{ 0x5B9CCA4Fu, 0x7763E373u }, { 0x682E6FF3u, 0xD6B2B8A3u },
	{ 0x748F82EEu, 0x5DEFB2FCu }, { 0x78A5636Fu, 0x43172F60u },
	{ 0x84C87814u, 0xA1F0AB72u }, { 0x8CC70208u, 0x1A6439ECu },
	{ 0x90BEFFFAu, 0x23631E28u }, { 0xA4506CEBu, 0xDE82BDE9u },
	{ 0xBEF9A3F7u, 0xB2C67915u }, { 0xC67178F2u, 0xE372532Bu },
	{ 0xCA273ECEu, 0xEA26619Cu }, { 0xD186B8C7u, 0x21C0C207u },
	{ 0xEADA7DD6u, 0xCDE0EB1Eu }, { 0xF57D4F7Fu, 0xEE6ED178u },
	{ 0x06F067AAu, 0x72176FBAu }, { 0x0A637DC5u, 0xA2C898A6u },
	{ 0x113F9804u, 0xBEF90DAEu }, { 0x1B710B35u, 0x131C471Bu },
	{ 0x28DB77F5u, 0x23047D84u }, { 0x32CAAB7Bu, 0x40C72493u },
	{ 0x3C9EBE0Au, 0x15C9BEBCu }, { 0x431D67C4u, 0x9C100D4Cu },
	{ 0x4CC5D4BEu, 0xCB3E42B6u }, { 0x597F299Cu, 0xFC657E2Au },
	{ 0x5FCB6FABu, 0x3AD6FAECu }, { 0x6C44198Cu, 0x4A475817u }
};

// SHA-512: 先頭8個の素数の平方根の小数部の上位64bit
const u32 s_sha512_iv[8][2] = {
	{ 0x6A09E667u, 0xF3BCC908u }, { 0xBB67AE85u, 0x84CAA73Bu },
	{ 0x3C6EF372u, 0xFE94F82Bu }, { 0xA54FF53Au, 0x5F1D36F1u },
	{ 0x510E527Fu, 0xADE682D1u }, { 0x9B05688Cu, 0x2B3E6C1Fu },
	{ 0x1F83D9ABu, 0xFB41BD6Bu }, { 0x5BE0CD19u, 0x137E2179u }
};

// SHA-384: 9～16個目の素数の平方根の小数部の上位64bit
const u32 s_sha384_iv[8][2] = {
	{ 0xCBBB9D5Du, 0xC1059ED8u }, { 0x629A292Au, 0x367CD507u },
	{ 0x9159015Au, 0x3070DD17u }, { 0x152FECD8u, 0xF70E5939u },
	{ 0x67332667u, 0xFFC00B31u }, { 0x8EB44A87u, 0x68581511u },
	{ 0xDB0C2E0Du, 0x64F98FA7u }, { 0x47B5481Du, 0xBEFA4FA4u }
};

// Keccak-f[1600] の ι ステップの定数
const u32 s_keccak_rc[24][2] = {
	{ 0x00000000u, 0x00000001u }, { 0x00000000u, 0x00008082u },
	{ 0x80000000u, 0x0000808Au }, { 0x80000000u, 0x80008000u },
	{ 0x00000000u, 0x0000808Bu }, { 0x00000000u, 0x80000001u },
	{ 0x80000000u, 0x80008081u }, { 0x80000000u, 0x00008009u },
	{ 0x00000000u, 0x0000008Au }, { 0x00000000u, 0x00000088u },
	{ 0x00000000u, 0x80008009u }, { 0x00000000u, 0x8000000Au },
	{ 0x00000000u, 0x8000808Bu }, { 0x80000000u, 0x0000008Bu },
	{ 0x80000000u, 0x00008089u }, { 0x80000000u, 0x00008003u },
	{ 0x80000000u, 0x00008002u }, { 0x80000000u, 0x00000080u },
	{ 0x00000000u, 0x0000800Au }, { 0x80000000u, 0x8000000Au },
	{ 0x80000000u, 0x80008081u }, { 0x80000000u, 0x00008080u },
	{ 0x00000000u, 0x80000001u }, { 0x80000000u, 0x80008008u }
};

// Keccak-f[1600] の ρ ステップの回転量(レーン x + 5 * y の順)
const unsigned int s_keccak_rho[25] = {
	0, 1, 62, 28, 27,
	36, 44, 6, 55, 20,
	3, 10, 43, 25, 39,
	41, 45, 15, 21, 8,
	18, 2, 61, 56, 14
};

//----- 名前の正規化 ---------------------------------------------------------------

// 名前を小文字の英数字だけにして key に書く。長すぎる名前や ASCII 以外の文字を含む名前は失敗
bool NormalizeName(const wchar_t *name, char *key, size_t key_size)
{
	size_t n = 0;
	for ( ; *name ; ++name ) {
		const unsigned long c = static_cast<unsigned long>(*name);
		char ch;
		if ( c >= '0' && c <= '9' ) {
			ch = static_cast<char>(c);
		}
		else if ( c >= 'a' && c <= 'z' ) {
			ch = static_cast<char>(c);
		}
		else if ( c >= 'A' && c <= 'Z' ) {
			ch = static_cast<char>(c - 'A' + 'a');
		}
		else if ( c == '-' || c == '_' || c == ' ' || c == '/' || c == '.' ) {
			continue;
		}
		else {
			return false;
		}
		if ( n + 1 >= key_size ) {
			return false;
		}
		key[n++] = ch;
	}
	key[n] = 0;
	return n > 0;
}

// 空白区切りの names の中に key があるか
bool HasName(const char *names, const char *key)
{
	const size_t key_len = strlen(key);
	while ( *names ) {
		const char *end = names;
		while ( *end && *end != ' ' ) {
			++end;
		}
		if ( static_cast<size_t>(end - names) == key_len && memcmp(names, key, key_len) == 0 ) {
			return true;
		}
		names = *end ? end + 1 : end;
	}
	return false;
}

} // namespace

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::Find
 * -----------------------------------------------------------------------
 */
const CDigest::Algo *CDigest::Find(const wchar_t *name)
{
	char key[32];
	if ( ! NormalizeName(name, key, sizeof(key)) ) {
		return NULL;
	}
	for ( size_t i = 0 ; i < s_algo_count ; ++i ) {
		if ( HasName(s_algos[i].names, key) ) {
			return &s_algos[i];
		}
	}
	return NULL;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::AlgoCount / AlgoAt / AlgoNames
 * -----------------------------------------------------------------------
 */
size_t CDigest::AlgoCount(void)
{
	return s_algo_count;
}

const CDigest::Algo *CDigest::AlgoAt(size_t index)
{
	return (index < s_algo_count) ? &s_algos[index] : NULL;
}

const char *CDigest::AlgoNames(const Algo *algo)
{
	return algo->names;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::CDigest
 * -----------------------------------------------------------------------
 */
CDigest::CDigest(const Algo *algo)
	: m_algo(algo)
{
	Reset();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::Size
 * -----------------------------------------------------------------------
 */
size_t CDigest::Size(void) const
{
	return m_algo->size;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::Reset
 * -----------------------------------------------------------------------
 */
void CDigest::Reset(void)
{
	memset(m_h, 0, sizeof(m_h));
	memset(m_buf, 0, sizeof(m_buf));
	m_fill = 0;
	m_count = 0;

	switch ( m_algo->family ) {
	case F_CRC:
		// 反転形(下位ビットから処理する形)では、レジスタも反転形で持つ
		m_h[0] = (m_algo->flags & CRC_REFLECT) ? Reflect(Pair(m_algo->init), m_algo->width) : Pair(m_algo->init);
		break;
	case F_ADLER32:
		m_h[0] = 1;
		break;
	case F_MD5:
		m_h[0] = 0x67452301u;
		m_h[1] = 0xEFCDAB89u;
		m_h[2] = 0x98BADCFEu;
		m_h[3] = 0x10325476u;
		break;
	case F_SHA1:
		m_h[0] = 0x67452301u;
		m_h[1] = 0xEFCDAB89u;
		m_h[2] = 0x98BADCFEu;
		m_h[3] = 0x10325476u;
		m_h[4] = 0xC3D2E1F0u;
		break;
	case F_SHA256:
		{
			for ( int i = 0 ; i < 8 ; ++i ) {
				m_h[i] = m_algo->variant ? s_sha224_iv[i] : s_sha256_iv[i];
			}
		}
		break;
	case F_SHA512:
		{
			for ( int i = 0 ; i < 8 ; ++i ) {
				m_h[i] = Pair(m_algo->variant ? s_sha384_iv[i] : s_sha512_iv[i]);
			}
		}
		break;
	default: // F_SHA3 は状態が0から始まる
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::Update
 * -----------------------------------------------------------------------
 */
void CDigest::Update(const void *data, size_t len)
{
	const unsigned char *p = static_cast<const unsigned char*>(data);

	if ( m_algo->family == F_CRC ) {
		UpdateCRC(p, len);
		return;
	}
	if ( m_algo->family == F_ADLER32 ) {
		UpdateAdler32(p, len);
		return;
	}

	// 以下はブロック型(MD5 / SHA-1 / SHA-2 / SHA-3)
	const size_t bs = m_algo->block;

	m_count += static_cast<u64>(len);

	if ( m_fill ) {
		size_t n = bs - m_fill;
		if ( n > len ) {
			n = len;
		}
		memcpy(m_buf + m_fill, p, n);
		p += n;
		len -= n;
		m_fill += n;
		if ( m_fill < bs ) {
			return;
		}
		Compress(m_buf);
		m_fill = 0;
	}

	while ( len >= bs ) {
		Compress(p);
		p += bs;
		len -= bs;
	}

	if ( len ) {
		memcpy(m_buf, p, len);
		m_fill = len;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::Final
 * -----------------------------------------------------------------------
 */
void CDigest::Final(unsigned char *out)
{
	const unsigned int family = m_algo->family;
	const size_t size = m_algo->size;

	if ( family == F_CRC ) {
		u64 crc = (m_h[0] ^ Pair(m_algo->xorout)) & WidthMask(m_algo->width);
		for ( size_t i = 0 ; i < size ; ++i ) {
			// 通常は上位バイトから、CRC_LE_OUT は下位バイトから
			const size_t shift_bytes = (m_algo->flags & CRC_LE_OUT) ? i : (size - 1 - i);
			out[i] = static_cast<unsigned char>((crc >> (8 * shift_bytes)) & 0xFFu);
		}
		return;
	}

	if ( family == F_ADLER32 ) {
		const u32 v = (static_cast<u32>(m_h[1]) << 16) | static_cast<u32>(m_h[0]);
		out[0] = static_cast<unsigned char>((v >> 24) & 0xFFu);
		out[1] = static_cast<unsigned char>((v >> 16) & 0xFFu);
		out[2] = static_cast<unsigned char>((v >> 8) & 0xFFu);
		out[3] = static_cast<unsigned char>(v & 0xFFu);
		return;
	}

	if ( family == F_SHA3 ) {
		FinalSHA3(out);
		return;
	}

	// MD5 / SHA-1 / SHA-2: 0x80 に続けて0を詰め、末尾に入力のビット数を置く
	//   MD5 は64bitのリトルエンディアン、SHA-1・SHA-256 は64bit・SHA-512 は128bitのビッグエンディアン
	const size_t bs = m_algo->block;
	const size_t lf = (family == F_SHA512) ? 16 : 8;
	const u64 bits_lo = m_count << 3;
	const u64 bits_hi = m_count >> 61;

	const size_t zero_len = ((m_fill < bs - lf) ? (bs - lf) : (2 * bs - lf)) - m_fill;

	unsigned char pad[256];
	memset(pad, 0, sizeof(pad));
	pad[0] = 0x80;

	unsigned char *len_pos = pad + zero_len;
	if ( family == F_MD5 ) {
		for ( int i = 0 ; i < 8 ; ++i ) {
			len_pos[i] = static_cast<unsigned char>((bits_lo >> (8 * i)) & 0xFFu);
		}
	}
	else {
		if ( lf == 16 ) {
			for ( int i = 0 ; i < 8 ; ++i ) {
				len_pos[i] = static_cast<unsigned char>((bits_hi >> (56 - 8 * i)) & 0xFFu);
			}
		}
		for ( int j = 0 ; j < 8 ; ++j ) {
			len_pos[lf - 8 + j] = static_cast<unsigned char>((bits_lo >> (56 - 8 * j)) & 0xFFu);
		}
	}

	Update(pad, zero_len + lf);

	// 連鎖変数を、MD5 はリトルエンディアン・それ以外はビッグエンディアンで並べる
	const size_t word_bytes = (family == F_SHA512) ? 8 : 4;
	for ( size_t k = 0 ; k < size ; ++k ) {
		const u64 w = m_h[k / word_bytes];
		const size_t pos = k % word_bytes;
		const size_t shift_bytes = (family == F_MD5) ? pos : (word_bytes - 1 - pos);
		out[k] = static_cast<unsigned char>((w >> (8 * shift_bytes)) & 0xFFu);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::UpdateCRC
 * -----------------------------------------------------------------------
 */
void CDigest::UpdateCRC(const unsigned char *p, size_t len)
{
	const u64 *tab = s_crc_tables.table[s_crc_tables.slot[m_algo - s_algos]];
	u64 c = m_h[0];

	if ( m_algo->flags & CRC_REFLECT ) {
		for ( size_t i = 0 ; i < len ; ++i ) {
			c = tab[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
		}
	}
	else {
		const unsigned int width = m_algo->width;
		const u64 mask = WidthMask(width);
		const unsigned int top_shift = width - 8;
		for ( size_t j = 0 ; j < len ; ++j ) {
			c = ((c << 8) & mask) ^ tab[((c >> top_shift) ^ p[j]) & 0xFFu];
		}
	}

	m_h[0] = c;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::UpdateAdler32
 * -----------------------------------------------------------------------
 */
void CDigest::UpdateAdler32(const unsigned char *p, size_t len)
{
	const u32 MOD = 65521;
	const size_t NMAX = 5552;	// 剰余を取らずに足し続けても32bitに収まる最大のバイト数

	u32 a = static_cast<u32>(m_h[0]);
	u32 b = static_cast<u32>(m_h[1]);

	while ( len ) {
		const size_t n = (len < NMAX) ? len : NMAX;
		for ( size_t i = 0 ; i < n ; ++i ) {
			a += p[i];
			b += a;
		}
		a %= MOD;
		b %= MOD;
		p += n;
		len -= n;
	}

	m_h[0] = a;
	m_h[1] = b;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::FinalSHA3
 * -----------------------------------------------------------------------
 */
void CDigest::FinalSHA3(unsigned char *out)
{
	// パディング: 末尾に 0x06(SHA-3 のドメイン分離ビット + 先頭の 1)を置き、rate の最後のバイトに 0x80 を足す
	const size_t rate = m_algo->block;
	memset(m_buf + m_fill, 0, rate - m_fill);
	m_buf[m_fill] ^= 0x06;
	m_buf[rate - 1] ^= 0x80;
	Compress(m_buf);

	// 状態の先頭から Size() バイトをリトルエンディアンで取り出す(出力は rate より短いので1回で足りる)
	for ( size_t i = 0 ; i < m_algo->size ; ++i ) {
		out[i] = static_cast<unsigned char>((m_h[i / 8] >> (8 * (i % 8))) & 0xFFu);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::Compress
 * -----------------------------------------------------------------------
 */
void CDigest::Compress(const unsigned char *block)
{
	switch ( m_algo->family ) {
	case F_MD5:
		TransformMD5(block);
		break;
	case F_SHA1:
		TransformSHA1(block);
		break;
	case F_SHA256:
		TransformSHA256(block);
		break;
	case F_SHA512:
		TransformSHA512(block);
		break;
	default: // F_SHA3
		TransformSHA3(block);
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::TransformMD5
 * -----------------------------------------------------------------------
 */
void CDigest::TransformMD5(const unsigned char *block)
{
	// 64バイトを16個のリトルエンディアン32bit値として読む
	u32 x[16];
	for ( int j = 0 ; j < 16 ; ++j ) {
		x[j] = static_cast<u32>(block[j * 4])
			| (static_cast<u32>(block[j * 4 + 1]) << 8)
			| (static_cast<u32>(block[j * 4 + 2]) << 16)
			| (static_cast<u32>(block[j * 4 + 3]) << 24);
	}

	u32 a = static_cast<u32>(m_h[0]);
	u32 b = static_cast<u32>(m_h[1]);
	u32 c = static_cast<u32>(m_h[2]);
	u32 d = static_cast<u32>(m_h[3]);

	for ( int i = 0 ; i < 64 ; ++i ) {
		u32 f;
		int g;
		if ( i < 16 ) {
			f = (b & c) | (~b & d);
			g = i;
		}
		else if ( i < 32 ) {
			f = (d & b) | (~d & c);
			g = (5 * i + 1) & 15;
		}
		else if ( i < 48 ) {
			f = b ^ c ^ d;
			g = (3 * i + 5) & 15;
		}
		else {
			f = c ^ (b | ~d);
			g = (7 * i) & 15;
		}

		const u32 t = d;
		d = c;
		c = b;
		b = b + RotateLeft(a + f + s_md5_k[i] + x[g], s_md5_shift[i >> 4][i & 3]);
		a = t;
	}

	m_h[0] = (m_h[0] + a) & 0xFFFFFFFFu;
	m_h[1] = (m_h[1] + b) & 0xFFFFFFFFu;
	m_h[2] = (m_h[2] + c) & 0xFFFFFFFFu;
	m_h[3] = (m_h[3] + d) & 0xFFFFFFFFu;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::TransformSHA1
 * -----------------------------------------------------------------------
 */
void CDigest::TransformSHA1(const unsigned char *block)
{
	// 64バイトを16個のビッグエンディアン32bit値として読み、80個に拡張する
	u32 w[80];
	for ( int j = 0 ; j < 16 ; ++j ) {
		w[j] = (static_cast<u32>(block[j * 4]) << 24)
			| (static_cast<u32>(block[j * 4 + 1]) << 16)
			| (static_cast<u32>(block[j * 4 + 2]) << 8)
			| static_cast<u32>(block[j * 4 + 3]);
	}
	for ( int m = 16 ; m < 80 ; ++m ) {
		w[m] = RotateLeft(w[m - 3] ^ w[m - 8] ^ w[m - 14] ^ w[m - 16], 1);
	}

	u32 a = static_cast<u32>(m_h[0]);
	u32 b = static_cast<u32>(m_h[1]);
	u32 c = static_cast<u32>(m_h[2]);
	u32 d = static_cast<u32>(m_h[3]);
	u32 e = static_cast<u32>(m_h[4]);

	for ( int i = 0 ; i < 80 ; ++i ) {
		u32 f, k;
		if ( i < 20 ) {
			f = (b & c) | (~b & d);
			k = 0x5A827999u;
		}
		else if ( i < 40 ) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1u;
		}
		else if ( i < 60 ) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDCu;
		}
		else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6u;
		}

		const u32 t = RotateLeft(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = RotateLeft(b, 30);
		b = a;
		a = t;
	}

	m_h[0] = (m_h[0] + a) & 0xFFFFFFFFu;
	m_h[1] = (m_h[1] + b) & 0xFFFFFFFFu;
	m_h[2] = (m_h[2] + c) & 0xFFFFFFFFu;
	m_h[3] = (m_h[3] + d) & 0xFFFFFFFFu;
	m_h[4] = (m_h[4] + e) & 0xFFFFFFFFu;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::TransformSHA256
 * -----------------------------------------------------------------------
 */
void CDigest::TransformSHA256(const unsigned char *block)
{
	u32 w[64];
	for ( int j = 0 ; j < 16 ; ++j ) {
		w[j] = (static_cast<u32>(block[j * 4]) << 24)
			| (static_cast<u32>(block[j * 4 + 1]) << 16)
			| (static_cast<u32>(block[j * 4 + 2]) << 8)
			| static_cast<u32>(block[j * 4 + 3]);
	}
	for ( int m = 16 ; m < 64 ; ++m ) {
		const u32 s0 = RotateRight(w[m - 15], 7) ^ RotateRight(w[m - 15], 18) ^ (w[m - 15] >> 3);
		const u32 s1 = RotateRight(w[m - 2], 17) ^ RotateRight(w[m - 2], 19) ^ (w[m - 2] >> 10);
		w[m] = w[m - 16] + s0 + w[m - 7] + s1;
	}

	u32 v[8];
	for ( int n = 0 ; n < 8 ; ++n ) {
		v[n] = static_cast<u32>(m_h[n]);
	}

	for ( int i = 0 ; i < 64 ; ++i ) {
		const u32 s1 = RotateRight(v[4], 6) ^ RotateRight(v[4], 11) ^ RotateRight(v[4], 25);
		const u32 ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
		const u32 t1 = v[7] + s1 + ch + s_sha256_k[i] + w[i];
		const u32 s0 = RotateRight(v[0], 2) ^ RotateRight(v[0], 13) ^ RotateRight(v[0], 22);
		const u32 maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
		const u32 t2 = s0 + maj;

		v[7] = v[6];
		v[6] = v[5];
		v[5] = v[4];
		v[4] = v[3] + t1;
		v[3] = v[2];
		v[2] = v[1];
		v[1] = v[0];
		v[0] = t1 + t2;
	}

	for ( int q = 0 ; q < 8 ; ++q ) {
		m_h[q] = (m_h[q] + v[q]) & 0xFFFFFFFFu;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::TransformSHA512
 * -----------------------------------------------------------------------
 */
void CDigest::TransformSHA512(const unsigned char *block)
{
	u64 w[80];
	for ( int j = 0 ; j < 16 ; ++j ) {
		u64 x = 0;
		for ( int b = 0 ; b < 8 ; ++b ) {
			x = (x << 8) | static_cast<u64>(block[j * 8 + b]);
		}
		w[j] = x;
	}
	for ( int m = 16 ; m < 80 ; ++m ) {
		const u64 s0 = RotateRight64(w[m - 15], 1) ^ RotateRight64(w[m - 15], 8) ^ (w[m - 15] >> 7);
		const u64 s1 = RotateRight64(w[m - 2], 19) ^ RotateRight64(w[m - 2], 61) ^ (w[m - 2] >> 6);
		w[m] = w[m - 16] + s0 + w[m - 7] + s1;
	}

	u64 v[8];
	for ( int n = 0 ; n < 8 ; ++n ) {
		v[n] = m_h[n];
	}

	for ( int i = 0 ; i < 80 ; ++i ) {
		const u64 s1 = RotateRight64(v[4], 14) ^ RotateRight64(v[4], 18) ^ RotateRight64(v[4], 41);
		const u64 ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
		const u64 t1 = v[7] + s1 + ch + Pair(s_sha512_k[i]) + w[i];
		const u64 s0 = RotateRight64(v[0], 28) ^ RotateRight64(v[0], 34) ^ RotateRight64(v[0], 39);
		const u64 maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
		const u64 t2 = s0 + maj;

		v[7] = v[6];
		v[6] = v[5];
		v[5] = v[4];
		v[4] = v[3] + t1;
		v[3] = v[2];
		v[2] = v[1];
		v[1] = v[0];
		v[0] = t1 + t2;
	}

	for ( int q = 0 ; q < 8 ; ++q ) {
		m_h[q] += v[q];
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDigest::TransformSHA3
 *  機能概要：  rate バイトを状態に XOR して Keccak-f[1600] を回す
 * -----------------------------------------------------------------------
 */
void CDigest::TransformSHA3(const unsigned char *block)
{
	const size_t lanes = m_algo->block / 8;
	for ( size_t j = 0 ; j < lanes ; ++j ) {
		u64 x = 0;
		for ( int b = 7 ; b >= 0 ; --b ) {
			x = (x << 8) | static_cast<u64>(block[j * 8 + b]);
		}
		m_h[j] ^= x;
	}

	u64 *a = m_h;
	for ( int round = 0 ; round < 24 ; ++round ) {
		// θ
		u64 c[5];
		for ( int x = 0 ; x < 5 ; ++x ) {
			c[x] = a[x] ^ a[x + 5] ^ a[x + 10] ^ a[x + 15] ^ a[x + 20];
		}
		for ( int x2 = 0 ; x2 < 5 ; ++x2 ) {
			const u64 d = c[(x2 + 4) % 5] ^ RotateLeft64(c[(x2 + 1) % 5], 1);
			for ( int y = 0 ; y < 25 ; y += 5 ) {
				a[x2 + y] ^= d;
			}
		}

		// ρ と π: レーン (x, y) を回転して (y, 2x + 3y) に移す
		u64 b[25];
		for ( int x3 = 0 ; x3 < 5 ; ++x3 ) {
			for ( int y3 = 0 ; y3 < 5 ; ++y3 ) {
				b[y3 + 5 * ((2 * x3 + 3 * y3) % 5)] = RotateLeft64(a[x3 + 5 * y3], s_keccak_rho[x3 + 5 * y3]);
			}
		}

		// χ
		for ( int y4 = 0 ; y4 < 25 ; y4 += 5 ) {
			for ( int x4 = 0 ; x4 < 5 ; ++x4 ) {
				a[x4 + y4] = b[x4 + y4] ^ (~b[(x4 + 1) % 5 + y4] & b[(x4 + 2) % 5 + y4]);
			}
		}

		// ι
		a[0] ^= Pair(s_keccak_rc[round]);
	}
}
