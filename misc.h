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

char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr);
char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr);
char	SplitOnly(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, yaya::char_t *sepstr);
char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr);
char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr);
int		SplitToMultiString(const yaya::string_t &str, std::vector<yaya::string_t> *array, const yaya::string_t &delimiter);

void	CutSpace(yaya::string_t &str);
void	CutStartSpace(yaya::string_t &str);
void	CutEndSpace(yaya::string_t &str);

void	CutDoubleQuote(yaya::string_t &str);
void	CutSingleQuote(yaya::string_t &str);
void	UnescapeSpecialString(yaya::string_t &str);
void	AddDoubleQuote(yaya::string_t &str);
void	CutCrLf(yaya::string_t &str);

yaya::string_t	GetDateString(void);

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

bool	IsUnicodeAware(void);

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
#define	CCALLLIMIT_LOOP_MAX			10000	//ループ処理限界デフォルト
class	CCallLimit
{
protected:
	int	depth;
	int	maxdepth;
	int maxloop;
	std::vector<yaya::string_t> stack;

public:
	CCallLimit(void) { depth = 0; maxdepth = CCALLLIMIT_CALLDEPTH_MAX; maxloop = CCALLLIMIT_LOOP_MAX; }

	void	SetMaxDepth(int value) { maxdepth = value; }
	int 	GetMaxDepth(void) { return maxdepth; }

	void	SetMaxLoop(int value) { maxloop = value; }
	int		GetMaxLoop(void) { return maxloop; }

	void	InitCall(void) { depth = 0; }

	char	AddCall(const yaya::string_t &str) {
		depth++;
		stack.emplace_back(str);
		if (maxdepth && depth > maxdepth)
			return 0;
		else
			return 1;
	}

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
		int aret=0;
		std::swap(aret, maxdepth);
		return aret;
	}
	void reset_lock(int lock) {
		std::swap(lock, maxdepth);
	}
};

//----

#endif
