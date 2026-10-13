// deelx.h
//
// DEELX Regular Expression Engine (v1.3)
//
// Copyright 2006 ~ 2013 (c) RegExLab.com
// All Rights Reserved.
//
// http://www.regexlab.com/deelx/
//
// Author: Shi Shouwei (sswater shi)
// sswater@gmail.com
//
// $Revision$
//
// Character types
//   CRegexpW (unsigned short) works on UTF-16 text. CRegexpA works on single
//   bytes and is NOT aware of multibyte encodings such as Shift_JIS, EUC-JP,
//   GBK or UTF-8:
//     * a trail byte may match an ASCII char: with Shift_JIS, "\\" matches
//       the second byte of 0x95 0x5C, and "[a-z]" may match inside a kanji
//     * '.', [...], \W, quantifiers etc. apply to single bytes, so "^.$" does
//       not match a one-char multibyte string
//     * UNICODE_MODE has no effect
//   Convert such text to UTF-16 and use CRegexpW instead.
//

#ifndef __DEELX_REGEXP__H__
#define __DEELX_REGEXP__H__

#include <memory.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <new>

// Largest explicit group number accepted in (?<n>...), (?<a-n>...), \k<n>
// and (?(n)...). Larger numbers are taken as names. Every match allocates
// per-group arrays up to the max group number, so keep this moderate.
#ifndef DEELX_MAX_GROUP_NUMBER
	#define DEELX_MAX_GROUP_NUMBER 65535
#endif

extern "C" {
	typedef int (*POSIX_FUNC)(int);
	int deelx_isblank(int c);
	int deelx_isascii(int c);
}

// Allocation failure throws std::bad_alloc. realloc() keeps the old block on
// failure, so callers must not update their pointer or capacity before this
// returns. VC6's operator new returns 0 instead of throwing, so results of
// new are passed through deelx_check_new().
inline void * deelx_realloc(void * p, size_t size)
{
	void * q = realloc(p, size);
	if( q == 0 && size != 0 ) throw std::bad_alloc();
	return q;
}

template <class T> inline T * deelx_check_new(T * p)
{
	if( p == 0 ) throw std::bad_alloc();
	return p;
}

// ctype functions accept only EOF or values representable as unsigned char;
// anything else (negative signed char, wide chars >= 256) is undefined behavior.
template <class CHART> inline int deelx_lt256(CHART c)
{
	return c >= 0 && c < 256;
}

template <class CHART> inline CHART deelx_toupper(CHART c)
{
	return deelx_lt256(c) ? (CHART)toupper((int)c) : c;
}

template <class CHART> inline int deelx_isspace(CHART c)
{
	return deelx_lt256(c) && isspace((int)c);
}

// Code point of a code unit: a char is a byte 0..255, never negative.
template <class CHART> inline unsigned int deelx_cp(CHART c)
{
	return sizeof(CHART) == 1 ? (unsigned int)(unsigned char)c : (unsigned int)c;
}

inline int deelx_is_high_surrogate(unsigned int c)
{
	return c >= 0xD800 && c <= 0xDBFF;
}

inline int deelx_is_low_surrogate(unsigned int c)
{
	return c >= 0xDC00 && c <= 0xDFFF;
}

// The char at s[pos] as a code point; returns its length in code units. With
// bpair, a UTF-16 surrogate pair is one char. pos must be inside the string.
template <class CHART> inline int deelx_char_at(const CHART * s, int pos, int length, int bpair, unsigned int & cp)
{
	cp = deelx_cp(s[pos]);

	if( bpair && sizeof(CHART) == 2 && deelx_is_high_surrogate(cp) && pos + 1 < length && deelx_is_low_surrogate(deelx_cp(s[pos + 1])) )
	{
		cp = 0x10000 + ((cp - 0xD800) << 10) + (deelx_cp(s[pos + 1]) - 0xDC00);
		return 2;
	}

	return 1;
}

// The same for the char that ends before s[pos]; pos must be > 0.
template <class CHART> inline int deelx_char_before(const CHART * s, int pos, int bpair, unsigned int & cp)
{
	cp = deelx_cp(s[pos - 1]);

	if( bpair && sizeof(CHART) == 2 && deelx_is_low_surrogate(cp) && pos >= 2 && deelx_is_high_surrogate(deelx_cp(s[pos - 2])) )
	{
		cp = 0x10000 + ((deelx_cp(s[pos - 2]) - 0xD800) << 10) + (cp - 0xDC00);
		return 2;
	}

	return 1;
}

// Unicode general categories, also the bit numbers in category masks
enum DEELX_UNICODE_CATEGORY
{
	DEELX_UC_Lu, DEELX_UC_Ll, DEELX_UC_Lt, DEELX_UC_Lm, DEELX_UC_Lo,
	DEELX_UC_Mn, DEELX_UC_Mc, DEELX_UC_Me,
	DEELX_UC_Nd, DEELX_UC_Nl, DEELX_UC_No,
	DEELX_UC_Pc, DEELX_UC_Pd, DEELX_UC_Ps, DEELX_UC_Pe, DEELX_UC_Pi, DEELX_UC_Pf, DEELX_UC_Po,
	DEELX_UC_Sm, DEELX_UC_Sc, DEELX_UC_Sk, DEELX_UC_So,
	DEELX_UC_Zs, DEELX_UC_Zl, DEELX_UC_Zp,
	DEELX_UC_Cc, DEELX_UC_Cf, DEELX_UC_Cs, DEELX_UC_Co, DEELX_UC_Cn,
	DEELX_UC_COUNT
};

// Unicode \w as in UTS #18: letters, marks, decimal digits, letter numbers
// (approximating Alphabetic), connector punctuation, and Join_Control
const unsigned int DEELX_UC_WORD_MASK = 0xFF | (1u << DEELX_UC_Nd) | (1u << DEELX_UC_Nl) | (1u << DEELX_UC_Pc);

// Defined with their tables at the end of this file
inline int          deelx_unicode_category(unsigned int cp);
inline unsigned int deelx_unicode_fold    (unsigned int cp);
inline const int *  deelx_unicode_fold_ranges(int & count);

inline int deelx_unicode_is_space(unsigned int cp) // White_Space property
{
	return (cp >= 0x09 && cp <= 0x0D) || cp == 0x20 || cp == 0x85 || cp == 0xA0 || cp == 0x1680 ||
		(cp >= 0x2000 && cp <= 0x200A) || cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000;
}

inline int deelx_unicode_is_word(unsigned int cp)
{
	if(cp < 0x80)
		return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9') || cp == '_';

	// ZWNJ and ZWJ (Join_Control) are word chars too, as in Perl
	return ((DEELX_UC_WORD_MASK >> deelx_unicode_category(cp)) & 1) || cp == 0x200C || cp == 0x200D;
}

// Compare n code units ignoring case by Unicode simple case folding.
// A surrogate pair is compared unit by unit, so it is not folded.
template <class CHART> inline int deelx_equal_fold(const CHART * a, const CHART * b, int n)
{
	for(int i=0; i<n; i++)
	{
		if( a[i] != b[i] && deelx_unicode_fold(deelx_cp(a[i])) != deelx_unicode_fold(deelx_cp(b[i])) )
			return 0;
	}

	return 1;
}

//
// Data Reference
//
template <class ELT> class CBufferRefT
{
public:
	CBufferRefT(const ELT * pcsz, int length);
	CBufferRefT(const ELT * pcsz);

public:
	int nCompare      (const ELT * pcsz) const;
	int nCompareNoCase(const ELT * pcsz) const;
	int  Compare      (const ELT * pcsz) const;
	int  CompareNoCase(const ELT * pcsz) const;
	int  Compare      (const CBufferRefT <ELT> &) const;
	int  CompareNoCase(const CBufferRefT <ELT> &) const;

	ELT At          (int nIndex, ELT def = 0) const;
	ELT operator [] (int nIndex) const;

	const ELT * GetBuffer() const;
	int GetSize() const;

public:
	virtual ~CBufferRefT();

// Content
protected:
	ELT * m_pBuffer;
	int         m_nSize;
};

//
// Implemenation
//
template <class ELT> CBufferRefT <ELT> :: CBufferRefT(const ELT * pcsz, int length)
{
	m_pBuffer  = (ELT *)pcsz;
	m_nSize = length;
}

template <class ELT> CBufferRefT <ELT> :: CBufferRefT(const ELT * pcsz)
{
	m_pBuffer  = (ELT *)pcsz;
	m_nSize = 0;

	if(pcsz != 0) while(m_pBuffer[m_nSize] != 0) m_nSize ++;
}

template <class ELT> int CBufferRefT <ELT> :: nCompare(const ELT * pcsz) const
{
	for(int i=0; i<m_nSize; i++)
	{
		if(m_pBuffer[i] != pcsz[i])
			return m_pBuffer[i] - pcsz[i];
	}

	return 0;
}

template <class ELT> int CBufferRefT <ELT> :: nCompareNoCase(const ELT * pcsz) const
{
	for(int i=0; i<m_nSize; i++)
	{
		if(m_pBuffer[i] != pcsz[i])
		{
			if(deelx_toupper(m_pBuffer[i]) != deelx_toupper(pcsz[i]))
				return m_pBuffer[i] - pcsz[i];
		}
	}

	return 0;
}

template <class ELT> inline int CBufferRefT <ELT> :: Compare(const ELT * pcsz) const
{
	return nCompare(pcsz) ? 1 : (int)pcsz[m_nSize];
}

template <class ELT> inline int CBufferRefT <ELT> :: CompareNoCase(const ELT * pcsz) const
{
	return nCompareNoCase(pcsz) ? 1 : (int)pcsz[m_nSize];
}

template <class ELT> inline int CBufferRefT <ELT> :: Compare(const CBufferRefT <ELT> & cref) const
{
	return m_nSize == cref.m_nSize ? nCompare(cref.GetBuffer()) : 1;
}

template <class ELT> inline int CBufferRefT <ELT> :: CompareNoCase(const CBufferRefT <ELT> & cref) const
{
	return m_nSize == cref.m_nSize ? nCompareNoCase(cref.GetBuffer()) : 1;
}

template <class ELT> inline ELT CBufferRefT <ELT> :: At(int nIndex, ELT def) const
{
	return (nIndex < 0 || nIndex >= m_nSize) ? def : m_pBuffer[nIndex];
}

template <class ELT> inline ELT CBufferRefT <ELT> :: operator [] (int nIndex) const
{
	return (nIndex < 0 || nIndex >= m_nSize) ? 0 : m_pBuffer[nIndex];
}

template <class ELT> const ELT * CBufferRefT <ELT> :: GetBuffer() const
{
	static const ELT _def[] = {0}; return m_pBuffer ? m_pBuffer : _def;
}

template <class ELT> inline int CBufferRefT <ELT> :: GetSize() const
{
	return m_nSize;
}

template <class ELT> CBufferRefT <ELT> :: ~CBufferRefT()
{
}

//
// Data Buffer
//
template <class ELT> class CBufferT : public CBufferRefT <ELT>
{
public:
	CBufferT(const ELT * pcsz, int length);
	CBufferT(const ELT * pcsz);
	CBufferT();
	CBufferT(const CBufferT <ELT> & from);
	CBufferT <ELT> & operator = (const CBufferT <ELT> & from);

public:
	ELT & operator [] (int nIndex);
	const ELT & operator [] (int nIndex) const;
	void  Append(const ELT * pcsz, int length, int eol = 0);
	void  Append(ELT el, int eol = 0);

public:
	void  Push(ELT   el);
	void  Push(const CBufferRefT<ELT> & buf);
	int   Pop (ELT & el);
	int   Pop (CBufferT<ELT> & buf);
	int   Peek(ELT & el) const;

public:
	const ELT * GetBuffer() const;
	ELT * GetBuffer();
	ELT * Detach();
	void  Release();
	void  Prepare(int index, int fill = 0);
	void  Restore(int size);

	ELT * PrepareInsert(int nPos, int nSize)
	{
		int nOldSize = CBufferRefT<ELT>::m_nSize;
		Restore((nPos > nOldSize ? nPos : nOldSize) + nSize);

		if( nPos < nOldSize )
		{
			ELT * from = CBufferRefT<ELT>::m_pBuffer + nPos, * to = CBufferRefT<ELT>::m_pBuffer + nPos + nSize;
			memmove(to, from, sizeof(ELT) * (nOldSize - nPos));
		}

		return CBufferRefT<ELT>::m_pBuffer + nPos;
	}

	void Insert(int nIndex, const ELT & rT)
	{
		Insert(nIndex, &rT, 1);
	}

	void Insert(int nIndex, const ELT * pT, int nSize)
	{
		memcpy(PrepareInsert(nIndex, nSize), pT, sizeof(ELT) * nSize);
	}

	void Remove(int nIndex)
	{
		Remove(nIndex, 1);
	}

	void Remove(int nIndex, int nSize)
	{
		if( nIndex < CBufferRefT <ELT> :: m_nSize )
		{
			if( nIndex + nSize >= CBufferRefT <ELT> :: m_nSize )
			{
				Restore(nIndex);
			}
			else
			{
				memmove(CBufferRefT <ELT> :: m_pBuffer + nIndex, CBufferRefT <ELT> :: m_pBuffer + nIndex + nSize, sizeof(ELT) * (CBufferRefT <ELT> :: m_nSize - nIndex - nSize));
				Restore(CBufferRefT <ELT> :: m_nSize - nSize);
			}
		}
	}

	void SetMaxLength(int nSize)
	{
		if( nSize > m_nMaxLength )
		{
			int nNewLength = m_nMaxLength;

			if( nNewLength < 8 )
				nNewLength = 8;

			if( nSize > nNewLength )
				nNewLength *= 2;

			if( nSize > nNewLength )
			{
				nNewLength  = nSize + 11;
				nNewLength -= nNewLength & 0x07;
			}

			CBufferRefT <ELT> :: m_pBuffer = (ELT *) deelx_realloc(CBufferRefT <ELT> :: m_pBuffer, sizeof(ELT) * nNewLength);
			m_nMaxLength = nNewLength;
		}
	}

public:
	virtual ~CBufferT();

// Content
protected:
	int   m_nMaxLength;
};

//
// Implemenation
//
template <class ELT> CBufferT <ELT> :: CBufferT(const ELT * pcsz, int length) : CBufferRefT <ELT> (0, length)
{
	m_nMaxLength = CBufferRefT <ELT> :: m_nSize + 1;

	CBufferRefT <ELT> :: m_pBuffer = (ELT *) deelx_realloc(0, sizeof(ELT) * m_nMaxLength);
	memcpy(CBufferRefT<ELT>::m_pBuffer, pcsz, sizeof(ELT) * CBufferRefT <ELT> :: m_nSize);
	CBufferRefT<ELT>::m_pBuffer[CBufferRefT <ELT> :: m_nSize] = 0;
}

template <class ELT> CBufferT <ELT> :: CBufferT(const ELT * pcsz) : CBufferRefT <ELT> (pcsz)
{
	m_nMaxLength = CBufferRefT <ELT> :: m_nSize + 1;

	CBufferRefT <ELT> :: m_pBuffer = (ELT *) deelx_realloc(0, sizeof(ELT) * m_nMaxLength);
	memcpy(CBufferRefT<ELT>::m_pBuffer, pcsz, sizeof(ELT) * CBufferRefT <ELT> :: m_nSize);
	CBufferRefT<ELT>::m_pBuffer[CBufferRefT <ELT> :: m_nSize] = 0;
}

template <class ELT> CBufferT <ELT> :: CBufferT() : CBufferRefT <ELT> (0, 0)
{
	m_nMaxLength = 0;
	CBufferRefT<ELT>::m_pBuffer    = 0;
}

template <class ELT> CBufferT <ELT> :: CBufferT(const CBufferT <ELT> & from) : CBufferRefT <ELT> (0, 0)
{
	m_nMaxLength = 0;

	if(from.m_nSize > 0)
		Append(from.m_pBuffer, from.m_nSize, 1);
}

template <class ELT> CBufferT <ELT> & CBufferT <ELT> :: operator = (const CBufferT <ELT> & from)
{
	if(this != &from)
	{
		Restore(0);

		if(from.m_nSize > 0)
			Append(from.m_pBuffer, from.m_nSize, 1);
	}

	return *this;
}

template <class ELT> inline ELT & CBufferT <ELT> :: operator [] (int nIndex)
{
	return CBufferRefT<ELT>::m_pBuffer[nIndex];
}

template <class ELT> inline const ELT & CBufferT <ELT> :: operator [] (int nIndex) const
{
	return CBufferRefT<ELT>::m_pBuffer[nIndex];
}

template <class ELT> void CBufferT <ELT> :: Append(const ELT * pcsz, int length, int eol)
{
	int nNewLength = m_nMaxLength;

	// Check length
	if(nNewLength < 8)
		nNewLength = 8;

	if(CBufferRefT <ELT> :: m_nSize + length + eol > nNewLength)
		nNewLength *= 2;

	if(CBufferRefT <ELT> :: m_nSize + length + eol > nNewLength)
	{
		nNewLength  = CBufferRefT <ELT> :: m_nSize + length + eol + 11;
		nNewLength -= nNewLength % 8;
	}

	// Realloc
	if(nNewLength > m_nMaxLength)
	{
		CBufferRefT <ELT> :: m_pBuffer = (ELT *) deelx_realloc(CBufferRefT<ELT>::m_pBuffer, sizeof(ELT) * nNewLength);
		m_nMaxLength = nNewLength;
	}

	// Append
	memcpy(CBufferRefT<ELT>::m_pBuffer + CBufferRefT <ELT> :: m_nSize, pcsz, sizeof(ELT) * length);
	CBufferRefT <ELT> :: m_nSize += length;

	if(eol > 0) CBufferRefT<ELT>::m_pBuffer[CBufferRefT <ELT> :: m_nSize] = 0;
}

template <class ELT> inline void CBufferT <ELT> :: Append(ELT el, int eol)
{
	Append(&el, 1, eol);
}

template <class ELT> void CBufferT <ELT> :: Push(ELT el)
{
	// Realloc
	if(CBufferRefT <ELT> :: m_nSize >= m_nMaxLength)
	{
		int nNewLength = m_nMaxLength * 2;
		if( nNewLength < 8 ) nNewLength = 8;

		CBufferRefT <ELT> :: m_pBuffer = (ELT *) deelx_realloc(CBufferRefT<ELT>::m_pBuffer, sizeof(ELT) * nNewLength);
		m_nMaxLength = nNewLength;
	}

	// Append
	CBufferRefT<ELT>::m_pBuffer[CBufferRefT <ELT> :: m_nSize++] = el;
}

template <class ELT> void CBufferT <ELT> :: Push(const CBufferRefT<ELT> & buf)
{
	for(int i=0; i<buf.GetSize(); i++)
	{
		Push(buf[i]);
	}

	Push((ELT)buf.GetSize());
}

template <class ELT> inline int CBufferT <ELT> :: Pop(ELT & el)
{
	if(CBufferRefT <ELT> :: m_nSize > 0)
	{
		el = CBufferRefT<ELT>::m_pBuffer[--CBufferRefT <ELT> :: m_nSize];
		return 1;
	}
	else
	{
		return 0;
	}
}

template <class ELT> int CBufferT <ELT> :: Pop (CBufferT<ELT> & buf)
{
	ELT esize = 0;
	int res = Pop(esize);
	int size = res ? (int)esize : 0;
	buf.Restore(size);

	for(int i=size-1; i>=0; i--)
	{
		res = res && Pop(buf[i]);
	}

	return res;
}

template <class ELT> inline int CBufferT <ELT> :: Peek(ELT & el) const
{
	if(CBufferRefT <ELT> :: m_nSize > 0)
	{
		el = CBufferRefT<ELT>::m_pBuffer[CBufferRefT <ELT> :: m_nSize - 1];
		return 1;
	}
	else
	{
		return 0;
	}
}

template <class ELT> const ELT * CBufferT <ELT> :: GetBuffer() const
{
	static const ELT _def[] = {0}; return CBufferRefT<ELT>::m_pBuffer ? CBufferRefT<ELT>::m_pBuffer : _def;
}

template <class ELT> ELT * CBufferT <ELT> :: GetBuffer()
{
	static const ELT _def[] = {0}; return CBufferRefT<ELT>::m_pBuffer ? CBufferRefT<ELT>::m_pBuffer : (ELT *)_def;
}

template <class ELT> ELT * CBufferT <ELT> :: Detach()
{
	ELT * pBuffer = CBufferRefT<ELT>::m_pBuffer;

	CBufferRefT <ELT> :: m_pBuffer = 0;
	CBufferRefT <ELT> :: m_nSize = m_nMaxLength = 0;

	return pBuffer;
}

template <class ELT> void CBufferT <ELT> :: Release()
{
	ELT * pBuffer = Detach();

	if(pBuffer != 0) free(pBuffer);
}

template <class ELT> void CBufferT <ELT> :: Prepare(int index, int fill)
{
	int nNewSize = index + 1;

	// Realloc
	if(nNewSize > m_nMaxLength)
	{
		int nNewLength = m_nMaxLength;

		if( nNewLength < 8 )
			nNewLength = 8;

		if( nNewSize > nNewLength )
			nNewLength *= 2;

		if( nNewSize > nNewLength )
		{
			nNewLength  = nNewSize + 11;
			nNewLength -= nNewLength % 8;
		}

		CBufferRefT <ELT> :: m_pBuffer = (ELT *) deelx_realloc(CBufferRefT<ELT>::m_pBuffer, sizeof(ELT) * nNewLength);
		m_nMaxLength = nNewLength;
	}

	// size
	if( CBufferRefT <ELT> :: m_nSize < nNewSize )
	{
		// assign element-wise: memset() would only be right for fill values 0 and -1
		for(int i = CBufferRefT <ELT> :: m_nSize; i < nNewSize; i++)
			CBufferRefT<ELT>::m_pBuffer[i] = (ELT)(ptrdiff_t)fill; // ELT may be a pointer type

		CBufferRefT <ELT> :: m_nSize = nNewSize;
	}
}

template <class ELT> inline void CBufferT <ELT> :: Restore(int size)
{
	SetMaxLength(size);
	CBufferRefT <ELT> :: m_nSize = size;
}

template <class ELT> CBufferT <ELT> :: ~CBufferT()
{
	if(CBufferRefT<ELT>::m_pBuffer != 0) free(CBufferRefT<ELT>::m_pBuffer);
}

template <class T> class CSortedBufferT : public CBufferT <T>
{
public:
	CSortedBufferT(int reverse = 0);
	CSortedBufferT(int(*)(const void *, const void *));

public:
	void Add(const T & rT);
	void Add(const T * pT, int nSize);
	int  Remove(const T & rT);
	void RemoveAll();

	void SortFreeze() { m_bSortFreezed = 1; }
	void SortUnFreeze();

public:
	int  Find(const T & rT, int(* compare)(const void *, const void *) = 0) { return FindAs(*(T*)&rT, compare); }
	int  FindAs(const T & rT, int(*)(const void *, const void *) = 0);
	int  GetSize() const { return CBufferRefT<T>::m_nSize; }
	T & operator [] (int nIndex) { return CBufferT <T> :: operator [] (nIndex); }

protected:
	int (* m_fncompare)(const void *, const void *);
	static int compareT(const void *, const void *);
	static int compareReverseT(const void *, const void *);

	int  m_bSortFreezed;
};

template <class T> CSortedBufferT <T> :: CSortedBufferT(int reverse)
{
	m_fncompare = reverse ? compareReverseT : compareT;
	m_bSortFreezed = 0;
}

template <class T> CSortedBufferT <T> :: CSortedBufferT(int (* compare)(const void *, const void *))
{
	m_fncompare = compare;
	m_bSortFreezed = 0;
}

template <class T> void CSortedBufferT <T> :: Add(const T & rT)
{
	if(m_bSortFreezed != 0)
	{
		CBufferT<T> :: Append(rT);
		return;
	}

	int a = 0, b = CBufferRefT<T>::m_nSize - 1, c = CBufferRefT<T>::m_nSize / 2;

	while(a <= b)
	{
		int r = m_fncompare(&rT, &CBufferRefT<T>::m_pBuffer[c]);

		if     ( r < 0 ) b = c - 1;
		else if( r > 0 ) a = c + 1;
		else break;

		c = (a + b + 1) / 2;
	}

	CBufferT<T> :: Insert(c, rT);
}

template <class T> void CSortedBufferT <T> :: Add(const T * pT, int nSize)
{
	CBufferT<T> :: Append(pT, nSize);

	if(m_bSortFreezed == 0)
	{
		qsort(CBufferRefT<T>::m_pBuffer, CBufferRefT<T>::m_nSize, sizeof(T), m_fncompare);
	}
}

template <class T> int CSortedBufferT <T> :: FindAs(const T & rT, int(* compare)(const void *, const void *))
{
	const T * pT = (const T *)bsearch(&rT, CBufferRefT<T>::m_pBuffer, CBufferRefT<T>::m_nSize, sizeof(T), compare == 0 ? m_fncompare : compare);

	if( pT != NULL )
		return (int)(pT - CBufferRefT<T>::m_pBuffer);
	else
		return -1;
}

template <class T> int CSortedBufferT <T> :: Remove(const T & rT)
{
	int pos = Find(rT);
	if( pos >= 0 ) CBufferT <T> :: Remove(pos);
	return pos;
}

template <class T> inline void CSortedBufferT <T> :: RemoveAll()
{
	CBufferT<T>::Restore(0);
}

template <class T> void CSortedBufferT <T> :: SortUnFreeze()
{
	if(m_bSortFreezed != 0)
	{
		m_bSortFreezed = 0;
		qsort(CBufferRefT<T>::m_pBuffer, CBufferRefT<T>::m_nSize, sizeof(T), m_fncompare);
	}
}

template <class T> int CSortedBufferT <T> :: compareT(const void * elem1, const void * elem2)
{
	if( *(const T *)elem1 == *(const T *)elem2 )
		return 0;
	else if( *(const T *)elem1 < *(const T *)elem2 )
		return -1;
	else
		return 1;
}

template <class T> int CSortedBufferT <T> :: compareReverseT(const void * elem1, const void * elem2)
{
	if( *(const T *)elem1 == *(const T *)elem2 )
		return 0;
	else if( *(const T *)elem1 > *(const T *)elem2 )
		return -1;
	else
		return 1;
}

//
// Thrown by CContext::Step() when the step limit is exceeded. It does not
// leave CRegexpT: Match() and MatchExact() catch it and report the match as
// abandoned (MatchResult::IsStepLimitExceeded()).
//
struct deelx_step_limit_exceeded
{
};

//
// Context
//
class CContext
{
public:
	CContext()
	{
		m_nCurrentPos = m_nBeginPos = m_nLastBeginPos = m_nParenZindex = m_nCursiveLimit = 0;
		m_nStepLimit = m_nSteps = 0;
		m_pMatchString = 0;
		m_pMatchStringLength = 0;
	}

	// count one backtracking step, give up when there are too many
	void Step()
	{
		if(m_nStepLimit > 0)
		{
			if(m_nSteps >= m_nStepLimit) throw deelx_step_limit_exceeded();
			m_nSteps ++;
		}
	}

public:
	int    m_nStepLimit; // 0: unlimited
	int    m_nSteps;

public:
	CBufferT <int> m_stack;
	CBufferT <int> m_capturestack, m_captureindex;

public:
	int    m_nCurrentPos;
	int    m_nBeginPos;
	int    m_nLastBeginPos;
	int    m_nParenZindex;
	int    m_nCursiveLimit;

	void * m_pMatchString;
	int    m_pMatchStringLength;
};

class CContextShot
{
public:
	CContextShot(CContext * pContext)
	{
		m_nCurrentPos = pContext->m_nCurrentPos;
		nsize  = pContext->m_stack.GetSize();
		ncsize = pContext->m_capturestack.GetSize();
	}

	void Restore(CContext * pContext)
	{
		pContext->m_stack.Restore(nsize);
		pContext->m_capturestack.Restore(ncsize);
		pContext->m_nCurrentPos = m_nCurrentPos;
	}

public:
	int m_nCurrentPos;
	int nsize ;
	int ncsize;
};

//
// Interface
//
class ElxInterface
{
public:
	virtual int Match    (CContext * pContext) const = 0;
	virtual int MatchNext(CContext * pContext) const = 0;

public:
	virtual ~ElxInterface() {};
};

//
// Alternative
//
template <int x> class CAlternativeElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CAlternativeElxT();

public:
	CBufferT <ElxInterface *> m_elxlist;
};

typedef CAlternativeElxT <0> CAlternativeElx;

//
// Assert
//
template <int x> class CAssertElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CAssertElxT(ElxInterface * pelx, int byes = 1);

public:
	ElxInterface * m_pelx;
	int m_byes;
};

typedef CAssertElxT <0> CAssertElx;

//
// Back reference elx
//
template <class CHART> class CBackrefElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CBackrefElxT(int nnumber, int brightleft, int bignorecase, int bfold = 0);

public:
	int m_nnumber;
	int m_brightleft;
	int m_bignorecase;
	int m_bfold; // ignore case by Unicode simple case folding

	CBufferT <CHART> m_szNamed;
};

//
// Implementation
//
template <class CHART> CBackrefElxT <CHART> :: CBackrefElxT(int nnumber, int brightleft, int bignorecase, int bfold)
{
	m_nnumber     = nnumber;
	m_brightleft  = brightleft;
	m_bignorecase = bignorecase;
	m_bfold       = bfold;
}

template <class CHART> int CBackrefElxT <CHART> :: Match(CContext * pContext) const
{
	// check number, for named
	if( m_nnumber < 0 || m_nnumber >= pContext->m_captureindex.GetSize() ) return 0;

	int index = pContext->m_captureindex[m_nnumber];
	if( index < 0 ) return 0;

	// check enclosed
	int pos1 = pContext->m_capturestack[index + 1];
	int pos2 = pContext->m_capturestack[index + 2];

	if( pos2 < 0 ) pos2 = pContext->m_nCurrentPos;

	// info
	int lpos = pos1 < pos2 ? pos1 : pos2;
	int rpos = pos1 < pos2 ? pos2 : pos1;
	int slen = rpos - lpos;

	const CHART * pcsz = (const CHART *)pContext->m_pMatchString;
	int npos = pContext->m_nCurrentPos;
	int tlen = pContext->m_pMatchStringLength;

	// compare
	int bsucc;
	CBufferRefT <CHART> refstr(pcsz + lpos, slen);

	if( m_brightleft )
	{
		if(npos < slen)
			return 0;

		if(m_bignorecase && m_bfold)
			bsucc = deelx_equal_fold(pcsz + lpos, pcsz + (npos - slen), slen);
		else if(m_bignorecase)
			bsucc = ! refstr.nCompareNoCase(pcsz + (npos - slen));
		else
			bsucc = ! refstr.nCompare      (pcsz + (npos - slen));

		if( bsucc )
		{
			pContext->m_stack.Push(npos);
			pContext->m_nCurrentPos -= slen;
		}
	}
	else
	{
		if(npos + slen > tlen)
			return 0;

		if(m_bignorecase && m_bfold)
			bsucc = deelx_equal_fold(pcsz + lpos, pcsz + npos, slen);
		else if(m_bignorecase)
			bsucc = ! refstr.nCompareNoCase(pcsz + npos);
		else
			bsucc = ! refstr.nCompare      (pcsz + npos);

		if( bsucc )
		{
			pContext->m_stack.Push(npos);
			pContext->m_nCurrentPos += slen;
		}
	}

	return bsucc;
}

template <class CHART> int CBackrefElxT <CHART> :: MatchNext(CContext * pContext) const
{
	int npos = 0;

	pContext->m_stack.Pop(npos);
	pContext->m_nCurrentPos = npos;

	return 0;
}

// RCHART
#ifndef RCHART
	#define RCHART(ch) ((CHART)(ch))
#endif

// BOUNDARY_TYPE
enum BOUNDARY_TYPE
{
	BOUNDARY_FILE_BEGIN, // begin of whole text
	BOUNDARY_FILE_END  , // end of whole text
	BOUNDARY_FILE_END_N, // end of whole text, or before newline at the end
	BOUNDARY_LINE_BEGIN, // begin of line
	BOUNDARY_LINE_END  , // end of line
	BOUNDARY_WORD_BEGIN, // begin of word
	BOUNDARY_WORD_END  , // end of word
	BOUNDARY_WORD_EDGE
};

//
// Boundary Elx
//
template <class CHART> class CBoundaryElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CBoundaryElxT(int ntype, int byes = 1, int bunicode = 0);

protected:
	static int IsWordChar(CHART ch);

public:
	int m_ntype;
	int m_byes;
	int m_bunicode; // \b and \B: Unicode word chars, a surrogate pair is one char
};

//
// Implementation
//
template <class CHART> CBoundaryElxT <CHART> :: CBoundaryElxT(int ntype, int byes, int bunicode)
{
	m_ntype    = ntype;
	m_byes     = byes;
	m_bunicode = bunicode;
}

template <class CHART> int CBoundaryElxT <CHART> :: Match(CContext * pContext) const
{
	const CHART * pcsz  = (const CHART *)pContext->m_pMatchString;
	int npos = pContext->m_nCurrentPos;
	int tlen = pContext->m_pMatchStringLength;

	CHART chL = npos > 0    ? pcsz[npos - 1] : 0;
	CHART chR = npos < tlen ? pcsz[npos    ] : 0;

	int bsucc = 0;

	// word chars on both sides
	int bwordL = 0, bwordR = 0;

	if( m_ntype >= BOUNDARY_WORD_BEGIN )
	{
		if( m_bunicode )
		{
			unsigned int cp;
			if( npos > 0    ) { deelx_char_before(pcsz, npos, 1, cp);   bwordL = deelx_unicode_is_word(cp); }
			if( npos < tlen ) { deelx_char_at(pcsz, npos, tlen, 1, cp); bwordR = deelx_unicode_is_word(cp); }
		}
		else
		{
			bwordL = IsWordChar(chL);
			bwordR = IsWordChar(chR);
		}
	}

	switch(m_ntype)
	{
	case BOUNDARY_FILE_BEGIN:
		bsucc = (npos <= 0);
		break;

	case BOUNDARY_FILE_END:
		bsucc = (npos >= tlen);
		break;

	case BOUNDARY_FILE_END_N:
		bsucc = (npos >= tlen) || (pcsz[tlen-1] == RCHART('\n') && (npos == tlen-1 || (pcsz[tlen-2] == RCHART('\r') && npos == tlen-2)));
		break;

	case BOUNDARY_LINE_BEGIN:
		bsucc = (npos <= 0   ) || (chL == RCHART('\n')) || ((chL == RCHART('\r')) && (chR != RCHART('\n')));
		break;

	case BOUNDARY_LINE_END:
		bsucc = (npos >= tlen) || (chR == RCHART('\r')) || ((chR == RCHART('\n')) && (chL != RCHART('\r')));
		break;

	case BOUNDARY_WORD_BEGIN:
		bsucc = ! bwordL &&   bwordR;
		break;

	case BOUNDARY_WORD_END:
		bsucc =   bwordL && ! bwordR;
		break;

	case BOUNDARY_WORD_EDGE:
		bsucc =   bwordL ?  ! bwordR : bwordR;
		break;
	}

	return m_byes ? bsucc : ! bsucc;
}

template <class CHART> int CBoundaryElxT <CHART> :: MatchNext(CContext *) const
{
	return 0;
}

template <class CHART> inline int CBoundaryElxT <CHART> :: IsWordChar(CHART ch)
{
	return (ch >= RCHART('A') && ch <= RCHART('Z')) || (ch >= RCHART('a') && ch <= RCHART('z')) || (ch >= RCHART('0') && ch <= RCHART('9')) || (ch == RCHART('_'));
}

//
// Bracket
//
template <class CHART> class CBracketElxT : public ElxInterface  
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CBracketElxT(int nnumber, int bright);
	static int CheckCaptureIndex(int & index, CContext * pContext, int number);

public:
	int m_nnumber;
	int m_bright;
	int m_balancing;

	CBufferT <CHART> m_szNamed;
	CBufferT <CHART> m_szBalancing;
};

template <class CHART> CBracketElxT <CHART> :: CBracketElxT(int nnumber, int bright)
{
	m_nnumber = nnumber;
	m_bright  = bright;
	m_balancing = -1;
}

template <class CHART> inline int CBracketElxT <CHART> :: CheckCaptureIndex(int & index, CContext * pContext, int number)
{
	if( index >= pContext->m_capturestack.GetSize() )
		index  = pContext->m_capturestack.GetSize() - 4;

	while(index >= 0)
	{
		if(pContext->m_capturestack[index] == number)
		{
			return 1;
		}

		index -= 4;
	}


	return 0;
}

//
// capturestack[index+0] => Group number
// capturestack[index+1] => Capture start pos
// capturestack[index+2] => Capture end pos
// capturestack[index+3] => Capture enclose z-index, zindex<0 means inner group with same name
//
template <class CHART> int CBracketElxT <CHART> :: Match(CContext * pContext) const
{
	// check, for named
	if(m_nnumber < 0) return 0;

	if( ! m_bright )
	{
		pContext->m_captureindex.Prepare(m_nnumber, -1);
		if(m_balancing >= 0) pContext->m_captureindex.Prepare(m_balancing, -1); // (?<a-99>) may refer beyond max group number
		int index = pContext->m_captureindex[m_nnumber];

		// check
		if(CheckCaptureIndex(index, pContext, m_nnumber) && pContext->m_capturestack[index+2] < 0)
		{
			pContext->m_capturestack[index+3] --;
			return 1;
		}

		// balancing left
		if(m_balancing >= 0)
		{
			int balancing_index = pContext->m_captureindex[m_balancing];
			if( ! CheckCaptureIndex(balancing_index, pContext, m_balancing) ||
				pContext->m_capturestack[balancing_index+2] < 0 )
			{
				return 0;
			}
		}

		// save
		pContext->m_captureindex[m_nnumber] = pContext->m_capturestack.GetSize();

		pContext->m_capturestack.Push(m_nnumber);
		pContext->m_capturestack.Push(pContext->m_nCurrentPos);
		pContext->m_capturestack.Push(-1);
		pContext->m_capturestack.Push( 0); // z-index
	}
	else
	{
		// check
		int index = pContext->m_captureindex[m_nnumber];

		if(CheckCaptureIndex(index, pContext, m_nnumber))
		{
			if(pContext->m_capturestack[index + 3] < 0) // check inner group with same name
			{
				pContext->m_capturestack[index + 3] ++;
				return 1;
			}

			// balancing right
			int balancing_index = -1;
			if(m_balancing >= 0)
			{
				balancing_index = pContext->m_captureindex[m_balancing];
				if( ! CheckCaptureIndex(balancing_index, pContext, m_balancing) )
				{
					// TODO ERROR
					return 0;
				}
			}

			// save
			pContext->m_capturestack[index + 2] = pContext->m_nCurrentPos;
			pContext->m_capturestack[index + 3] = pContext->m_nParenZindex ++;

			// balancing right
			if(m_balancing >= 0)
			{
				// backup index
				pContext->m_stack.Push(balancing_index);

				if(balancing_index >= 0)
				{
					pContext->m_capturestack[index+2] = pContext->m_capturestack[index+1];
					pContext->m_capturestack[index+1] = pContext->m_capturestack[balancing_index+2];

					// destopy capture
					pContext->m_capturestack[balancing_index] = -1;
					balancing_index -= 4;
					CheckCaptureIndex(balancing_index, pContext, m_balancing);
					pContext->m_captureindex[m_balancing] = balancing_index;
				}
			}
		}
	}

	return 1;
}

template <class CHART> int CBracketElxT <CHART> :: MatchNext(CContext * pContext) const
{
	int index = pContext->m_captureindex[m_nnumber];
	if( ! CheckCaptureIndex(index, pContext, m_nnumber) )
	{
		return 0;
	}

	if( ! m_bright )
	{
		if(pContext->m_capturestack[index + 3] < 0)
		{
			pContext->m_capturestack[index + 3] ++;
			return 0;
		}

		pContext->m_capturestack.Restore(pContext->m_capturestack.GetSize() - 4);

		// to find
		CheckCaptureIndex(index, pContext, m_nnumber);

		// new index
		pContext->m_captureindex[m_nnumber] = index;
	}
	else
	{
		if( pContext->m_capturestack[index + 2] >= 0 )
		{
			// balancing right
			if(m_balancing >= 0)
			{
				int balancing_index = -1;
				pContext->m_stack.Pop(balancing_index);

				if(balancing_index >= 0)
				{
					pContext->m_capturestack[balancing_index] = m_balancing;
					pContext->m_captureindex[m_balancing] = balancing_index;
				}
			}

			pContext->m_capturestack[index + 2] = -1;
			pContext->m_capturestack[index + 3] =  0;
		}
		else
		{
			pContext->m_capturestack[index + 3] --;
		}
	}

	return 0;
}

//
// Deletage
//
template <class CHART> class CDelegateElxT : public ElxInterface  
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CDelegateElxT(int ndata = 0);

public:
	ElxInterface * m_pelx;
	int m_ndata; // +0 : recursive to
	             // -3 : named recursive

	CBufferT <CHART> m_szNamed;
};

template <class CHART> CDelegateElxT <CHART> :: CDelegateElxT(int ndata)
{
	m_pelx  = 0;
	m_ndata = ndata;
}

template <class CHART> int CDelegateElxT <CHART> :: Match(CContext * pContext) const
{
	pContext->Step();

	if(m_pelx != 0)
	{
		if(pContext->m_nCursiveLimit > 0)
		{
			pContext->m_nCursiveLimit --;
			int result = m_pelx->Match(pContext);
			pContext->m_nCursiveLimit ++;
			return result;
		}
		else
			return 0;
	}
	else
		return 0; // unresolved recursion target, e.g. (?R99) or (?R<nosuchname>)
}

template <class CHART> int CDelegateElxT <CHART> :: MatchNext(CContext * pContext) const
{
	if(m_pelx != 0)
		return m_pelx->MatchNext(pContext);
	else
		return 0;
}

//
// Empty
//
template <int x> class CEmptyElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CEmptyElxT();
};

typedef CEmptyElxT <0> CEmptyElx;

//
// Global
//
template <int x> class CGlobalElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CGlobalElxT();
};

typedef CGlobalElxT <0> CGlobalElx;

//
// Keep (\K): the match starts here, the text before is kept
//
template <int x> class CKeepElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;
};

typedef CKeepElxT <0> CKeepElx;

//
// Repeat
//
template <int x> class CRepeatElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CRepeatElxT(ElxInterface * pelx, int ntimes);

protected:
	int MatchFixed    (CContext * pContext) const;
	int MatchNextFixed(CContext * pContext) const;
	int MatchForward  (CContext * pContext) const
	{
		CContextShot shot(pContext);
		
		if( ! m_pelx->Match(pContext) )
			return 0;

		if(pContext->m_nCurrentPos != shot.m_nCurrentPos)
			return 1;

		if( ! m_pelx->MatchNext(pContext) )
			return 0;

		if(pContext->m_nCurrentPos != shot.m_nCurrentPos)
			return 1;

		shot.Restore(pContext);
		return 0;
	}

public:
	ElxInterface * m_pelx;
	int m_nfixed;
};

typedef CRepeatElxT <0> CRepeatElx;

//
// Greedy
//
template <int x> class CGreedyElxT : public CRepeatElxT <x>
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CGreedyElxT(ElxInterface * pelx, int nmin = 0, int nmax = INT_MAX);

protected:
	int MatchVart    (CContext * pContext) const;
	int MatchNextVart(CContext * pContext) const;

public:
	int m_nvart;
};

typedef CGreedyElxT <0> CGreedyElx;

//
// Independent
//
template <int x> class CIndependentElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CIndependentElxT(ElxInterface * pelx);

public:
	ElxInterface * m_pelx;
};

typedef CIndependentElxT <0> CIndependentElx;

//
// List
//
template <int x> class CListElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CListElxT(int brightleft);

public:
	CBufferT <ElxInterface *> m_elxlist;
	int m_brightleft;
};

typedef CListElxT <0> CListElx;

//
// Posix Elx
//
template <class CHART> class CPosixElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CPosixElxT(const char * posix, int brightleft, int bpair = 0);

public:
	POSIX_FUNC m_posixfun;
	int m_brightleft;
	int m_byes;
	int m_bpair; // a UTF-16 surrogate pair is one char (UNICODE_MODE)
};

//
// Implementation
//
template <class CHART> CPosixElxT <CHART> :: CPosixElxT(const char * posix, int brightleft, int bpair)
{
	m_brightleft = brightleft;
	m_bpair      = bpair;

	if(posix[1] == '^')
	{
		m_byes = 0;
		posix += 2;
	}
	else
	{
		m_byes = 1;
		posix += 1;
	}

	if     (!strncmp(posix, "alnum:", 6)) m_posixfun = ::isalnum ;
	else if(!strncmp(posix, "alpha:", 6)) m_posixfun = ::isalpha ;
	else if(!strncmp(posix, "ascii:", 6)) m_posixfun =  deelx_isascii;
	else if(!strncmp(posix, "cntrl:", 6)) m_posixfun = ::iscntrl ;
	else if(!strncmp(posix, "digit:", 6)) m_posixfun = ::isdigit ;
	else if(!strncmp(posix, "graph:", 6)) m_posixfun = ::isgraph ;
	else if(!strncmp(posix, "lower:", 6)) m_posixfun = ::islower ;
	else if(!strncmp(posix, "print:", 6)) m_posixfun = ::isprint ;
	else if(!strncmp(posix, "punct:", 6)) m_posixfun = ::ispunct ;
	else if(!strncmp(posix, "space:", 6)) m_posixfun = ::isspace ;
	else if(!strncmp(posix, "upper:", 6)) m_posixfun = ::isupper ;
	else if(!strncmp(posix, "xdigit:",7)) m_posixfun = ::isxdigit;
	else if(!strncmp(posix, "blank:", 6)) m_posixfun =  deelx_isblank;
	else                                  m_posixfun = 0         ;
}

inline int deelx_isblank(int c)
{
	return c == 0x20 || c == '\t';
}

inline int deelx_isascii(int c) // isascii() is POSIX, not standard C/C++
{
	return c >= 0 && c < 0x80;
}

template <class CHART> int CPosixElxT <CHART> :: Match(CContext * pContext) const
{
	if(m_posixfun == 0) return 0;

	int tlen = pContext->m_pMatchStringLength;
	int npos = pContext->m_nCurrentPos;

	// check
	int at   = m_brightleft ? npos - 1 : npos;
	if( at < 0 || at >= tlen )
		return 0;

	const CHART * pcsz = (const CHART *)pContext->m_pMatchString;

	unsigned int ch;
	int width = m_brightleft ? deelx_char_before(pcsz, npos, m_bpair, ch) : deelx_char_at(pcsz, npos, tlen, m_bpair, ch);

	int bsucc = deelx_lt256(ch) && (*m_posixfun)((int)ch);

	if( ! m_byes )
		bsucc = ! bsucc;

	if( bsucc )
	{
		pContext->m_nCurrentPos += m_brightleft ? -width : width;
		if( m_bpair ) pContext->m_stack.Push(width);
	}

	return bsucc;
}

template <class CHART> int CPosixElxT <CHART> :: MatchNext(CContext * pContext) const
{
	int width = 1;
	if( m_bpair ) pContext->m_stack.Pop(width);

	pContext->m_nCurrentPos -= m_brightleft ? -width : width;
	return 0;
}

//
// Possessive
//
template <int x> class CPossessiveElxT : public CGreedyElxT <x>
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CPossessiveElxT(ElxInterface * pelx, int nmin = 0, int nmax = INT_MAX);
};

typedef CPossessiveElxT <0> CPossessiveElx;

//
// Range Elx
//
template <class CHART> class CRangeElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CRangeElxT(int brightleft, int byes);

public:
	int IsContainChar(unsigned int ch) const;

public:
	CBufferT <unsigned int> m_ranges; // code points: first, last, first, last, ...
	CBufferT <unsigned int> m_chars;  // code points
	CBufferT <ElxInterface *> m_embeds;
	unsigned int m_ncategories;       // bit (1 << DEELX_UC_xx) for each general category

public:
	int m_brightleft;
	int m_byes;
	int m_bpair; // a UTF-16 surrogate pair is one char (UNICODE_MODE)
	int m_bfold; // also try the case folded char against m_ranges and m_chars
};

//
// Implementation
//
template <class CHART> CRangeElxT <CHART> :: CRangeElxT(int brightleft, int byes)
{
	m_brightleft  = brightleft;
	m_byes        = byes;
	m_ncategories = 0;
	m_bpair       = 0;
	m_bfold       = 0;
}

template <class CHART> int CRangeElxT <CHART> :: Match(CContext * pContext) const
{
	const CHART * pcsz = (const CHART *)pContext->m_pMatchString;
	int tlen = pContext->m_pMatchStringLength;
	int npos = pContext->m_nCurrentPos;

	// check
	int at   = m_brightleft ? npos - 1 : npos;
	if( at < 0 || at >= tlen )
		return 0;

	unsigned int ch;
	int width = m_brightleft ? deelx_char_before(pcsz, npos, m_bpair, ch) : deelx_char_at(pcsz, npos, tlen, m_bpair, ch);

	// compare
	int bsucc = IsContainChar(ch);

	if( ! bsucc && m_bfold )
		bsucc = IsContainChar(deelx_unicode_fold(ch));

	if( ! bsucc && m_ncategories != 0 )
		bsucc = (m_ncategories >> deelx_unicode_category(ch)) & 1;

	for(int i=0; !bsucc && i<m_embeds.GetSize(); i++)
	{
		int nsize = pContext->m_stack.GetSize();

		if(m_embeds[i]->Match(pContext))
		{
			pContext->m_stack.Restore(nsize); // it may have pushed the length of the char
			pContext->m_nCurrentPos = npos;
			bsucc = 1;
		}
	}

	if( ! m_byes )
		bsucc = ! bsucc;

	if( bsucc )
	{
		pContext->m_nCurrentPos += m_brightleft ? -width : width;
		if( m_bpair ) pContext->m_stack.Push(width);
	}

	return bsucc;
}

template <class CHART> int CRangeElxT <CHART> :: IsContainChar(unsigned int ch) const
{
	int bsucc = 0, i;

	// compare
	for(i=0; !bsucc && i<m_ranges.GetSize(); i+=2)
	{
		if(m_ranges[i] <= ch && ch <= m_ranges[i+1]) bsucc = 1;
	}

	for(i=0; !bsucc && i<m_chars.GetSize(); i++)
	{
		if(m_chars[i] == ch) bsucc = 1;
	}

	return bsucc;
}

template <class CHART> int CRangeElxT <CHART> :: MatchNext(CContext * pContext) const
{
	int width = 1;
	if( m_bpair ) pContext->m_stack.Pop(width);

	pContext->m_nCurrentPos -= m_brightleft ? -width : width;
	return 0;
}

//
// Reluctant
//
template <int x> class CReluctantElxT : public CRepeatElxT <x>
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CReluctantElxT(ElxInterface * pelx, int nmin = 0, int nmax = INT_MAX);

protected:
	int MatchVart    (CContext * pContext) const;
	int MatchNextVart(CContext * pContext) const;

public:
	int m_nvart;
};

typedef CReluctantElxT <0> CReluctantElx;

//
// String Elx
//
template <class CHART> class CStringElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CStringElxT(const CHART * fixed, int nlength, int brightleft, int bignorecase, int bfold = 0);

public:
	CBufferT <CHART> m_szPattern;
	int m_brightleft;
	int m_bignorecase;
	int m_bfold; // ignore case by Unicode simple case folding
};

//
// Implementation
//
template <class CHART> CStringElxT <CHART> :: CStringElxT(const CHART * fixed, int nlength, int brightleft, int bignorecase, int bfold) : m_szPattern(fixed, nlength)
{
	m_brightleft  = brightleft;
	m_bignorecase = bignorecase;
	m_bfold       = bfold;
}

template <class CHART> int CStringElxT <CHART> :: Match(CContext * pContext) const
{
	const CHART * pcsz  = (const CHART *)pContext->m_pMatchString;
	int npos = pContext->m_nCurrentPos;
	int tlen = pContext->m_pMatchStringLength;
	int slen = m_szPattern.GetSize();

	int bsucc;

	if(m_brightleft)
	{
		if(npos < slen)
			return 0;

		if(m_bignorecase && m_bfold)
			bsucc = deelx_equal_fold(m_szPattern.GetBuffer(), pcsz + (npos - slen), slen);
		else if(m_bignorecase)
			bsucc = ! m_szPattern.nCompareNoCase(pcsz + (npos - slen));
		else
			bsucc = ! m_szPattern.nCompare      (pcsz + (npos - slen));

		if( bsucc )
			pContext->m_nCurrentPos -= slen;
	}
	else
	{
		if(npos + slen > tlen)
			return 0;

		if(m_bignorecase && m_bfold)
			bsucc = deelx_equal_fold(m_szPattern.GetBuffer(), pcsz + npos, slen);
		else if(m_bignorecase)
			bsucc = ! m_szPattern.nCompareNoCase(pcsz + npos);
		else
			bsucc = ! m_szPattern.nCompare      (pcsz + npos);

		if( bsucc )
			pContext->m_nCurrentPos += slen;
	}

	return bsucc;
}

template <class CHART> int CStringElxT <CHART> :: MatchNext(CContext * pContext) const
{
	int slen = m_szPattern.GetSize();

	if(m_brightleft)
		pContext->m_nCurrentPos += slen;
	else
		pContext->m_nCurrentPos -= slen;

	return 0;
}

//
// CConditionElx
//
template <class CHART> class CConditionElxT : public ElxInterface
{
public:
	int Match    (CContext * pContext) const;
	int MatchNext(CContext * pContext) const;

public:
	CConditionElxT();

public:
	// backref condition
	int m_nnumber;
	CBufferT <CHART> m_szNamed;

	// elx condition
	ElxInterface * m_pelxask;

	// selection
	ElxInterface * m_pelxyes, * m_pelxno;
};

template <class CHART> CConditionElxT <CHART> :: CConditionElxT()
{
	m_nnumber = -1;
	m_pelxask = 0;
	m_pelxyes = 0;
	m_pelxno  = 0;
}

template <class CHART> int CConditionElxT <CHART> :: Match(CContext * pContext) const
{
	// status
	int nbegin = pContext->m_nCurrentPos;
	int nsize  = pContext->m_stack.GetSize();
	int ncsize = pContext->m_capturestack.GetSize();

	// condition result
	int condition_yes = 0;

	// backref type
	if( m_nnumber >= 0 )
	{
		do
		{
			if(m_nnumber >= pContext->m_captureindex.GetSize()) break;

			int index = pContext->m_captureindex[m_nnumber];
			if( index < 0) break;

			// else valid
			condition_yes = 1;
		}
		while(0);
	}
	else
	{
		if( m_pelxask == 0 )
			condition_yes = 1;
		else
			condition_yes = m_pelxask->Match(pContext);

		pContext->m_stack.Restore(nsize);
		pContext->m_nCurrentPos = nbegin;
	}

	// elx result
	int bsucc;
	if( condition_yes )
		bsucc = m_pelxyes == 0 ? 1 : m_pelxyes->Match(pContext);
	else
		bsucc = m_pelxno  == 0 ? 1 : m_pelxno ->Match(pContext);

	if( bsucc )
	{
		pContext->m_stack.Push(ncsize);
		pContext->m_stack.Push(condition_yes);
	}
	else
	{
		pContext->m_capturestack.Restore(ncsize);
	}

	return bsucc;
}

template <class CHART> int CConditionElxT <CHART> :: MatchNext(CContext * pContext) const
{
	// pop
	int ncsize = 0, condition_yes = 0;

	pContext->m_stack.Pop(condition_yes);
	pContext->m_stack.Pop(ncsize);

	// elx result
	int bsucc;
	if( condition_yes )
		bsucc = m_pelxyes == 0 ? 0 : m_pelxyes->MatchNext(pContext);
	else
		bsucc = m_pelxno  == 0 ? 0 : m_pelxno ->MatchNext(pContext);

	if( bsucc )
	{
		pContext->m_stack.Push(ncsize);
		pContext->m_stack.Push(condition_yes);
	}
	else
	{
		pContext->m_capturestack.Restore(ncsize);
	}

	return bsucc;
}

//
// MatchResult
//
template <int x> class MatchResultT
{
public:
	int IsMatched() const;

public:
	int GetStart() const;
	int GetEnd  () const;

public:
	int MaxGroupNumber() const;
	int GetGroupStart(int nGroupNumber) const;
	int GetGroupEnd  (int nGroupNumber) const;

	// not matched because the step limit (CRegexpT::SetStepLimit) was exceeded
	int IsStepLimitExceeded() const;

public:
	MatchResultT(const MatchResultT <x> & from) { m_bStepLimitExceeded = 0; *this = from; }
	MatchResultT(CContext * pContext = 0, int nMaxNumber = -1);
	MatchResultT <x> & operator = (const MatchResultT <x> &);
	inline operator int() const { return IsMatched(); }

public:
	CBufferT <int> m_result;
	int m_bStepLimitExceeded;
};

typedef MatchResultT <0> MatchResult;

// Stocked Elx IDs
enum STOCKELX_ID_DEFINES
{
	STOCKELX_EMPTY = 0,

	///////////////////////

	STOCKELX_DOT_ALL,
	STOCKELX_DOT_NOT_ALL,

	STOCKELX_WORD,
	STOCKELX_WORD_NOT,

	STOCKELX_SPACE,
	STOCKELX_SPACE_NOT,

	STOCKELX_DIGITAL,
	STOCKELX_DIGITAL_NOT,

	//////////////////////

	STOCKELX_DOT_ALL_RIGHTLEFT,
	STOCKELX_DOT_NOT_ALL_RIGHTLEFT,

	STOCKELX_WORD_RIGHTLEFT,
	STOCKELX_WORD_RIGHTLEFT_NOT,

	STOCKELX_SPACE_RIGHTLEFT,
	STOCKELX_SPACE_RIGHTLEFT_NOT,

	STOCKELX_DIGITAL_RIGHTLEFT,
	STOCKELX_DIGITAL_RIGHTLEFT_NOT,

	/////////////////////

	STOCKELX_COUNT
};

// REGEX_FLAGS
#ifndef _REGEX_FLAGS_DEFINED
	enum REGEX_FLAGS
	{
		NO_FLAG        = 0,
		SINGLELINE     = 0x01,
		MULTILINE      = 0x02,
		GLOBAL         = 0x04,
		IGNORECASE     = 0x08,
		RIGHTTOLEFT    = 0x10,
		EXTENDED       = 0x20,
		UNICODE_MODE   = 0x40  // Unicode \w \d \s \b and case folding, surrogate pairs as one char; no effect on CRegexpA
	};
	#define _REGEX_FLAGS_DEFINED
#endif

//
// Builder T
//
template <class CHART> class CBuilderT
{
public:
	typedef CDelegateElxT  <CHART> CDelegateElx;
	typedef CBracketElxT   <CHART> CBracketElx;
	typedef CBackrefElxT   <CHART> CBackrefElx;
	typedef CConditionElxT <CHART> CConditionElx;

// Methods
public:
	ElxInterface * Build(const CBufferRefT <CHART> & pattern, int flags);
	int GetNamedNumber(const CBufferRefT <CHART> & named) const;
	const CHART * GetNamedName(int nnumber) const;
	void Clear();

public:
	 CBuilderT();
	~CBuilderT();

// Public Attributes
public:
	ElxInterface * m_pTopElx;
	int            m_nFlags;
	int            m_nMaxNumber;
	int            m_nNextNamed;
	int            m_nGroupCount;
	int            m_nNextBalancing;

	CBufferT <ElxInterface  *> m_objlist;
	CBufferT <ElxInterface  *> m_grouplist;
	CBufferT <CDelegateElx  *> m_recursivelist;
	CBufferT <CListElx      *> m_namedlist;
	CBufferT <CBackrefElx   *> m_namedbackreflist;
	CBufferT <CConditionElx *> m_namedconditionlist;
	CBufferT <CListElx      *> m_purebalancinglist;

// CHART_INFO
protected:
	struct CHART_INFO
	{
	public:
		CHART ch;
		int   type;
		int   pos;
		int   len;

	public:
		CHART_INFO(CHART c, int t, int p = 0, int l = 0)     { ch = c; type = t; pos = p; len = l;    }
		inline int operator == (const CHART_INFO & ci) const { return ch == ci.ch && type == ci.type; }
		inline int operator != (const CHART_INFO & ci) const { return ! operator == (ci);             }
	};

protected:
	static unsigned int Hex2Int(const CHART * pcsz, int length, int & used);
	int HexLimit(int pos, int length) const // clip to pattern, which need not be NUL-terminated
	{
		int rest = m_pattern.GetSize() - pos;
		return rest < length ? rest : length;
	}
	static int ReadDec(char * & str, unsigned int & dec);
	static int ReadGroupNumber(char * str, unsigned int & number)
	{
		return ReadDec(str, number) && *str == '\0' && number <= DEELX_MAX_GROUP_NUMBER;
	}
	void MoveNext();
	int  GetNext2();
	void Retokenize();
	void SetExtended(int flags);

	ElxInterface * BuildAlternative(int vaflags);
	ElxInterface * BuildList       (int & flags);
	ElxInterface * BuildRepeat     (int & flags);
	ElxInterface * BuildSimple     (int & flags);
	ElxInterface * BuildCharset    (int & flags);
	ElxInterface * BuildRecursive  (int & flags);
	ElxInterface * BuildBoundary   (int & flags);
	ElxInterface * BuildBackref    (int & flags);
	ElxInterface * BuildEscape     (int & flags);

	ElxInterface * GetStockElx     (int nStockId);
	ElxInterface * Keep(ElxInterface * pElx);

	static int IsUnicode(int flags)
	{
		return (flags & UNICODE_MODE) && sizeof(CHART) > 1;
	}
	CRangeElxT <CHART> * NewRange(int flags, int byes);
	unsigned int ReadClassChar(int flags);
	ElxInterface * BuildProperty(int pos, int len, int flags);
	static int SetProperty(CRangeElxT <CHART> * pRange, const char * name);
	static void AddFoldedChars(CRangeElxT <CHART> * pRange);

// Private Attributes
protected:
	CBufferRefT <CHART> m_pattern;
	CHART_INFO prev, curr, next, nex2;
	int m_nNextPos;
	int m_nCharsetDepth;
	int m_bQuoted;
	POSIX_FUNC m_quote_fun;
	unsigned int m_nPendingLow; // low surrogate still to be read after \u{10000} or above

	// tokenizer state before reading a token, to read it again with other flags
	struct TokenizerState
	{
		int m_nNextPos;
		int m_nCharsetDepth;
		int m_bQuoted;
		POSIX_FUNC m_quote_fun;
		unsigned int m_nPendingLow;
	};
	TokenizerState m_nextFrom, m_nex2From; // before next and nex2 were read

	void SaveState(TokenizerState * pstate) const
	{
		pstate->m_nNextPos      = m_nNextPos;
		pstate->m_nCharsetDepth = m_nCharsetDepth;
		pstate->m_bQuoted       = m_bQuoted;
		pstate->m_quote_fun     = m_quote_fun;
		pstate->m_nPendingLow   = m_nPendingLow;
	}
	void LoadState(const TokenizerState * pstate)
	{
		m_nNextPos      = pstate->m_nNextPos;
		m_nCharsetDepth = pstate->m_nCharsetDepth;
		m_bQuoted       = pstate->m_bQuoted;
		m_quote_fun     = pstate->m_quote_fun;
		m_nPendingLow   = pstate->m_nPendingLow;
	}

	// Backup current pos
	struct Snapshot
	{
		CHART_INFO prev, curr, next, nex2;
		TokenizerState state, nextFrom, nex2From;
		Snapshot():prev(0,0),curr(0,0),next(0,0),nex2(0,0) {}
	};
	void Backup (Snapshot * pdata)
	{
		pdata->prev = prev; pdata->curr = curr; pdata->next = next; pdata->nex2 = nex2;
		SaveState(&pdata->state);
		pdata->nextFrom = m_nextFrom;
		pdata->nex2From = m_nex2From;
	}
	void Restore(Snapshot * pdata)
	{
		prev = pdata->prev; curr = pdata->curr; next = pdata->next; nex2 = pdata->nex2;
		LoadState(&pdata->state);
		m_nextFrom = pdata->nextFrom;
		m_nex2From = pdata->nex2From;
	}

	ElxInterface * m_pStockElxs[STOCKELX_COUNT];

private:
	// owns the objects in m_objlist: copying would delete them twice
	CBuilderT(const CBuilderT <CHART> &);
	CBuilderT <CHART> & operator = (const CBuilderT <CHART> &);
};

//
// Implementation
//
template <class CHART> CBuilderT <CHART> :: CBuilderT() : m_pattern(0, 0), prev(0, 0), curr(0, 0), next(0, 0), nex2(0, 0)
{
	Clear();
}

template <class CHART> CBuilderT <CHART> :: ~CBuilderT()
{
	Clear();
}

template <class CHART> int CBuilderT <CHART> :: GetNamedNumber(const CBufferRefT <CHART> & named) const
{
	for(int i=0; i<m_namedlist.GetSize(); i++)
	{
		if( ! ((CBracketElx *)m_namedlist[i]->m_elxlist[0])->m_szNamed.CompareNoCase(named) )
			return ((CBracketElx *)m_namedlist[i]->m_elxlist[0])->m_nnumber;
	}

	return -3;
}

//
// Name of group nnumber as a NUL-terminated string, or "" if it has none.
// The string is owned by the builder and valid until the next Build/Clear.
//
template <class CHART> const CHART * CBuilderT <CHART> :: GetNamedName(int nnumber) const
{
	static const CHART _def[] = {0};

	for(int i=0; i<m_namedlist.GetSize(); i++)
	{
		const CBracketElx * pleft = (const CBracketElx *)m_namedlist[i]->m_elxlist[0];

		// (?<2>...) is kept in m_namedlist too, but has no name
		if( pleft->m_nnumber == nnumber && pleft->m_szNamed.GetSize() > 0 )
			return pleft->m_szNamed.GetBuffer();
	}

	return _def;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: Build(const CBufferRefT <CHART> & pattern, int flags)
{
	// init
	m_pattern       = pattern;
	m_nNextPos      = 0;
	m_nCharsetDepth = 0;
	m_nMaxNumber    = 0;
	m_nNextNamed    = 0;
	m_nNextBalancing= 0;
	m_nFlags        = flags;
	m_bQuoted       = 0;
	m_quote_fun     = 0;
	m_nPendingLow   = 0;

	SaveState(&m_nextFrom);
	SaveState(&m_nex2From);

	m_grouplist         .Restore(0);
	m_recursivelist     .Restore(0);
	m_namedlist         .Restore(0);
	m_namedbackreflist  .Restore(0);
	m_namedconditionlist.Restore(0);
	m_purebalancinglist .Restore(0);

	int i;
	for(i=0; i<3; i++) MoveNext();

	// build
	m_pTopElx = BuildAlternative(flags);

	// (?x) may have changed EXTENDED for the tokenizer
	m_nFlags = flags;

	// group 0
	m_grouplist.Prepare(0);
	m_grouplist[0] = m_pTopElx;

	// append named to unnamed
	m_nGroupCount = m_grouplist.GetSize();

	m_grouplist.Prepare(m_nMaxNumber + m_namedlist.GetSize());

	for(i=0; i<m_namedlist.GetSize(); i++)
	{
		CBracketElx * pleft  = (CBracketElx *)m_namedlist[i]->m_elxlist[0];
		CBracketElx * pright = (CBracketElx *)m_namedlist[i]->m_elxlist[2];

		// append
		m_grouplist[m_nGroupCount ++] = m_namedlist[i];

		if( pleft->m_nnumber > 0 )
			continue;

		// same name
		int find_same_name = GetNamedNumber(pleft->m_szNamed);
		if( find_same_name >= 0 )
		{
			pleft ->m_nnumber = find_same_name;
			pright->m_nnumber = find_same_name;
		}
		else
		{
			m_nMaxNumber ++;

			pleft ->m_nnumber = m_nMaxNumber;
			pright->m_nnumber = m_nMaxNumber;
		}
	}

	for(i=0; i<m_namedlist.GetSize(); i++)
	{
		CBracketElx * pleft  = (CBracketElx *)m_namedlist[i]->m_elxlist[0];
		CBracketElx * pright = (CBracketElx *)m_namedlist[i]->m_elxlist[2];

		// balancing
		if(pleft->m_szBalancing.GetSize() > 0)
		{
			int balancing_to = GetNamedNumber(pleft->m_szBalancing);
			if(balancing_to >= 0)
			{
				pleft ->m_balancing = balancing_to;
				pright->m_balancing = balancing_to;
			}
			else
			{
				// TODO ERROR
			}
		}
	}

	for(i=1; i<m_nGroupCount; i++)
	{
		CBracketElx * pleft = (CBracketElx *)((CListElx*)m_grouplist[i])->m_elxlist[0];

		if( pleft->m_nnumber > m_nMaxNumber )
			m_nMaxNumber = pleft->m_nnumber;
	}

	// pure balancing group
	int nMaxNumber = m_nMaxNumber;
	for(i=0; i<m_purebalancinglist.GetSize(); i++)
	{
		CBracketElx * pleft  = (CBracketElx *)m_purebalancinglist[i]->m_elxlist[0];
		CBracketElx * pright = (CBracketElx *)m_purebalancinglist[i]->m_elxlist[2];

		nMaxNumber ++;
		
		pleft ->m_nnumber = nMaxNumber;
		pright->m_nnumber = nMaxNumber;

		// balancing
		if(pleft->m_szBalancing.GetSize() > 0)
		{
			int balancing_to = GetNamedNumber(pleft->m_szBalancing);
			if(balancing_to >= 0)
			{
				pleft ->m_balancing = balancing_to;
				pright->m_balancing = balancing_to;
			}
			else
			{
				// TODO ERROR
			}
		}
	}

	// connect recursive
	for(i=0; i<m_recursivelist.GetSize(); i++)
	{
		if( m_recursivelist[i]->m_ndata == -3 )
			m_recursivelist[i]->m_ndata = GetNamedNumber(m_recursivelist[i]->m_szNamed);

		if( m_recursivelist[i]->m_ndata >= 0 && m_recursivelist[i]->m_ndata <= m_nMaxNumber )
		{
			if( m_recursivelist[i]->m_ndata == 0 )
				m_recursivelist[i]->m_pelx = m_pTopElx;
			else for(int j=1; j<m_grouplist.GetSize(); j++)
			{
				if(m_recursivelist[i]->m_ndata == ((CBracketElx *)((CListElx*)m_grouplist[j])->m_elxlist[0])->m_nnumber)
				{
					m_recursivelist[i]->m_pelx = m_grouplist[j];
					break;
				}
			}
		}
	}

	// named backref
	for(i=0; i<m_namedbackreflist.GetSize(); i++)
	{
		m_namedbackreflist[i]->m_nnumber = GetNamedNumber(m_namedbackreflist[i]->m_szNamed);
	}

	// named condition
	for(i=0; i<m_namedconditionlist.GetSize(); i++)
	{
		int nn = GetNamedNumber(m_namedconditionlist[i]->m_szNamed);
		if( nn >= 0 )
		{
			m_namedconditionlist[i]->m_nnumber = nn;
			m_namedconditionlist[i]->m_pelxask = 0;
		}
	}

	return m_pTopElx;
}

template <class CHART> void CBuilderT <CHART> :: Clear()
{
	for(int i=0; i<m_objlist.GetSize(); i++)
	{
		delete m_objlist[i];
	}

	m_objlist.Restore(0);
	m_pTopElx = 0;
	m_nMaxNumber = 0;

	// these hold pointers into m_objlist, which have just been deleted
	m_grouplist         .Restore(0);
	m_recursivelist     .Restore(0);
	m_namedlist         .Restore(0);
	m_namedbackreflist  .Restore(0);
	m_namedconditionlist.Restore(0);
	m_purebalancinglist .Restore(0);

	memset(m_pStockElxs, 0, sizeof(m_pStockElxs));
}

//
// hex to int
//
template <class CHART> unsigned int CBuilderT <CHART> :: Hex2Int(const CHART * pcsz, int length, int & used)
{
	unsigned int result = 0;
	int & i = used;

	for(i=0; i<length; i++)
	{
		if(pcsz[i] >= RCHART('0') && pcsz[i] <= RCHART('9'))
			result = (result << 4) + (pcsz[i] - RCHART('0'));
		else if(pcsz[i] >= RCHART('A') && pcsz[i] <= RCHART('F'))
			result = (result << 4) + (0x0A + (pcsz[i] - RCHART('A')));
		else if(pcsz[i] >= RCHART('a') && pcsz[i] <= RCHART('f'))
			result = (result << 4) + (0x0A + (pcsz[i] - RCHART('a')));
		else
			break;
	}

	return result;
}

template <class CHART> inline ElxInterface * CBuilderT <CHART> :: Keep(ElxInterface * pelx)
{
	deelx_check_new(pelx);

	try
	{
		m_objlist.Push(pelx);
	}
	catch(...)
	{
		delete pelx; // not owned by m_objlist yet
		throw;
	}

	return pelx;
}

template <class CHART> void CBuilderT <CHART> :: MoveNext()
{
	// forwards
	prev = curr;
	curr = next;
	next = nex2;

	m_nextFrom = m_nex2From;
	SaveState(&m_nex2From);

	// get nex2
	while( ! GetNext2() ) {};
}

//
// Read next and nex2 again. They were read ahead before (?x) or the end of
// its group changed EXTENDED, which decides whether white space is skipped.
//
template <class CHART> void CBuilderT <CHART> :: Retokenize()
{
	CHART_INFO p = prev, c = curr;
	TokenizerState nextFrom, nex2From;

	LoadState(&m_nextFrom);

	// GetNext2() looks at the two tokens before the one it reads
	curr = p;
	next = c;
	SaveState(&nextFrom);
	while( ! GetNext2() ) {};

	CHART_INFO n = nex2;
	curr = c;
	next = n;
	SaveState(&nex2From);
	while( ! GetNext2() ) {};

	prev = p;
	m_nextFrom = nextFrom;
	m_nex2From = nex2From;
}

template <class CHART> void CBuilderT <CHART> :: SetExtended(int flags)
{
	if( (m_nFlags ^ flags) & EXTENDED )
	{
		m_nFlags ^= EXTENDED;
		Retokenize();
	}
}

template <class CHART> int CBuilderT <CHART> :: GetNext2()
{
	// second half of \u{...} above U+FFFF
	if(m_nPendingLow != 0)
	{
		nex2 = CHART_INFO(RCHART(m_nPendingLow), 0, m_nNextPos, 0);
		m_nPendingLow = 0;
		return 1;
	}

	// check length
	if(m_nNextPos >= m_pattern.GetSize())
	{
		nex2 = CHART_INFO(0, 1, m_nNextPos, 0);
		return 1;
	}

	int   delta = 1;
	CHART ch    = m_pattern[m_nNextPos];

	// if quoted
	if(m_bQuoted)
	{
		if(ch == RCHART('\\'))
		{
			if(m_pattern[m_nNextPos + 1] == RCHART('E'))
			{
				m_quote_fun = 0;
				m_bQuoted   = 0;
				m_nNextPos += 2;
				return 0;
			}
		}

		if(m_quote_fun != 0 && deelx_lt256(ch))
			nex2 = CHART_INFO((CHART)(*m_quote_fun)((int)ch), 0, m_nNextPos, delta);
		else
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);

		m_nNextPos += delta;

		return 1;
	}

	// common
	switch(ch)
	{
	case RCHART('\\'):
		{
			CHART ch1 = m_pattern[m_nNextPos+1];

			// backref
			if(ch1 >= RCHART('0') && ch1 <= RCHART('9'))
			{
				nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
				break;
			}

			// escape
			delta     = 2;

			switch(ch1)
			{
			case RCHART('A'):
			case RCHART('Z'):
			case RCHART('z'):
			case RCHART('w'):
			case RCHART('W'):
			case RCHART('s'):
			case RCHART('S'):
			case RCHART('B'):
			case RCHART('d'):
			case RCHART('D'):
			case RCHART('k'):
			case RCHART('g'):
				nex2 = CHART_INFO(ch1, 1, m_nNextPos, delta);
				break;

			case RCHART('b'):
				if(m_nCharsetDepth > 0)
					nex2 = CHART_INFO('\b', 0, m_nNextPos, delta);
				else
					nex2 = CHART_INFO(ch1, 1, m_nNextPos, delta);
				break;

			/*
			case RCHART('<'):
			case RCHART('>'):
				if(m_nCharsetDepth > 0)
					nex2 = CHART_INFO(ch1, 0, m_nNextPos, delta);
				else
					nex2 = CHART_INFO(ch1, 1, m_nNextPos, delta);
				break;
			*/

			case RCHART('x'):
				if(m_pattern[m_nNextPos+2] != '{')
				{
					int red = 0;
					unsigned int ch2 = Hex2Int(m_pattern.GetBuffer() + m_nNextPos + 2, HexLimit(m_nNextPos + 2, 2), red);

					delta += red;

					if(red > 0)
						nex2 = CHART_INFO(RCHART(ch2), 0, m_nNextPos, delta);
					else
						nex2 = CHART_INFO(ch1, 0, m_nNextPos, delta);

					break;
				}
				// fall through - "\x{...}" is handled as "\u{...}"

			case RCHART('u'):
				if(m_pattern[m_nNextPos+2] != '{')
				{
					int red = 0;
					unsigned int ch2 = Hex2Int(m_pattern.GetBuffer() + m_nNextPos + 2, HexLimit(m_nNextPos + 2, 4), red);

					delta += red;

					if(red > 0)
						nex2 = CHART_INFO(RCHART(ch2), 0, m_nNextPos, delta);
					else
						nex2 = CHART_INFO(ch1, 0, m_nNextPos, delta);
				}
				else
				{
					int red = 0;
					unsigned int ch2 = Hex2Int(m_pattern.GetBuffer() + m_nNextPos + 3, HexLimit(m_nNextPos + 3, sizeof(int) * 2), red);

					delta += red;

					while(m_nNextPos + delta < m_pattern.GetSize() && m_pattern.At(m_nNextPos + delta) != RCHART('}'))
						delta ++;

					delta ++; // skip '}'

					// UTF-16: a surrogate pair, the low one is the next token
					if(sizeof(CHART) == 2 && ch2 >= 0x10000 && ch2 <= 0x10FFFF)
					{
						m_nPendingLow = 0xDC00 + ((ch2 - 0x10000) & 0x3FF);
						ch2           = 0xD800 + ((ch2 - 0x10000) >> 10);
					}

					nex2 = CHART_INFO(RCHART(ch2), 0, m_nNextPos, delta);
				}
				break;

			case RCHART('h'):
			case RCHART('H'):
				nex2 = CHART_INFO(ch1, 1, m_nNextPos, delta);
				break;

			case RCHART('R'):
			case RCHART('K'):
				// line break and keep, literal in [...]
				nex2 = CHART_INFO(ch1, m_nCharsetDepth > 0 ? 0 : 1, m_nNextPos, delta);
				break;

			case RCHART('p'):
			case RCHART('P'):
				// property \pL or \p{Name}; len covers it, the builder reads the name
				if(m_pattern.At(m_nNextPos + 2, 0) == RCHART('{'))
				{
					int end = m_nNextPos + 3;
					while(end < m_pattern.GetSize() && m_pattern[end] != RCHART('}'))
						end ++;

					if(end < m_pattern.GetSize())
					{
						delta = end + 1 - m_nNextPos;
						nex2  = CHART_INFO(ch1, 1, m_nNextPos, delta);
					}
					else
						nex2  = CHART_INFO(ch1, 0, m_nNextPos, delta); // no '}': literal
				}
				else if(m_nNextPos + 2 < m_pattern.GetSize() &&
					((m_pattern[m_nNextPos + 2] >= RCHART('A') && m_pattern[m_nNextPos + 2] <= RCHART('Z')) ||
					 (m_pattern[m_nNextPos + 2] >= RCHART('a') && m_pattern[m_nNextPos + 2] <= RCHART('z'))))
				{
					delta = 3;
					nex2  = CHART_INFO(ch1, 1, m_nNextPos, delta);
				}
				else
					nex2  = CHART_INFO(ch1, 0, m_nNextPos, delta);
				break;

			case RCHART('a'): nex2 = CHART_INFO(RCHART('\a'), 0, m_nNextPos, delta); break;
			case RCHART('f'): nex2 = CHART_INFO(RCHART('\f'), 0, m_nNextPos, delta); break;
			case RCHART('n'): nex2 = CHART_INFO(RCHART('\n'), 0, m_nNextPos, delta); break;
			case RCHART('r'): nex2 = CHART_INFO(RCHART('\r'), 0, m_nNextPos, delta); break;
			case RCHART('t'): nex2 = CHART_INFO(RCHART('\t'), 0, m_nNextPos, delta); break;
			case RCHART('v'): nex2 = CHART_INFO(RCHART('\v'), 0, m_nNextPos, delta); break;
			case RCHART('e'): nex2 = CHART_INFO(RCHART( 27 ), 0, m_nNextPos, delta); break;

			case RCHART('G'):  // skip '\G'
				if(m_nCharsetDepth > 0)
				{
					m_nNextPos += 2;
					return 0;
				}
				else
				{
					nex2 = CHART_INFO(ch1, 1, m_nNextPos, delta);
					break;
				}

			case RCHART('L'):
				if( ! m_quote_fun ) m_quote_fun = ::tolower;
				// fall through

			case RCHART('U'):
				if( ! m_quote_fun ) m_quote_fun = ::toupper;
				// fall through

			case RCHART('Q'):
				{
					m_bQuoted   = 1;
					m_nNextPos += 2;
					return 0;
				}

			case RCHART('E'):
				{
					m_quote_fun = 0;
					m_bQuoted   = 0;
					m_nNextPos += 2;
					return 0;
				}

			case 0:
				if(m_nNextPos+1 >= m_pattern.GetSize())
				{
					delta = 1;
					nex2 = CHART_INFO(ch , 0, m_nNextPos, delta);
				}
				else
					nex2 = CHART_INFO(ch1, 0, m_nNextPos, delta); // common '\0' char
				break;

			default:
				nex2 = CHART_INFO(ch1, 0, m_nNextPos, delta);
				break;
			}
		}
		break;

	case RCHART('*'):
	case RCHART('+'):
	case RCHART('?'):
	case RCHART('.'):
	case RCHART('{'):
	case RCHART('}'):
	case RCHART(')'):
	case RCHART('|'):
	case RCHART('$'):
		if(m_nCharsetDepth > 0)
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		else
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
		break;

	case RCHART('-'):
		if(m_nCharsetDepth > 0)
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
		else
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		break;

	case RCHART('('):
		{
			CHART ch1 = m_pattern[m_nNextPos+1];
			CHART ch2 = m_pattern[m_nNextPos+2];

			// skip remark
			if(ch1 == RCHART('?') && ch2 == RCHART('#'))
			{
				m_nNextPos += 2;
				while(m_nNextPos < m_pattern.GetSize())
				{
					if(m_pattern[m_nNextPos] == RCHART(')'))
						break;

					m_nNextPos ++;
				}

				// unterminated remark runs to the end of the pattern
				if(m_nNextPos < m_pattern.GetSize())
					m_nNextPos ++; // skip ')'

				// get next nex2
				return 0;
			}
			else
			{
				if(m_nCharsetDepth > 0)
					nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
				else
					nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
			}
		}
		break;

	case RCHART('#'):
		if((m_nFlags & EXTENDED) && m_nCharsetDepth == 0) // literal inside [...], as in Perl
		{
			// skip remark
			m_nNextPos ++;

			while(m_nNextPos < m_pattern.GetSize())
			{
				if(m_pattern[m_nNextPos] == RCHART('\n') || m_pattern[m_nNextPos] == RCHART('\r'))
					break;

				m_nNextPos ++;
			}

			// get next nex2
			return 0;
		}
		else
		{
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		}
		break;

	case RCHART(' '):
	case RCHART('\f'):
	case RCHART('\n'):
	case RCHART('\r'):
	case RCHART('\t'):
	case RCHART('\v'):
		if((m_nFlags & EXTENDED) && m_nCharsetDepth == 0) // literal inside [...], as in Perl
		{
			m_nNextPos ++;

			// get next nex2
			return 0;
		}
		else
		{
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		}
		break;

	case RCHART('['):
		if( m_nCharsetDepth == 0 || m_pattern.At(m_nNextPos + 1, 0) == RCHART(':') )
		{
			m_nCharsetDepth ++;
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
		}
		else
		{
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		}
		break;

	case RCHART(']'):
		if(m_nCharsetDepth > 0)
		{
			m_nCharsetDepth --;
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
		}
		else
		{
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		}
		break;

	case RCHART(':'):
		if(next == CHART_INFO(RCHART('['), 1))
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
		else
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		break;

	case RCHART('^'):
		if(m_nCharsetDepth == 0 || next == CHART_INFO(RCHART('['), 1) || (curr == CHART_INFO(RCHART('['), 1) && next == CHART_INFO(RCHART(':'), 1)))
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta);
		else
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		break;

	case 0:
		if(m_nNextPos >= m_pattern.GetSize())
			nex2 = CHART_INFO(ch, 1, m_nNextPos, delta); // end of string
		else
			nex2 = CHART_INFO(ch, 0, m_nNextPos, delta); // common '\0' char
		break;

	default:
		nex2 = CHART_INFO(ch, 0, m_nNextPos, delta);
		break;
	}

	m_nNextPos += delta;

	return 1;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: GetStockElx(int nStockId)
{
	ElxInterface ** pStockElxs = m_pStockElxs;

	// check
	if(nStockId < 0 || nStockId >= STOCKELX_COUNT)
		return GetStockElx(0);

	// create if no
	if(pStockElxs[nStockId] == 0)
	{
		switch(nStockId)
		{
		case STOCKELX_EMPTY:
			pStockElxs[nStockId] = Keep(new CEmptyElx());
			break;

		case STOCKELX_WORD:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 1));

				pRange->m_ranges.Push(RCHART('A')); pRange->m_ranges.Push(RCHART('Z'));
				pRange->m_ranges.Push(RCHART('a')); pRange->m_ranges.Push(RCHART('z'));
				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));
				pRange->m_chars .Push(RCHART('_'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_WORD_NOT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 0));

				pRange->m_ranges.Push(RCHART('A')); pRange->m_ranges.Push(RCHART('Z'));
				pRange->m_ranges.Push(RCHART('a')); pRange->m_ranges.Push(RCHART('z'));
				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));
				pRange->m_chars .Push(RCHART('_'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_DOT_ALL:
			pStockElxs[nStockId] = Keep(new CRangeElxT <CHART> (0, 0));
			break;

		case STOCKELX_DOT_NOT_ALL:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 0));

				pRange->m_chars .Push(RCHART('\n'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_SPACE:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 1));

				pRange->m_chars .Push(RCHART(' '));
				pRange->m_chars .Push(RCHART('\t'));
				pRange->m_chars .Push(RCHART('\r'));
				pRange->m_chars .Push(RCHART('\n'));
				pRange->m_chars .Push(RCHART('\f'));
				pRange->m_chars .Push(RCHART('\v'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_SPACE_NOT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 0));

				pRange->m_chars .Push(RCHART(' '));
				pRange->m_chars .Push(RCHART('\t'));
				pRange->m_chars .Push(RCHART('\r'));
				pRange->m_chars .Push(RCHART('\n'));
				pRange->m_chars .Push(RCHART('\f'));
				pRange->m_chars .Push(RCHART('\v'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_DIGITAL:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 1));

				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_DIGITAL_NOT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (0, 0));

				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_WORD_RIGHTLEFT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 1));

				pRange->m_ranges.Push(RCHART('A')); pRange->m_ranges.Push(RCHART('Z'));
				pRange->m_ranges.Push(RCHART('a')); pRange->m_ranges.Push(RCHART('z'));
				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));
				pRange->m_chars .Push(RCHART('_'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_WORD_RIGHTLEFT_NOT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 0));

				pRange->m_ranges.Push(RCHART('A')); pRange->m_ranges.Push(RCHART('Z'));
				pRange->m_ranges.Push(RCHART('a')); pRange->m_ranges.Push(RCHART('z'));
				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));
				pRange->m_chars .Push(RCHART('_'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_DOT_ALL_RIGHTLEFT:
			pStockElxs[nStockId] = Keep(new CRangeElxT <CHART> (1, 0));
			break;

		case STOCKELX_DOT_NOT_ALL_RIGHTLEFT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 0));

				pRange->m_chars .Push(RCHART('\n'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_SPACE_RIGHTLEFT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 1));

				pRange->m_chars .Push(RCHART(' '));
				pRange->m_chars .Push(RCHART('\t'));
				pRange->m_chars .Push(RCHART('\r'));
				pRange->m_chars .Push(RCHART('\n'));
				pRange->m_chars .Push(RCHART('\f'));
				pRange->m_chars .Push(RCHART('\v'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_SPACE_RIGHTLEFT_NOT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 0));

				pRange->m_chars .Push(RCHART(' '));
				pRange->m_chars .Push(RCHART('\t'));
				pRange->m_chars .Push(RCHART('\r'));
				pRange->m_chars .Push(RCHART('\n'));
				pRange->m_chars .Push(RCHART('\f'));
				pRange->m_chars .Push(RCHART('\v'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_DIGITAL_RIGHTLEFT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 1));

				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));

				pStockElxs[nStockId] = pRange;
			}
			break;

		case STOCKELX_DIGITAL_RIGHTLEFT_NOT:
			{
				CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (1, 0));

				pRange->m_ranges.Push(RCHART('0')); pRange->m_ranges.Push(RCHART('9'));

				pStockElxs[nStockId] = pRange;
			}
			break;
		}
	}

	// return
	return pStockElxs[nStockId];
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildAlternative(int vaflags)
{
	if(curr == CHART_INFO(0, 1))
		return GetStockElx(STOCKELX_EMPTY);

	// flag instance
	int flags = vaflags;

	// first part
	ElxInterface * pAlternativeOne = BuildList(flags);

	// check alternative
	if(curr == CHART_INFO(RCHART('|'), 1))
	{
		CAlternativeElx * pAlternative = (CAlternativeElx *)Keep(new CAlternativeElx());
		pAlternative->m_elxlist.Push(pAlternativeOne);

		// loop
		while(curr == CHART_INFO(RCHART('|'), 1))
		{
			// skip '|' itself
			MoveNext();

			pAlternativeOne = BuildList(flags);
			pAlternative->m_elxlist.Push(pAlternativeOne);
		}

		return pAlternative;
	}

	return pAlternativeOne;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildList(int & flags)
{
	if(curr == CHART_INFO(0, 1) || curr == CHART_INFO(RCHART('|'), 1) || curr == CHART_INFO(RCHART(')'), 1))
		return GetStockElx(STOCKELX_EMPTY);

	// first
	ElxInterface * pListOne = BuildRepeat(flags);

	if(curr != CHART_INFO(0, 1) && curr != CHART_INFO(RCHART('|'), 1) && curr != CHART_INFO(RCHART(')'), 1))
	{
		CListElx * pList = (CListElx *)Keep(new CListElx(flags & RIGHTTOLEFT));
		pList->m_elxlist.Push(pListOne);

		while(curr != CHART_INFO(0, 1) && curr != CHART_INFO(RCHART('|'), 1) && curr != CHART_INFO(RCHART(')'), 1))
		{
			pListOne = BuildRepeat(flags);

			// add
			pList->m_elxlist.Push(pListOne);
		}

		return pList;
	}

	return pListOne;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildRepeat(int & flags)
{
	// simple
	ElxInterface * pSimple = BuildSimple(flags);

	if(curr.type == 0) return pSimple;

	// is quantifier or not
	int bIsQuantifier = 1;

	// quantifier range
	unsigned int nMin = 0, nMax = 0;

	switch(curr.ch)
	{
	case RCHART('{'):
		{
			// a '{' that does not start {n}, {n,} or {n,m} is a literal char
			Snapshot shot;
			Backup(&shot);

			CBufferT <char> re;
			int bValid = 1;

			// skip '{'
			MoveNext();

			// copy
			while(curr != CHART_INFO(0, 1) && curr != CHART_INFO(RCHART('}'), 1))
			{
				CHART ch = curr.ch;

				if( curr.type != 0 || ! ((ch >= RCHART('0') && ch <= RCHART('9')) || ch == RCHART(',') || ch == RCHART(' ') || ch == RCHART('\t')) )
					bValid = 0;

				re.Append(bValid ? (char)ch : 0, 1);
				MoveNext();
			}

			if(curr != CHART_INFO(RCHART('}'), 1))
				bValid = 0;

			// skip '}'
			MoveNext();

			// read
			int red = -1;
			char * str = re.GetBuffer();

			if( bValid && ReadDec(str, nMin) )
			{
				if( *str == '\0' )
					red = 1;
				else if( *str == ',' )
				{
					str ++;

					if( *str == '\0' )
						red = 2;
					else if( ReadDec(str, nMax) && *str == '\0' )
						red = 3;
				}
			}

			if(red < 0)
			{
				Restore(&shot);
				bIsQuantifier = 0;
				break;
			}

			// check
			if(red  ==  1 ) nMax = nMin;
			if(red  ==  2 ) nMax = INT_MAX;
			if(nMax < nMin) nMax = nMin;
		}
		break;

	case RCHART('?'):
		nMin = 0;
		nMax = 1;

		// skip '?'
		MoveNext();
		break;

	case RCHART('*'):
		nMin = 0;
		nMax = INT_MAX;

		// skip '*'
		MoveNext();
		break;

	case RCHART('+'):
		nMin = 1;
		nMax = INT_MAX;

		// skip '+'
		MoveNext();
		break;

	default:
		bIsQuantifier = 0;
		break;
	}

	// do quantify
	if(bIsQuantifier)
	{
		// 0 times
		if(nMax == 0)
			return GetStockElx(STOCKELX_EMPTY);

		// fixed times
		if(nMin == nMax)
		{
			if(curr == CHART_INFO(RCHART('?'), 1) || curr == CHART_INFO(RCHART('+'), 1))
				MoveNext();

			return Keep(new CRepeatElx(pSimple, nMin));
		}

		// range times
		if(curr == CHART_INFO(RCHART('?'), 1))
		{
			MoveNext();
			return Keep(new CReluctantElx(pSimple, nMin, nMax));
		}
		else if(curr == CHART_INFO(RCHART('+'), 1))
		{
			MoveNext();
			return Keep(new CPossessiveElx(pSimple, nMin, nMax));
		}
		else
		{
			return Keep(new CGreedyElx(pSimple, nMin, nMax));
		}
	}

	return pSimple;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildSimple(int & flags)
{
	CBufferT <CHART> fixed;

	while(curr != CHART_INFO(0, 1))
	{
		if(curr.type == 0)
		{
			// UNICODE_MODE: a quantifier after a surrogate pair applies to both halves
			if(IsUnicode(flags) && deelx_is_high_surrogate(deelx_cp(curr.ch)) && next.type == 0 && deelx_is_low_surrogate(deelx_cp(next.ch)))
			{
				if(nex2 == CHART_INFO(RCHART('{'), 1) || nex2 == CHART_INFO(RCHART('?'), 1) || nex2 == CHART_INFO(RCHART('*'), 1) || nex2 == CHART_INFO(RCHART('+'), 1))
				{
					if(fixed.GetSize() > 0)
						break;
				}

				fixed.Append(curr.ch, 1);
				MoveNext();
				fixed.Append(curr.ch, 1);
				MoveNext();

				if(curr.type == 1 && (curr.ch == RCHART('{') || curr.ch == RCHART('?') || curr.ch == RCHART('*') || curr.ch == RCHART('+')))
					break;

				continue;
			}

			if(next == CHART_INFO(RCHART('{'), 1) || next == CHART_INFO(RCHART('?'), 1) || next == CHART_INFO(RCHART('*'), 1) || next == CHART_INFO(RCHART('+'), 1))
			{
				if(fixed.GetSize() == 0)
				{
					fixed.Append(curr.ch, 1);
					MoveNext();
				}

				break;
			}
			else
			{
				fixed.Append(curr.ch, 1);
				MoveNext();
			}
		}
		else if(curr.type == 1)
		{
			CHART vch = curr.ch;

			// end of simple
			if(vch == RCHART(')') || vch == RCHART('|'))
				break;

			// has fixed already
			if(fixed.GetSize() > 0)
				break;

			// left parentheses
			if(vch == RCHART('('))
			{
				return BuildRecursive(flags);
			}

			// char set
			if( vch == RCHART('[') || vch == RCHART('.') || vch == RCHART('w') || vch == RCHART('W') ||
				vch == RCHART('s') || vch == RCHART('S') || vch == RCHART('d') || vch == RCHART('D') ||
				vch == RCHART('h') || vch == RCHART('H') || vch == RCHART('p') || vch == RCHART('P')
			)
			{
				return BuildCharset(flags);
			}

			// line break, keep
			if( vch == RCHART('R') || vch == RCHART('K') )
			{
				return BuildEscape(flags);
			}

			// boundary
			if( vch == RCHART('^') || vch == RCHART('$') || vch == RCHART('A') || vch == RCHART('Z') || vch == RCHART('z') ||
				vch == RCHART('b') || vch == RCHART('B') || vch == RCHART('G') // vch == RCHART('<') || vch == RCHART('>')
			)
			{
				return BuildBoundary(flags);
			}

			// backref
			if(vch == RCHART('\\') || vch == RCHART('k') || vch == RCHART('g'))
			{
				return BuildBackref(flags);
			}

			// treat vchar as char
			fixed.Append(curr.ch, 1);
			MoveNext();
		}
	}

	if(fixed.GetSize() > 0)
		return Keep(new CStringElxT <CHART> (fixed.GetBuffer(), fixed.GetSize(), flags & RIGHTTOLEFT, flags & IGNORECASE, IsUnicode(flags)));
	else
		return GetStockElx(STOCKELX_EMPTY);
}

// not macros named max/min: those would break std::max, std::min and
// numeric_limits<T>::max() in every file that includes deelx.h
template <class T> inline T deelx_max(T a, T b)
{
	return a > b ? a : b;
}

template <class T> inline T deelx_min(T a, T b)
{
	return a < b ? a : b;
}

//
// New, empty char class in the direction of flags. With UNICODE_MODE a
// surrogate pair is one char.
//
template <class CHART> CRangeElxT <CHART> * CBuilderT <CHART> :: NewRange(int flags, int byes)
{
	CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (flags & RIGHTTOLEFT, byes));
	pRange->m_bpair = IsUnicode(flags);

	return pRange;
}

//
// One char of a [...] class, as a code point. With UNICODE_MODE a surrogate
// pair is one char.
//
template <class CHART> unsigned int CBuilderT <CHART> :: ReadClassChar(int flags)
{
	unsigned int cp = deelx_cp(curr.ch);
	MoveNext();

	if( IsUnicode(flags) && deelx_is_high_surrogate(cp) && curr.type == 0 && deelx_is_low_surrogate(deelx_cp(curr.ch)) )
	{
		cp = 0x10000 + ((cp - 0xD800) << 10) + (deelx_cp(curr.ch) - 0xDC00);
		MoveNext();
	}

	return cp;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildCharset(int & flags)
{
	// char
	CHART ch  = curr.ch;
	int   pos = curr.pos;
	int   len = curr.len;

	// skip
	MoveNext();

	// Unicode \w \d \s and '.'
	if( IsUnicode(flags) )
	{
		CRangeElxT <CHART> * pRange = 0;

		switch(ch)
		{
		case RCHART('.'):
			pRange = NewRange(flags, 0);
			if( ! (flags & SINGLELINE) ) pRange->m_chars.Push(RCHART('\n'));
			return pRange;

		case RCHART('w'):
		case RCHART('W'):
			pRange = NewRange(flags, ch == RCHART('w'));
			SetProperty(pRange, "Word");
			return pRange;

		case RCHART('s'):
		case RCHART('S'):
			pRange = NewRange(flags, ch == RCHART('s'));
			SetProperty(pRange, "White_Space");
			return pRange;

		case RCHART('d'):
		case RCHART('D'):
			pRange = NewRange(flags, ch == RCHART('d'));
			pRange->m_ncategories = 1u << DEELX_UC_Nd;
			return pRange;
		}
	}

	switch(ch)
	{
	case RCHART('.'):
		return GetStockElx(
			flags & RIGHTTOLEFT ?
			((flags & SINGLELINE) ? STOCKELX_DOT_ALL_RIGHTLEFT : STOCKELX_DOT_NOT_ALL_RIGHTLEFT) :
			((flags & SINGLELINE) ? STOCKELX_DOT_ALL : STOCKELX_DOT_NOT_ALL)
		);

	case RCHART('w'):
		return GetStockElx(flags & RIGHTTOLEFT ? STOCKELX_WORD_RIGHTLEFT : STOCKELX_WORD);

	case RCHART('W'):
		return GetStockElx(flags & RIGHTTOLEFT ? STOCKELX_WORD_RIGHTLEFT_NOT : STOCKELX_WORD_NOT);

	case RCHART('s'):
		return GetStockElx(flags & RIGHTTOLEFT ? STOCKELX_SPACE_RIGHTLEFT : STOCKELX_SPACE);

	case RCHART('S'):
		return GetStockElx(flags & RIGHTTOLEFT ? STOCKELX_SPACE_RIGHTLEFT_NOT : STOCKELX_SPACE_NOT);

	case RCHART('d'):
		return GetStockElx(flags & RIGHTTOLEFT ? STOCKELX_DIGITAL_RIGHTLEFT : STOCKELX_DIGITAL);

	case RCHART('D'):
		return GetStockElx(flags & RIGHTTOLEFT ? STOCKELX_DIGITAL_RIGHTLEFT_NOT : STOCKELX_DIGITAL_NOT);

	case RCHART('h'):
	case RCHART('H'):
		{
			// horizontal white space
			CRangeElxT <CHART> * pRange = NewRange(flags, ch == RCHART('h'));

			pRange->m_chars.Push(RCHART(' '));
			pRange->m_chars.Push(RCHART('\t'));

			// not in CRegexpA: there bytes like 0xA0 are parts of multibyte chars
			if(sizeof(CHART) > 1)
			{
				pRange->m_chars .Push(0xA0);
				pRange->m_chars .Push(0x1680);
				pRange->m_ranges.Push(0x2000); pRange->m_ranges.Push(0x200A);
				pRange->m_chars .Push(0x202F);
				pRange->m_chars .Push(0x205F);
				pRange->m_chars .Push(0x3000);
			}

			return pRange;
		}

	case RCHART('p'):
	case RCHART('P'):
		return BuildProperty(pos, len, flags);

	case RCHART('['):
		{
			CRangeElxT <CHART> * pRange;

			// create
			if(curr == CHART_INFO(RCHART(':'), 1))
			{
				// Backup before posix
				Snapshot shot;
				Backup(&shot);

				CBufferT <char> posix;

				do {
					posix.Append(((curr.ch & (CHART)0xff) == curr.ch) ? (char)curr.ch : 0, 1);
					MoveNext();
				}
				while(curr.ch != RCHART(0) && curr != CHART_INFO(RCHART(']'), 1));

				MoveNext(); // skip ']'

				// posix
				CPosixElxT<CHART> * pposix = (CPosixElxT<CHART> *) Keep(new CPosixElxT <CHART> (posix.GetBuffer(), flags & RIGHTTOLEFT, IsUnicode(flags)));
				if(pposix->m_posixfun != 0)
				{
					return pposix;
				}

				// restore if not posix
				Restore(&shot);
			}

			if(curr == CHART_INFO(RCHART('^'), 1))
			{
				MoveNext(); // skip '^'
				pRange = NewRange(flags, 0);
			}
			else
			{
				pRange = NewRange(flags, 1);
			}

			// parse
			while(curr != CHART_INFO(0, 1) && curr != CHART_INFO(RCHART(']'), 1))
			{
				ch = curr.ch;

				if(curr.type == 1 && (
					ch == RCHART('.') || ch == RCHART('w') || ch == RCHART('W') || ch == RCHART('s') || ch == RCHART('S') || ch == RCHART('d') || ch == RCHART('D') ||
					ch == RCHART('h') || ch == RCHART('H') || ch == RCHART('p') || ch == RCHART('P') ||
					(ch == RCHART('[') && next == CHART_INFO(RCHART(':'), 1))
				))
				{
					pRange->m_embeds.Push(BuildCharset(flags));
				}
				else
				{
					unsigned int first = ReadClassChar(flags);

					if(curr == CHART_INFO(RCHART('-'), 1) && next.type == 0)
					{
						MoveNext(); // skip '-'

						unsigned int last = ReadClassChar(flags);

						pRange->m_ranges.Push(first); pRange->m_ranges.Push(last);
					}
					else
					{
						pRange->m_chars.Push(first);
					}
				}
			}

			// skip ']'
			MoveNext();

			if( (flags & IGNORECASE) && IsUnicode(flags) )
			{
				AddFoldedChars(pRange);
			}
			else if( flags & IGNORECASE )
			{
				CBufferT <unsigned int> & ranges = pRange->m_ranges;
				int i, oldcount = ranges.GetSize() / 2;

				for(i=0; i<oldcount; i++)
				{
					unsigned int newmin, newmax;

					if( ranges[i*2] <= 'Z' && ranges[i*2+1] >= 'A' )
					{
						newmin = tolower( (int)deelx_max((unsigned int)'A', ranges[i*2  ]) );
						newmax = tolower( (int)deelx_min((unsigned int)'Z', ranges[i*2+1]) );

						if( newmin < ranges[i*2] || newmax > ranges[i*2+1] )
						{
							ranges.Push(newmin);
							ranges.Push(newmax);
						}
					}

					if( ranges[i*2] <= 'z' && ranges[i*2+1] >= 'a' )
					{
						newmin = toupper( (int)deelx_max((unsigned int)'a', ranges[i*2  ]) );
						newmax = toupper( (int)deelx_min((unsigned int)'z', ranges[i*2+1]) );

						if( newmin < ranges[i*2] || newmax > ranges[i*2+1] )
						{
							ranges.Push(newmin);
							ranges.Push(newmax);
						}
					}
				}

				// bytes 0x80..0xFF of CRegexpA are parts of multibyte chars
				const unsigned int nlimit = sizeof(CHART) == 1 ? 0x80 : 0x100;

				CBufferT <unsigned int> & chars = pRange->m_chars;
				oldcount = chars.GetSize();
				for(i=0; i<oldcount; i++)
				{
					if(chars[i] < nlimit && isupper((int)chars[i]) && ! pRange->IsContainChar(tolower((int)chars[i])) )
						chars.Push(tolower((int)chars[i]));

					if(chars[i] < nlimit &&  islower((int)chars[i]) && ! pRange->IsContainChar(toupper((int)chars[i])) )
						chars.Push(toupper((int)chars[i]));
				}
			}

			return pRange;
		}
	}

	return GetStockElx(STOCKELX_EMPTY);
}

//
// \pL, \p{Name}, \PL, \P{Name}, \p{^Name} (negative) from the token at pos
//
template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildProperty(int pos, int len, int flags)
{
	int bnegative = m_pattern[pos + 1] == RCHART('P');

	int from = len == 3 ? pos + 2 : pos + 3;
	int to   = len == 3 ? pos + 3 : pos + len - 1;

	if(from < to && m_pattern[from] == RCHART('^'))
	{
		bnegative = ! bnegative;
		from ++;
	}

	CBufferT <char> name;

	for(int i=from; i<to; i++)
	{
		unsigned int c = deelx_cp(m_pattern[i]);
		name.Append(c > 0 && c < 0x80 ? (char)c : '?', 1);
	}

	CRangeElxT <CHART> * pRange = NewRange(flags, ! bnegative);

	// an unknown name matches no char (\P: any char)
	if(name.GetSize() > 0)
		SetProperty(pRange, name.GetBuffer());

	return pRange;
}

//
// Add the chars of a property to pRange: a general category ("Lu"), all of
// one major class ("L"), or one of a few properties derived from them.
// Returns 0 for an unknown name.
//
template <class CHART> int CBuilderT <CHART> :: SetProperty(CRangeElxT <CHART> * pRange, const char * name)
{
	static const char cats[] = "LuLlLtLmLoMnMcMeNdNlNoPcPdPsPePiPfPoSmScSkSoZsZlZpCcCfCsCoCn";

	unsigned int mask = 0;
	int i;

	if(strlen(name) == 2)
	{
		for(i=0; i<DEELX_UC_COUNT; i++)
		{
			if(name[0] == cats[i*2] && name[1] == cats[i*2+1])
				mask = 1u << i;
		}
	}
	else if(strlen(name) == 1)
	{
		for(i=0; i<DEELX_UC_COUNT; i++)
		{
			if(name[0] == cats[i*2])
				mask |= 1u << i;
		}
	}

	if(mask != 0)
	{
		pRange->m_ncategories |= mask;
		return 1;
	}

	if(!strcmp(name, "L&") || !strcmp(name, "LC")) // cased letter
	{
		pRange->m_ncategories |= (1u << DEELX_UC_Lu) | (1u << DEELX_UC_Ll) | (1u << DEELX_UC_Lt);
	}
	else if(!strcmp(name, "Any"))
	{
		pRange->m_ncategories |= (1u << DEELX_UC_COUNT) - 1;
	}
	else if(!strcmp(name, "Assigned"))
	{
		pRange->m_ncategories |= ((1u << DEELX_UC_COUNT) - 1) & ~(1u << DEELX_UC_Cn);
	}
	else if(!strcmp(name, "ASCII"))
	{
		pRange->m_ranges.Push(0); pRange->m_ranges.Push(0x7F);
	}
	else if(!strcmp(name, "White_Space") || !strcmp(name, "Space") || !strcmp(name, "Xsp") || !strcmp(name, "Xps"))
	{
		pRange->m_ranges.Push(0x09);   pRange->m_ranges.Push(0x0D);
		pRange->m_ranges.Push(0x2000); pRange->m_ranges.Push(0x200A);
		pRange->m_ranges.Push(0x2028); pRange->m_ranges.Push(0x2029);
		pRange->m_chars .Push(0x20);
		pRange->m_chars .Push(0x85);
		pRange->m_chars .Push(0xA0);
		pRange->m_chars .Push(0x1680);
		pRange->m_chars .Push(0x202F);
		pRange->m_chars .Push(0x205F);
		pRange->m_chars .Push(0x3000);
	}
	else if(!strcmp(name, "Word")) // same as \w with UNICODE_MODE
	{
		pRange->m_ncategories |= DEELX_UC_WORD_MASK;
		pRange->m_chars.Push(0x200C);
		pRange->m_chars.Push(0x200D);
	}
	else if(!strcmp(name, "Xan")) // PCRE: letters and numbers
	{
		pRange->m_ncategories |= 0x1F | (1u << DEELX_UC_Nd) | (1u << DEELX_UC_Nl) | (1u << DEELX_UC_No);
	}
	else if(!strcmp(name, "Xwd")) // PCRE: letters, numbers and '_'
	{
		pRange->m_ncategories |= 0x1F | (1u << DEELX_UC_Nd) | (1u << DEELX_UC_Nl) | (1u << DEELX_UC_No);
		pRange->m_chars.Push(RCHART('_'));
	}
	else
	{
		return 0;
	}

	return 1;
}

//
// UNICODE_MODE with IGNORECASE: add the case folded form of every char in
// the class. Match() then also tries the folded form of the text char.
//
template <class CHART> void CBuilderT <CHART> :: AddFoldedChars(CRangeElxT <CHART> * pRange)
{
	int count = 0;
	const int * ranges = deelx_unicode_fold_ranges(count);

	for(int i=0; i<count; i+=4)
	{
		for(unsigned int cp = (unsigned int)ranges[i]; cp <= (unsigned int)ranges[i+1]; cp += (unsigned int)ranges[i+3])
		{
			unsigned int folded = cp + ranges[i+2];

			if( pRange->IsContainChar(cp) && ! pRange->IsContainChar(folded) )
				pRange->m_chars.Push(folded);
		}
	}

	pRange->m_bfold = 1;
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildRecursive(int & flags)
{
	// skip '('
	MoveNext();

	if(curr == CHART_INFO(RCHART('?'), 1))
	{
		ElxInterface * pElx = 0;

		// skip '?'
		MoveNext();

		int bNegative = 0;
		CHART named_end = RCHART('>');

		switch(curr.ch)
		{
		case RCHART('!'):
			bNegative = 1;
			// fall through

		case RCHART('='):
			{
				MoveNext(); // skip '!' or '='
				pElx = Keep(new CAssertElx(BuildAlternative(flags & ~RIGHTTOLEFT), !bNegative));
			}
			break;

		case RCHART('<'):
			switch(next.ch)
			{
			case RCHART('!'):
				bNegative = 1;
				// fall through

			case RCHART('='):
				MoveNext(); // skip '<'
				MoveNext(); // skip '!' or '='
				{
					pElx = Keep(new CAssertElx(BuildAlternative(flags | RIGHTTOLEFT), !bNegative));
				}
				break;

			default: // named group
				break;
			}
			// break if assertion // else named
			if(pElx != 0) break;
			// fall through

		case RCHART('P'):
			if(curr.ch == RCHART('P')) MoveNext(); // skip 'P'
			// fall through

		case RCHART('\''):
			if     (curr.ch == RCHART('<' )) named_end = RCHART('>' );
			else if(curr.ch == RCHART('\'')) named_end = RCHART('\'');
			MoveNext(); // skip '<' or '\''
			{
				CListElx    * pList  = (CListElx    *)Keep(new CListElx(flags & RIGHTTOLEFT));
				CBracketElx * pleft  = (CBracketElx *)Keep(new CBracketElx(-1, flags & RIGHTTOLEFT ? 1 : 0));
				CBracketElx * pright = (CBracketElx *)Keep(new CBracketElx(-1, flags & RIGHTTOLEFT ? 0 : 1));

				// save name
				CBufferT <CHART> & name = pleft->m_szNamed, & balancing_name = pleft->m_szBalancing, * pname = &name;
				CBufferT <char> num, balancing_num, * pnum = &num;

				while(curr.ch != RCHART(0) && curr.ch != named_end)
				{
					if(curr.ch == RCHART('-'))
					{
						pname = &balancing_name;
						pnum  = &balancing_num;
						MoveNext();
						continue;
					}

					pname->Append(curr.ch, 1);
					pnum ->Append(((curr.ch & (CHART)0xff) == curr.ch) ? (char)curr.ch : 0, 1);
					MoveNext();
				}
				MoveNext(); // skip '>' or '\''

				// check <num>
				unsigned int number = 0;
				char * str = num.GetBuffer();

				if( ReadGroupNumber(str, number) )
				{
					pleft ->m_nnumber = number;
					pright->m_nnumber = number;

					name.Release();
				}

				str = balancing_num.GetBuffer();
				if( ReadGroupNumber(str, number) )
				{
					pleft ->m_balancing = number;
					pright->m_balancing = number;

					balancing_name.Release();
				}

				// left, center, right
				pList->m_elxlist.Push(pleft);
				pList->m_elxlist.Push(BuildAlternative(flags));
				pList->m_elxlist.Push(pright);

				// named number
				if(pleft->m_nnumber >= 0 || name.GetSize() > 0)
				{
					int nThisBackref = m_nNextNamed ++;
					m_namedlist.Prepare(nThisBackref);
					m_namedlist[nThisBackref] = pList;
				}
				else if(pleft->m_balancing >= 0 || balancing_name.GetSize() > 0)
				{
					int nThisBalancing = m_nNextBalancing ++;
					m_purebalancinglist.Prepare(nThisBalancing, 0);
					m_purebalancinglist[nThisBalancing] = pList;
				}
				else
				{
					// TODO ERROR
				}

				pElx = pList;
			}
			break;

		case RCHART('>'):
			{
				MoveNext(); // skip '>'
				pElx = Keep(new CIndependentElx(BuildAlternative(flags)));
			}
			break;

		case RCHART('|'):
			{
				MoveNext(); // skip '|'

				// branch reset: the groups of each alternative are numbered
				// from the same number, the groups after continue from the
				// largest one
				CAlternativeElx * pAlternative = (CAlternativeElx *)Keep(new CAlternativeElx());
				int nBase = m_nMaxNumber, nMax = m_nMaxNumber, altflags = flags;

				for(;;)
				{
					m_nMaxNumber = nBase;
					pAlternative->m_elxlist.Push(BuildList(altflags));

					if(m_nMaxNumber > nMax)
						nMax = m_nMaxNumber;

					if(curr != CHART_INFO(RCHART('|'), 1))
						break;

					MoveNext(); // skip '|'
				}

				m_nMaxNumber = nMax;
				pElx = pAlternative;
			}
			break;

		case RCHART('R'):
			MoveNext(); // skip 'R'
			while(curr.ch != RCHART(0) && deelx_isspace(curr.ch)) MoveNext(); // skip space

			if(curr.ch == RCHART('<') || curr.ch == RCHART('\''))
			{
				named_end = curr.ch == RCHART('<') ? RCHART('>') : RCHART('\'');
				CDelegateElx * pDelegate = (CDelegateElx *)Keep(new CDelegateElx(-3));

				MoveNext(); // skip '<' or '\\'

				// save name
				CBufferT <CHART> & name = pDelegate->m_szNamed;
				CBufferT <char> num;

				while(curr.ch != RCHART(0) && curr.ch != named_end)
				{
					name.Append(curr.ch, 1);
					num .Append(((curr.ch & (CHART)0xff) == curr.ch) ? (char)curr.ch : 0, 1);
					MoveNext();
				}
				MoveNext(); // skip '>' or '\''

				// check <num>
				unsigned int number = 0;
				char * str = num.GetBuffer();

				if( ReadGroupNumber(str, number) )
				{
					pDelegate->m_ndata = number;
					name.Release();
				}

				m_recursivelist.Push(pDelegate);
				pElx = pDelegate;
			}
			else
			{
				CBufferT <char> rto;
				while(curr.ch != RCHART(0) && curr.ch != RCHART(')'))
				{
					rto.Append(((curr.ch & (CHART)0xff) == curr.ch) ? (char)curr.ch : 0, 1);
					MoveNext();
				}

				unsigned int rtono = 0;
				char * str = rto.GetBuffer();
				ReadDec(str, rtono);

				CDelegateElx * pDelegate = (CDelegateElx *)Keep(new CDelegateElx(rtono));

				m_recursivelist.Push(pDelegate);
				pElx = pDelegate;
			}
			break;

		case RCHART('('):
			{
				CConditionElx * pConditionElx = (CConditionElx *)Keep(new CConditionElx());

				// condition
				ElxInterface * & pCondition = pConditionElx->m_pelxask;

				if(next == CHART_INFO(RCHART('?'), 1))
				{
					pCondition = BuildRecursive(flags);
				}
				else // named, assert or number
				{
					MoveNext(); // skip '('
					int pos0 = curr.pos;

					// save elx condition
					pCondition = Keep(new CAssertElx(BuildAlternative(flags), 1));

					// save name
					pConditionElx->m_szNamed.Append(m_pattern.GetBuffer() + pos0, curr.pos - pos0, 1);

					// save number
					CBufferT <char> numstr;
					while(pos0 < curr.pos)
					{
						CHART ch = m_pattern[pos0];
						numstr.Append(((ch & (CHART)0xff) == ch) ? (char)ch : 0, 1);
						pos0 ++;
					}

					unsigned int number = 0;
					char * str = numstr.GetBuffer();

					// valid group number
					if( ReadGroupNumber(str, number) )
					{
						pConditionElx->m_nnumber = number;
						pCondition = 0;
					}
					else // maybe elx, maybe named
					{
						pConditionElx->m_nnumber = -1;
						m_namedconditionlist.Push(pConditionElx);
					}

					MoveNext(); // skip ')'
				}

				// alternative
				{
					int newflags = flags;

					pConditionElx->m_pelxyes = BuildList(newflags);

					// (?x) in the yes branch does not reach the no branch
					SetExtended(flags);
				}

				if(curr.ch == RCHART('|'))
				{
					MoveNext(); // skip '|'

					pConditionElx->m_pelxno = BuildAlternative(flags);
				}
				else
				{
					pConditionElx->m_pelxno = 0;
				}

				pElx = pConditionElx;
			}
			break;

		default:
			while(curr.ch != RCHART(0) && deelx_isspace(curr.ch)) MoveNext(); // skip space

			if(curr.ch >= RCHART('0') && curr.ch <= RCHART('9')) // recursive (?1) => (?R1)
			{
				CBufferT <char> rto;
				while(curr.ch != RCHART(0) && curr.ch != RCHART(')'))
				{
					rto.Append(((curr.ch & (CHART)0xff) == curr.ch) ? (char)curr.ch : 0, 1);
					MoveNext();
				}

				unsigned int rtono = 0;
				char * str = rto.GetBuffer();
				ReadDec(str, rtono);

				CDelegateElx * pDelegate = (CDelegateElx *)Keep(new CDelegateElx(rtono));

				m_recursivelist.Push(pDelegate);
				pElx = pDelegate;
			}
			else
			{
				// flag
				int newflags = flags;
				while(curr != CHART_INFO(0, 1) && curr.ch != RCHART(':') && curr.ch != RCHART(')') && curr != CHART_INFO(RCHART('('), 1))
				{
					int tochange = 0;

					switch(curr.ch)
					{
					case RCHART('i'):
					case RCHART('I'):
						tochange = IGNORECASE;
						break;

					case RCHART('s'):
					case RCHART('S'):
						tochange = SINGLELINE;
						break;

					case RCHART('m'):
					case RCHART('M'):
						tochange = MULTILINE;
						break;

					case RCHART('g'):
					case RCHART('G'):
						tochange = GLOBAL;
						break;

					case RCHART('x'):
					case RCHART('X'):
						tochange = EXTENDED;
						break;

					case RCHART('u'):
					case RCHART('U'):
						tochange = UNICODE_MODE;
						break;

					case RCHART('-'):
						bNegative = 1;
						break;
					}

					if(bNegative)
						newflags &= ~tochange;
					else
						newflags |=  tochange;

					// move to next char
					MoveNext();
				}

				if(curr.ch == RCHART(':') || curr == CHART_INFO(RCHART('('), 1))
				{
					// read the group with its EXTENDED
					SetExtended(newflags);

					// skip ':'
					if(curr.ch == RCHART(':')) MoveNext();

					pElx = BuildAlternative(newflags);
				}
				else
				{
					// change parent flags
					flags = newflags;

					pElx = GetStockElx(STOCKELX_EMPTY);
				}
			}
			break;
		}

		// back to the EXTENDED after the group, or on to the one set by (?x)
		SetExtended(flags);

		MoveNext(); // skip ')'

		return pElx;
	}
	else
	{
		// group and number
		CListElx * pList = (CListElx *)Keep(new CListElx(flags & RIGHTTOLEFT));
		int nThisBackref = ++ m_nMaxNumber;

		// left, center, right
		pList->m_elxlist.Push(Keep(new CBracketElx(nThisBackref, flags & RIGHTTOLEFT ? 1 : 0)));
		pList->m_elxlist.Push(BuildAlternative(flags));
		pList->m_elxlist.Push(Keep(new CBracketElx(nThisBackref, flags & RIGHTTOLEFT ? 0 : 1)));

		// for recursive; in (?|...) the first group with this number
		m_grouplist.Prepare(nThisBackref);
		if(m_grouplist[nThisBackref] == 0)
			m_grouplist[nThisBackref] = pList;

		// back to the EXTENDED after the group
		SetExtended(flags);

		// right
		MoveNext(); // skip ')'

		return pList;
	}
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildBoundary(int & flags)
{
	// char
	CHART ch = curr.ch;

	// skip
	MoveNext();

	switch(ch)
	{
	case RCHART('^'):
		return Keep(new CBoundaryElxT <CHART> ((flags & MULTILINE) ? BOUNDARY_LINE_BEGIN : BOUNDARY_FILE_BEGIN));

	case RCHART('$'):
		return Keep(new CBoundaryElxT <CHART> ((flags & MULTILINE) ? BOUNDARY_LINE_END : BOUNDARY_FILE_END));

	case RCHART('b'):
		return Keep(new CBoundaryElxT <CHART> (BOUNDARY_WORD_EDGE, 1, IsUnicode(flags)));

	case RCHART('B'):
		return Keep(new CBoundaryElxT <CHART> (BOUNDARY_WORD_EDGE, 0, IsUnicode(flags)));

	case RCHART('A'):
		return Keep(new CBoundaryElxT <CHART> (BOUNDARY_FILE_BEGIN));

	case RCHART('Z'):
		return Keep(new CBoundaryElxT <CHART> (BOUNDARY_FILE_END_N));

	case RCHART('z'):
		return Keep(new CBoundaryElxT <CHART> (BOUNDARY_FILE_END));

	case RCHART('G'):
		if(flags & GLOBAL)
			return Keep(new CGlobalElx());
		else
			return GetStockElx(STOCKELX_EMPTY);

	default:
		return GetStockElx(STOCKELX_EMPTY);
	}
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildBackref(int & flags)
{
	// skip '\\' or '\k' or '\g'
	MoveNext();

	if(curr.ch == RCHART('<') || curr.ch == RCHART('\''))
	{
		CHART named_end = curr.ch == RCHART('<') ? RCHART('>') : RCHART('\'');
		CBackrefElxT <CHART> * pbackref = (CBackrefElxT <CHART> *)Keep(new CBackrefElxT <CHART> (-1, flags & RIGHTTOLEFT, flags & IGNORECASE, IsUnicode(flags)));

		MoveNext(); // skip '<' or '\''

		// save name
		CBufferT <CHART> & name = pbackref->m_szNamed;
		CBufferT <char> num;

		while(curr.ch != RCHART(0) && curr.ch != named_end)
		{
			name.Append(curr.ch, 1);
			num .Append(((curr.ch & (CHART)0xff) == curr.ch) ? (char)curr.ch : 0, 1);
			MoveNext();
		}
		MoveNext(); // skip '>' or '\''

		// check <num>
		unsigned int number = 0;
		char * str = num.GetBuffer();

		if( ReadGroupNumber(str, number) )
		{
			pbackref->m_nnumber = number;
			name.Release();
		}
		else
		{
			m_namedbackreflist.Push(pbackref);
		}

		return pbackref;
	}
	else
	{
		unsigned int nbackref = 0;

		for(int i=0; i<3; i++)
		{
			if(curr.ch >= RCHART('0') && curr.ch <= RCHART('9'))
				nbackref = nbackref * 10 + (curr.ch - RCHART('0'));
			else
				break;

			MoveNext();
		}

		return Keep(new CBackrefElxT <CHART> (nbackref, flags & RIGHTTOLEFT, flags & IGNORECASE, IsUnicode(flags)));
	}
}

template <class CHART> ElxInterface * CBuilderT <CHART> :: BuildEscape(int & flags)
{
	// char
	CHART ch = curr.ch;

	// skip
	MoveNext();

	if(ch == RCHART('K'))
		return Keep(new CKeepElx());

	// \R: (?>\r\n|[\n\v\f\r\x85\x{2028}\x{2029}])
	CAlternativeElx * pAlternative = (CAlternativeElx *)Keep(new CAlternativeElx());

	CHART crlf[2] = { RCHART('\r'), RCHART('\n') };
	pAlternative->m_elxlist.Push(Keep(new CStringElxT <CHART> (crlf, 2, flags & RIGHTTOLEFT, 0)));

	CRangeElxT <CHART> * pRange = (CRangeElxT <CHART> *)Keep(new CRangeElxT <CHART> (flags & RIGHTTOLEFT, 1));
	pRange->m_ranges.Push(0x0A); pRange->m_ranges.Push(0x0D);

	// not in CRegexpA: there 0x85 is a part of a multibyte char
	if(sizeof(CHART) > 1)
	{
		pRange->m_chars.Push(0x85);
		pRange->m_chars.Push(0x2028);
		pRange->m_chars.Push(0x2029);
	}

	pAlternative->m_elxlist.Push(pRange);

	return Keep(new CIndependentElx(pAlternative));
}

template <class CHART> int CBuilderT <CHART> :: ReadDec(char * & str, unsigned int & dec)
{
	int s = 0;
	while(str[s] != 0 && isspace((unsigned char)str[s])) s++;

	if(str[s] < '0' || str[s] > '9') return 0;

	dec = 0;
	unsigned int i = 0;

	// read all digits, saturating at INT_MAX (callers store the value in int)
	for(i = s; str[i] >= '0' && str[i] <= '9'; i++)
	{
		unsigned int digit = str[i] - '0';

		if(dec > (INT_MAX - digit) / 10)
			dec = INT_MAX;
		else
			dec = dec * 10 + digit;
	}

	while(str[i] != 0 && isspace((unsigned char)str[i])) i++;
	str += i;

	return 1;
}

//
// Regexp
//
template <class CHART> class CRegexpT
{
public:
	CRegexpT(const CHART * pattern = 0, int flags = 0);
	CRegexpT(const CHART * pattern, int length, int flags);
	void Compile(const CHART * pattern, int flags = 0);
	void Compile(const CHART * pattern, int length, int flags);

public:
	MatchResult MatchExact(const CHART * tstring, CContext * pContext = 0) const;
	MatchResult MatchExact(const CHART * tstring, int length, CContext * pContext = 0) const;
	MatchResult Match(const CHART * tstring, int start = -1, CContext * pContext = 0) const;
	MatchResult Match(const CHART * tstring, int length, int start, CContext * pContext = 0) const;
	MatchResult Match(CContext * pContext) const;
	CContext * PrepareMatch(const CHART * tstring, int start = -1, CContext * pContext = 0) const;
	CContext * PrepareMatch(const CHART * tstring, int length, int start, CContext * pContext = 0) const;
	CHART * Replace(const CHART * tstring, const CHART * replaceto, int start = -1, int ntimes = -1, MatchResult * result = 0, CContext * pContext = 0) const;
	CHART * Replace(const CHART * tstring, int string_length, const CHART * replaceto, int to_length, int & result_length, int start = -1, int ntimes = -1, MatchResult * result = 0, CContext * pContext = 0) const;
	int GetNamedGroupNumber(const CHART * group_name) const;
	const CHART * GetNamedGroupName(int group_number) const;

	// Limit the backtracking steps of one Match() or MatchExact() call. Beyond
	// it the call gives up: the result is "not matched" and
	// MatchResult::IsStepLimitExceeded() is true. Replace() applies the limit
	// to each search and stops replacing when one gives up.
	//   nLimit > 0 : at most nLimit steps
	//   nLimit = 0 : no limit (STEP_LIMIT_NONE)
	//   nLimit < 0 : automatic, the default (STEP_LIMIT_AUTO): the limit grows
	//                with the length of the text, see GetEffectiveStepLimit()
	void SetStepLimit(int nLimit);
	int  GetStepLimit() const;                         // 0, a positive value or STEP_LIMIT_AUTO
	int  GetEffectiveStepLimit(int length) const;      // the limit for a text of this length, 0: none

	enum
	{
		STEP_LIMIT_NONE = 0,
		STEP_LIMIT_AUTO = -1,

		// automatic limit = BASE + PER_CHAR * length. Ordinary patterns need
		// about 1 to 5 steps per character, so only runaway backtracking
		// (exponential, or quadratic on long texts) is stopped.
		STEP_LIMIT_AUTO_BASE     = 1000000,
		STEP_LIMIT_AUTO_PER_CHAR = 256
	};

public:
	static void ReleaseString (CHART    * tstring );
	static void ReleaseContext(CContext * pContext);

protected:
	static int FindReplaceToken(const CHART * s, int length, int from, int & tstart, int & tend);
	static MatchResult StepLimitExceeded(CContext * pContext, int endpos);

public:
	CBuilderT <CHART> m_builder;

protected:
	int m_nStepLimit;
};

//
// Implementation
//
template <class CHART> CRegexpT <CHART> :: CRegexpT(const CHART * pattern, int flags)
{
	m_nStepLimit = STEP_LIMIT_AUTO;
	Compile(pattern, CBufferRefT<CHART>(pattern).GetSize(), flags);
}

template <class CHART> CRegexpT <CHART> :: CRegexpT(const CHART * pattern, int length, int flags)
{
	m_nStepLimit = STEP_LIMIT_AUTO;
	Compile(pattern, length, flags);
}

template <class CHART> inline void CRegexpT <CHART> :: Compile(const CHART * pattern, int flags)
{
	Compile(pattern, CBufferRefT<CHART>(pattern).GetSize(), flags);
}

template <class CHART> void CRegexpT <CHART> :: Compile(const CHART * pattern, int length, int flags)
{
	m_builder.Clear();

	try
	{
		if(pattern != 0) m_builder.Build(CBufferRefT<CHART>(pattern, length), flags);
	}
	catch(...)
	{
		m_builder.Clear(); // leave an empty (non-matching) regexp, not a half-built one
		throw;
	}
}

template <class CHART> inline void CRegexpT <CHART> :: SetStepLimit(int nLimit)
{
	m_nStepLimit = nLimit > 0 ? nLimit : (nLimit == 0 ? (int)STEP_LIMIT_NONE : (int)STEP_LIMIT_AUTO);
}

template <class CHART> inline int CRegexpT <CHART> :: GetStepLimit() const
{
	return m_nStepLimit;
}

template <class CHART> int CRegexpT <CHART> :: GetEffectiveStepLimit(int length) const
{
	if(m_nStepLimit >= 0)
		return m_nStepLimit;

	if(length < 0) length = 0;
	if(length > (INT_MAX - STEP_LIMIT_AUTO_BASE) / STEP_LIMIT_AUTO_PER_CHAR)
		return INT_MAX;

	return STEP_LIMIT_AUTO_BASE + STEP_LIMIT_AUTO_PER_CHAR * length;
}

//
// The match was given up: clear the context, so that a later Match() with it
// finds nothing more, and report it.
//
template <class CHART> MatchResult CRegexpT <CHART> :: StepLimitExceeded(CContext * pContext, int endpos)
{
	pContext->m_stack       .Restore(0);
	pContext->m_capturestack.Restore(0);
	pContext->m_captureindex.Restore(0);
	pContext->m_nCurrentPos = endpos;

	MatchResult result;
	result.m_bStepLimitExceeded = 1;

	return result;
}

template <class CHART> inline MatchResult CRegexpT <CHART> :: MatchExact(const CHART * tstring, CContext * pContext) const
{
	return MatchExact(tstring, CBufferRefT<CHART>(tstring).GetSize(), pContext);
}

template <class CHART> MatchResult CRegexpT <CHART> :: MatchExact(const CHART * tstring, int length, CContext * pContext) const
{
	if(m_builder.m_pTopElx == 0)
		return 0;

	// info
	int endpos = 0;

	CContext context;
	if(pContext == 0) pContext = &context;

	pContext->m_stack.Restore(0);
	pContext->m_capturestack.Restore(0);
	pContext->m_captureindex.Restore(0);

	pContext->m_nParenZindex  = 0;
	pContext->m_nLastBeginPos = -1;
	pContext->m_pMatchString  = (void*)tstring;
	pContext->m_pMatchStringLength = length;
	pContext->m_nCursiveLimit = 100;
	pContext->m_nStepLimit    = GetEffectiveStepLimit(length);
	pContext->m_nSteps        = 0;

	if(m_builder.m_nFlags & RIGHTTOLEFT)
	{
		pContext->m_nBeginPos   = length;
		pContext->m_nCurrentPos = length;
		endpos = 0;
	}
	else
	{
		pContext->m_nBeginPos   = 0;
		pContext->m_nCurrentPos = 0;
		endpos = length;
	}

	pContext->m_captureindex.Prepare(m_builder.m_nMaxNumber, -1);
	pContext->m_captureindex[0] = 0;
	pContext->m_capturestack.Push(0);
	pContext->m_capturestack.Push(pContext->m_nCurrentPos);
	pContext->m_capturestack.Push(-1);
	pContext->m_capturestack.Push(-1);

	try
	{
		// match
		if( ! m_builder.m_pTopElx->Match( pContext ) )
			return 0;

		// backtrack until the match spans the whole string. (A former guard gave
		// up after two consecutive empty results, missing later alternatives.)
		while( pContext->m_nCurrentPos != endpos )
		{
			if( ! m_builder.m_pTopElx->MatchNext( pContext ) )
				return 0;
		}
	}
	catch(deelx_step_limit_exceeded &)
	{
		return StepLimitExceeded(pContext, endpos);
	}

	// end pos
	pContext->m_capturestack[2] = pContext->m_nCurrentPos;

	return MatchResult( pContext, m_builder.m_nMaxNumber );
}

template <class CHART> MatchResult CRegexpT <CHART> :: Match(const CHART * tstring, int start, CContext * pContext) const
{
	return Match(tstring, CBufferRefT<CHART>(tstring).GetSize(), start, pContext);
}

template <class CHART> MatchResult CRegexpT <CHART> :: Match(const CHART * tstring, int length, int start, CContext * pContext) const
{
	if(m_builder.m_pTopElx == 0)
		return 0;

	CContext context;
	if(pContext == 0) pContext = &context;

	PrepareMatch(tstring, length, start, pContext);

	return Match( pContext );
}

template <class CHART> MatchResult CRegexpT <CHART> :: Match(CContext * pContext) const
{
	if(m_builder.m_pTopElx == 0)
		return 0;

	int endpos, delta;

	if(m_builder.m_nFlags & RIGHTTOLEFT)
	{
		endpos = -1;
		delta  = -1;
	}
	else
	{
		endpos = pContext->m_pMatchStringLength + 1;
		delta  = 1;
	}

	const CHART * pcsz = (const CHART *)pContext->m_pMatchString;
	int bpair = (m_builder.m_nFlags & UNICODE_MODE) && sizeof(CHART) == 2;

	pContext->m_nSteps = 0;

	try
	{
		while(pContext->m_nCurrentPos != endpos)
		{
			int npos = pContext->m_nCurrentPos;

			// UNICODE_MODE: do not start between the halves of a surrogate pair
			if( bpair && npos > 0 && npos < pContext->m_pMatchStringLength &&
				deelx_is_high_surrogate(deelx_cp(pcsz[npos - 1])) && deelx_is_low_surrogate(deelx_cp(pcsz[npos])) )
			{
				pContext->m_nCurrentPos += delta;
				continue;
			}

			pContext->m_captureindex.Restore(0);
			pContext->m_stack       .Restore(0);
			pContext->m_capturestack.Restore(0);

			pContext->m_captureindex.Prepare(m_builder.m_nMaxNumber, -1);
			pContext->m_captureindex[0] = 0;
			pContext->m_capturestack.Push(0);
			pContext->m_capturestack.Push(pContext->m_nCurrentPos);
			pContext->m_capturestack.Push(-1);
			pContext->m_capturestack.Push(-1);

			if( m_builder.m_pTopElx->Match( pContext ) )
			{
				pContext->m_capturestack[2] = pContext->m_nCurrentPos;

				// zero width: nothing consumed from where the search started
				// (with \K the match itself may start later)
				if( npos == pContext->m_nCurrentPos )
				{
					pContext->m_nCurrentPos += delta;
				}

				// save pos
				pContext->m_nLastBeginPos   = pContext->m_nBeginPos;
				pContext->m_nBeginPos       = pContext->m_nCurrentPos;

				// return
				return MatchResult( pContext, m_builder.m_nMaxNumber );
			}
			else
			{
				pContext->m_nCurrentPos += delta;
			}
		}
	}
	catch(deelx_step_limit_exceeded &)
	{
		return StepLimitExceeded(pContext, endpos);
	}

	return 0;
}

template <class CHART> inline CContext * CRegexpT <CHART> :: PrepareMatch(const CHART * tstring, int start, CContext * pContext) const
{
	return PrepareMatch(tstring, CBufferRefT<CHART>(tstring).GetSize(), start, pContext);
}

template <class CHART> CContext * CRegexpT <CHART> :: PrepareMatch(const CHART * tstring, int length, int start, CContext * pContext) const
{
	if(m_builder.m_pTopElx == 0)
		return 0;

	if(pContext == 0) pContext = deelx_check_new(new CContext());

	pContext->m_nParenZindex  =  0;
	pContext->m_nLastBeginPos = -1;
	pContext->m_pMatchString  = (void*)tstring;
	pContext->m_pMatchStringLength = length;
	pContext->m_nCursiveLimit = 100;
	pContext->m_nStepLimit    = GetEffectiveStepLimit(length);

	if(start < 0)
	{
		if(m_builder.m_nFlags & RIGHTTOLEFT)
		{
			pContext->m_nBeginPos   = length;
			pContext->m_nCurrentPos = length;
		}
		else
		{
			pContext->m_nBeginPos   = 0;
			pContext->m_nCurrentPos = 0;
		}
	}
	else
	{
		if(start > length) start = length + ((m_builder.m_nFlags & RIGHTTOLEFT)?0:1);

		pContext->m_nBeginPos   = start;
		pContext->m_nCurrentPos = start;
	}

	return pContext;
}

template <class CHART> inline int CRegexpT <CHART> :: GetNamedGroupNumber(const CHART * group_name) const
{
	return m_builder.GetNamedNumber(group_name);
}

template <class CHART> inline const CHART * CRegexpT <CHART> :: GetNamedGroupName(int group_number) const
{
	return m_builder.GetNamedName(group_number);
}

template <class CHART> CHART * CRegexpT <CHART> :: Replace(const CHART * tstring, const CHART * replaceto, int start, int ntimes, MatchResult * result, CContext * pContext) const
{
	int result_length = 0;
	return Replace(tstring, CBufferRefT<CHART>(tstring).GetSize(), replaceto, CBufferRefT<CHART>(replaceto).GetSize(), result_length, start, ntimes, result, pContext);
}

template <class CHART> CHART * CRegexpT <CHART> :: Replace(const CHART * tstring, int string_length, const CHART * replaceto, int to_length, int & result_length, int start, int ntimes, MatchResult * remote_result, CContext * oContext) const
{
	if(m_builder.m_pTopElx == 0) return 0;

	// --- compile replace to ---

	CBufferT <int> compiledto;

	MatchResult local_result(0), * result = remote_result ? remote_result : & local_result;

	int lastIndex = 0, nmatch = 0, tstart = 0, tend = 0;

	while( FindReplaceToken(replaceto, to_length, lastIndex, tstart, tend) )
	{
		int delta = tstart - lastIndex;
		if( delta > 0 )
		{
			compiledto.Push(lastIndex);
			compiledto.Push(delta);
		}

		lastIndex = tstart;
		delta     = 2;

		switch(replaceto[lastIndex + 1])
		{
		case RCHART('$'):
			compiledto.Push(lastIndex);
			compiledto.Push(1);
			break;

		case RCHART('&'):
		case RCHART('`'):
		case RCHART('\''):
		case RCHART('+'):
		case RCHART('_'):
			compiledto.Push(-1);
			compiledto.Push((int)replaceto[lastIndex + 1]);
			break;

		case RCHART('{'):
			delta  = tend - tstart;
			nmatch = m_builder.GetNamedNumber(CBufferRefT <CHART> (replaceto + (lastIndex + 2), delta - 3));

			if(nmatch > 0 && nmatch <= m_builder.m_nMaxNumber)
			{
				compiledto.Push(-2);
				compiledto.Push(nmatch);
			}
			else
			{
				compiledto.Push(lastIndex);
				compiledto.Push(delta);
			}
			break;

		default:
			nmatch = 0;
			for(delta=1; delta<=3 && lastIndex + delta < to_length; delta++)
			{
				CHART ch = replaceto[lastIndex + delta];

				if(ch < RCHART('0') || ch > RCHART('9'))
					break;

				nmatch = nmatch * 10 + (ch - RCHART('0'));
			}

			if(nmatch > m_builder.m_nMaxNumber)
			{
				while(nmatch > m_builder.m_nMaxNumber)
				{
					nmatch /= 10;
					delta --;
				}

				if(nmatch == 0)
				{
					delta = 1;
				}
			}

			if(delta == 1)
			{
				compiledto.Push(lastIndex);
				compiledto.Push(1);
			}
			else
			{
				compiledto.Push(-2);
				compiledto.Push(nmatch);
			}
			break;
		}

		lastIndex += delta;
	}

	if(lastIndex < to_length)
	{
		compiledto.Push(lastIndex);
		compiledto.Push(to_length - lastIndex);
	}

	int rightleft = m_builder.m_nFlags & RIGHTTOLEFT;

	int tb = rightleft ? compiledto.GetSize() - 2 : 0;
	int te = rightleft ? -2 : compiledto.GetSize();
	int ts = rightleft ? -2 : 2;

	// --- compile complete ---

	int beginpos  = rightleft ? string_length : 0;
	int endpos    = rightleft ? 0 : string_length;

	int toIndex0  = 0;
	int toIndex1  = 0;
	int i = 0, ntime = 0, bStepLimitExceeded = 0;

	CBufferT <const CHART *> buffer;

	// prepare (a local context needs no release, even if an exception is thrown)
	CContext context;
	CContext * pContext = PrepareMatch(tstring, string_length, start, oContext ? oContext : &context);
	lastIndex = beginpos;

	// Match
	for(ntime = 0; ntimes < 0 || ntime < ntimes; ntime ++)
	{
		(*result) = Match(pContext);

		// the step limit counts per Match(): the replacements done so far are kept
		bStepLimitExceeded = result->IsStepLimitExceeded();

		if( ! result->IsMatched() )
			break;

		// before
		if( rightleft )
		{
			int distance = lastIndex - result->GetEnd();
			if( distance )
			{
				buffer.Push(tstring + result->GetEnd());
				buffer.Push((const CHART *)(ptrdiff_t)distance);

				toIndex1 -= distance;
			}
			lastIndex = result->GetStart();
		}
		else
		{
			int distance = result->GetStart() - lastIndex;
			if( distance )
			{
				buffer.Push(tstring + lastIndex);
				buffer.Push((const CHART *)(ptrdiff_t)distance);

				toIndex1 += distance;
			}
			lastIndex = result->GetEnd();
		}

		toIndex0 = toIndex1;

		// middle
		for(i=tb; i!=te; i+=ts)
		{
			int off = compiledto[i];
			int len = compiledto[i + 1];

			const CHART * sub = replaceto + off;

			if( off == -1 )
			{
				switch(RCHART(len))
				{
				case RCHART('&'):
					sub = tstring + result->GetStart();
					len = result->GetEnd() - result->GetStart();
					break;

				case RCHART('`'):
					sub = tstring;
					len = result->GetStart();
					break;

				case RCHART('\''):
					sub = tstring + result->GetEnd();
					len = string_length - result->GetEnd();
					break;

				case RCHART('+'):
					for(nmatch = result->MaxGroupNumber(); nmatch >= 0; nmatch --)
					{
						if(result->GetGroupStart(nmatch) >= 0) break;
					}
					sub = tstring + result->GetGroupStart(nmatch);
					len = result->GetGroupEnd(nmatch) - result->GetGroupStart(nmatch);
					break;

				case RCHART('_'):
					sub = tstring;
					len = string_length;
					break;
				}
			}
			else if( off == -2 )
			{
				int gstart = result->GetGroupStart(len);
				int gend   = result->GetGroupEnd  (len);

				// unmatched group: empty, without forming tstring - 1
				sub = gstart >= 0 ? tstring + gstart : tstring;
				len = gstart >= 0 ? gend - gstart : 0;
			}

			buffer.Push(sub);
			buffer.Push((const CHART *)(ptrdiff_t)len);

			toIndex1 += rightleft ? (-len) : len;
		}
	}

	// after
	if(rightleft)
	{
		if(endpos < lastIndex)
		{
			buffer.Push(tstring + endpos);
			buffer.Push((const CHART *)(ptrdiff_t)(lastIndex - endpos));
		}
	}
	else
	{
		if(lastIndex < endpos)
		{
			buffer.Push(tstring + lastIndex);
			buffer.Push((const CHART *)(ptrdiff_t)(endpos - lastIndex));
		}
	}

	// join string
	result_length = 0;
	for(i=0; i<buffer.GetSize(); i+=2)
	{
		result_length += (int)(ptrdiff_t)buffer[i+1];
	}

	CBufferT <CHART> result_string;
	result_string.Prepare(result_length);
	result_string.Restore(0);

	if(rightleft)
	{
		for(i=buffer.GetSize()-2; i>=0; i-=2)
		{
			result_string.Append(buffer[i], (int)(ptrdiff_t)buffer[i+1]);
		}
	}
	else
	{
		for(i=0; i<buffer.GetSize(); i+=2)
		{
			result_string.Append(buffer[i], (int)(ptrdiff_t)buffer[i+1]);
		}
	}

	result_string.Append(0);

	// *result may still hold the last match when ntimes stopped the loop
	result->m_result.Restore(0);
	result->m_result.Append(result_length, 3);
	result->m_result.Append(ntime);
	result->m_bStepLimitExceeded = bStepLimitExceeded;

	if(rightleft)
	{
		result->m_result.Append(result_length - toIndex1);
		result->m_result.Append(result_length - toIndex0);
	}
	else
	{
		result->m_result.Append(toIndex0);
		result->m_result.Append(toIndex1);
	}

	return result_string.Detach();
}

//
// Find next "$x" or "${name}" token in a replacement string, where x is one of
// $ & ` ' + _ or a digit, and name contains no newline.
// (Formerly matched with a static CRegexpT, which was not thread-safe.)
//
template <class CHART> int CRegexpT <CHART> :: FindReplaceToken(const CHART * s, int length, int from, int & tstart, int & tend)
{
	for(int i = from; i + 1 < length; i++)
	{
		if(s[i] != RCHART('$'))
			continue;

		CHART ch = s[i + 1];

		if( ch == RCHART('$') || ch == RCHART('&') || ch == RCHART('`') || ch == RCHART('\'') || ch == RCHART('+') || ch == RCHART('_') ||
			(ch >= RCHART('0') && ch <= RCHART('9')) )
		{
			tstart = i;
			tend   = i + 2;
			return 1;
		}

		if( ch == RCHART('{') )
		{
			for(int j = i + 2; j < length && s[j] != RCHART('\n'); j++)
			{
				if(s[j] == RCHART('}'))
				{
					tstart = i;
					tend   = j + 1;
					return 1;
				}
			}
		}
	}

	return 0;
}

template <class CHART> inline void CRegexpT <CHART> :: ReleaseString(CHART * tstring)
{
	if(tstring != 0) free(tstring);
}

template <class CHART> inline void CRegexpT <CHART> :: ReleaseContext(CContext * pContext)
{
	if(pContext != 0) delete pContext;
}

//
// All implementations
//
template <int x> CAlternativeElxT <x> :: CAlternativeElxT()
{
}

template <int x> int CAlternativeElxT <x> :: Match(CContext * pContext) const
{
	pContext->Step();

	if(m_elxlist.GetSize() == 0)
		return 1;

	// try all
	for(int n = 0; n < m_elxlist.GetSize(); n++)
	{
		if(m_elxlist[n]->Match(pContext))
		{
			pContext->m_stack.Push(n);
			return 1;
		}
	}

	return 0;
}

template <int x> int CAlternativeElxT <x> :: MatchNext(CContext * pContext) const
{
	pContext->Step();

	if(m_elxlist.GetSize() == 0)
		return 0;

	int n = 0;

	// recall prev
	pContext->m_stack.Pop(n);

	// prev
	if(m_elxlist[n]->MatchNext(pContext))
	{
		pContext->m_stack.Push(n);
		return 1;
	}
	else
	{
		// try rest
		for(n++; n < m_elxlist.GetSize(); n++)
		{
			if(m_elxlist[n]->Match(pContext))
			{
				pContext->m_stack.Push(n);
				return 1;
			}
		}

		return 0;
	}
}

// assertx.cpp: implementation of the CAssertElx class.
//
template <int x> CAssertElxT <x> :: CAssertElxT(ElxInterface * pelx, int byes)
{
	m_pelx = pelx;
	m_byes = byes;
}

template <int x> int CAssertElxT <x> :: Match(CContext * pContext) const
{
	int nbegin = pContext->m_nCurrentPos;
	int nsize  = pContext->m_stack.GetSize();
	int ncsize = pContext->m_capturestack.GetSize();
	int bsucc;

	// match
	if( m_byes )
		bsucc =   m_pelx->Match(pContext);
	else
		bsucc = ! m_pelx->Match(pContext);

	// status
	pContext->m_stack.Restore(nsize);
	pContext->m_nCurrentPos = nbegin;

	if( bsucc )
		pContext->m_stack.Push(ncsize);
	else
		pContext->m_capturestack.Restore(ncsize);

	return bsucc;
}

template <int x> int CAssertElxT <x> :: MatchNext(CContext * pContext) const
{
	int ncsize = 0;

	pContext->m_stack.Pop(ncsize);
	pContext->m_capturestack.Restore(ncsize);

	return 0;
}

// emptyelx.cpp: implementation of the CEmptyElx class.
//
template <int x> CEmptyElxT <x> :: CEmptyElxT()
{
}

template <int x> int CEmptyElxT <x> :: Match(CContext *) const
{
	return 1;
}

template <int x> int CEmptyElxT <x> :: MatchNext(CContext *) const
{
	return 0;
}

// globalx.cpp: implementation of the CGlobalElx class.
//
template <int x> CGlobalElxT <x> ::CGlobalElxT()
{
}

template <int x> int CGlobalElxT <x> :: Match(CContext * pContext) const
{
	return pContext->m_nCurrentPos == pContext->m_nBeginPos;
}

template <int x> int CGlobalElxT <x> :: MatchNext(CContext *) const
{
	return 0;
}

// keepelx: implementation of the CKeepElx class.
//
template <int x> int CKeepElxT <x> :: Match(CContext * pContext) const
{
	// m_capturestack[0..3] is group 0, [1] is where the match starts
	pContext->m_stack.Push(pContext->m_capturestack[1]);
	pContext->m_capturestack[1] = pContext->m_nCurrentPos;

	return 1;
}

template <int x> int CKeepElxT <x> :: MatchNext(CContext * pContext) const
{
	int nstart = 0;

	pContext->m_stack.Pop(nstart);
	pContext->m_capturestack[1] = nstart;

	return 0;
}

// greedelx.cpp: implementation of the CGreedyElx class.
//
template <int x> CGreedyElxT <x> :: CGreedyElxT(ElxInterface * pelx, int nmin, int nmax) : CRepeatElxT <x> (pelx, nmin)
{
	m_nvart = nmax - nmin;
}

template <int x> int CGreedyElxT <x> :: Match(CContext * pContext) const
{
	if( ! CRepeatElxT <x> :: MatchFixed(pContext) )
		return 0;

	while( ! MatchVart(pContext) )
	{
		if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
			return 0;
	}

	return 1;
}

template <int x> int CGreedyElxT <x> :: MatchNext(CContext * pContext) const
{
	pContext->Step();

	if( MatchNextVart(pContext) )
		return 1;

	if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
		return 0;

	while( ! MatchVart(pContext) )
	{
		if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
			return 0;
	}

	return 1;
}

template <int x> int CGreedyElxT <x> :: MatchVart(CContext * pContext) const
{
	int n      = 0;
	int nbegin00 = pContext->m_nCurrentPos;
	int nsize    = pContext->m_stack.GetSize();
	int ncsize   = pContext->m_capturestack.GetSize();

	while(n < m_nvart && CRepeatElx::MatchForward(pContext))
	{
		n ++;
	}

	pContext->m_stack.Push(ncsize);
	pContext->m_stack.Push(nsize);
	pContext->m_stack.Push(pContext->m_nCurrentPos);
	pContext->m_stack.Push(1);
	pContext->m_stack.Push(nbegin00);
	pContext->m_stack.Push(n);

	return 1;
}

template <int x> int CGreedyElxT <x> :: MatchNextVart(CContext * pContext) const
{
	int n = 0, nbegin00 = 0, nsize = 0, ncsize = 0;
	CSortedBufferT <int> nbegin99;
	pContext->m_stack.Pop(n);
	pContext->m_stack.Pop(nbegin00);
	pContext->m_stack.Pop(nbegin99);
	pContext->m_stack.Pop(nsize);
	pContext->m_stack.Pop(ncsize);

	if(n == 0) return 0;

	int n0 = n;

	if( ! CRepeatElxT<x>::m_pelx->MatchNext(pContext) )
	{
		n --;
	}

	// not to re-match
	else if(pContext->m_nCurrentPos == nbegin00)
	{
		pContext->m_stack.Restore(nsize);
		pContext->m_capturestack.Restore(ncsize);
		pContext->m_nCurrentPos = nbegin00;

		return 0;
	}

	// fix 2012-10-26, thanks to chenlx01@sohu.com
	else
	{
		CContextShot shot(pContext);

		while(n < m_nvart && CRepeatElx::MatchForward(pContext))
		{
			n ++;
		}

		if(nbegin99.Find(pContext->m_nCurrentPos) >= 0)
		{
			shot.Restore(pContext);
			n = n0;
		}
		else
		{
			nbegin99.Add(pContext->m_nCurrentPos);
		}
	}

	pContext->m_stack.Push(ncsize);
	pContext->m_stack.Push(nsize);
	pContext->m_stack.Push(nbegin99);
	pContext->m_stack.Push(nbegin00);
	pContext->m_stack.Push(n);

	return 1;
}

// indepelx.cpp: implementation of the CIndependentElx class.
//
template <int x> CIndependentElxT <x> :: CIndependentElxT(ElxInterface * pelx)
{
	m_pelx = pelx;
}

template <int x> int CIndependentElxT <x> :: Match(CContext * pContext) const
{
	int nbegin = pContext->m_nCurrentPos;
	int nsize  = pContext->m_stack.GetSize();
	int ncsize = pContext->m_capturestack.GetSize();

	// match
	int bsucc  = m_pelx->Match(pContext);

	// status
	pContext->m_stack.Restore(nsize);

	if( bsucc )
	{
		pContext->m_stack.Push(nbegin);
		pContext->m_stack.Push(ncsize);
	}

	return bsucc;
}

template <int x> int CIndependentElxT <x> :: MatchNext(CContext * pContext) const
{
	int nbegin = 0, ncsize = 0;

	pContext->m_stack.Pop(ncsize);
	pContext->m_stack.Pop(nbegin);

	pContext->m_capturestack.Restore(ncsize);
	pContext->m_nCurrentPos = nbegin;

	return 0;
}

// listelx.cpp: implementation of the CListElx class.
//
template <int x> CListElxT <x> :: CListElxT(int brightleft)
{
	m_brightleft = brightleft;
}

template <int x> int CListElxT <x> :: Match(CContext * pContext) const
{
	pContext->Step();

	if(m_elxlist.GetSize() == 0)
		return 1;

	// prepare
	int bol = m_brightleft ? m_elxlist.GetSize() : -1;
	int stp = m_brightleft ? -1 : 1;
	int eol = m_brightleft ? -1 : m_elxlist.GetSize();

	// from first
	int n = bol + stp;

	// match all
	while(n != eol)
	{
		if(m_elxlist[n]->Match(pContext))
		{
			n += stp;
		}
		else
		{
			n -= stp;

			while(n != bol && ! m_elxlist[n]->MatchNext(pContext))
				n -= stp;

			if(n != bol)
				n += stp;
			else
				return 0;
		}
	}

	return 1;
}

template <int x> int CListElxT <x> :: MatchNext(CContext * pContext) const
{
	pContext->Step();

	if(m_elxlist.GetSize() == 0)
		return 0;

	// prepare
	int bol = m_brightleft ? m_elxlist.GetSize() : -1;
	int stp = m_brightleft ? -1 : 1;
	int eol = m_brightleft ? -1 : m_elxlist.GetSize();

	// from last
	int n = eol - stp;

	while(n != bol && ! m_elxlist[n]->MatchNext(pContext))
		n -= stp;

	if(n != bol)
		n += stp;
	else
		return 0;

	// match rest
	while(n != eol)
	{
		if(m_elxlist[n]->Match(pContext))
		{
			n += stp;
		}
		else
		{
			n -= stp;

			while(n != bol && ! m_elxlist[n]->MatchNext(pContext))
				n -= stp;

			if(n != bol)
				n += stp;
			else
				return 0;
		}
	}

	return 1;
}

// mresult.cpp: implementation of the MatchResult class.
//
template <int x> MatchResultT <x> :: MatchResultT(CContext * pContext, int nMaxNumber)
{
	m_bStepLimitExceeded = 0;

	if(pContext != 0)
	{
		m_result.Prepare(nMaxNumber * 2 + 3, -1);

		// matched
		m_result[0] = 1;
		m_result[1] = nMaxNumber;

		for(int n = 0; n <= nMaxNumber; n++)
		{
			int index = pContext->m_captureindex[n];
			//if( index < 0 ) continue;
			if( ! CBracketElxT<char>::CheckCaptureIndex(index, pContext, n) ) continue;

			// check enclosed
			int pos1 = pContext->m_capturestack[index + 1];
			int pos2 = pContext->m_capturestack[index + 2];

			// info
			m_result[n*2 + 2] = pos1 < pos2 ? pos1 : pos2;
			m_result[n*2 + 3] = pos1 < pos2 ? pos2 : pos1;
		}
	}
}

template <int x> inline int MatchResultT <x> :: IsMatched() const
{
	return m_result.At(0, 0);
}

template <int x> inline int MatchResultT <x> :: MaxGroupNumber() const
{
	return m_result.At(1, 0);
}

template <int x> inline int MatchResultT <x> :: GetStart() const
{
	return m_result.At(2, -1);
}

template <int x> inline int MatchResultT <x> :: GetEnd() const
{
	return m_result.At(3, -1);
}

template <int x> inline int MatchResultT <x> :: GetGroupStart(int nGroupNumber) const
{
	return m_result.At(2 + nGroupNumber * 2, -1);
}

template <int x> inline int MatchResultT <x> :: GetGroupEnd(int nGroupNumber) const
{
	return m_result.At(2 + nGroupNumber * 2 + 1, -1);
}

template <int x> inline int MatchResultT <x> :: IsStepLimitExceeded() const
{
	return m_bStepLimitExceeded;
}

template <int x> MatchResultT <x> & MatchResultT <x> :: operator = (const MatchResultT <x> & result)
{
	if(this == &result) return *this;

	m_result.Restore(0);
	if(result.m_result.GetSize() > 0) m_result.Append(result.m_result.GetBuffer(), result.m_result.GetSize());
	m_bStepLimitExceeded = result.m_bStepLimitExceeded;

	return *this;
}

// posselx.cpp: implementation of the CPossessiveElx class.
//
template <int x> CPossessiveElxT <x> :: CPossessiveElxT(ElxInterface * pelx, int nmin, int nmax) : CGreedyElxT <x> (pelx, nmin, nmax)
{
}

template <int x> int CPossessiveElxT <x> :: Match(CContext * pContext) const
{
	int nbegin = pContext->m_nCurrentPos;
	int nsize  = pContext->m_stack.GetSize();
	int ncsize = pContext->m_capturestack.GetSize();
	int bsucc  = 1;

	// match
	if( ! CRepeatElxT <x> :: MatchFixed(pContext) )
	{
		bsucc = 0;
	}
	else
	{
		while( ! CGreedyElxT <x> :: MatchVart(pContext) )
		{
			if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
			{
				bsucc = 0;
				break;
			}
		}
	}

	// status
	pContext->m_stack.Restore(nsize);

	if( bsucc )
	{
		pContext->m_stack.Push(nbegin);
		pContext->m_stack.Push(ncsize);
	}

	return bsucc;
}

template <int x> int CPossessiveElxT <x> :: MatchNext(CContext * pContext) const
{
	int nbegin = 0, ncsize = 0;

	pContext->m_stack.Pop(ncsize);
	pContext->m_stack.Pop(nbegin);

	pContext->m_capturestack.Restore(ncsize);
	pContext->m_nCurrentPos = nbegin;

	return 0;
}

// reluctx.cpp: implementation of the CReluctantElx class.
//
template <int x> CReluctantElxT <x> :: CReluctantElxT(ElxInterface * pelx, int nmin, int nmax) : CRepeatElxT <x> (pelx, nmin)
{
	m_nvart = nmax - nmin;
}

template <int x> int CReluctantElxT <x> :: Match(CContext * pContext) const
{
	if( ! CRepeatElxT <x> :: MatchFixed(pContext) )
		return 0;

	while( ! MatchVart(pContext) )
	{
		if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
			return 0;
	}

	return 1;
}

template <int x> int CReluctantElxT <x> :: MatchNext(CContext * pContext) const
{
	pContext->Step();

	if( MatchNextVart(pContext) )
		return 1;

	if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
		return 0;

	while( ! MatchVart(pContext) )
	{
		if( ! CRepeatElxT <x> :: MatchNextFixed(pContext) )
			return 0;
	}

	return 1;
}

template <int x> int CReluctantElxT <x> :: MatchVart(CContext * pContext) const
{
	pContext->m_stack.Push(0);

	return 1;
}

template <int x> int CReluctantElxT <x> :: MatchNextVart(CContext * pContext) const
{
	int n = 0, nbegin = pContext->m_nCurrentPos;

	pContext->m_stack.Pop(n);

	if(n < m_nvart && CRepeatElxT <x> :: m_pelx->Match(pContext))
	{
		while(pContext->m_nCurrentPos == nbegin)
		{
			if( ! CRepeatElxT <x> :: m_pelx->MatchNext(pContext) ) break;
		}

		if(pContext->m_nCurrentPos != nbegin)
		{
			n ++;

			pContext->m_stack.Push(nbegin);
			pContext->m_stack.Push(n);

			return 1;
		}
	}

	while(n > 0)
	{
		pContext->m_stack.Pop(nbegin);

		while( CRepeatElxT <x> :: m_pelx->MatchNext(pContext) )
		{
			if(pContext->m_nCurrentPos != nbegin)
			{
				pContext->m_stack.Push(nbegin);
				pContext->m_stack.Push(n);

				return 1;
			}
		}

		n --;
	}

	return 0;
}

// repeatx.cpp: implementation of the CRepeatElx class.
//
template <int x> CRepeatElxT <x> :: CRepeatElxT(ElxInterface * pelx, int ntimes)
{
	m_pelx   = pelx;
	m_nfixed = ntimes;
}

template <int x> int CRepeatElxT <x> :: Match(CContext * pContext) const
{
	return MatchFixed(pContext);
}

template <int x> int CRepeatElxT <x> :: MatchNext(CContext * pContext) const
{
	pContext->Step();

	return MatchNextFixed(pContext);
}

template <int x> int CRepeatElxT <x> :: MatchFixed(CContext * pContext) const
{
	if(m_nfixed == 0)
		return 1;

	int n = 0;

	while(n < m_nfixed)
	{
		if(m_pelx->Match(pContext))
		{
			n ++;
		}
		else
		{
			n --;

			while(n >= 0 && ! m_pelx->MatchNext(pContext))
				n --;

			if(n >= 0)
				n ++;
			else
				return 0;
		}
	}

	return 1;
}

template <int x> int CRepeatElxT <x> :: MatchNextFixed(CContext * pContext) const
{
	if(m_nfixed == 0)
		return 0;

	// from last
	int n = m_nfixed - 1;

	while(n >= 0 && ! m_pelx->MatchNext(pContext))
		n --;

	if(n >= 0)
		n ++;
	else
		return 0;

	// match rest
	while(n < m_nfixed)
	{
		if(m_pelx->Match(pContext))
		{
			n ++;
		}
		else
		{
			n --;

			while(n >= 0 && ! m_pelx->MatchNext(pContext))
				n --;

			if(n >= 0)
				n ++;
			else
				return 0;
		}
	}

	return 1;
}

//
// Unicode tables, generated from the Unicode Character Database 16.0.0
// (UnicodeData.txt and CaseFolding.txt)
//

// Runs of code points with the same general category, up to U+10FFFF:
// (first code point << 5) | DEELX_UC_xx
inline const unsigned int * deelx_unicode_category_runs(int & count)
{
	static const unsigned int runs[] =
	{
		0x0000019, 0x0000416, 0x0000431, 0x0000493, 0x00004B1, 0x000050D, 0x000052E, 0x0000551,
		0x0000572, 0x0000591, 0x00005AC, 0x00005D1, 0x0000608, 0x0000751, 0x0000792, 0x00007F1,
		0x0000820, 0x0000B6D, 0x0000B91, 0x0000BAE, 0x0000BD4, 0x0000BEB, 0x0000C14, 0x0000C21,
		0x0000F6D, 0x0000F92, 0x0000FAE, 0x0000FD2, 0x0000FF9, 0x0001416, 0x0001431, 0x0001453,
		0x00014D5, 0x00014F1, 0x0001514, 0x0001535, 0x0001544, 0x000156F, 0x0001592, 0x00015BA,
		0x00015D5, 0x00015F4, 0x0001615, 0x0001632, 0x000164A, 0x0001694, 0x00016A1, 0x00016D1,
		0x0001714, 0x000172A, 0x0001744, 0x0001770, 0x000178A, 0x00017F1, 0x0001800, 0x0001AF2,
		0x0001B00, 0x0001BE1, 0x0001EF2, 0x0001F01, 0x0002000, 0x0002021, 0x0002040, 0x0002061,
		0x0002080, 0x00020A1, 0x00020C0, 0x00020E1, 0x0002100, 0x0002121, 0x0002140, 0x0002161,
		0x0002180, 0x00021A1, 0x00021C0, 0x00021E1, 0x0002200, 0x0002221, 0x0002240, 0x0002261,
		0x0002280, 0x00022A1, 0x00022C0, 0x00022E1, 0x0002300, 0x0002321, 0x0002340, 0x0002361,
		0x0002380, 0x00023A1, 0x00023C0, 0x00023E1, 0x0002400, 0x0002421, 0x0002440, 0x0002461,
		0x0002480, 0x00024A1, 0x00024C0, 0x00024E1, 0x0002500, 0x0002521, 0x0002540, 0x0002561,
		0x0002580, 0x00025A1, 0x00025C0, 0x00025E1, 0x0002600, 0x0002621, 0x0002640, 0x0002661,
		0x0002680, 0x00026A1, 0x00026C0, 0x00026E1, 0x0002720, 0x0002741, 0x0002760, 0x0002781,
		0x00027A0, 0x00027C1, 0x00027E0, 0x0002801, 0x0002820, 0x0002841, 0x0002860, 0x0002881,
		0x00028A0, 0x00028C1, 0x00028E0, 0x0002901, 0x0002940, 0x0002961, 0x0002980, 0x00029A1,
		0x00029C0, 0x00029E1, 0x0002A00, 0x0002A21, 0x0002A40, 0x0002A61, 0x0002A80, 0x0002AA1,
		0x0002AC0, 0x0002AE1, 0x0002B00, 0x0002B21, 0x0002B40, 0x0002B61, 0x0002B80, 0x0002BA1,
		0x0002BC0, 0x0002BE1, 0x0002C00, 0x0002C21, 0x0002C40, 0x0002C61, 0x0002C80, 0x0002CA1,
		0x0002CC0, 0x0002CE1, 0x0002D00, 0x0002D21, 0x0002D40, 0x0002D61, 0x0002D80, 0x0002DA1,
		0x0002DC0, 0x0002DE1, 0x0002E00, 0x0002E21, 0x0002E40, 0x0002E61, 0x0002E80, 0x0002EA1,
		0x0002EC0, 0x0002EE1, 0x0002F00, 0x0002F41, 0x0002F60, 0x0002F81, 0x0002FA0, 0x0002FC1,
		0x0003020, 0x0003061, 0x0003080, 0x00030A1, 0x00030C0, 0x0003101, 0x0003120, 0x0003181,
		0x00031C0, 0x0003241, 0x0003260, 0x00032A1, 0x00032C0, 0x0003321, 0x0003380, 0x00033C1,
		0x00033E0, 0x0003421, 0x0003440, 0x0003461, 0x0003480, 0x00034A1, 0x00034C0, 0x0003501,
		0x0003520, 0x0003541, 0x0003580, 0x00035A1, 0x00035C0, 0x0003601, 0x0003620, 0x0003681,
		0x00036A0, 0x00036C1, 0x00036E0, 0x0003721, 0x0003764, 0x0003780, 0x00037A1, 0x0003804,
		0x0003880, 0x00038A2, 0x00038C1, 0x00038E0, 0x0003902, 0x0003921, 0x0003940, 0x0003962,
		0x0003981, 0x00039A0, 0x00039C1, 0x00039E0, 0x0003A01, 0x0003A20, 0x0003A41, 0x0003A60,
		0x0003A81, 0x0003AA0, 0x0003AC1, 0x0003AE0, 0x0003B01, 0x0003B20, 0x0003B41, 0x0003B60,
		0x0003B81, 0x0003BC0, 0x0003BE1, 0x0003C00, 0x0003C21, 0x0003C40, 0x0003C61, 0x0003C80,
		0x0003CA1, 0x0003CC0, 0x0003CE1, 0x0003D00, 0x0003D21, 0x0003D40, 0x0003D61, 0x0003D80,
		0x0003DA1, 0x0003DC0, 0x0003DE1, 0x0003E20, 0x0003E42, 0x0003E61, 0x0003E80, 0x0003EA1,
		0x0003EC0, 0x0003F21, 0x0003F40, 0x0003F61, 0x0003F80, 0x0003FA1, 0x0003FC0, 0x0003FE1,
		0x0004000, 0x0004021, 0x0004040, 0x0004061, 0x0004080, 0x00040A1, 0x00040C0, 0x00040E1,
		0x0004100, 0x0004121, 0x0004140, 0x0004161, 0x0004180, 0x00041A1, 0x00041C0, 0x00041E1,
		0x0004200, 0x0004221, 0x0004240, 0x0004261, 0x0004280, 0x00042A1, 0x00042C0, 0x00042E1,
		0x0004300, 0x0004321, 0x0004340, 0x0004361, 0x0004380, 0x00043A1, 0x00043C0, 0x00043E1,
		0x0004400, 0x0004421, 0x0004440, 0x0004461, 0x0004480, 0x00044A1, 0x00044C0, 0x00044E1,
		0x0004500, 0x0004521, 0x0004540, 0x0004561, 0x0004580, 0x00045A1, 0x00045C0, 0x00045E1,
		0x0004600, 0x0004621, 0x0004640, 0x0004661, 0x0004740, 0x0004781, 0x00047A0, 0x00047E1,
		0x0004820, 0x0004841, 0x0004860, 0x00048E1, 0x0004900, 0x0004921, 0x0004940, 0x0004961,
		0x0004980, 0x00049A1, 0x00049C0, 0x00049E1, 0x0005284, 0x00052A1, 0x0005603, 0x0005854,
		0x00058C3, 0x0005A54, 0x0005C03, 0x0005CB4, 0x0005D83, 0x0005DB4, 0x0005DC3, 0x0005DF4,
		0x0006005, 0x0006E00, 0x0006E21, 0x0006E40, 0x0006E61, 0x0006E83, 0x0006EB4, 0x0006EC0,
		0x0006EE1, 0x0006F1D, 0x0006F43, 0x0006F61, 0x0006FD1, 0x0006FE0, 0x000701D, 0x0007094,
		0x00070C0, 0x00070F1, 0x0007100, 0x000717D, 0x0007180, 0x00071BD, 0x00071C0, 0x0007201,
		0x0007220, 0x000745D, 0x0007460, 0x0007581, 0x00079E0, 0x0007A01, 0x0007A40, 0x0007AA1,
		0x0007B00, 0x0007B21, 0x0007B40, 0x0007B61, 0x0007B80, 0x0007BA1, 0x0007BC0, 0x0007BE1,
		0x0007C00, 0x0007C21, 0x0007C40, 0x0007C61, 0x0007C80, 0x0007CA1, 0x0007CC0, 0x0007CE1,
		0x0007D00, 0x0007D21, 0x0007D40, 0x0007D61, 0x0007D80, 0x0007DA1, 0x0007DC0, 0x0007DE1,
		0x0007E80, 0x0007EA1, 0x0007ED2, 0x0007EE0, 0x0007F01, 0x0007F20, 0x0007F61, 0x0007FA0,
		0x0008601, 0x0008C00, 0x0008C21, 0x0008C40, 0x0008C61, 0x0008C80, 0x0008CA1, 0x0008CC0,
		0x0008CE1, 0x0008D00, 0x0008D21, 0x0008D40, 0x0008D61, 0x0008D80, 0x0008DA1, 0x0008DC0,
		0x0008DE1, 0x0008E00, 0x0008E21, 0x0008E40, 0x0008E61, 0x0008E80, 0x0008EA1, 0x0008EC0,
		0x0008EE1, 0x0008F00, 0x0008F21, 0x0008F40, 0x0008F61, 0x0008F80, 0x0008FA1, 0x0008FC0,
		0x0008FE1, 0x0009000, 0x0009021, 0x0009055, 0x0009065, 0x0009107, 0x0009140, 0x0009161,
		0x0009180, 0x00091A1, 0x00091C0, 0x00091E1, 0x0009200, 0x0009221, 0x0009240, 0x0009261,
		0x0009280, 0x00092A1, 0x00092C0, 0x00092E1, 0x0009300, 0x0009321, 0x0009340, 0x0009361,
		0x0009380, 0x00093A1, 0x00093C0, 0x00093E1, 0x0009400, 0x0009421, 0x0009440, 0x0009461,
		0x0009480, 0x00094A1, 0x00094C0, 0x00094E1, 0x0009500, 0x0009521, 0x0009540, 0x0009561,
		0x0009580, 0x00095A1, 0x00095C0, 0x00095E1, 0x0009600, 0x0009621, 0x0009640, 0x0009661,
		0x0009680, 0x00096A1, 0x00096C0, 0x00096E1, 0x0009700, 0x0009721, 0x0009740, 0x0009761,
		0x0009780, 0x00097A1, 0x00097C0, 0x00097E1, 0x0009800, 0x0009841, 0x0009860, 0x0009881,
		0x00098A0, 0x00098C1, 0x00098E0, 0x0009901, 0x0009920, 0x0009941, 0x0009960, 0x0009981,
		0x00099A0, 0x00099C1, 0x0009A00, 0x0009A21, 0x0009A40, 0x0009A61, 0x0009A80, 0x0009AA1,
		0x0009AC0, 0x0009AE1, 0x0009B00, 0x0009B21, 0x0009B40, 0x0009B61, 0x0009B80, 0x0009BA1,
		0x0009BC0, 0x0009BE1, 0x0009C00, 0x0009C21, 0x0009C40, 0x0009C61, 0x0009C80, 0x0009CA1,
		0x0009CC0, 0x0009CE1, 0x0009D00, 0x0009D21, 0x0009D40, 0x0009D61, 0x0009D80, 0x0009DA1,
		0x0009DC0, 0x0009DE1, 0x0009E00, 0x0009E21, 0x0009E40, 0x0009E61, 0x0009E80, 0x0009EA1,
		0x0009EC0, 0x0009EE1, 0x0009F00, 0x0009F21, 0x0009F40, 0x0009F61, 0x0009F80, 0x0009FA1,
		0x0009FC0, 0x0009FE1, 0x000A000, 0x000A021, 0x000A040, 0x000A061, 0x000A080, 0x000A0A1,
		0x000A0C0, 0x000A0E1, 0x000A100, 0x000A121, 0x000A140, 0x000A161, 0x000A180, 0x000A1A1,
		0x000A1C0, 0x000A1E1, 0x000A200, 0x000A221, 0x000A240, 0x000A261, 0x000A280, 0x000A2A1,
		0x000A2C0, 0x000A2E1, 0x000A300, 0x000A321, 0x000A340, 0x000A361, 0x000A380, 0x000A3A1,
		0x000A3C0, 0x000A3E1, 0x000A400, 0x000A421, 0x000A440, 0x000A461, 0x000A480, 0x000A4A1,
		0x000A4C0, 0x000A4E1, 0x000A500, 0x000A521, 0x000A540, 0x000A561, 0x000A580, 0x000A5A1,
		0x000A5C0, 0x000A5E1, 0x000A61D, 0x000A620, 0x000AAFD, 0x000AB23, 0x000AB51, 0x000AC01,
		0x000B131, 0x000B14C, 0x000B17D, 0x000B1B5, 0x000B1F3, 0x000B21D, 0x000B225, 0x000B7CC,
		0x000B7E5, 0x000B811, 0x000B825, 0x000B871, 0x000B885, 0x000B8D1, 0x000B8E5, 0x000B91D,
		0x000BA04, 0x000BD7D, 0x000BDE4, 0x000BE71, 0x000BEBD, 0x000C01A, 0x000C0D2, 0x000C131,
		0x000C173, 0x000C191, 0x000C1D5, 0x000C205, 0x000C371, 0x000C39A, 0x000C3B1, 0x000C404,
		0x000C803, 0x000C824, 0x000C965, 0x000CC08, 0x000CD51, 0x000CDC4, 0x000CE05, 0x000CE24,
		0x000DA91, 0x000DAA4, 0x000DAC5, 0x000DBBA, 0x000DBD5, 0x000DBE5, 0x000DCA3, 0x000DCE5,
		0x000DD35, 0x000DD45, 0x000DDC4, 0x000DE08, 0x000DF44, 0x000DFB5, 0x000DFE4, 0x000E011,
		0x000E1DD, 0x000E1FA, 0x000E204, 0x000E225, 0x000E244, 0x000E605, 0x000E97D, 0x000E9A4,
		0x000F4C5, 0x000F624, 0x000F65D, 0x000F808, 0x000F944, 0x000FD65, 0x000FE83, 0x000FED5,
		0x000FEF1, 0x000FF43, 0x000FF7D, 0x000FFA5, 0x000FFD3, 0x0010004, 0x00102C5, 0x0010343,
		0x0010365, 0x0010483, 0x00104A5, 0x0010503, 0x0010525, 0x00105DD, 0x0010611, 0x00107FD,
		0x0010804, 0x0010B25, 0x0010B9D, 0x0010BD1, 0x0010BFD, 0x0010C04, 0x0010D7D, 0x0010E04,
		0x0011114, 0x0011124, 0x00111FD, 0x001121A, 0x001125D, 0x00112E5, 0x0011404, 0x0011923,
		0x0011945, 0x0011C5A, 0x0011C65, 0x0012066, 0x0012084, 0x0012745, 0x0012766, 0x0012785,
		0x00127A4, 0x00127C6, 0x0012825, 0x0012926, 0x00129A5, 0x00129C6, 0x0012A04, 0x0012A25,
		0x0012B04, 0x0012C45, 0x0012C91, 0x0012CC8, 0x0012E11, 0x0012E23, 0x0012E44, 0x0013025,
		0x0013046, 0x001309D, 0x00130A4, 0x00131BD, 0x00131E4, 0x001323D, 0x0013264, 0x001353D,
		0x0013544, 0x001363D, 0x0013644, 0x001367D, 0x00136C4, 0x001375D, 0x0013785, 0x00137A4,
		0x00137C6, 0x0013825, 0x00138BD, 0x00138E6, 0x001393D, 0x0013966, 0x00139A5, 0x00139C4,
		0x00139FD, 0x0013AE6, 0x0013B1D, 0x0013B84, 0x0013BDD, 0x0013BE4, 0x0013C45, 0x0013C9D,
		0x0013CC8, 0x0013E04, 0x0013E53, 0x0013E8A, 0x0013F55, 0x0013F73, 0x0013F84, 0x0013FB1,
		0x0013FC5, 0x0013FFD, 0x0014025, 0x0014066, 0x001409D, 0x00140A4, 0x001417D, 0x00141E4,
		0x001423D, 0x0014264, 0x001453D, 0x0014544, 0x001463D, 0x0014644, 0x001469D, 0x00146A4,
		0x00146FD, 0x0014704, 0x001475D, 0x0014785, 0x00147BD, 0x00147C6, 0x0014825, 0x001487D,
		0x00148E5, 0x001493D, 0x0014965, 0x00149DD, 0x0014A25, 0x0014A5D, 0x0014B24, 0x0014BBD,
		0x0014BC4, 0x0014BFD, 0x0014CC8, 0x0014E05, 0x0014E44, 0x0014EA5, 0x0014ED1, 0x0014EFD,
		0x0015025, 0x0015066, 0x001509D, 0x00150A4, 0x00151DD, 0x00151E4, 0x001525D, 0x0015264,
		0x001553D, 0x0015544, 0x001563D, 0x0015644, 0x001569D, 0x00156A4, 0x001575D, 0x0015785,
		0x00157A4, 0x00157C6, 0x0015825, 0x00158DD, 0x00158E5, 0x0015926, 0x001595D, 0x0015966,
		0x00159A5, 0x00159DD, 0x0015A04, 0x0015A3D, 0x0015C04, 0x0015C45, 0x0015C9D, 0x0015CC8,
		0x0015E11, 0x0015E33, 0x0015E5D, 0x0015F24, 0x0015F45, 0x001601D, 0x0016025, 0x0016046,
		0x001609D, 0x00160A4, 0x00161BD, 0x00161E4, 0x001623D, 0x0016264, 0x001653D, 0x0016544,
		0x001663D, 0x0016644, 0x001669D, 0x00166A4, 0x001675D, 0x0016785, 0x00167A4, 0x00167C6,
		0x00167E5, 0x0016806, 0x0016825, 0x00168BD, 0x00168E6, 0x001693D, 0x0016966, 0x00169A5,
		0x00169DD, 0x0016AA5, 0x0016AE6, 0x0016B1D, 0x0016B84, 0x0016BDD, 0x0016BE4, 0x0016C45,
		0x0016C9D, 0x0016CC8, 0x0016E15, 0x0016E24, 0x0016E4A, 0x0016F1D, 0x0017045, 0x0017064,
		0x001709D, 0x00170A4, 0x001717D, 0x00171C4, 0x001723D, 0x0017244, 0x00172DD, 0x0017324,
		0x001737D, 0x0017384, 0x00173BD, 0x00173C4, 0x001741D, 0x0017464, 0x00174BD, 0x0017504,
		0x001757D, 0x00175C4, 0x001775D, 0x00177C6, 0x0017805, 0x0017826, 0x001787D, 0x00178C6,
		0x001793D, 0x0017946, 0x00179A5, 0x00179DD, 0x0017A04, 0x0017A3D, 0x0017AE6, 0x0017B1D,
		0x0017CC8, 0x0017E0A, 0x0017E75, 0x0017F33, 0x0017F55, 0x0017F7D, 0x0018005, 0x0018026,
		0x0018085, 0x00180A4, 0x00181BD, 0x00181C4, 0x001823D, 0x0018244, 0x001853D, 0x0018544,
		0x001875D, 0x0018785, 0x00187A4, 0x00187C5, 0x0018826, 0x00188BD, 0x00188C5, 0x001893D,
		0x0018945, 0x00189DD, 0x0018AA5, 0x0018AFD, 0x0018B04, 0x0018B7D, 0x0018BA4, 0x0018BDD,
		0x0018C04, 0x0018C45, 0x0018C9D, 0x0018CC8, 0x0018E1D, 0x0018EF1, 0x0018F0A, 0x0018FF5,
		0x0019004, 0x0019025, 0x0019046, 0x0019091, 0x00190A4, 0x00191BD, 0x00191C4, 0x001923D,
		0x0019244, 0x001953D, 0x0019544, 0x001969D, 0x00196A4, 0x001975D, 0x0019785, 0x00197A4,
		0x00197C6, 0x00197E5, 0x0019806, 0x00198BD, 0x00198C5, 0x00198E6, 0x001993D, 0x0019946,
		0x0019985, 0x00199DD, 0x0019AA6, 0x0019AFD, 0x0019BA4, 0x0019BFD, 0x0019C04, 0x0019C45,
		0x0019C9D, 0x0019CC8, 0x0019E1D, 0x0019E24, 0x0019E66, 0x0019E9D, 0x001A005, 0x001A046,
		0x001A084, 0x001A1BD, 0x001A1C4, 0x001A23D, 0x001A244, 0x001A765, 0x001A7A4, 0x001A7C6,
		0x001A825, 0x001A8BD, 0x001A8C6, 0x001A93D, 0x001A946, 0x001A9A5, 0x001A9C4, 0x001A9F5,
		0x001AA1D, 0x001AA84, 0x001AAE6, 0x001AB0A, 0x001ABE4, 0x001AC45, 0x001AC9D, 0x001ACC8,
		0x001AE0A, 0x001AF35, 0x001AF44, 0x001B01D, 0x001B025, 0x001B046, 0x001B09D, 0x001B0A4,
		0x001B2FD, 0x001B344, 0x001B65D, 0x001B664, 0x001B79D, 0x001B7A4, 0x001B7DD, 0x001B804,
		0x001B8FD, 0x001B945, 0x001B97D, 0x001B9E6, 0x001BA45, 0x001BABD, 0x001BAC5, 0x001BAFD,
		0x001BB06, 0x001BC1D, 0x001BCC8, 0x001BE1D, 0x001BE46, 0x001BE91, 0x001BEBD, 0x001C024,
		0x001C625, 0x001C644, 0x001C685, 0x001C77D, 0x001C7F3, 0x001C804, 0x001C8C3, 0x001C8E5,
		0x001C9F1, 0x001CA08, 0x001CB51, 0x001CB9D, 0x001D024, 0x001D07D, 0x001D084, 0x001D0BD,
		0x001D0C4, 0x001D17D, 0x001D184, 0x001D49D, 0x001D4A4, 0x001D4DD, 0x001D4E4, 0x001D625,
		0x001D644, 0x001D685, 0x001D7A4, 0x001D7DD, 0x001D804, 0x001D8BD, 0x001D8C3, 0x001D8FD,
		0x001D905, 0x001D9FD, 0x001DA08, 0x001DB5D, 0x001DB84, 0x001DC1D, 0x001E004, 0x001E035,
		0x001E091, 0x001E275, 0x001E291, 0x001E2B5, 0x001E305, 0x001E355, 0x001E408, 0x001E54A,
		0x001E695, 0x001E6A5, 0x001E6D5, 0x001E6E5, 0x001E715, 0x001E725, 0x001E74D, 0x001E76E,
		0x001E78D, 0x001E7AE, 0x001E7C6, 0x001E804, 0x001E91D, 0x001E924, 0x001EDBD, 0x001EE25,
		0x001EFE6, 0x001F005, 0x001F0B1, 0x001F0C5, 0x001F104, 0x001F1A5, 0x001F31D, 0x001F325,
		0x001F7BD, 0x001F7D5, 0x001F8C5, 0x001F8F5, 0x001F9BD, 0x001F9D5, 0x001FA11, 0x001FAB5,
		0x001FB31, 0x001FB7D, 0x0020004, 0x0020566, 0x00205A5, 0x0020626, 0x0020645, 0x0020706,
		0x0020725, 0x0020766, 0x00207A5, 0x00207E4, 0x0020808, 0x0020951, 0x0020A04, 0x0020AC6,
		0x0020B05, 0x0020B44, 0x0020BC5, 0x0020C24, 0x0020C46, 0x0020CA4, 0x0020CE6, 0x0020DC4,
		0x0020E25, 0x0020EA4, 0x0021045, 0x0021066, 0x00210A5, 0x00210E6, 0x00211A5, 0x00211C4,
		0x00211E6, 0x0021208, 0x0021346, 0x00213A5, 0x00213D5, 0x0021400, 0x00218DD, 0x00218E0,
		0x002191D, 0x00219A0, 0x00219DD, 0x0021A01, 0x0021F71, 0x0021F83, 0x0021FA1, 0x0022004,
		0x002493D, 0x0024944, 0x00249DD, 0x0024A04, 0x0024AFD, 0x0024B04, 0x0024B3D, 0x0024B44,
		0x0024BDD, 0x0024C04, 0x002513D, 0x0025144, 0x00251DD, 0x0025204, 0x002563D, 0x0025644,
		0x00256DD, 0x0025704, 0x00257FD, 0x0025804, 0x002583D, 0x0025844, 0x00258DD, 0x0025904,
		0x0025AFD, 0x0025B04, 0x002623D, 0x0026244, 0x00262DD, 0x0026304, 0x0026B7D, 0x0026BA5,
		0x0026C11, 0x0026D2A, 0x0026FBD, 0x0027004, 0x0027215, 0x002735D, 0x0027400, 0x0027EDD,
		0x0027F01, 0x0027FDD, 0x002800C, 0x0028024, 0x002CDB5, 0x002CDD1, 0x002CDE4, 0x002D016,
		0x002D024, 0x002D36D, 0x002D38E, 0x002D3BD, 0x002D404, 0x002DD71, 0x002DDC9, 0x002DE24,
		0x002DF3D, 0x002E004, 0x002E245, 0x002E2A6, 0x002E2DD, 0x002E3E4, 0x002E645, 0x002E686,
		0x002E6B1, 0x002E6FD, 0x002E804, 0x002EA45, 0x002EA9D, 0x002EC04, 0x002EDBD, 0x002EDC4,
		0x002EE3D, 0x002EE45, 0x002EE9D, 0x002F004, 0x002F685, 0x002F6C6, 0x002F6E5, 0x002F7C6,
		0x002F8C5, 0x002F8E6, 0x002F925, 0x002FA91, 0x002FAE3, 0x002FB11, 0x002FB73, 0x002FB84,
		0x002FBA5, 0x002FBDD, 0x002FC08, 0x002FD5D, 0x002FE0A, 0x002FF5D, 0x0030011, 0x00300CC,
		0x00300F1, 0x0030165, 0x00301DA, 0x00301E5, 0x0030208, 0x003035D, 0x0030404, 0x0030863,
		0x0030884, 0x0030F3D, 0x0031004, 0x00310A5, 0x00310E4, 0x0031525, 0x0031544, 0x003157D,
		0x0031604, 0x0031EDD, 0x0032004, 0x00323FD, 0x0032405, 0x0032466, 0x00324E5, 0x0032526,
		0x003259D, 0x0032606, 0x0032645, 0x0032666, 0x0032725, 0x003279D, 0x0032815, 0x003283D,
		0x0032891, 0x00328C8, 0x0032A04, 0x0032DDD, 0x0032E04, 0x0032EBD, 0x0033004, 0x003359D,
		0x0033604, 0x003395D, 0x0033A08, 0x0033B4A, 0x0033B7D, 0x0033BD5, 0x0034004, 0x00342E5,
		0x0034326, 0x0034365, 0x003439D, 0x00343D1, 0x0034404, 0x0034AA6, 0x0034AC5, 0x0034AE6,
		0x0034B05, 0x0034BFD, 0x0034C05, 0x0034C26, 0x0034C45, 0x0034C66, 0x0034CA5, 0x0034DA6,
		0x0034E65, 0x0034FBD, 0x0034FE5, 0x0035008, 0x003515D, 0x0035208, 0x003535D, 0x0035411,
		0x00354E3, 0x0035511, 0x00355DD, 0x0035605, 0x00357C7, 0x00357E5, 0x00359FD, 0x0036005,
		0x0036086, 0x00360A4, 0x0036685, 0x00366A6, 0x00366C5, 0x0036766, 0x0036785, 0x00367A6,
		0x0036845, 0x0036866, 0x00368A4, 0x00369BD, 0x00369D1, 0x0036A08, 0x0036B51, 0x0036C35,
		0x0036D65, 0x0036E95, 0x0036FB1, 0x0037005, 0x0037046, 0x0037064, 0x0037426, 0x0037445,
		0x00374C6, 0x0037505, 0x0037546, 0x0037565, 0x00375C4, 0x0037608, 0x0037744, 0x0037CC5,
		0x0037CE6, 0x0037D05, 0x0037D46, 0x0037DA5, 0x0037DC6, 0x0037DE5, 0x0037E46, 0x0037E9D,
		0x0037F91, 0x0038004, 0x0038486, 0x0038585, 0x0038686, 0x00386C5, 0x003871D, 0x0038771,
		0x0038808, 0x003895D, 0x00389A4, 0x0038A08, 0x0038B44, 0x0038F03, 0x0038FD1, 0x0039001,
		0x0039120, 0x0039141, 0x003917D, 0x0039200, 0x003977D, 0x00397A0, 0x0039811, 0x003991D,
		0x0039A05, 0x0039A71, 0x0039A85, 0x0039C26, 0x0039C45, 0x0039D24, 0x0039DA5, 0x0039DC4,
		0x0039E85, 0x0039EA4, 0x0039EE6, 0x0039F05, 0x0039F44, 0x0039F7D, 0x003A001, 0x003A583,
		0x003AD61, 0x003AF03, 0x003AF21, 0x003B363, 0x003B805, 0x003C000, 0x003C021, 0x003C040,
		0x003C061, 0x003C080, 0x003C0A1, 0x003C0C0, 0x003C0E1, 0x003C100, 0x003C121, 0x003C140,
		0x003C161, 0x003C180, 0x003C1A1, 0x003C1C0, 0x003C1E1, 0x003C200, 0x003C221, 0x003C240,
		0x003C261, 0x003C280, 0x003C2A1, 0x003C2C0, 0x003C2E1, 0x003C300, 0x003C321, 0x003C340,
		0x003C361, 0x003C380, 0x003C3A1, 0x003C3C0, 0x003C3E1, 0x003C400, 0x003C421, 0x003C440,
		0x003C461, 0x003C480, 0x003C4A1, 0x003C4C0, 0x003C4E1, 0x003C500, 0x003C521, 0x003C540,
		0x003C561, 0x003C580, 0x003C5A1, 0x003C5C0, 0x003C5E1, 0x003C600, 0x003C621, 0x003C640,
		0x003C661, 0x003C680, 0x003C6A1, 0x003C6C0, 0x003C6E1, 0x003C700, 0x003C721, 0x003C740,
		0x003C761, 0x003C780, 0x003C7A1, 0x003C7C0, 0x003C7E1, 0x003C800, 0x003C821, 0x003C840,
		0x003C861, 0x003C880, 0x003C8A1, 0x003C8C0, 0x003C8E1, 0x003C900, 0x003C921, 0x003C940,
		0x003C961, 0x003C980, 0x003C9A1, 0x003C9C0, 0x003C9E1, 0x003CA00, 0x003CA21, 0x003CA40,
		0x003CA61, 0x003CA80, 0x003CAA1, 0x003CAC0, 0x003CAE1, 0x003CB00, 0x003CB21, 0x003CB40,
		0x003CB61, 0x003CB80, 0x003CBA1, 0x003CBC0, 0x003CBE1, 0x003CC00, 0x003CC21, 0x003CC40,
		0x003CC61, 0x003CC80, 0x003CCA1, 0x003CCC0, 0x003CCE1, 0x003CD00, 0x003CD21, 0x003CD40,
		0x003CD61, 0x003CD80, 0x003CDA1, 0x003CDC0, 0x003CDE1, 0x003CE00, 0x003CE21, 0x003CE40,
		0x003CE61, 0x003CE80, 0x003CEA1, 0x003CEC0, 0x003CEE1, 0x003CF00, 0x003CF21, 0x003CF40,
		0x003CF61, 0x003CF80, 0x003CFA1, 0x003CFC0, 0x003CFE1, 0x003D000, 0x003D021, 0x003D040,
		0x003D061, 0x003D080, 0x003D0A1, 0x003D0C0, 0x003D0E1, 0x003D100, 0x003D121, 0x003D140,
		0x003D161, 0x003D180, 0x003D1A1, 0x003D1C0, 0x003D1E1, 0x003D200, 0x003D221, 0x003D240,
		0x003D261, 0x003D280, 0x003D2A1, 0x003D3C0, 0x003D3E1, 0x003D400, 0x003D421, 0x003D440,
		0x003D461, 0x003D480, 0x003D4A1, 0x003D4C0, 0x003D4E1, 0x003D500, 0x003D521, 0x003D540,
		0x003D561, 0x003D580, 0x003D5A1, 0x003D5C0, 0x003D5E1, 0x003D600, 0x003D621, 0x003D640,
		0x003D661, 0x003D680, 0x003D6A1, 0x003D6C0, 0x003D6E1, 0x003D700, 0x003D721, 0x003D740,
		0x003D761, 0x003D780, 0x003D7A1, 0x003D7C0, 0x003D7E1, 0x003D800, 0x003D821, 0x003D840,
		0x003D861, 0x003D880, 0x003D8A1, 0x003D8C0, 0x003D8E1, 0x003D900, 0x003D921, 0x003D940,
		0x003D961, 0x003D980, 0x003D9A1, 0x003D9C0, 0x003D9E1, 0x003DA00, 0x003DA21, 0x003DA40,
		0x003DA61, 0x003DA80, 0x003DAA1, 0x003DAC0, 0x003DAE1, 0x003DB00, 0x003DB21, 0x003DB40,
		0x003DB61, 0x003DB80, 0x003DBA1, 0x003DBC0, 0x003DBE1, 0x003DC00, 0x003DC21, 0x003DC40,
		0x003DC61, 0x003DC80, 0x003DCA1, 0x003DCC0, 0x003DCE1, 0x003DD00, 0x003DD21, 0x003DD40,
		0x003DD61, 0x003DD80, 0x003DDA1, 0x003DDC0, 0x003DDE1, 0x003DE00, 0x003DE21, 0x003DE40,
		0x003DE61, 0x003DE80, 0x003DEA1, 0x003DEC0, 0x003DEE1, 0x003DF00, 0x003DF21, 0x003DF40,
		0x003DF61, 0x003DF80, 0x003DFA1, 0x003DFC0, 0x003DFE1, 0x003E100, 0x003E201, 0x003E2DD,
		0x003E300, 0x003E3DD, 0x003E401, 0x003E500, 0x003E601, 0x003E700, 0x003E801, 0x003E8DD,
		0x003E900, 0x003E9DD, 0x003EA01, 0x003EB1D, 0x003EB20, 0x003EB5D, 0x003EB60, 0x003EB9D,
		0x003EBA0, 0x003EBDD, 0x003EBE0, 0x003EC01, 0x003ED00, 0x003EE01, 0x003EFDD, 0x003F001,
		0x003F102, 0x003F201, 0x003F302, 0x003F401, 0x003F502, 0x003F601, 0x003F6BD, 0x003F6C1,
		0x003F700, 0x003F782, 0x003F7B4, 0x003F7C1, 0x003F7F4, 0x003F841, 0x003F8BD, 0x003F8C1,
		0x003F900, 0x003F982, 0x003F9B4, 0x003FA01, 0x003FA9D, 0x003FAC1, 0x003FB00, 0x003FB9D,
		0x003FBB4, 0x003FC01, 0x003FD00, 0x003FDB4, 0x003FE1D, 0x003FE41, 0x003FEBD, 0x003FEC1,
		0x003FF00, 0x003FF82, 0x003FFB4, 0x003FFFD, 0x0040016, 0x004017A, 0x004020C, 0x00402D1,
		0x004030F, 0x0040330, 0x004034D, 0x004036F, 0x00403B0, 0x00403CD, 0x00403EF, 0x0040411,
		0x0040517, 0x0040538, 0x004055A, 0x00405F6, 0x0040611, 0x004072F, 0x0040750, 0x0040771,
		0x00407EB, 0x0040831, 0x0040892, 0x00408AD, 0x00408CE, 0x00408F1, 0x0040A52, 0x0040A71,
		0x0040A8B, 0x0040AB1, 0x0040BF6, 0x0040C1A, 0x0040CBD, 0x0040CDA, 0x0040E0A, 0x0040E23,
		0x0040E5D, 0x0040E8A, 0x0040F52, 0x0040FAD, 0x0040FCE, 0x0040FE3, 0x004100A, 0x0041152,
		0x00411AD, 0x00411CE, 0x00411FD, 0x0041203, 0x00413BD, 0x0041413, 0x004183D, 0x0041A05,
		0x0041BA7, 0x0041C25, 0x0041C47, 0x0041CA5, 0x0041E3D, 0x0042015, 0x0042040, 0x0042075,
		0x00420E0, 0x0042115, 0x0042141, 0x0042160, 0x00421C1, 0x0042200, 0x0042261, 0x0042295,
		0x00422A0, 0x00422D5, 0x0042312, 0x0042320, 0x00423D5, 0x0042480, 0x00424B5, 0x00424C0,
		0x00424F5, 0x0042500, 0x0042535, 0x0042540, 0x00425D5, 0x00425E1, 0x0042600, 0x0042681,
		0x00426A4, 0x0042721, 0x0042755, 0x0042781, 0x00427C0, 0x0042812, 0x00428A0, 0x00428C1,
		0x0042955, 0x0042972, 0x0042995, 0x00429C1, 0x00429F5, 0x0042A0A, 0x0042C09, 0x0043060,
		0x0043081, 0x00430A9, 0x004312A, 0x0043155, 0x004319D, 0x0043212, 0x00432B5, 0x0043352,
		0x0043395, 0x0043412, 0x0043435, 0x0043472, 0x0043495, 0x00434D2, 0x00434F5, 0x00435D2,
		0x00435F5, 0x00439D2, 0x0043A15, 0x0043A52, 0x0043A75, 0x0043A92, 0x0043AB5, 0x0043E92,
		0x0046015, 0x004610D, 0x004612E, 0x004614D, 0x004616E, 0x0046195, 0x0046412, 0x0046455,
		0x004652D, 0x004654E, 0x0046575, 0x0046F92, 0x0046FB5, 0x0047372, 0x0047695, 0x0047B92,
		0x0047C55, 0x004855D, 0x0048815, 0x004897D, 0x0048C0A, 0x0049395, 0x0049D4A, 0x004A015,
		0x004B6F2, 0x004B715, 0x004B832, 0x004B855, 0x004BF12, 0x004C015, 0x004CDF2, 0x004CE15,
		0x004ED0D, 0x004ED2E, 0x004ED4D, 0x004ED6E, 0x004ED8D, 0x004EDAE, 0x004EDCD, 0x004EDEE,
		0x004EE0D, 0x004EE2E, 0x004EE4D, 0x004EE6E, 0x004EE8D, 0x004EEAE, 0x004EECA, 0x004F295,
		0x004F812, 0x004F8AD, 0x004F8CE, 0x004F8F2, 0x004FCCD, 0x004FCEE, 0x004FD0D, 0x004FD2E,
		0x004FD4D, 0x004FD6E, 0x004FD8D, 0x004FDAE, 0x004FDCD, 0x004FDEE, 0x004FE12, 0x0050015,
		0x0052012, 0x005306D, 0x005308E, 0x00530AD, 0x00530CE, 0x00530ED, 0x005310E, 0x005312D,
		0x005314E, 0x005316D, 0x005318E, 0x00531AD, 0x00531CE, 0x00531ED, 0x005320E, 0x005322D,
		0x005324E, 0x005326D, 0x005328E, 0x00532AD, 0x00532CE, 0x00532ED, 0x005330E, 0x0053332,
		0x0053B0D, 0x0053B2E, 0x0053B4D, 0x0053B6E, 0x0053B92, 0x0053F8D, 0x0053FAE, 0x0053FD2,
		0x0056015, 0x0056612, 0x00568B5, 0x00568F2, 0x00569B5, 0x0056E9D, 0x0056ED5, 0x00572DD,
		0x00572F5, 0x0058000, 0x0058601, 0x0058C00, 0x0058C21, 0x0058C40, 0x0058CA1, 0x0058CE0,
		0x0058D01, 0x0058D20, 0x0058D41, 0x0058D60, 0x0058D81, 0x0058DA0, 0x0058E21, 0x0058E40,
		0x0058E61, 0x0058EA0, 0x0058EC1, 0x0058F83, 0x0058FC0, 0x0059021, 0x0059040, 0x0059061,
		0x0059080, 0x00590A1, 0x00590C0, 0x00590E1, 0x0059100, 0x0059121, 0x0059140, 0x0059161,
		0x0059180, 0x00591A1, 0x00591C0, 0x00591E1, 0x0059200, 0x0059221, 0x0059240, 0x0059261,
		0x0059280, 0x00592A1, 0x00592C0, 0x00592E1, 0x0059300, 0x0059321, 0x0059340, 0x0059361,
		0x0059380, 0x00593A1, 0x00593C0, 0x00593E1, 0x0059400, 0x0059421, 0x0059440, 0x0059461,
		0x0059480, 0x00594A1, 0x00594C0, 0x00594E1, 0x0059500, 0x0059521, 0x0059540, 0x0059561,
		0x0059580, 0x00595A1, 0x00595C0, 0x00595E1, 0x0059600, 0x0059621, 0x0059640, 0x0059661,
		0x0059680, 0x00596A1, 0x00596C0, 0x00596E1, 0x0059700, 0x0059721, 0x0059740, 0x0059761,
		0x0059780, 0x00597A1, 0x00597C0, 0x00597E1, 0x0059800, 0x0059821, 0x0059840, 0x0059861,
		0x0059880, 0x00598A1, 0x00598C0, 0x00598E1, 0x0059900, 0x0059921, 0x0059940, 0x0059961,
		0x0059980, 0x00599A1, 0x00599C0, 0x00599E1, 0x0059A00, 0x0059A21, 0x0059A40, 0x0059A61,
		0x0059A80, 0x0059AA1, 0x0059AC0, 0x0059AE1, 0x0059B00, 0x0059B21, 0x0059B40, 0x0059B61,
		0x0059B80, 0x0059BA1, 0x0059BC0, 0x0059BE1, 0x0059C00, 0x0059C21, 0x0059C40, 0x0059C61,
		0x0059CB5, 0x0059D60, 0x0059D81, 0x0059DA0, 0x0059DC1, 0x0059DE5, 0x0059E40, 0x0059E61,
		0x0059E9D, 0x0059F31, 0x0059FAA, 0x0059FD1, 0x005A001, 0x005A4DD, 0x005A4E1, 0x005A51D,
		0x005A5A1, 0x005A5DD, 0x005A604, 0x005AD1D, 0x005ADE3, 0x005AE11, 0x005AE3D, 0x005AFE5,
		0x005B004, 0x005B2FD, 0x005B404, 0x005B4FD, 0x005B504, 0x005B5FD, 0x005B604, 0x005B6FD,
		0x005B704, 0x005B7FD, 0x005B804, 0x005B8FD, 0x005B904, 0x005B9FD, 0x005BA04, 0x005BAFD,
		0x005BB04, 0x005BBFD, 0x005BC05, 0x005C011, 0x005C04F, 0x005C070, 0x005C08F, 0x005C0B0,
		0x005C0D1, 0x005C12F, 0x005C150, 0x005C171, 0x005C18F, 0x005C1B0, 0x005C1D1, 0x005C2EC,
		0x005C311, 0x005C34C, 0x005C371, 0x005C38F, 0x005C3B0, 0x005C3D1, 0x005C40F, 0x005C430,
		0x005C44D, 0x005C46E, 0x005C48D, 0x005C4AE, 0x005C4CD, 0x005C4EE, 0x005C50D, 0x005C52E,
		0x005C551, 0x005C5E3, 0x005C611, 0x005C74C, 0x005C791, 0x005C80C, 0x005C831, 0x005C84D,
		0x005C871, 0x005CA15, 0x005CA51, 0x005CAAD, 0x005CACE, 0x005CAED, 0x005CB0E, 0x005CB2D,
		0x005CB4E, 0x005CB6D, 0x005CB8E, 0x005CBAC, 0x005CBDD, 0x005D015, 0x005D35D, 0x005D375,
		0x005DE9D, 0x005E015, 0x005FADD, 0x005FE15, 0x0060016, 0x0060031, 0x0060095, 0x00600A3,
		0x00600C4, 0x00600E9, 0x006010D, 0x006012E, 0x006014D, 0x006016E, 0x006018D, 0x00601AE,
		0x00601CD, 0x00601EE, 0x006020D, 0x006022E, 0x0060255, 0x006028D, 0x00602AE, 0x00602CD,
		0x00602EE, 0x006030D, 0x006032E, 0x006034D, 0x006036E, 0x006038C, 0x00603AD, 0x00603CE,
		0x0060415, 0x0060429, 0x0060545, 0x00605C6, 0x006060C, 0x0060623, 0x00606D5, 0x0060709,
		0x0060763, 0x0060784, 0x00607B1, 0x00607D5, 0x006081D, 0x0060824, 0x00612FD, 0x0061325,
		0x0061374, 0x00613A3, 0x00613E4, 0x006140C, 0x0061424, 0x0061F71, 0x0061F83, 0x0061FE4,
		0x006201D, 0x00620A4, 0x006261D, 0x0062624, 0x00631FD, 0x0063215, 0x006324A, 0x00632D5,
		0x0063404, 0x0063815, 0x0063CDD, 0x0063DF5, 0x0063E04, 0x0064015, 0x00643FD, 0x006440A,
		0x0064555, 0x006490A, 0x0064A15, 0x0064A2A, 0x0064C15, 0x006500A, 0x0065155, 0x006562A,
		0x0065815, 0x0068004, 0x009B815, 0x009C004, 0x01402A3, 0x01402C4, 0x01491BD, 0x0149215,
		0x01498FD, 0x0149A04, 0x0149F03, 0x0149FD1, 0x014A004, 0x014C183, 0x014C1B1, 0x014C204,
		0x014C408, 0x014C544, 0x014C59D, 0x014C800, 0x014C821, 0x014C840, 0x014C861, 0x014C880,
		0x014C8A1, 0x014C8C0, 0x014C8E1, 0x014C900, 0x014C921, 0x014C940, 0x014C961, 0x014C980,
		0x014C9A1, 0x014C9C0, 0x014C9E1, 0x014CA00, 0x014CA21, 0x014CA40, 0x014CA61, 0x014CA80,
		0x014CAA1, 0x014CAC0, 0x014CAE1, 0x014CB00, 0x014CB21, 0x014CB40, 0x014CB61, 0x014CB80,
		0x014CBA1, 0x014CBC0, 0x014CBE1, 0x014CC00, 0x014CC21, 0x014CC40, 0x014CC61, 0x014CC80,
		0x014CCA1, 0x014CCC0, 0x014CCE1, 0x014CD00, 0x014CD21, 0x014CD40, 0x014CD61, 0x014CD80,
		0x014CDA1, 0x014CDC4, 0x014CDE5, 0x014CE07, 0x014CE71, 0x014CE85, 0x014CFD1, 0x014CFE3,
		0x014D000, 0x014D021, 0x014D040, 0x014D061, 0x014D080, 0x014D0A1, 0x014D0C0, 0x014D0E1,
		0x014D100, 0x014D121, 0x014D140, 0x014D161, 0x014D180, 0x014D1A1, 0x014D1C0, 0x014D1E1,
		0x014D200, 0x014D221, 0x014D240, 0x014D261, 0x014D280, 0x014D2A1, 0x014D2C0, 0x014D2E1,
		0x014D300, 0x014D321, 0x014D340, 0x014D361, 0x014D383, 0x014D3C5, 0x014D404, 0x014DCC9,
		0x014DE05, 0x014DE51, 0x014DF1D, 0x014E014, 0x014E2E3, 0x014E414, 0x014E440, 0x014E461,
		0x014E480, 0x014E4A1, 0x014E4C0, 0x014E4E1, 0x014E500, 0x014E521, 0x014E540, 0x014E561,
		0x014E580, 0x014E5A1, 0x014E5C0, 0x014E5E1, 0x014E640, 0x014E661, 0x014E680, 0x014E6A1,
		0x014E6C0, 0x014E6E1, 0x014E700, 0x014E721, 0x014E740, 0x014E761, 0x014E780, 0x014E7A1,
		0x014E7C0, 0x014E7E1, 0x014E800, 0x014E821, 0x014E840, 0x014E861, 0x014E880, 0x014E8A1,
		0x014E8C0, 0x014E8E1, 0x014E900, 0x014E921, 0x014E940, 0x014E961, 0x014E980, 0x014E9A1,
		0x014E9C0, 0x014E9E1, 0x014EA00, 0x014EA21, 0x014EA40, 0x014EA61, 0x014EA80, 0x014EAA1,
		0x014EAC0, 0x014EAE1, 0x014EB00, 0x014EB21, 0x014EB40, 0x014EB61, 0x014EB80, 0x014EBA1,
		0x014EBC0, 0x014EBE1, 0x014EC00, 0x014EC21, 0x014EC40, 0x014EC61, 0x014EC80, 0x014ECA1,
		0x014ECC0, 0x014ECE1, 0x014ED00, 0x014ED21, 0x014ED40, 0x014ED61, 0x014ED80, 0x014EDA1,
		0x014EDC0, 0x014EDE1, 0x014EE03, 0x014EE21, 0x014EF20, 0x014EF41, 0x014EF60, 0x014EF81,
		0x014EFA0, 0x014EFE1, 0x014F000, 0x014F021, 0x014F040, 0x014F061, 0x014F080, 0x014F0A1,
		0x014F0C0, 0x014F0E1, 0x014F103, 0x014F134, 0x014F160, 0x014F181, 0x014F1A0, 0x014F1C1,
		0x014F1E4, 0x014F200, 0x014F221, 0x014F240, 0x014F261, 0x014F2C0, 0x014F2E1, 0x014F300,
		0x014F321, 0x014F340, 0x014F361, 0x014F380, 0x014F3A1, 0x014F3C0, 0x014F3E1, 0x014F400,
		0x014F421, 0x014F440, 0x014F461, 0x014F480, 0x014F4A1, 0x014F4C0, 0x014F4E1, 0x014F500,
		0x014F521, 0x014F540, 0x014F5E1, 0x014F600, 0x014F6A1, 0x014F6C0, 0x014F6E1, 0x014F700,
		0x014F721, 0x014F740, 0x014F761, 0x014F780, 0x014F7A1, 0x014F7C0, 0x014F7E1, 0x014F800,
		0x014F821, 0x014F840, 0x014F861, 0x014F880, 0x014F901, 0x014F920, 0x014F941, 0x014F960,
		0x014F9A1, 0x014F9DD, 0x014FA00, 0x014FA21, 0x014FA5D, 0x014FA61, 0x014FA9D, 0x014FAA1,
		0x014FAC0, 0x014FAE1, 0x014FB00, 0x014FB21, 0x014FB40, 0x014FB61, 0x014FB80, 0x014FBBD,
		0x014FE43, 0x014FEA0, 0x014FEC1, 0x014FEE4, 0x014FF03, 0x014FF41, 0x014FF64, 0x0150045,
		0x0150064, 0x01500C5, 0x01500E4, 0x0150165, 0x0150184, 0x0150466, 0x01504A5, 0x01504E6,
		0x0150515, 0x0150585, 0x01505BD, 0x015060A, 0x01506D5, 0x0150713, 0x0150735, 0x015075D,
		0x0150804, 0x0150E91, 0x0150F1D, 0x0151006, 0x0151044, 0x0151686, 0x0151885, 0x01518DD,
		0x01519D1, 0x0151A08, 0x0151B5D, 0x0151C05, 0x0151E44, 0x0151F11, 0x0151F64, 0x0151F91,
		0x0151FA4, 0x0151FE5, 0x0152008, 0x0152144, 0x01524C5, 0x01525D1, 0x0152604, 0x01528E5,
		0x0152A46, 0x0152A9D, 0x0152BF1, 0x0152C04, 0x0152FBD, 0x0153005, 0x0153066, 0x0153084,
		0x0153665, 0x0153686, 0x01536C5, 0x0153746, 0x0153785, 0x01537C6, 0x0153831, 0x01539DD,
		0x01539E3, 0x0153A08, 0x0153B5D, 0x0153BD1, 0x0153C04, 0x0153CA5, 0x0153CC3, 0x0153CE4,
		0x0153E08, 0x0153F44, 0x0153FFD, 0x0154004, 0x0154525, 0x01545E6, 0x0154625, 0x0154666,
		0x01546A5, 0x01546FD, 0x0154804, 0x0154865, 0x0154884, 0x0154985, 0x01549A6, 0x01549DD,
		0x0154A08, 0x0154B5D, 0x0154B91, 0x0154C04, 0x0154E03, 0x0154E24, 0x0154EF5, 0x0154F44,
		0x0154F66, 0x0154F85, 0x0154FA6, 0x0154FC4, 0x0155605, 0x0155624, 0x0155645, 0x01556A4,
		0x01556E5, 0x0155724, 0x01557C5, 0x0155804, 0x0155825, 0x0155844, 0x015587D, 0x0155B64,
		0x0155BA3, 0x0155BD1, 0x0155C04, 0x0155D66, 0x0155D85, 0x0155DC6, 0x0155E11, 0x0155E44,
		0x0155E63, 0x0155EA6, 0x0155EC5, 0x0155EFD, 0x0156024, 0x01560FD, 0x0156124, 0x01561FD,
		0x0156224, 0x01562FD, 0x0156404, 0x01564FD, 0x0156504, 0x01565FD, 0x0156601, 0x0156B74,
		0x0156B83, 0x0156C01, 0x0156D23, 0x0156D54, 0x0156D9D, 0x0156E01, 0x0157804, 0x0157C66,
		0x0157CA5, 0x0157CC6, 0x0157D05, 0x0157D26, 0x0157D71, 0x0157D86, 0x0157DA5, 0x0157DDD,
		0x0157E08, 0x0157F5D, 0x0158004, 0x01AF49D, 0x01AF604, 0x01AF8FD, 0x01AF964, 0x01AFF9D,
		0x01B001B, 0x01C001C, 0x01F2004, 0x01F4DDD, 0x01F4E04, 0x01F5B5D, 0x01F6001, 0x01F60FD,
		0x01F6261, 0x01F631D, 0x01F63A4, 0x01F63C5, 0x01F63E4, 0x01F6532, 0x01F6544, 0x01F66FD,
		0x01F6704, 0x01F67BD, 0x01F67C4, 0x01F67FD, 0x01F6804, 0x01F685D, 0x01F6864, 0x01F68BD,
		0x01F68C4, 0x01F7654, 0x01F787D, 0x01F7A64, 0x01FA7CE, 0x01FA7ED, 0x01FA815, 0x01FAA04,
		0x01FB21D, 0x01FB244, 0x01FB91D, 0x01FB9F5, 0x01FBA1D, 0x01FBE04, 0x01FBF93, 0x01FBFB5,
		0x01FC005, 0x01FC211, 0x01FC2ED, 0x01FC30E, 0x01FC331, 0x01FC35D, 0x01FC405, 0x01FC611,
		0x01FC62C, 0x01FC66B, 0x01FC6AD, 0x01FC6CE, 0x01FC6ED, 0x01FC70E, 0x01FC72D, 0x01FC74E,
		0x01FC76D, 0x01FC78E, 0x01FC7AD, 0x01FC7CE, 0x01FC7ED, 0x01FC80E, 0x01FC82D, 0x01FC84E,
		0x01FC86D, 0x01FC88E, 0x01FC8B1, 0x01FC8ED, 0x01FC90E, 0x01FC931, 0x01FC9AB, 0x01FCA11,
		0x01FCA7D, 0x01FCA91, 0x01FCB0C, 0x01FCB2D, 0x01FCB4E, 0x01FCB6D, 0x01FCB8E, 0x01FCBAD,
		0x01FCBCE, 0x01FCBF1, 0x01FCC52, 0x01FCC6C, 0x01FCC92, 0x01FCCFD, 0x01FCD11, 0x01FCD33,
		0x01FCD51, 0x01FCD9D, 0x01FCE04, 0x01FCEBD, 0x01FCEC4, 0x01FDFBD, 0x01FDFFA, 0x01FE01D,
		0x01FE031, 0x01FE093, 0x01FE0B1, 0x01FE10D, 0x01FE12E, 0x01FE151, 0x01FE172, 0x01FE191,
		0x01FE1AC, 0x01FE1D1, 0x01FE208, 0x01FE351, 0x01FE392, 0x01FE3F1, 0x01FE420, 0x01FE76D,
		0x01FE791, 0x01FE7AE, 0x01FE7D4, 0x01FE7EB, 0x01FE814, 0x01FE821, 0x01FEB6D, 0x01FEB92,
		0x01FEBAE, 0x01FEBD2, 0x01FEBED, 0x01FEC0E, 0x01FEC31, 0x01FEC4D, 0x01FEC6E, 0x01FEC91,
		0x01FECC4, 0x01FEE03, 0x01FEE24, 0x01FF3C3, 0x01FF404, 0x01FF7FD, 0x01FF844, 0x01FF91D,
		0x01FF944, 0x01FFA1D, 0x01FFA44, 0x01FFB1D, 0x01FFB44, 0x01FFBBD, 0x01FFC13, 0x01FFC52,
		0x01FFC74, 0x01FFC95, 0x01FFCB3, 0x01FFCFD, 0x01FFD15, 0x01FFD32, 0x01FFDB5, 0x01FFDFD,
		0x01FFF3A, 0x01FFF95, 0x01FFFDD, 0x0200004, 0x020019D, 0x02001A4, 0x02004FD, 0x0200504,
		0x020077D, 0x0200784, 0x02007DD, 0x02007E4, 0x02009DD, 0x0200A04, 0x0200BDD, 0x0201004,
		0x0201F7D, 0x0202011, 0x020207D, 0x02020EA, 0x020269D, 0x02026F5, 0x0202809, 0x0202EAA,
		0x0202F35, 0x020314A, 0x0203195, 0x02031FD, 0x0203215, 0x02033BD, 0x0203415, 0x020343D,
		0x0203A15, 0x0203FA5, 0x0203FDD, 0x0205004, 0x02053BD, 0x0205404, 0x0205A3D, 0x0205C05,
		0x0205C2A, 0x0205F9D, 0x0206004, 0x020640A, 0x020649D, 0x02065A4, 0x0206829, 0x0206844,
		0x0206949, 0x020697D, 0x0206A04, 0x0206EC5, 0x0206F7D, 0x0207004, 0x02073DD, 0x02073F1,
		0x0207404, 0x020789D, 0x0207904, 0x0207A11, 0x0207A29, 0x0207ADD, 0x0208000, 0x0208501,
		0x0208A04, 0x02093DD, 0x0209408, 0x020955D, 0x0209600, 0x0209A9D, 0x0209B01, 0x0209F9D,
		0x020A004, 0x020A51D, 0x020A604, 0x020AC9D, 0x020ADF1, 0x020AE00, 0x020AF7D, 0x020AF80,
		0x020B17D, 0x020B180, 0x020B27D, 0x020B280, 0x020B2DD, 0x020B2E1, 0x020B45D, 0x020B461,
		0x020B65D, 0x020B661, 0x020B75D, 0x020B761, 0x020B7BD, 0x020B804, 0x020BE9D, 0x020C004,
		0x020E6FD, 0x020E804, 0x020EADD, 0x020EC04, 0x020ED1D, 0x020F003, 0x020F0DD, 0x020F0E3,
		0x020F63D, 0x020F643, 0x020F77D, 0x0210004, 0x02100DD, 0x0210104, 0x021013D, 0x0210144,
		0x02106DD, 0x02106E4, 0x021073D, 0x0210784, 0x02107BD, 0x02107E4, 0x0210ADD, 0x0210AF1,
		0x0210B0A, 0x0210C04, 0x0210EF5, 0x0210F2A, 0x0211004, 0x02113FD, 0x02114EA, 0x021161D,
		0x0211C04, 0x0211E7D, 0x0211E84, 0x0211EDD, 0x0211F6A, 0x0212004, 0x02122CA, 0x021239D,
		0x02123F1, 0x0212404, 0x021275D, 0x02127F1, 0x021281D, 0x0213004, 0x021371D, 0x021378A,
		0x02137C4, 0x021380A, 0x0213A1D, 0x0213A4A, 0x0214004, 0x0214025, 0x021409D, 0x02140A5,
		0x02140FD, 0x0214185, 0x0214204, 0x021429D, 0x02142A4, 0x021431D, 0x0214324, 0x02146DD,
		0x0214705, 0x021477D, 0x02147E5, 0x021480A, 0x021493D, 0x0214A11, 0x0214B3D, 0x0214C04,
		0x0214FAA, 0x0214FF1, 0x0215004, 0x02153AA, 0x021541D, 0x0215804, 0x0215915, 0x0215924,
		0x0215CA5, 0x0215CFD, 0x0215D6A, 0x0215E11, 0x0215EFD, 0x0216004, 0x02166DD, 0x0216731,
		0x0216804, 0x0216ADD, 0x0216B0A, 0x0216C04, 0x0216E7D, 0x0216F0A, 0x0217004, 0x021725D,
		0x0217331, 0x02173BD, 0x021752A, 0x021761D, 0x0218004, 0x021893D, 0x0219000, 0x021967D,
		0x0219801, 0x0219E7D, 0x0219F4A, 0x021A004, 0x021A485, 0x021A51D, 0x021A608, 0x021A75D,
		0x021A808, 0x021A944, 0x021A9C3, 0x021A9E4, 0x021AA00, 0x021ACDD, 0x021AD25, 0x021ADCC,
		0x021ADE3, 0x021AE01, 0x021B0DD, 0x021B1D2, 0x021B21D, 0x021CC0A, 0x021CFFD, 0x021D004,
		0x021D55D, 0x021D565, 0x021D5AC, 0x021D5DD, 0x021D604, 0x021D65D, 0x021D844, 0x021D8BD,
		0x021DF85, 0x021E004, 0x021E3AA, 0x021E4E4, 0x021E51D, 0x021E604, 0x021E8C5, 0x021EA2A,
		0x021EAB1, 0x021EB5D, 0x021EE04, 0x021F045, 0x021F0D1, 0x021F15D, 0x021F604, 0x021F8AA,
		0x021F99D, 0x021FC04, 0x021FEFD, 0x0220006, 0x0220025, 0x0220046, 0x0220064, 0x0220705,
		0x02208F1, 0x02209DD, 0x0220A4A, 0x0220CC8, 0x0220E05, 0x0220E24, 0x0220E65, 0x0220EA4,
		0x0220EDD, 0x0220FE5, 0x0221046, 0x0221064, 0x0221606, 0x0221665, 0x02216E6, 0x0221725,
		0x0221771, 0x02217BA, 0x02217D1, 0x0221845, 0x022187D, 0x02219BA, 0x02219DD, 0x0221A04,
		0x0221D3D, 0x0221E08, 0x0221F5D, 0x0222005, 0x0222064, 0x02224E5, 0x0222586, 0x02225A5,
		0x02226BD, 0x02226C8, 0x0222811, 0x0222884, 0x02228A6, 0x02228E4, 0x022291D, 0x0222A04,
		0x0222E65, 0x0222E91, 0x0222EC4, 0x0222EFD, 0x0223005, 0x0223046, 0x0223064, 0x0223666,
		0x02236C5, 0x02237E6, 0x0223824, 0x02238B1, 0x0223925, 0x02239B1, 0x02239C6, 0x02239E5,
		0x0223A08, 0x0223B44, 0x0223B71, 0x0223B84, 0x0223BB1, 0x0223C1D, 0x0223C2A, 0x0223EBD,
		0x0224004, 0x022425D, 0x0224264, 0x0224586, 0x02245E5, 0x0224646, 0x0224685, 0x02246A6,
		0x02246C5, 0x0224711, 0x02247C5, 0x02247E4, 0x0224825, 0x022485D, 0x0225004, 0x02250FD,
		0x0225104, 0x022513D, 0x0225144, 0x02251DD, 0x02251E4, 0x02253DD, 0x02253E4, 0x0225531,
		0x022555D, 0x0225604, 0x0225BE5, 0x0225C06, 0x0225C65, 0x0225D7D, 0x0225E08, 0x0225F5D,
		0x0226005, 0x0226046, 0x022609D, 0x02260A4, 0x02261BD, 0x02261E4, 0x022623D, 0x0226264,
		0x022653D, 0x0226544, 0x022663D, 0x0226644, 0x022669D, 0x02266A4, 0x022675D, 0x0226765,
		0x02267A4, 0x02267C6, 0x0226805, 0x0226826, 0x02268BD, 0x02268E6, 0x022693D, 0x0226966,
		0x02269DD, 0x0226A04, 0x0226A3D, 0x0226AE6, 0x0226B1D, 0x0226BA4, 0x0226C46, 0x0226C9D,
		0x0226CC5, 0x0226DBD, 0x0226E05, 0x0226EBD, 0x0227004, 0x022715D, 0x0227164, 0x022719D,
		0x02271C4, 0x02271FD, 0x0227204, 0x02276DD, 0x02276E4, 0x0227706, 0x0227765, 0x022783D,
		0x0227846, 0x022787D, 0x02278A6, 0x02278DD, 0x02278E6, 0x022797D, 0x0227986, 0x02279C5,
		0x02279E6, 0x0227A05, 0x0227A24, 0x0227A45, 0x0227A64, 0x0227A91, 0x0227ADD, 0x0227AF1,
		0x0227B3D, 0x0227C25, 0x0227C7D, 0x0228004, 0x02286A6, 0x0228705, 0x0228806, 0x0228845,
		0x02288A6, 0x02288C5, 0x02288E4, 0x0228971, 0x0228A08, 0x0228B51, 0x0228B9D, 0x0228BB1,
		0x0228BC5, 0x0228BE4, 0x0228C5D, 0x0229004, 0x0229606, 0x0229665, 0x0229726, 0x0229745,
		0x0229766, 0x02297E5, 0x0229826, 0x0229845, 0x0229884, 0x02298D1, 0x02298E4, 0x022991D,
		0x0229A08, 0x0229B5D, 0x022B004, 0x022B5E6, 0x022B645, 0x022B6DD, 0x022B706, 0x022B785,
		0x022B7C6, 0x022B7E5, 0x022B831, 0x022BB04, 0x022BB85, 0x022BBDD, 0x022C004, 0x022C606,
		0x022C665, 0x022C766, 0x022C7A5, 0x022C7C6, 0x022C7E5, 0x022C831, 0x022C884, 0x022C8BD,
		0x022CA08, 0x022CB5D, 0x022CC11, 0x022CDBD, 0x022D004, 0x022D565, 0x022D586, 0x022D5A5,
		0x022D5C6, 0x022D605, 0x022D6C6, 0x022D6E5, 0x022D704, 0x022D731, 0x022D75D, 0x022D808,
		0x022D95D, 0x022DA08, 0x022DC9D, 0x022E004, 0x022E37D, 0x022E3A5, 0x022E3C6, 0x022E3E5,
		0x022E406, 0x022E445, 0x022E4C6, 0x022E4E5, 0x022E59D, 0x022E608, 0x022E74A, 0x022E791,
		0x022E7F5, 0x022E804, 0x022E8FD, 0x0230004, 0x0230586, 0x02305E5, 0x0230706, 0x0230725,
		0x0230771, 0x023079D, 0x0231400, 0x0231801, 0x0231C08, 0x0231D4A, 0x0231E7D, 0x0231FE4,
		0x02320FD, 0x0232124, 0x023215D, 0x0232184, 0x023229D, 0x02322A4, 0x02322FD, 0x0232304,
		0x0232606, 0x02326DD, 0x02326E6, 0x023273D, 0x0232765, 0x02327A6, 0x02327C5, 0x02327E4,
		0x0232806, 0x0232824, 0x0232846, 0x0232865, 0x0232891, 0x02328FD, 0x0232A08, 0x0232B5D,
		0x0233404, 0x023351D, 0x0233544, 0x0233A26, 0x0233A85, 0x0233B1D, 0x0233B45, 0x0233B86,
		0x0233C05, 0x0233C24, 0x0233C51, 0x0233C64, 0x0233C86, 0x0233CBD, 0x0234004, 0x0234025,
		0x0234164, 0x0234665, 0x0234726, 0x0234744, 0x0234765, 0x02347F1, 0x02348E5, 0x023491D,
		0x0234A04, 0x0234A25, 0x0234AE6, 0x0234B25, 0x0234B84, 0x0235145, 0x02352E6, 0x0235305,
		0x0235351, 0x02353A4, 0x02353D1, 0x023547D, 0x0235604, 0x0235F3D, 0x0236011, 0x023615D,
		0x0237804, 0x0237C31, 0x0237C5D, 0x0237E08, 0x0237F5D, 0x0238004, 0x023813D, 0x0238144,
		0x02385E6, 0x0238605, 0x02386FD, 0x0238705, 0x02387C6, 0x02387E5, 0x0238804, 0x0238831,
		0x02388DD, 0x0238A08, 0x0238B4A, 0x0238DBD, 0x0238E11, 0x0238E44, 0x023921D, 0x0239245,
		0x023951D, 0x0239526, 0x0239545, 0x0239626, 0x0239645, 0x0239686, 0x02396A5, 0x02396FD,
		0x023A004, 0x023A0FD, 0x023A104, 0x023A15D, 0x023A164, 0x023A625, 0x023A6FD, 0x023A745,
		0x023A77D, 0x023A785, 0x023A7DD, 0x023A7E5, 0x023A8C4, 0x023A8E5, 0x023A91D, 0x023AA08,
		0x023AB5D, 0x023AC04, 0x023ACDD, 0x023ACE4, 0x023AD3D, 0x023AD44, 0x023B146, 0x023B1FD,
		0x023B205, 0x023B25D, 0x023B266, 0x023B2A5, 0x023B2C6, 0x023B2E5, 0x023B304, 0x023B33D,
		0x023B408, 0x023B55D, 0x023DC04, 0x023DE65, 0x023DEA6, 0x023DEF1, 0x023DF3D, 0x023E005,
		0x023E044, 0x023E066, 0x023E084, 0x023E23D, 0x023E244, 0x023E686, 0x023E6C5, 0x023E77D,
		0x023E7C6, 0x023E805, 0x023E826, 0x023E845, 0x023E871, 0x023EA08, 0x023EB45, 0x023EB7D,
		0x023F604, 0x023F63D, 0x023F80A, 0x023FAB5, 0x023FBB3, 0x023FC35, 0x023FE5D, 0x023FFF1,
		0x0240004, 0x024735D, 0x0248009, 0x0248DFD, 0x0248E11, 0x0248EBD, 0x0249004, 0x024A89D,
		0x025F204, 0x025FE31, 0x025FE7D, 0x0260004, 0x026861A, 0x0268805, 0x0268824, 0x02688E5,
		0x0268ADD, 0x0268C04, 0x0287F7D, 0x0288004, 0x028C8FD, 0x02C2004, 0x02C23C5, 0x02C2546,
		0x02C25A5, 0x02C2608, 0x02C275D, 0x02D0004, 0x02D473D, 0x02D4804, 0x02D4BFD, 0x02D4C08,
		0x02D4D5D, 0x02D4DD1, 0x02D4E04, 0x02D57FD, 0x02D5808, 0x02D595D, 0x02D5A04, 0x02D5DDD,
		0x02D5E05, 0x02D5EB1, 0x02D5EDD, 0x02D6004, 0x02D6605, 0x02D66F1, 0x02D6795, 0x02D6803,
		0x02D6891, 0x02D68B5, 0x02D68DD, 0x02D6A08, 0x02D6B5D, 0x02D6B6A, 0x02D6C5D, 0x02D6C64,
		0x02D6F1D, 0x02D6FA4, 0x02D721D, 0x02DA803, 0x02DA864, 0x02DAD63, 0x02DADB1, 0x02DAE08,
		0x02DAF5D, 0x02DC800, 0x02DCC01, 0x02DD00A, 0x02DD2F1, 0x02DD37D, 0x02DE004, 0x02DE97D,
		0x02DE9E5, 0x02DEA04, 0x02DEA26, 0x02DF11D, 0x02DF1E5, 0x02DF263, 0x02DF41D, 0x02DFC03,
		0x02DFC51, 0x02DFC63, 0x02DFC85, 0x02DFCBD, 0x02DFE06, 0x02DFE5D, 0x02E0004, 0x030FF1D,
		0x0310004, 0x0319ADD, 0x0319FE4, 0x031A13D, 0x035FE03, 0x035FE9D, 0x035FEA3, 0x035FF9D,
		0x035FFA3, 0x035FFFD, 0x0360004, 0x036247D, 0x0362644, 0x036267D, 0x0362A04, 0x0362A7D,
		0x0362AA4, 0x0362ADD, 0x0362C84, 0x0362D1D, 0x0362E04, 0x0365F9D, 0x0378004, 0x0378D7D,
		0x0378E04, 0x0378FBD, 0x0379004, 0x037913D, 0x0379204, 0x037935D, 0x0379395, 0x03793A5,
		0x03793F1, 0x037941A, 0x037949D, 0x0398015, 0x0399E08, 0x0399F5D, 0x039A015, 0x039D69D,
		0x039E005, 0x039E5DD, 0x039E605, 0x039E8FD, 0x039EA15, 0x039F89D, 0x03A0015, 0x03A1EDD,
		0x03A2015, 0x03A24FD, 0x03A2535, 0x03A2CA6, 0x03A2CE5, 0x03A2D55, 0x03A2DA6, 0x03A2E7A,
		0x03A2F65, 0x03A3075, 0x03A30A5, 0x03A3195, 0x03A3545, 0x03A35D5, 0x03A3D7D, 0x03A4015,
		0x03A4845, 0x03A48B5, 0x03A48DD, 0x03A580A, 0x03A5A9D, 0x03A5C0A, 0x03A5E9D, 0x03A6015,
		0x03A6AFD, 0x03A6C0A, 0x03A6F3D, 0x03A8000, 0x03A8341, 0x03A8680, 0x03A89C1, 0x03A8ABD,
		0x03A8AC1, 0x03A8D00, 0x03A9041, 0x03A9380, 0x03A93BD, 0x03A93C0, 0x03A941D, 0x03A9440,
		0x03A947D, 0x03A94A0, 0x03A94FD, 0x03A9520, 0x03A95BD, 0x03A95C0, 0x03A96C1, 0x03A975D,
		0x03A9761, 0x03A979D, 0x03A97A1, 0x03A989D, 0x03A98A1, 0x03A9A00, 0x03A9D41, 0x03AA080,
		0x03AA0DD, 0x03AA0E0, 0x03AA17D, 0x03AA1A0, 0x03AA2BD, 0x03AA2C0, 0x03AA3BD, 0x03AA3C1,
		0x03AA700, 0x03AA75D, 0x03AA760, 0x03AA7FD, 0x03AA800, 0x03AA8BD, 0x03AA8C0, 0x03AA8FD,
		0x03AA940, 0x03AAA3D, 0x03AAA41, 0x03AAD80, 0x03AB0C1, 0x03AB400, 0x03AB741, 0x03ABA80,
		0x03ABDC1, 0x03AC100, 0x03AC441, 0x03AC780, 0x03ACAC1, 0x03ACE00, 0x03AD141, 0x03AD4DD,
		0x03AD500, 0x03AD832, 0x03AD841, 0x03ADB72, 0x03ADB81, 0x03ADC40, 0x03ADF72, 0x03ADF81,
		0x03AE2B2, 0x03AE2C1, 0x03AE380, 0x03AE6B2, 0x03AE6C1, 0x03AE9F2, 0x03AEA01, 0x03AEAC0,
		0x03AEDF2, 0x03AEE01, 0x03AF132, 0x03AF141, 0x03AF200, 0x03AF532, 0x03AF541, 0x03AF872,
		0x03AF881, 0x03AF940, 0x03AF961, 0x03AF99D, 0x03AF9C8, 0x03B0015, 0x03B4005, 0x03B46F5,
		0x03B4765, 0x03B4DB5, 0x03B4EA5, 0x03B4ED5, 0x03B5085, 0x03B50B5, 0x03B50F1, 0x03B519D,
		0x03B5365, 0x03B541D, 0x03B5425, 0x03B561D, 0x03BE001, 0x03BE144, 0x03BE161, 0x03BE3FD,
		0x03BE4A1, 0x03BE57D, 0x03C0005, 0x03C00FD, 0x03C0105, 0x03C033D, 0x03C0365, 0x03C045D,
		0x03C0465, 0x03C04BD, 0x03C04C5, 0x03C057D, 0x03C0603, 0x03C0DDD, 0x03C11E5, 0x03C121D,
		0x03C2004, 0x03C25BD, 0x03C2605, 0x03C26E3, 0x03C27DD, 0x03C2808, 0x03C295D, 0x03C29C4,
		0x03C29F5, 0x03C2A1D, 0x03C5204, 0x03C55C5, 0x03C55FD, 0x03C5804, 0x03C5D85, 0x03C5E08,
		0x03C5F5D, 0x03C5FF3, 0x03C601D, 0x03C9A04, 0x03C9D63, 0x03C9D85, 0x03C9E08, 0x03C9F5D,
		0x03CBA04, 0x03CBDC5, 0x03CBE04, 0x03CBE28, 0x03CBF7D, 0x03CBFF1, 0x03CC01D, 0x03CFC04,
		0x03CFCFD, 0x03CFD04, 0x03CFD9D, 0x03CFDA4, 0x03CFDFD, 0x03CFE04, 0x03CFFFD, 0x03D0004,
		0x03D18BD, 0x03D18EA, 0x03D1A05, 0x03D1AFD, 0x03D2000, 0x03D2441, 0x03D2885, 0x03D2963,
		0x03D299D, 0x03D2A08, 0x03D2B5D, 0x03D2BD1, 0x03D2C1D, 0x03D8E2A, 0x03D9595, 0x03D95AA,
		0x03D9613, 0x03D962A, 0x03D96BD, 0x03DA02A, 0x03DA5D5, 0x03DA5EA, 0x03DA7DD, 0x03DC004,
		0x03DC09D, 0x03DC0A4, 0x03DC41D, 0x03DC424, 0x03DC47D, 0x03DC484, 0x03DC4BD, 0x03DC4E4,
		0x03DC51D, 0x03DC524, 0x03DC67D, 0x03DC684, 0x03DC71D, 0x03DC724, 0x03DC75D, 0x03DC764,
		0x03DC79D, 0x03DC844, 0x03DC87D, 0x03DC8E4, 0x03DC91D, 0x03DC924, 0x03DC95D, 0x03DC964,
		0x03DC99D, 0x03DC9A4, 0x03DCA1D, 0x03DCA24, 0x03DCA7D, 0x03DCA84, 0x03DCABD, 0x03DCAE4,
		0x03DCB1D, 0x03DCB24, 0x03DCB5D, 0x03DCB64, 0x03DCB9D, 0x03DCBA4, 0x03DCBDD, 0x03DCBE4,
		0x03DCC1D, 0x03DCC24, 0x03DCC7D, 0x03DCC84, 0x03DCCBD, 0x03DCCE4, 0x03DCD7D, 0x03DCD84,
		0x03DCE7D, 0x03DCE84, 0x03DCF1D, 0x03DCF24, 0x03DCFBD, 0x03DCFC4, 0x03DCFFD, 0x03DD004,
		0x03DD15D, 0x03DD164, 0x03DD39D, 0x03DD424, 0x03DD49D, 0x03DD4A4, 0x03DD55D, 0x03DD564,
		0x03DD79D, 0x03DDE12, 0x03DDE5D, 0x03E0015, 0x03E059D, 0x03E0615, 0x03E129D, 0x03E1415,
		0x03E15FD, 0x03E1635, 0x03E181D, 0x03E1835, 0x03E1A1D, 0x03E1A35, 0x03E1EDD, 0x03E200A,
		0x03E21B5, 0x03E35DD, 0x03E3CD5, 0x03E407D, 0x03E4215, 0x03E479D, 0x03E4815, 0x03E493D,
		0x03E4A15, 0x03E4A5D, 0x03E4C15, 0x03E4CDD, 0x03E6015, 0x03E7F74, 0x03E8015, 0x03EDB1D,
		0x03EDB95, 0x03EDDBD, 0x03EDE15, 0x03EDFBD, 0x03EE015, 0x03EEEFD, 0x03EEF75, 0x03EFB5D,
		0x03EFC15, 0x03EFD9D, 0x03EFE15, 0x03EFE3D, 0x03F0015, 0x03F019D, 0x03F0215, 0x03F091D,
		0x03F0A15, 0x03F0B5D, 0x03F0C15, 0x03F111D, 0x03F1215, 0x03F15DD, 0x03F1615, 0x03F179D,
		0x03F1815, 0x03F185D, 0x03F2015, 0x03F4A9D, 0x03F4C15, 0x03F4DDD, 0x03F4E15, 0x03F4FBD,
		0x03F5015, 0x03F515D, 0x03F51F5, 0x03F58FD, 0x03F59D5, 0x03F5BBD, 0x03F5BF5, 0x03F5D5D,
		0x03F5E15, 0x03F5F3D, 0x03F6015, 0x03F727D, 0x03F7295, 0x03F7E08, 0x03F7F5D, 0x0400004,
		0x054DC1D, 0x054E004, 0x056E75D, 0x056E804, 0x05703DD, 0x0570404, 0x059D45D, 0x059D604,
		0x05D7C3D, 0x05D7E04, 0x05DCBDD, 0x05F0004, 0x05F43DD, 0x0600004, 0x062697D, 0x0626A04,
		0x064761D, 0x1C0003A, 0x1C0005D, 0x1C0041A, 0x1C0101D, 0x1C02005, 0x1C03E1D, 0x1E0001C,
		0x1FFFFDD, 0x200001C, 0x21FFFDD,
	};

	count = (int)(sizeof(runs) / sizeof(runs[0]));
	return runs;
}

inline int deelx_unicode_category(unsigned int cp)
{
	if(cp > 0x10FFFF)
		return DEELX_UC_Cn;

	int count = 0;
	const unsigned int * runs = deelx_unicode_category_runs(count);

	// the last run starting at or before cp; runs[0] starts at 0
	int lo = 0, hi = count - 1;

	while(lo < hi)
	{
		int mid = (lo + hi + 1) / 2;

		if((runs[mid] >> 5) <= cp)
			lo = mid;
		else
			hi = mid - 1;
	}

	return (int)(runs[lo] & 0x1F);
}

// Simple case folding (status C and S): first, last, delta, step. The code
// points first, first + step, ..., last fold to themselves + delta. No range
// spans a code point of another range.
inline const int * deelx_unicode_fold_ranges(int & count)
{
	static const int ranges[] =
	{
		0x00041, 0x0005A,     32, 1, 0x000B5, 0x000B5,    775, 1, 0x000C0, 0x000D6,     32, 1, 0x000D8, 0x000DE,     32, 1,
		0x00100, 0x0012E,      1, 2, 0x00132, 0x00136,      1, 2, 0x00139, 0x00147,      1, 2, 0x0014A, 0x00176,      1, 2,
		0x00178, 0x00178,   -121, 1, 0x00179, 0x0017D,      1, 2, 0x0017F, 0x0017F,   -268, 1, 0x00181, 0x00181,    210, 1,
		0x00182, 0x00184,      1, 2, 0x00186, 0x00186,    206, 1, 0x00187, 0x00187,      1, 1, 0x00189, 0x0018A,    205, 1,
		0x0018B, 0x0018B,      1, 1, 0x0018E, 0x0018E,     79, 1, 0x0018F, 0x0018F,    202, 1, 0x00190, 0x00190,    203, 1,
		0x00191, 0x00191,      1, 1, 0x00193, 0x00193,    205, 1, 0x00194, 0x00194,    207, 1, 0x00196, 0x00196,    211, 1,
		0x00197, 0x00197,    209, 1, 0x00198, 0x00198,      1, 1, 0x0019C, 0x0019C,    211, 1, 0x0019D, 0x0019D,    213, 1,
		0x0019F, 0x0019F,    214, 1, 0x001A0, 0x001A4,      1, 2, 0x001A6, 0x001A6,    218, 1, 0x001A7, 0x001A7,      1, 1,
		0x001A9, 0x001A9,    218, 1, 0x001AC, 0x001AC,      1, 1, 0x001AE, 0x001AE,    218, 1, 0x001AF, 0x001AF,      1, 1,
		0x001B1, 0x001B2,    217, 1, 0x001B3, 0x001B5,      1, 2, 0x001B7, 0x001B7,    219, 1, 0x001B8, 0x001B8,      1, 1,
		0x001BC, 0x001BC,      1, 1, 0x001C4, 0x001C4,      2, 1, 0x001C5, 0x001C5,      1, 1, 0x001C7, 0x001C7,      2, 1,
		0x001C8, 0x001C8,      1, 1, 0x001CA, 0x001CA,      2, 1, 0x001CB, 0x001DB,      1, 2, 0x001DE, 0x001EE,      1, 2,
		0x001F1, 0x001F1,      2, 1, 0x001F2, 0x001F4,      1, 2, 0x001F6, 0x001F6,    -97, 1, 0x001F7, 0x001F7,    -56, 1,
		0x001F8, 0x0021E,      1, 2, 0x00220, 0x00220,   -130, 1, 0x00222, 0x00232,      1, 2, 0x0023A, 0x0023A,  10795, 1,
		0x0023B, 0x0023B,      1, 1, 0x0023D, 0x0023D,   -163, 1, 0x0023E, 0x0023E,  10792, 1, 0x00241, 0x00241,      1, 1,
		0x00243, 0x00243,   -195, 1, 0x00244, 0x00244,     69, 1, 0x00245, 0x00245,     71, 1, 0x00246, 0x0024E,      1, 2,
		0x00345, 0x00345,    116, 1, 0x00370, 0x00372,      1, 2, 0x00376, 0x00376,      1, 1, 0x0037F, 0x0037F,    116, 1,
		0x00386, 0x00386,     38, 1, 0x00388, 0x0038A,     37, 1, 0x0038C, 0x0038C,     64, 1, 0x0038E, 0x0038F,     63, 1,
		0x00391, 0x003A1,     32, 1, 0x003A3, 0x003AB,     32, 1, 0x003C2, 0x003C2,      1, 1, 0x003CF, 0x003CF,      8, 1,
		0x003D0, 0x003D0,    -30, 1, 0x003D1, 0x003D1,    -25, 1, 0x003D5, 0x003D5,    -15, 1, 0x003D6, 0x003D6,    -22, 1,
		0x003D8, 0x003EE,      1, 2, 0x003F0, 0x003F0,    -54, 1, 0x003F1, 0x003F1,    -48, 1, 0x003F4, 0x003F4,    -60, 1,
		0x003F5, 0x003F5,    -64, 1, 0x003F7, 0x003F7,      1, 1, 0x003F9, 0x003F9,     -7, 1, 0x003FA, 0x003FA,      1, 1,
		0x003FD, 0x003FF,   -130, 1, 0x00400, 0x0040F,     80, 1, 0x00410, 0x0042F,     32, 1, 0x00460, 0x00480,      1, 2,
		0x0048A, 0x004BE,      1, 2, 0x004C0, 0x004C0,     15, 1, 0x004C1, 0x004CD,      1, 2, 0x004D0, 0x0052E,      1, 2,
		0x00531, 0x00556,     48, 1, 0x010A0, 0x010C5,   7264, 1, 0x010C7, 0x010C7,   7264, 1, 0x010CD, 0x010CD,   7264, 1,
		0x013F8, 0x013FD,     -8, 1, 0x01C80, 0x01C80,  -6222, 1, 0x01C81, 0x01C81,  -6221, 1, 0x01C82, 0x01C82,  -6212, 1,
		0x01C83, 0x01C84,  -6210, 1, 0x01C85, 0x01C85,  -6211, 1, 0x01C86, 0x01C86,  -6204, 1, 0x01C87, 0x01C87,  -6180, 1,
		0x01C88, 0x01C88,  35267, 1, 0x01C89, 0x01C89,      1, 1, 0x01C90, 0x01CBA,  -3008, 1, 0x01CBD, 0x01CBF,  -3008, 1,
		0x01E00, 0x01E94,      1, 2, 0x01E9B, 0x01E9B,    -58, 1, 0x01E9E, 0x01E9E,  -7615, 1, 0x01EA0, 0x01EFE,      1, 2,
		0x01F08, 0x01F0F,     -8, 1, 0x01F18, 0x01F1D,     -8, 1, 0x01F28, 0x01F2F,     -8, 1, 0x01F38, 0x01F3F,     -8, 1,
		0x01F48, 0x01F4D,     -8, 1, 0x01F59, 0x01F5F,     -8, 2, 0x01F68, 0x01F6F,     -8, 1, 0x01F88, 0x01F8F,     -8, 1,
		0x01F98, 0x01F9F,     -8, 1, 0x01FA8, 0x01FAF,     -8, 1, 0x01FB8, 0x01FB9,     -8, 1, 0x01FBA, 0x01FBB,    -74, 1,
		0x01FBC, 0x01FBC,     -9, 1, 0x01FBE, 0x01FBE,  -7173, 1, 0x01FC8, 0x01FCB,    -86, 1, 0x01FCC, 0x01FCC,     -9, 1,
		0x01FD3, 0x01FD3,  -7235, 1, 0x01FD8, 0x01FD9,     -8, 1, 0x01FDA, 0x01FDB,   -100, 1, 0x01FE3, 0x01FE3,  -7219, 1,
		0x01FE8, 0x01FE9,     -8, 1, 0x01FEA, 0x01FEB,   -112, 1, 0x01FEC, 0x01FEC,     -7, 1, 0x01FF8, 0x01FF9,   -128, 1,
		0x01FFA, 0x01FFB,   -126, 1, 0x01FFC, 0x01FFC,     -9, 1, 0x02126, 0x02126,  -7517, 1, 0x0212A, 0x0212A,  -8383, 1,
		0x0212B, 0x0212B,  -8262, 1, 0x02132, 0x02132,     28, 1, 0x02160, 0x0216F,     16, 1, 0x02183, 0x02183,      1, 1,
		0x024B6, 0x024CF,     26, 1, 0x02C00, 0x02C2F,     48, 1, 0x02C60, 0x02C60,      1, 1, 0x02C62, 0x02C62, -10743, 1,
		0x02C63, 0x02C63,  -3814, 1, 0x02C64, 0x02C64, -10727, 1, 0x02C67, 0x02C6B,      1, 2, 0x02C6D, 0x02C6D, -10780, 1,
		0x02C6E, 0x02C6E, -10749, 1, 0x02C6F, 0x02C6F, -10783, 1, 0x02C70, 0x02C70, -10782, 1, 0x02C72, 0x02C72,      1, 1,
		0x02C75, 0x02C75,      1, 1, 0x02C7E, 0x02C7F, -10815, 1, 0x02C80, 0x02CE2,      1, 2, 0x02CEB, 0x02CED,      1, 2,
		0x02CF2, 0x02CF2,      1, 1, 0x0A640, 0x0A66C,      1, 2, 0x0A680, 0x0A69A,      1, 2, 0x0A722, 0x0A72E,      1, 2,
		0x0A732, 0x0A76E,      1, 2, 0x0A779, 0x0A77B,      1, 2, 0x0A77D, 0x0A77D, -35332, 1, 0x0A77E, 0x0A786,      1, 2,
		0x0A78B, 0x0A78B,      1, 1, 0x0A78D, 0x0A78D, -42280, 1, 0x0A790, 0x0A792,      1, 2, 0x0A796, 0x0A7A8,      1, 2,
		0x0A7AA, 0x0A7AA, -42308, 1, 0x0A7AB, 0x0A7AB, -42319, 1, 0x0A7AC, 0x0A7AC, -42315, 1, 0x0A7AD, 0x0A7AD, -42305, 1,
		0x0A7AE, 0x0A7AE, -42308, 1, 0x0A7B0, 0x0A7B0, -42258, 1, 0x0A7B1, 0x0A7B1, -42282, 1, 0x0A7B2, 0x0A7B2, -42261, 1,
		0x0A7B3, 0x0A7B3,    928, 1, 0x0A7B4, 0x0A7C2,      1, 2, 0x0A7C4, 0x0A7C4,    -48, 1, 0x0A7C5, 0x0A7C5, -42307, 1,
		0x0A7C6, 0x0A7C6, -35384, 1, 0x0A7C7, 0x0A7C9,      1, 2, 0x0A7CB, 0x0A7CB, -42343, 1, 0x0A7CC, 0x0A7CC,      1, 1,
		0x0A7D0, 0x0A7D0,      1, 1, 0x0A7D6, 0x0A7DA,      1, 2, 0x0A7DC, 0x0A7DC, -42561, 1, 0x0A7F5, 0x0A7F5,      1, 1,
		0x0AB70, 0x0ABBF, -38864, 1, 0x0FB05, 0x0FB05,      1, 1, 0x0FF21, 0x0FF3A,     32, 1, 0x10400, 0x10427,     40, 1,
		0x104B0, 0x104D3,     40, 1, 0x10570, 0x1057A,     39, 1, 0x1057C, 0x1058A,     39, 1, 0x1058C, 0x10592,     39, 1,
		0x10594, 0x10595,     39, 1, 0x10C80, 0x10CB2,     64, 1, 0x10D50, 0x10D65,     32, 1, 0x118A0, 0x118BF,     32, 1,
		0x16E40, 0x16E5F,     32, 1, 0x1E900, 0x1E921,     34, 1,
	};

	count = (int)(sizeof(ranges) / sizeof(ranges[0]));
	return ranges;
}

inline unsigned int deelx_unicode_fold(unsigned int cp)
{
	if(cp < 0x41)
		return cp;

	if(cp <= 0x5A) // ASCII fast path
		return cp + 0x20;

	if(cp < 0x80)
		return cp;

	int count = 0;
	const int * ranges = deelx_unicode_fold_ranges(count);

	// the last range starting at or before cp
	int lo = 0, hi = count / 4 - 1;

	if(cp < (unsigned int)ranges[0])
		return cp;

	while(lo < hi)
	{
		int mid = (lo + hi + 1) / 2;

		if((unsigned int)ranges[mid * 4] <= cp)
			lo = mid;
		else
			hi = mid - 1;
	}

	const int * r = ranges + lo * 4;

	if(cp <= (unsigned int)r[1] && (cp - (unsigned int)r[0]) % (unsigned int)r[3] == 0)
		return cp + r[2];

	return cp;
}

// Regexp
typedef CRegexpT <char> CRegexpA;
typedef CRegexpT <unsigned short> CRegexpW;

#if defined(_UNICODE) || defined(UNICODE)
	typedef CRegexpW CRegexp;
#else
	typedef CRegexpA CRegexp;
#endif

#endif//__DEELX_REGEXP__H__
