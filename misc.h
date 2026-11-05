// 
// AYA version 5
//
// 雑用関数
// written by umeici. 2004
// 

#ifndef	MISCH
#define	MISCH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "globaldef.h"

yaya::string_t::size_type Find_IgnoreDQ(const yaya::string_t &str, const yaya::char_t *findstr);
yaya::string_t::size_type Find_IgnoreDQ(const yaya::string_t &str, const yaya::string_t &findstr);

yaya::string_t::size_type find_last_str(const yaya::string_t &str, const yaya::char_t *findstr);
yaya::string_t::size_type find_last_str(const yaya::string_t &str, const yaya::string_t &findstr);

void	SplitPathParts(const yaya::string_t &path, yaya::string_t &drive, yaya::string_t &dir, yaya::string_t &fname, yaya::string_t &ext);

char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr);
char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr);
char	SplitOnly(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, yaya::char_t *sepstr);
char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr);
char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr);
size_t	SplitToMultiString(const yaya::string_t &str, std::vector<yaya::string_t> *array, const yaya::string_t &delimiter);

void	CutSpace(yaya::string_t &str);
void	CutStartSpace(yaya::string_t &str);
void	CutEndSpace(yaya::string_t &str);

void	CutDoubleQuote(yaya::string_t &str);
void	CutSingleQuote(yaya::string_t &str);
void	EscapingInsideDoubleDoubleQuote(yaya::string_t &str);
void	EscapingInsideDoubleSingleQuote(yaya::string_t &str);
void	UnescapeSpecialString(yaya::string_t &str);
void	AddDoubleQuote(yaya::string_t &str);
void	CutCrLf(yaya::string_t &str);

yaya::string_t	GetDateString(void);

// タイムゾーンの指定。ローカルタイム（OSの設定）か、UTCからの固定オフセット
struct CTimeZone
{
	bool is_local;
	int offset_sec; // is_local が false のときの UTC からのオフセット（秒、東が正）

	CTimeZone() : is_local(true), offset_sec(0) { }

	static CTimeZone Fixed(int sec)
	{
		CTimeZone tz;
		tz.is_local = false;
		tz.offset_sec = sec;
		return tz;
	}
};

yaya::time_t GetEpochTime();
#if defined(WIN32) || defined(_WIN32_WCE)
// FILETIME（UTC）をEPOCH秒にする。タイムゾーンは関係しない
yaya::time_t FileTimeToEpochTime(const FILETIME &ft);
#endif
// EPOCH秒を tz の年月日時分秒にする（tm_yday は0始まり）。範囲外なら false
bool EpochTimeToTM(yaya::time_t tv, const CTimeZone &tz, struct tm &out);
// tz の年月日時分秒をEPOCH秒にする。月や日などは範囲外でもよい（桁上げ・桁下げする）。範囲外なら false
bool TMToEpochTime(const struct tm &in, const CTimeZone &tz, yaya::time_t &out);

// ローカルタイムゾーンの、utc の時点でのUTCオフセット（秒、東が正）・夏時間か・名前
// 名前は Windows ではOSの表示名、POSIX では略称（"JST"）。範囲外なら false
bool GetLocalTimeZoneInfo(yaya::time_t utc, int &offset, int &isdst, yaya::string_t &name);
// ローカルタイムゾーンの IANA の名前（"Asia/Tokyo"）。分からなければ空文字列
// POSIX は TZ か /etc/localtime、Windows は ICU (icu.dll) で Windows の名前から変換する
yaya::string_t GetLocalTimeZoneId();

extern const yaya::string_t::size_type IsInDQ_notindq;
extern const yaya::string_t::size_type IsInDQ_runaway;
extern const yaya::string_t::size_type IsInDQ_npos;
yaya::string_t::size_type IsInDQ(const yaya::string_t &str, yaya::string_t::size_type startpoint, yaya::string_t::size_type checkpoint);

char	IsDoubleButNotIntString(const yaya::string_t &str);
char	IsIntString(const yaya::string_t &str);
char	IsIntBinString(const yaya::string_t &str, char header);
char	IsIntHexString(const yaya::string_t &str, char header);

char	IsLegalFunctionName(const yaya::string_t &str);
char	IsLegalVariableName(const yaya::string_t &str);
char	IsLegalStrLiteral(const yaya::string_t &str);
char	IsLegalPlainStrLiteral(const yaya::string_t &str);

void	EscapeString(yaya::string_t &wstr);
void	UnescapeString(yaya::string_t &wstr);

void	EncodeBase64(yaya::string_t &out,const char *in,size_t in_len);
void	DecodeBase64(std::string &out,const yaya::char_t *in,size_t in_len);
void	EncodeURL(yaya::string_t &out,const char *in,size_t in_len,bool isPlusPercent);
void	DecodeURL(std::string &out,const yaya::char_t *in,size_t in_len,bool isPlusPercent);

inline bool IsSpace(const yaya::char_t &c) {
#if !defined(POSIX) && !defined(__MINGW32__)
	return c == L' ' || c == L'\t' || c == L'　';
#else
	return c == L' ' || c == L'\t' || c == L'\u3000';
#endif
}

//----

// 関数呼び出しの限界を検査するためのクラス

#define	CCALLLIMIT_CALLDEPTH_MAX	32		//呼び出し限界デフォルト
#define	CCALLLIMIT_LOOP_MAX			1000000	//ループ処理限界デフォルト
#define	CCALLLIMIT_STACK_MARGIN		(128 * 1024)	//残りスタックがこれを下回ったら呼び出しを止める（上限）

// 現在のスレッドのスタックの下端（低いほうのアドレス）を返します。取得できないときは0
size_t	GetCurrentThreadStackLow(void);

class	CCallLimit
{
protected:
	size_t	depth;
	size_t	maxdepth;
	size_t maxloop;
	std::vector<yaya::string_t> stack;

	// ユーザー関数の再帰でC++のスタックを使い切らないための検査
	// fncdepthを0（無制限）や大きな値にしても、スタックが尽きる前に呼び出しを止める
	size_t	stack_low;			// スタックの下端。0なら不明（検査しない）
	size_t	stack_margin;		// 残りがこれを下回ったら止める
	bool	stack_failed;		// 直前のAddCallが失敗した原因がスタック不足か
	bool	in_stack_handler;	// スタック不足で呼んだshiori.OnCallLimitの実行中か

public:
	CCallLimit(void) {
		depth = 0; maxdepth = CCALLLIMIT_CALLDEPTH_MAX; maxloop = CCALLLIMIT_LOOP_MAX;
		stack_low = 0; stack_margin = 0; stack_failed = false; in_stack_handler = false;
	}

	void	SetMaxDepth(size_t value) { maxdepth = value; }
	size_t	GetMaxDepth(void) { return maxdepth; }

	void	SetMaxLoop(size_t value) { maxloop = value; }
	size_t	GetMaxLoop(void) { return maxloop; }

	void	InitCall(void) {
		depth = 0; stack.clear();
		stack_failed = false; in_stack_handler = false;

		// リクエストの入口で、このスレッドのスタックの範囲を取り直す（呼び出し元のスレッドが変わりうるため）
		char	probe;
		const size_t	sp = reinterpret_cast<size_t>(&probe);
		stack_low = GetCurrentThreadStackLow();
		if (stack_low && sp > stack_low) {
			// 小さなスタックでは、いきなり止まらないよう残りの1/4までにする
			const size_t	remain = sp - stack_low;
			stack_margin = remain / 4 < CCALLLIMIT_STACK_MARGIN ? remain / 4 : CCALLLIMIT_STACK_MARGIN;
		}
		else {
			stack_low = 0;
			stack_margin = 0;
		}
	}

	char	AddCall(const yaya::string_t &str) {
		depth++;
		stack.emplace_back(str);
		stack_failed = false;
		if (maxdepth && depth > maxdepth)
			return 0;
		if (stack_low) {
			// shiori.OnCallLimitの実行中は、それ自身が動けるよう余裕を半分まで使わせる
			char	probe;
			const size_t	sp = reinterpret_cast<size_t>(&probe);
			const size_t	margin = in_stack_handler ? stack_margin / 2 : stack_margin;
			// InitCallを通らず別のスレッドから呼ばれた場合に備え、sp が記録した範囲の中にあるときだけ見る
			// （記録より下にある別のスタックを、残りが少ないと誤って判定しない）
			if (stack_low <= sp && sp < stack_low + margin) {
				stack_failed = true;
				return 0;
			}
		}
		return 1;
	}

	// 直前のAddCallが、スタック不足で失敗したか
	bool	StackFailed(void) const { return stack_failed; }
	// スタック不足で呼んだshiori.OnCallLimitの実行中か（その中でさらに足りなくなったらハンドラを呼ばない）
	bool	InStackHandler(void) const { return in_stack_handler; }
	void	SetInStackHandler(bool value) { in_stack_handler = value; }

	void	DeleteCall(void) {
		if ( depth ) {
			depth--;
			stack.erase(stack.end()-1);
		}
	}

	std::vector<yaya::string_t> &StackCall(void) {
		return stack;
	}

	int temp_unlock() {
		size_t aret=0;
		using std::swap;
		swap(aret, maxdepth);
		return aret;
	}
	void reset_lock(size_t lock) {
		using std::swap;
		swap(lock, maxdepth);
	}
};

//----

#endif
