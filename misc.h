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

aya::string_t::size_type Find_IgnoreDQ(const aya::string_t &str, const aya::char_t *findstr);
aya::string_t::size_type Find_IgnoreDQ(const aya::string_t &str, const aya::string_t &findstr);

aya::string_t::size_type find_last_str(const aya::string_t &str, const aya::char_t *findstr);
aya::string_t::size_type find_last_str(const aya::string_t &str, const aya::string_t &findstr);

char	Split(const aya::string_t &str, aya::string_t &dstr0, aya::string_t &dstr1, const aya::char_t *sepstr);
char	Split(const aya::string_t &str, aya::string_t &dstr0, aya::string_t &dstr1, const aya::string_t &sepstr);
char	SplitOnly(const aya::string_t &str, aya::string_t &dstr0, aya::string_t &dstr1, aya::char_t *sepstr);
char	Split_IgnoreDQ(const aya::string_t &str, aya::string_t &dstr0, aya::string_t &dstr1, const aya::char_t *sepstr);
char	Split_IgnoreDQ(const aya::string_t &str, aya::string_t &dstr0, aya::string_t &dstr1, const aya::string_t &sepstr);
int		SplitToMultiString(const aya::string_t &str, std::vector<aya::string_t> *array, const aya::string_t &delimiter);

void	CutSpace(aya::string_t &str);
void	CutStartSpace(aya::string_t &str);
void	CutEndSpace(aya::string_t &str);

void	CutDoubleQuote(aya::string_t &str);
void	CutSingleQuote(aya::string_t &str);
void	UnescapeSpecialString(aya::string_t &str);
void	AddDoubleQuote(aya::string_t &str);
void	CutCrLf(aya::string_t &str);

aya::string_t	GetDateString(void);

extern const aya::string_t::size_type IsInDQ_notindq;
extern const aya::string_t::size_type IsInDQ_runaway;
extern const aya::string_t::size_type IsInDQ_npos;
aya::string_t::size_type IsInDQ(const aya::string_t &str, aya::string_t::size_type startpoint, aya::string_t::size_type checkpoint);

char	IsDoubleButNotIntString(const aya::string_t &str);
char	IsIntString(const aya::string_t &str);
char	IsIntBinString(const aya::string_t &str, char header);
char	IsIntHexString(const aya::string_t &str, char header);

char	IsLegalFunctionName(const aya::string_t &str);
char	IsLegalVariableName(const aya::string_t &str);
char	IsLegalStrLiteral(const aya::string_t &str);
char	IsLegalPlainStrLiteral(const aya::string_t &str);

char	IsNt(void);

void	EscapeString(aya::string_t &wstr);
void	UnescapeString(aya::string_t &wstr);

void	EncodeBase64(aya::string_t &out,const char *in,size_t in_len);
void	DecodeBase64(std::string &out,const aya::char_t *in,size_t in_len);
void	EncodeURL(aya::string_t &out,const char *in,size_t in_len,bool isPlusPercent);
void	DecodeURL(std::string &out,const aya::char_t *in,size_t in_len,bool isPlusPercent);

inline bool IsSpace(const aya::char_t &c) {
#if !defined(POSIX) && !defined(__MINGW32__)
	return c == L' ' || c == L'\t' || c == L'　';
#else
	return c == L' ' || c == L'\t' || c == L'\u3000';
#endif
}

//----

// 関数呼び出しの深さを検査するためのクラス

class	CCallDepth
{
protected:
	size_t	depth;
	std::vector<aya::string_t> stack;

public:
	CCallDepth(void) { depth = 0; }

	void	Init(void) { depth = 0; }

	void	Add(const aya::string_t &str) {
		depth++;
		stack.push_back(str);
	}

	void	Del(void) {
		if ( depth ) {
			depth--;
			stack.erase(stack.end()-1);
		}
	}

	std::vector<aya::string_t> &Stack(void) {
		return stack;
	}
};

//----

#endif
