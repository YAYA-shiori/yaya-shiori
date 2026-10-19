//
// AYA version 5
//
// ダイジェスト(MD5 / SHA-1 / SHA-2 / SHA-3 / Adler-32 / 各種 CRC)の計算
//   STRDIGEST / FDIGEST から使う。仕様書(RFC 1321 / FIPS 180-4 / FIPS 202 / RFC 1950 / CRC カタログ)をもとにした自前実装
//

#ifndef	DIGEST_H
#define	DIGEST_H

#include <stddef.h>

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int64	digest_u64;		// VC6 には stdint.h が無い
#else
#include <stdint.h>
typedef uint64_t			digest_u64;
#endif
typedef unsigned int		digest_u32;		// 32bit 符号なし整数(VC6 / gcc / clang とも int は32bit)

class CDigest
{
public:
	// アルゴリズムの定義(実体は digest.cpp)
	struct Algo;

	// Final で書き込むバイト列の最大長(SHA-512 / SHA3-512 の 64)
	enum { MAX_SIZE = 64 };

private:
	const Algo		*m_algo;
	digest_u64		m_h[25];		// 連鎖変数 / CRC・Adler-32 の途中値 / SHA-3 の状態
	unsigned char	m_buf[144];		// 端数のバイト列(ブロック型のみ。SHA3-224 の rate が最大の144)
	size_t			m_fill;			// m_buf に溜まっているバイト数
	digest_u64		m_count;		// 入力の総バイト数(ブロック型のみ)

public:
	// 名前からアルゴリズムを探す。見つからなければ NULL
	//   英数字以外と大文字小文字は無視する("SHA-256" "sha_256" "sha256" は同じ)
	static const Algo	*Find(const wchar_t *name);

	// 定義済みのアルゴリズムの一覧(テストや文書化のため)
	static size_t		AlgoCount(void);
	static const Algo	*AlgoAt(size_t index);
	// 空白区切りの名前の並び。先頭が正式名
	static const char	*AlgoNames(const Algo *algo);

	explicit CDigest(const Algo *algo);

	// ダイジェスト値のバイト数
	size_t	Size(void) const;

	// データを追加する。何度に分けて呼んでもよい
	void	Update(const void *data, size_t len);

	// 計算を終えて Size() バイトを out に書く。呼んだ後は Update / Final を呼ばないこと
	void	Final(unsigned char *out);

private:
	void	Reset(void);
	void	Compress(const unsigned char *block);
	void	UpdateCRC(const unsigned char *p, size_t len);
	void	UpdateAdler32(const unsigned char *p, size_t len);
	void	FinalSHA3(unsigned char *out);
	void	TransformMD5(const unsigned char *block);
	void	TransformSHA1(const unsigned char *block);
	void	TransformSHA256(const unsigned char *block);
	void	TransformSHA512(const unsigned char *block);
	void	TransformSHA3(const unsigned char *block);
};

#endif
