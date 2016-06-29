// 
// AYA version 5
//
// 配列に積まれている値を扱うクラス　CValueSub
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <math.h>

#include "globaldef.h"
#include "value.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CValueSub::CValueSub
 *  機能概要：  CValueからCValueSubを構築します
 * -----------------------------------------------------------------------
 */
CValueSub::CValueSub(const CValue &v)
{
	switch(v.type) {
	case F_TAG_INT:
		i_value = v.i_value;
		type = v.type;
		return;
	case F_TAG_DOUBLE:
		d_value = v.d_value;
		type = v.type;
		return;
	case F_TAG_STRING:
		s_value = v.s_value;
		type = v.type;
		return;
	default:
		type = F_TAG_VOID;
		return;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValueSub::GetValueInt
 *  機能概要：  値をintで取得します
 *
 *  返値　　：  0/1/2=エラー発生/取得できた/取得できた(型が読み替えられた)
 * -----------------------------------------------------------------------
 */
int	CValueSub::GetValueInt(void) const
{
	switch(type) {
	case F_TAG_INT:
		return i_value;
	case F_TAG_DOUBLE:
		{
			if ( d_value > static_cast<double>(INT_MAX) ) {
				return INT_MAX;
			}
			else if ( d_value < static_cast<double>(INT_MIN) ) {
				return INT_MIN;
			}
			else {
				return static_cast<int>(d_value);
			}
		}
	case F_TAG_STRING:
		return yaya::ws_atoi(s_value, 10);
	default:
		return 0;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValueSub::GetValueDouble
 *  機能概要：  値をdoubleで取得します
 *
 *  返値　　：  0/1/2=エラー発生/取得できた/取得できた(型が読み替えられた)
 * -----------------------------------------------------------------------
 */
double	CValueSub::GetValueDouble(void) const
{
	switch(type) {
	case F_TAG_INT:
		return (double)i_value;
	case F_TAG_DOUBLE:
		return d_value;
	case F_TAG_STRING:
		return yaya::ws_atof(s_value);
	default:
		return 0.0;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValueSub::GetValue
 *  機能概要：  値をyaya::string_tで返します
 * -----------------------------------------------------------------------
 */
yaya::string_t	CValueSub::GetValueString(void) const
{
	switch(type) {
	case F_TAG_INT: {
			return yaya::ws_itoa(i_value);
		}
	case F_TAG_DOUBLE: {
			return yaya::ws_ftoa(d_value);
		}
	case F_TAG_STRING:
		return s_value;
	default:
		return yaya::string_t(L"");
	};
}

/* -----------------------------------------------------------------------
 *  operator = (int)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(int value)
{
	type    = F_TAG_INT;
	i_value = value;
	s_value = L"";

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (double)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(double value)
{
	type    = F_TAG_DOUBLE;
	d_value = value;
	s_value = L"";

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (yaya::string_t)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(const yaya::string_t &value)
{
	type    = F_TAG_STRING;
	s_value = value;

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (yaya::char_t*)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(const yaya::char_t *value)
{
	type    = F_TAG_STRING;
	s_value = value;

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (CValue)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(const CValue &v)
{
	switch(v.type) {
	case F_TAG_INT:
		i_value = v.i_value;
		type = v.type;
		return *this;
	case F_TAG_DOUBLE:
		d_value = v.d_value;
		type = v.type;
		return *this;
	case F_TAG_STRING:
		s_value = v.s_value;
		type = v.type;
		return *this;
	default:
		type = F_TAG_VOID;
		return *this;
	}
}


/* -----------------------------------------------------------------------
 *  CalcEscalationTypeNum
 *
 *  型の昇格ルールを扱います（数値優先）
 *  基本的にDOUBLE>INTです。
 * -----------------------------------------------------------------------
 */
int CValueSub::CalcEscalationTypeNum(const int rhs) const
{
	int result = type > rhs ? type : rhs;
	if ( result != F_TAG_STRING ) { return result; }

	switch ( type <= rhs ? type : rhs ) {
	case F_TAG_VOID:
	case F_TAG_INT:
		return F_TAG_INT;
	case F_TAG_DOUBLE:
	case F_TAG_STRING:
		return F_TAG_DOUBLE;
	}
	return F_TAG_VOID;
}

/* -----------------------------------------------------------------------
 *  CalcEscalationTypeStr
 *
 *  型の昇格ルールを扱います（文字列優先）
 *  基本的にSTRING>DOUBLE>INTです。
 * -----------------------------------------------------------------------
 */
int CValueSub::CalcEscalationTypeStr(const int rhs) const
{
	return type > rhs ? type : rhs;
}

/* -----------------------------------------------------------------------
 *  operator + (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator +(const CValueSub &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	switch(t) {
	case F_TAG_INT:
		return CValueSub(GetValueInt()+value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValueSub(GetValueDouble()+value.GetValueDouble());
	case F_TAG_STRING:
		return CValueSub(GetValueString()+value.GetValueString());
	}

	return CValueSub(value);
}

void CValueSub::operator +=(const CValueSub &value)
{
	int t = CalcEscalationTypeStr(value.type);
	if ( t != type ) {
		*this = operator+(value);
		return;
	}

	switch(t) {
	case F_TAG_INT:
		i_value += value.GetValueInt();
	case F_TAG_DOUBLE:
		d_value += value.GetValueDouble();
	case F_TAG_STRING:
		s_value += value.GetValueString();
	}
	SetType(t);
}

/* -----------------------------------------------------------------------
 *  operator - (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator -(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
		return CValueSub(GetValueInt()-value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValueSub(GetValueDouble()-value.GetValueDouble());
	}

	return CValueSub(value);
}

void CValueSub::operator -=(const CValueSub &value)
{
	*this = operator-(value);
}

/* -----------------------------------------------------------------------
 *  operator * (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator *(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
		return CValueSub(GetValueInt()*value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValueSub(GetValueDouble()*value.GetValueDouble());
	}

	return CValueSub(value);
}

void CValueSub::operator *=(const CValueSub &value)
{
	*this = operator*(value);
}

/* -----------------------------------------------------------------------
 *  operator / (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator /(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
		{
			int denom = value.GetValueInt();
			if ( denom ) {
				return CValueSub(GetValueInt() / denom);
			}
			else {
				return CValueSub(GetValueInt());
			}
		}
	case F_TAG_DOUBLE:
		{
			double denom = value.GetValueDouble();
			if ( denom ) {
				return CValueSub(GetValueDouble() / denom);
			}
			else {
				return CValueSub(GetValueDouble());
			}
		}
	}

	return CValueSub(value);
}

void CValueSub::operator /=(const CValueSub &value)
{
	*this = operator/(value);
}

/* -----------------------------------------------------------------------
 *  operator % (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator %(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
	case F_TAG_DOUBLE:
		{
			int denom = value.GetValueInt();
			if ( denom ) {
				return CValueSub(GetValueInt() % denom);
			}
			else {
				return CValueSub(GetValueInt());
			}
		}
	}

	return CValueSub(value);
}

void CValueSub::operator %=(const CValueSub &value)
{
	*this = operator%(value);
}

/* -----------------------------------------------------------------------
 *  Compare (CValueSub)
 *
 *  operator == の実体です。
 *  intとdoubleの演算はdouble扱いです。yaya::string_tとの演算は空文字列を返します。
 * -----------------------------------------------------------------------
 */
int CValueSub::Compare(const CValueSub &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	if (t == F_TAG_INT) {
		return GetValueInt() == value.GetValueInt();
	}
	else if (t == F_TAG_DOUBLE) {
		return GetValueDouble() == value.GetValueDouble();
	}
	else if (t == F_TAG_STRING) {
		return GetValueString() == value.GetValueString();
	}
	else {
		return 0;
	}
}

bool CValueSub::Less(const CValueSub &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	if (t == F_TAG_INT) {
		return GetValueInt() < value.GetValueInt();
	}
	else if (t == F_TAG_DOUBLE) {
		return GetValueDouble() < value.GetValueDouble();
	}
	else if (t == F_TAG_STRING) {
		return GetValueString() < value.GetValueString();
	}
	return false;
}
