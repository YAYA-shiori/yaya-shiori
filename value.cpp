// 
// AYA version 5
//
// 値を扱うクラス　CValue
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <math.h>
#include <vector>
#include <iterator>

#include "misc.h"
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

//からっぽ変数（ダミー用）
const CValue emptyvalue;

/* -----------------------------------------------------------------------
 *  関数名  ：  CValue::GetValueInt
 *  機能概要：  値をintで返します
 *
 *  型が配列であった場合は0を返します
 * -----------------------------------------------------------------------
 */
yaya::int_t	CValue::GetValueInt(void) const
{
	switch(type) {
	case F_TAG_INT:
		return i_value;
	case F_TAG_DOUBLE:
		{
			if ( d_value > static_cast<double>(LLONG_MAX) ) {
				return LLONG_MAX;
			}
			else if ( d_value < static_cast<double>(LLONG_MIN) ) {
				return LLONG_MIN;
			}
			else {
				return static_cast<yaya::int_t>(d_value);
			}
		}
	case F_TAG_STRING:
		return yaya::ws_atoll(s_value, 10);
	case F_TAG_ARRAY:
		return 0;
    case F_TAG_HASH:
        return 0;
	default:
		return 0;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValue::GetValueDouble
 *  機能概要：  値をdoubleで返します
 *
 *  型が配列であった場合は0.0を返します
 * -----------------------------------------------------------------------
 */
double	CValue::GetValueDouble(void) const
{
	switch(type) {
	case F_TAG_INT:
		return (double)i_value;
	case F_TAG_DOUBLE:
		return d_value;
	case F_TAG_STRING:
		return yaya::ws_atof(s_value);
	case F_TAG_ARRAY:
		return 0.0;
	case F_TAG_HASH:
		return 0.0;
	default:
		return 0.0;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValue::GetValueString
 *  機能概要：  値をyaya::string_tで返します
 *
 *  型が配列であった場合はデリミタで結合した文字列に変換して返します
 * -----------------------------------------------------------------------
 */
yaya::string_t	CValue::GetValueString(void) const
{
	switch(type) {
	case F_TAG_INT: {
			return yaya::ws_lltoa(i_value, 10);
		}
	case F_TAG_DOUBLE: {
			return yaya::ws_ftoa(d_value);
		}
	case F_TAG_STRING:
		return s_value;
	case F_TAG_ARRAY: {
			yaya::string_t	result;
			for(CValueArray::const_iterator it = array().begin();
				it != array().end(); it++) {
				if (it != array().begin())
					result += VAR_DELIMITER;
				result += it->GetValueString();
			}
			return result;
		}
	case F_TAG_HASH: {
			yaya::string_t	result;
			for(CValueHash::const_iterator it = hash().begin();
				it != hash().end(); it++) {
				if (it != hash().begin())
					result += VAR_DELIMITER;
				result += it->first.GetValueString();
				result += L"=";
				result += it->second.GetValueString();
			}
			return result;
		}
	default:
		return yaya::string_t();
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValue::GetValueStringForLogging
 *  機能概要：  値をyaya::string_tで返します（ロガー用）
 *
 *  GetValueStringとの違いは、文字列をダブルクォートするか否かです。
 * -----------------------------------------------------------------------
 */
yaya::string_t	CValue::GetValueStringForLogging(void) const
{
	switch(type) {
	case F_TAG_INT: {
			return yaya::ws_lltoa(i_value);
		}
	case F_TAG_DOUBLE: {
			return yaya::ws_ftoa(d_value);
		}
	case F_TAG_STRING: {
			yaya::string_t	result = s_value;
			AddDoubleQuote(result);
			return result;
		}
	case F_TAG_ARRAY: {
			yaya::string_t	result;
			yaya::string_t	tmpstr;

			for(CValueArray::const_iterator it = array().begin();
				it != array().end(); it++) {
				if (it != array().begin())
					result += VAR_DELIMITER;
				tmpstr = it->GetValueString();
				if (it->GetType() == F_TAG_STRING)
					AddDoubleQuote(tmpstr);
				result += tmpstr;
			}
			return result;
		}
	case F_TAG_HASH: {
			yaya::string_t	result;
			yaya::string_t	tmpstr;

			for(CValueHash::const_iterator it = hash().begin();
				it != hash().end(); it++) {
				if (it != hash().begin())
					result += VAR_DELIMITER;

				tmpstr = it->first.GetValueString();
				if (it->first.GetType() == F_TAG_STRING)
					AddDoubleQuote(tmpstr);
				result += tmpstr;

				result += L"=";

				tmpstr = it->second.GetValueString();
				if (it->second.GetType() == F_TAG_STRING)
					AddDoubleQuote(tmpstr);
				result += tmpstr;
			}
			return result;
		}
	default:
		return yaya::string_t();
	};
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CValue::SetArrayValue
 *  機能概要：  配列の指定した位置へ値を設定します。必要に応じて型変換を行います
 *
 *  元の型が簡易配列と汎用配列の場合はそのまま処理しますが、整数/実数だった場合は
 *  汎用配列に型変換され、元の値は[0]に格納されます。
 * -----------------------------------------------------------------------
 */
void	CValue::SetArrayValue(const CValue &oval, const CValue &value)
{
	// 序数とデリミタの取得
	size_t	order, order1;
	yaya::string_t	delimiter;
	int	aoflg = oval.DecodeArrayOrder(order, order1, delimiter);

	// 値を更新する
	if ( type == F_TAG_STRING ) {
		// 簡易配列
		// 元の文字列をデリミタで分割
		std::vector<yaya::string_t>	s_array;
		size_t	sz = SplitToMultiString(s_value, &s_array, delimiter);
		// 更新
		if (aoflg) {
			// 範囲つき
			if (order1 < 0)
				return;
			else if(order < sz) {
				size_t	s_index = (size_t)std::max<yaya::int_t>(static_cast<yaya::int_t>(order), 0);
				size_t	e_index = (size_t)std::min<yaya::int_t>(static_cast<yaya::int_t>(order1) + 1, sz);

				if ( value.GetType() == F_TAG_ARRAY ) {
					std::vector<yaya::string_t>::iterator it = s_array.erase(s_array.begin() + s_index,s_array.begin() + e_index);
					
					if ( ! value.array().empty() ) {
						for ( CValueArray::const_iterator ita = value.array().begin() ;
							ita != value.array().end() ; ita++ ) {
							it = s_array.insert(it,ita->GetValueString());
							it++;
						}
					}
				}
				else {
					s_array.erase(s_array.begin() + s_index + 1,s_array.begin() + e_index);
					s_array[s_index] = value.GetValueString();
				}
			}
			else {
				size_t addsize = (size_t)(order - sz);
				for(size_t i = 0; i < addsize; i++) {
					s_array.emplace_back(yaya::string_t());
				}
	
				if ( value.GetType() == F_TAG_ARRAY ) {
					if ( ! value.array().empty() ) {
						for ( CValueArray::const_iterator ita = value.array().begin() ;
							ita != value.array().end() ; ita++ ) {
							s_array.emplace_back(ita->GetValueString());
						}
					}
				}
				else {
					s_array.emplace_back(value.GetValueString());
				}
			}
		}
		else {
			// 範囲なし
			if (order < 0)
				return;
			else if (order < sz) {
				if ( value.GetType() == F_TAG_ARRAY ) {
					std::vector<yaya::string_t>::iterator it = s_array.erase(s_array.begin() + order);
					
					if ( ! value.array().empty() ) {
						for ( CValueArray::const_iterator ita = value.array().begin() ;
							ita != value.array().end() ; ita++ ) {
							it = s_array.insert(it,ita->GetValueString());
							it++;
						}
					}
				}
				else {
					s_array[order] = value.GetValueString();
				}
			}
			else {
				int	addsize = order - sz;
				for(int i = 0; i < addsize; i++) {
					s_array.emplace_back(yaya::string_t());
				}

				if ( value.GetType() == F_TAG_ARRAY ) {
					if ( ! value.array().empty() ) {
						for ( CValueArray::const_iterator ita = value.array().begin() ;
							ita != value.array().end() ; ita++ ) {
							s_array.emplace_back(ita->GetValueString());
						}
					}
				}
				else {
					s_array.emplace_back(value.GetValueString());
				}
			}
		}
		// 文字列の復元
		sz = s_array.size();
		if(!sz)
			s_value.erase();
		else {
			s_value = s_array[0];
			for(size_t i = 1; i < sz; i++) {
				s_value += delimiter + s_array[i];
			}
		}
	}
	else {
		// 汎用配列（もしくは未初期化）
		if ( type != F_TAG_ARRAY ) {
			type = F_TAG_ARRAY;
			array().clear();
			array().emplace_back(CValueSub(*this));
		}

		if(aoflg) {
			size_t	sz = array_size();
			// 範囲つき
			if (order1 < 0)
				return;
			if (order < sz) {
				// 配列中途の書き換え
				size_t	s_index = (size_t)std::max<yaya::int_t>(static_cast<yaya::int_t>(order), 0);
				size_t	e_index = (size_t)std::min<yaya::int_t>(static_cast<yaya::int_t>(order1) + 1, sz);
				
				if ( value.GetType() == F_TAG_ARRAY ) {
					CValueArray::iterator it = array().erase(array().begin() + s_index,array().begin() + e_index);
					if ( ! value.array().empty() ) {
						array().insert(it, value.array().begin(), value.array().end());
					}
				}
				else {
					array().erase(array().begin() + s_index + 1,array().begin() + e_index);
					array()[s_index] = value;
				}
			}
			else {
				// 後端への追加
				int	addsize = order - array().size();
				for(int i = 1; i <= addsize; i++) {
					array().emplace_back(CValueSub());
				}
				
				if (value.GetType() == F_TAG_ARRAY) {
					if ( ! value.array().empty() ) {
						array().insert(array().end(),value.array().begin(), value.array().end());
					}
				}
				else {
					array().emplace_back(CValueSub(value));
				}
			}
		}
		else {
			// 範囲なし
			if (order < 0)
				return;
			if(order < array_size() ) {
				// 配列中途の書き換え				
				if (value.GetType() == F_TAG_ARRAY ) {
					CValueArray::iterator it = array().erase(array().begin() + order);
					if ( ! value.array().empty() ) {
						array().insert(it, value.array().begin(), value.array().end());
					}
				}
				else {
					array()[order] = value;
				}
			}
			else {
				// 後端への追加
				int	addsize = order - array().size();
				for(int i = 1; i <= addsize; i++) {
					array().emplace_back(CValueSub());
				}
				
				if (value.GetType() == F_TAG_ARRAY) {
					if ( ! value.array().empty() ) {
						array().insert(array().end(),value.array().begin(), value.array().end());
					}
				}
				else {
					array().emplace_back(CValueSub(value));
				}
			}
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CValue::DecodeArrayOrder
 *  機能概要：  既定の書式で配列序数を格納しているthisから序数とデリミタを取得します
 *
 *  返値　　：  0/1=order1(範囲指定)無効/有効
 * -----------------------------------------------------------------------
 */
bool CValue::DecodeArrayOrder(size_t&order, size_t&order1, yaya::string_t &delimiter) const
{
	order  = 0;
	order1 = 0;
	delimiter = VAR_DELIMITER;

	if (type == F_TAG_ARRAY) {
		int	sz = array_size();
		if (sz) {
			// 要素0:序数
			if (array()[0].GetType() == F_TAG_INT)
				order = (size_t)array()[0].i_value;
			else if (array()[0].GetType() == F_TAG_DOUBLE)
				order = (size_t)floor(array()[0].d_value);
			else
				return 0;
			if (sz == 1)
				return 0;
			// 要素1:数値なら範囲指定、文字列ならデリミタ
			switch(array()[1].GetType()) {
			case F_TAG_INT:
				order1  = (size_t)array()[1].i_value;
				if (order > order1)
					exchange(order, order1);
				break;
			case F_TAG_DOUBLE:
				order1  = (size_t)floor(array()[1].d_value);
				if(order > order1)
					exchange(order, order1);
				break;
			case F_TAG_STRING:
				if (array()[1].s_value.size())
					delimiter = array()[1].s_value;
				return 0;
			default:
				return 0;
			};
			if (sz == 2)
				return 1;
			// 要素2:要素1が範囲指定だった場合はデリミタ
			if (array()[2].GetType() == F_TAG_STRING &&
				array()[2].s_value.size())
				delimiter = array()[2].s_value;
			return 1;
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  operator = (int)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(yaya::int_t value) LVALUE_MODIFIER
{
	type    = F_TAG_INT;
	i_value = value;
	m_array.reset();
	s_value.erase();

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (double)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(double value) LVALUE_MODIFIER
{
	type    = F_TAG_DOUBLE;
	d_value = value;
	m_array.reset();
	s_value.erase();

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (yaya::string_t)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(const yaya::string_t &value) LVALUE_MODIFIER
{
	type    = F_TAG_STRING;
	s_value = value;
	m_array.reset();

	return *this;
}

#if CPP_STD_VER > 2011
CValue& CValue::operator =(yaya::string_t&& value) LVALUE_MODIFIER
{
	type = F_TAG_STRING;
	std::swap(s_value,value);
	m_array.reset();

	return *this;
}
#endif

/* -----------------------------------------------------------------------
 *  operator = (yaya::char_t*)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(const yaya::char_t *value) LVALUE_MODIFIER
{
	type    = F_TAG_STRING;
	s_value = value;
	m_array.reset();

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (CValueArray)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(const CValueArray &value) LVALUE_MODIFIER
{
	type    = F_TAG_ARRAY;
	array().assign(value.begin(), value.end());
	s_value.erase();

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (CValueHash)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(const CValueHash &value) LVALUE_MODIFIER
{
	type    = F_TAG_HASH;
	hash() = value;

	return *this;
}

void CValue::SubstToArray(CValueArray &value) LVALUE_MODIFIER
{
	type    = F_TAG_ARRAY;
	array().swap(value);
	s_value.erase();
}

/* -----------------------------------------------------------------------
 *  operator = (CValueSub)
 * -----------------------------------------------------------------------
 */
CValue &CValue::operator =(const CValueSub &value) LVALUE_MODIFIER
{
	switch(value.GetType()) {
	case F_TAG_INT:
		*this = value.i_value;
		break;
	case F_TAG_DOUBLE:
		*this = value.d_value;
		break;
	case F_TAG_STRING:
		*this = value.s_value;
		break;
	case F_TAG_VOID:
		type = F_TAG_VOID;
		i_value = 0;
		d_value = 0;
		m_array.reset();
		s_value.erase();
		break;
	default:
		*this = yaya::string_t();
		break;
	};

	return *this;
}

/* -----------------------------------------------------------------------
 *  CalcEscalationTypeNum
 *
 *  型の昇格ルールを扱います（数値優先）
 *  基本的にARRAY>DOUBLE>INTです。
 * -----------------------------------------------------------------------
 */
int CValue::CalcEscalationTypeNum(const int rhs) const
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
	case F_TAG_ARRAY:
		return F_TAG_ARRAY;
    case F_TAG_HASH:
        return F_TAG_HASH;
	}
	return F_TAG_VOID;
}

/* -----------------------------------------------------------------------
 *  CalcEscalationTypeStr
 *
 *  型の昇格ルールを扱います（文字列優先）
 *  基本的にARRAY>STRING>DOUBLE>INT>VOIDです。
 *  型の#define時にそうなるように定義されているため、値比較で大きいほうを返します。
 * -----------------------------------------------------------------------
 */
int CValue::CalcEscalationTypeStr(const int rhs) const
{
	return type > rhs ? type : rhs;
}

/* -----------------------------------------------------------------------
 *  配列演算用補助テンプレートとファンクタ
 * -----------------------------------------------------------------------
 */
template<class Fn>
CValue CValue_ArrayCalc(const CValue &param1_left,const CValue &param2_right,Fn calc_fn)
{
	CValue result;
	if (param1_left.GetType() == F_TAG_ARRAY && param2_right.GetType() == F_TAG_ARRAY) {
		if ( param1_left.array_size() == 0 ) {
			return param2_right;
		}
		else {
			if ( param2_right.array_size() == 0 ) {
				return param1_left;
			}
			else {
				result.SetType(F_TAG_ARRAY);
				CValueArray::const_iterator it, it2;
				for(it = param1_left.array().begin() ; it != param1_left.array().end() ; ++it) {
					for(it2 = param2_right.array().begin() ; it2 != param2_right.array().end() ; ++it2) {
						result.array().emplace_back(calc_fn((*it),(*it2)));
					}
				}
			}
		}
	}
	else if (param1_left.GetType() == F_TAG_ARRAY) {
		if ( param1_left.array_size() == 0 ) {
			return param2_right;
		}
		else {
			result.SetType(F_TAG_ARRAY);
			const CValueSub t_vs(param2_right);
			for(CValueArray::const_iterator it = param1_left.array().begin(); it != param1_left.array().end(); it++) {
				result.array().emplace_back(calc_fn(*it,t_vs));
			}
		}
	}
	else if (param2_right.GetType() == F_TAG_ARRAY) {
		if ( param2_right.array_size() == 0 ) {
			return param1_left;
		}
		else {
			result.SetType(F_TAG_ARRAY);
			const CValueSub t_vs(param1_left);
			for(CValueArray::const_iterator it = param2_right.array().begin(); it != param2_right.array().end(); it++) {
				result.array().emplace_back(calc_fn(t_vs,*it));
			}
		}
	}
	return result;
}

template<class Fn>
bool CValue_ArrayCalc_Subst(CValue &param1_subst,const CValue &param2_right,Fn calc_fn_subst)
{
	CValue result;

	if ( param2_right.GetType() == F_TAG_ARRAY) {
		return false;
	}

	if ( param1_subst.array_size() == 0 ) { //演算対象がない
		param1_subst = param2_right;
		return true;
	}

	param1_subst.SetType(F_TAG_ARRAY);
	const CValueSub t_vs(param2_right);
	for(CValueArray::iterator it = param1_subst.array().begin(); it != param1_subst.array().end(); it++) {
		calc_fn_subst(*it,t_vs);
	}
	return true;
}

//for normal
class CValueSub_Add {
public:
	CValueSub operator()(const CValueSub &v1,const CValueSub &v2) const {
		return v1 + v2;
	}
};
class CValueSub_Sub {
public:
	CValueSub operator()(const CValueSub &v1,const CValueSub &v2) const {
		return v1 - v2;
	}
};
class CValueSub_Mul {
public:
	CValueSub operator()(const CValueSub &v1,const CValueSub &v2) const {
		return v1 * v2;
	}
};
class CValueSub_Div {
public:
	CValueSub operator()(const CValueSub &v1,const CValueSub &v2) const {
		return v1 / v2;
	}
};
class CValueSub_Mod {
public:
	CValueSub operator()(const CValueSub &v1,const CValueSub &v2) const {
		return v1 % v2;
	}
};

//for subst
class CValueSub_Add_Subst {
public:
	void operator()(CValueSub &v1,const CValueSub &v2) const {
		v1 += v2;
	}
};
class CValueSub_Sub_Subst {
public:
	void operator()(CValueSub &v1,const CValueSub &v2) const {
		v1 -= v2;
	}
};
class CValueSub_Mul_Subst {
public:
	void operator()(CValueSub &v1,const CValueSub &v2) const {
		v1 *= v2;
	}
};
class CValueSub_Div_Subst {
public:
	void operator()(CValueSub &v1,const CValueSub &v2) const {
		v1 /= v2;
	}
};
class CValueSub_Mod_Subst {
public:
	void operator()(CValueSub &v1,const CValueSub &v2) const {
		v1 %= v2;
	}
};


/* -----------------------------------------------------------------------
 *  operator + (CValue)
 * -----------------------------------------------------------------------
 */
CValue CValue::operator +(const CValue &value) const
{
	int t = CalcEscalationTypeStr(value.type);
	
	switch(t) {
	case F_TAG_INT:
		return CValue(GetValueInt() + value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValue(GetValueDouble() + value.GetValueDouble());
	case F_TAG_STRING:
		return CValue(GetValueString() + value.GetValueString());
	case F_TAG_ARRAY:
		return CValue_ArrayCalc(*this,value,CValueSub_Add());
	};
	
	return CValue(value);
}

void CValue::operator +=(const CValue &value) LVALUE_MODIFIER
{
	int t = CalcEscalationTypeStr(value.type);
	if ( t == type ) { //左辺(自身)の型と同じ場合に限り
		if ( t == F_TAG_INT ) {
			i_value += value.GetValueInt();
			return;
		}
		if ( t == F_TAG_DOUBLE ) {
			d_value += value.GetValueDouble();
			return;
		}
		if ( t == F_TAG_STRING ) { //文字列時用パフォーマンス向上コード 長い文字列結合時にだいぶマシに
			s_value += value.GetValueString();
			return;
		}
		if ( t == F_TAG_ARRAY ) { //配列時用パフォーマンス向上コード
			if ( CValue_ArrayCalc_Subst(*this,value,CValueSub_Add_Subst()) ) { return; }
		}
	}
	*this = operator+(value);
}

/* -----------------------------------------------------------------------
 *  operator - (CValue)
 * -----------------------------------------------------------------------
 */
CValue CValue::operator -(const CValue &value) const
{
	int t = CalcEscalationTypeNum(value.type);
	
	switch(t) {
	case F_TAG_INT:
		return CValue(GetValueInt() - value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValue(GetValueDouble() - value.GetValueDouble());
	case F_TAG_ARRAY:
		return CValue_ArrayCalc(*this,value,CValueSub_Sub());
	};
	
	return CValue(value);
}

void CValue::operator -=(const CValue &value) LVALUE_MODIFIER
{
	int t = CalcEscalationTypeStr(value.type);
	if ( t == type ) { //左辺(自身)の型と同じ場合に限り
		if ( t == F_TAG_INT ) {
			i_value -= value.GetValueInt();
			return;
		}
		if ( t == F_TAG_DOUBLE ) {
			d_value -= value.GetValueDouble();
			return;
		}
		if ( t == F_TAG_ARRAY ) { //配列時用パフォーマンス向上コード
			if ( CValue_ArrayCalc_Subst(*this,value,CValueSub_Sub_Subst()) ) { return; }
		}
	}
	*this = operator-(value);
}

/* -----------------------------------------------------------------------
 *  operator * (CValue)
 * -----------------------------------------------------------------------
 */
CValue CValue::operator *(const CValue &value) const
{
	int t = CalcEscalationTypeNum(value.type);
	
	switch(t) {
	case F_TAG_INT:
		return CValue(GetValueInt() * value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValue(GetValueDouble() * value.GetValueDouble());
	case F_TAG_ARRAY:
		return CValue_ArrayCalc(*this,value,CValueSub_Mul());
	};
	
	return CValue(value);
}

void CValue::operator *=(const CValue &value) LVALUE_MODIFIER
{
	int t = CalcEscalationTypeStr(value.type);
	if ( t == type ) { //左辺(自身)の型と同じ場合に限り
		if ( t == F_TAG_ARRAY ) { //配列時用パフォーマンス向上コード
			if ( CValue_ArrayCalc_Subst(*this,value,CValueSub_Mul_Subst()) ) { return; }
		}
	}
	*this = operator*(value);
}

/* -----------------------------------------------------------------------
 *  operator / (CValue)
 * -----------------------------------------------------------------------
 */
CValue CValue::operator /(const CValue &value) const
{
	int t = CalcEscalationTypeNum(value.type);
	
	switch(t) {
	case F_TAG_INT:
		{
			yaya::int_t denom = value.GetValueInt();
			if ( denom ) {
				return CValue(GetValueInt() / denom);
			}
			else {
				return CValue(GetValueInt());
			}
		}
	case F_TAG_DOUBLE:
		{
			double denom = value.GetValueDouble();
			if ( denom ) {
				return CValue(GetValueDouble() / denom);
			}
			else {
				return CValue(GetValueDouble());
			}
		}
	case F_TAG_ARRAY:
		return CValue_ArrayCalc(*this,value,CValueSub_Div());
	};
	
	return CValue(value);
}

void CValue::operator /=(const CValue &value) LVALUE_MODIFIER
{
	int t = CalcEscalationTypeStr(value.type);
	if ( t == type ) { //左辺(自身)の型と同じ場合に限り
		if ( t == F_TAG_ARRAY ) { //配列時用パフォーマンス向上コード
			if ( CValue_ArrayCalc_Subst(*this,value,CValueSub_Div_Subst()) ) { return; }
		}
	}
	*this = operator/(value);
}

/* -----------------------------------------------------------------------
 *  operator % (CValue)
 * -----------------------------------------------------------------------
 */
CValue CValue::operator %(const CValue &value) const
{
	int t = CalcEscalationTypeNum(value.type);
	
	switch(t) {
	case F_TAG_INT:
	case F_TAG_DOUBLE:
		{
			yaya::int_t denom = value.GetValueInt();
			if ( denom ) {
				return CValue(GetValueInt() % denom);
			}
			else {
				return CValue(GetValueInt());
			}
		}
	case F_TAG_ARRAY:
		return CValue_ArrayCalc(*this,value,CValueSub_Mod());
	};
	
	return CValue(value);
}

void CValue::operator %=(const CValue &value) LVALUE_MODIFIER
{
	int t = CalcEscalationTypeStr(value.type);
	if ( t == type ) { //左辺(自身)の型と同じ場合に限り
		if ( t == F_TAG_ARRAY ) { //配列時用パフォーマンス向上コード
			if ( CValue_ArrayCalc_Subst(*this,value,CValueSub_Mod_Subst()) ) { return; }
		}
	}
	*this = operator%(value);
}

/* -----------------------------------------------------------------------
 *  operator [] (CValue)
 *
 *  thisの型がyaya::string_tの場合は簡易配列、array()の場合は配列扱いです。
 *  int/doubleでは序数によらずその値が返されます。
 *
 *  序数が範囲外の場合は空文字列を返します。
 *
 *  引数の型は常にarray()であり、特定のフォーマットに準拠している必要があります。
 *  （呼び出し側でそのように成形する必要があります）
 * -----------------------------------------------------------------------
 */
CValue CValue::operator [](const CValue &value) const
{
	size_t	order, order1;
	yaya::string_t	delimiter;
	int	aoflg = value.DecodeArrayOrder(order, order1, delimiter);

	if (type == F_TAG_INT || type == F_TAG_DOUBLE) {
		// 数値　序数が0ならthis、1以外では空文字列を返す
		if (!order)
			return *this;
		else
			return CValue();
	}
	if (type == F_TAG_STRING) {
		// 簡易配列

		// 文字列をデリミタで分割
		std::vector<yaya::string_t>	s_array;
		size_t	sz = SplitToMultiString(s_value, &s_array, delimiter);
		// 値の取得
		if (aoflg) {
			// 範囲あり
			if (order1 < 0 || order >= sz)
				return CValue();
			else {
				size_t	s_index = (size_t)std::max<yaya::int_t>(static_cast<yaya::int_t>(order), 0);
				size_t	e_index = (size_t)std::min<yaya::int_t>(static_cast<yaya::int_t>(order1) + 1, sz);
				size_t	i       = 0;
				size_t	j       = 0;
				yaya::string_t	result_str;
				for(std::vector<yaya::string_t>::iterator it = s_array.begin();
					it != s_array.end(); it++, i++) {
					if (s_index <= i && i < e_index) {
						if (j)
							result_str += delimiter;
						j = 1;
						result_str += *it;
					}
					else if (i >= e_index)
						break;
				}
				return result_str;
			}
		}
		else {
			// 範囲なし
			if (0 <= order && order < sz)
				return CValue(s_array[order]);
			else 
				return CValue();
		}
	}
	else if (type == F_TAG_ARRAY) {
		// 汎用配列

		size_t	sz = array_size();
		// 値の取得
		if (aoflg) {
			// 範囲あり
			if (order1 < 0 || order >= sz)
				return CValue(F_TAG_ARRAY, 0/*dmy*/);
			else {
				size_t	s_index = (size_t)std::max<yaya::int_t>(order, 0);
				size_t	e_index = (size_t)std::min<yaya::int_t>((yaya::int_t)order1 + 1, sz);
				size_t	i       = 0;
				CValue	result_array(F_TAG_ARRAY, 0/*dmy*/);
				for(CValueArray::const_iterator it = array().begin();
					it != array().end(); it++, i++) {
					if (s_index <= i && i < e_index)
						result_array.array().emplace_back(*it);
					else if (i >= e_index)
						break;
				}
				return result_array;
			}
		}
		else {
			// 範囲なし
			if (0 <= order && order < sz) {
				return CValue(array()[order]);
			}
			else {
				return yaya::string_t();
			}
		}
	}
    else if (type == F_TAG_HASH)
    {
        if (!aoflg)
        {
            if (hash().count(value.array()[0]))
            {
                return CValue(hash().find(value.array()[0])->second);
            }
            else {
                return CValue(L"");
            }
        }
    }

	return yaya::string_t();
}

/* -----------------------------------------------------------------------
 *  operator == / != (CValue)
 * -----------------------------------------------------------------------
 */
int CValue::Compare(const CValue &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	switch(t) {
	case F_TAG_VOID:
		return 1; //VOID型同士の演算はTRUE
	case F_TAG_INT:
		return GetValueInt() == value.GetValueInt();
	case F_TAG_DOUBLE:
		return GetValueDouble() == value.GetValueDouble();
	case F_TAG_STRING:
		return GetValueString() == value.GetValueString();
	case F_TAG_ARRAY: {
			if (value.type == F_TAG_ARRAY) {
				size_t	len = array_size();
				if (len != value.array_size())
					return 0;
				else {
					CValueArray::const_iterator it, it2;
					size_t	i = 0;
					for(it = array().begin(), it2 = value.array().begin();
						it != array().end() && it2 != value.array().end(); it++, it2++)		
						i += it->Compare(*it2);
					return (i == len);
				}
			}
			else {
				return 0;
			}
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  operator > | <= (CValue)
 * -----------------------------------------------------------------------
 */
int CValue::Great(const CValue &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	if (t == F_TAG_INT) {
		return GetValueInt() > value.GetValueInt();
	}
	else if (t == F_TAG_DOUBLE) {
		return GetValueDouble() > value.GetValueDouble();
	}
	else if (t == F_TAG_STRING) {
		return GetValueString() > value.GetValueString();
	}
	return 0;

}

/* -----------------------------------------------------------------------
 *  operator < / >= (CValue)
 * -----------------------------------------------------------------------
 */
int CValue::Less(const CValue &value) const
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
	return 0;
}




