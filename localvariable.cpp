// 
// AYA version 5
//
// ローカル変数を扱うクラス　CLocalVariable
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "globaldef.h"
#include "variable.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  CLocalVariableコンストラクタ
 * -----------------------------------------------------------------------
 */
CLocalVariable::CLocalVariable(void)
{
	depth = -1;

	AddDepth();
}

/* -----------------------------------------------------------------------
 *  CLocalVariableデストラクタ
 * -----------------------------------------------------------------------
 */
CLocalVariable::~CLocalVariable(void)
{
	stack.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetArgvAdr
 *  機能概要：  _argvが格納されている位置のポインタを返します
 *
 *  vectorは再割り当てによってアドレスが変化するので、ここで得られる値は
 *  一時的に利用することしか出来ません。
 * -----------------------------------------------------------------------
 */
CVariable	*CLocalVariable::GetArgvPtr(void)
{
	return &(stack[0].substack[0]);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::AddDepth
 *  機能概要：  一階層深いローカル変数スタックを作成します
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::AddDepth(void)
{
	CLVSubStack	addsubstack;
	stack.emplace_back(addsubstack);
	depth++;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::DelDepth
 *  機能概要：  最も深いローカル変数スタックを破棄します
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::DelDepth(void)
{
	if (stack.size()) {
		stack.erase(stack.end() - 1);
		depth--;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::Make
 *  機能概要：  ローカル変数を作成します
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::Make(const yaya::char_t *name)
{
	CVariable	addlv(name);
	stack[depth].substack.emplace_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name)
{
	CVariable	addlv(name);
	stack[depth].substack.emplace_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::char_t *name, const CValue &value)
{
	CVariable	addlv(name);
	addlv.value() = value;
	stack[depth].substack.emplace_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name, const CValue &value)
{
	CVariable	addlv(name);
	addlv.value() = value;
	stack[depth].substack.emplace_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name, const CValueSub &value)
{
	CVariable	addlv(name);
	addlv.value() = value;
	stack[depth].substack.emplace_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::char_t *name, const yaya::string_t &delimiter)
{
	CVariable	addlv(name);
	addlv.delimiter = delimiter;
	stack[depth].substack.emplace_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name, const yaya::string_t &delimiter)
{
	CVariable	addlv(name);
	addlv.delimiter = delimiter;
	stack[depth].substack.emplace_back(addlv);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetIndex
 *  機能概要：  指定された名前のローカル変数をスタックから検索します
 *
 *  見つからなかった場合は-1を返します
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::GetIndex(const yaya::char_t *name, int &id, int &dp)
{
	for(int i = stack.size() - 1; i >= 0; i--)
		for(int j = stack[i].substack.size() - 1; j >= 0; j--)
			if (stack[i].substack[j].name == name) {
				if (stack[i].substack[j].IsErased()) {
					id = -1;
					dp = -1;
				}
				else {
					dp = i;
					id = j;
				}
				return;
			}

	id = -1;
	dp = -1;
}

//----

void	CLocalVariable::GetIndex(const yaya::string_t &name, int &id, int &dp)
{
	for(int i = stack.size() - 1; i >= 0; i--)
		for(int j = stack[i].substack.size() - 1; j >= 0; j--)
			if (stack[i].substack[j].name == name) {
				if (stack[i].substack[j].IsErased()) {
					id = -1;
					dp = -1;
				}
				else {
					dp = i;
					id = j;
				}
				return;
			}

	id = -1;
	dp = -1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetValue
 *  機能概要：  指定されたローカル変数の値を取得します
 *
 *  指定された変数が存在しなかった場合は空文字列が返ります。
 * -----------------------------------------------------------------------
 */

const CValue& CLocalVariable::GetValue(const yaya::char_t *name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].value();

	return emptyvalue;
}

//----

const CValue& CLocalVariable::GetValue(const yaya::string_t &name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].value();

	return emptyvalue;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetValuePtr
 *  機能概要：  指定されたローカル変数のポインタを取得します
 *
 *  指定された変数が存在しなかった場合はNULLが返ります。
 * -----------------------------------------------------------------------
 */

CValue*	CLocalVariable::GetValuePtr(const yaya::char_t *name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return &(stack[dp].substack[id].value());

	return NULL;
}

//----

CValue*	CLocalVariable::GetValuePtr(const yaya::string_t &name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return &(stack[dp].substack[id].value());

	return NULL;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetNumber
 *  機能概要：  指定されたスタックのローカル変数の数を取得します
 * -----------------------------------------------------------------------
 */
int		CLocalVariable::GetNumber(int depth)
{
	if (depth < 0 || depth >= (int)stack.size()) {
		return 0;
	}
	return stack[depth].substack.size();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetPtr
 *  機能概要：  指定されたスタック、位置のローカル変数へのポインタを取得します
 * -----------------------------------------------------------------------
 */
CVariable	*CLocalVariable::GetPtr(size_t depth,size_t index)
{
	if (depth >= stack.size()) {
		return NULL;
	}
	if (index >= stack[depth].substack.size()) {
		return NULL;
	}
	return &(stack[depth].substack[index]);
}

CVariable* CLocalVariable::GetPtr(const yaya::char_t* name)
{
	int a,b;
	GetIndex(name, a, b);
	return GetPtr(a,b);
}

CVariable* CLocalVariable::GetPtr(const yaya::string_t& name)
{
	int a,b;
	GetIndex(name, a, b);
	return GetPtr(a,b);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetDelimiter
 *  機能概要：  指定されたローカル変数のデリミタを取得します
 * -----------------------------------------------------------------------
 */
yaya::string_t	CLocalVariable::GetDelimiter(const yaya::char_t *name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].delimiter;

	return yaya::string_t(L"");
}

//----

yaya::string_t	CLocalVariable::GetDelimiter(const yaya::string_t &name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].delimiter;

	return yaya::string_t(L"");
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::SetDelimiter
 *  機能概要：  指定されたローカル変数に値を格納します
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::SetDelimiter(const yaya::char_t *name, const yaya::string_t &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].delimiter = value;
		return;
	}

	// スタック内に存在しなければ変数を新たに作成して代入
	Make(name, value);
}

//----

void	CLocalVariable::SetDelimiter(const yaya::string_t &name, const yaya::string_t &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].delimiter = value;
		return;
	}

	// スタック内に存在しなければ変数を新たに作成して代入
	Make(name, value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::SetValue
 *  機能概要：  指定されたローカル変数に値を格納します
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::SetValue(const yaya::char_t *name, const CValue &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].value() = value;
		return;
	}

	// スタック内に存在しなければ変数を新たに作成して代入
	Make(name, value);
}

//----

void	CLocalVariable::SetValue(const yaya::string_t &name, const CValue &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].value() = value;
		return;
	}

	// スタック内に存在しなければ変数を新たに作成して代入
	Make(name, value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::GetMacthedLongestNameLength
 *  機能概要：  指定された文字列にマッチする名前を持つ変数を探し、マッチした長さを返します
 *
 *  複数見つかった場合は最長のものを返します。見つからなかった場合は0を返します
 * -----------------------------------------------------------------------
 */
size_t	CLocalVariable::GetMacthedLongestNameLength(const yaya::string_t &name)
{
	size_t	max_len = 0;

	for(yaya::native_signed i = stack.size() - 1; i >= 0; i--)
		for(yaya::native_signed j = stack[i].substack.size() - 1; j >= 0; j--) {
			size_t	len = stack[i].substack[j].name.size();
			if (!stack[i].substack[j].IsErased() &&
				max_len < len &&
				!name.compare(0,len,stack[i].substack[j].name,0,len))
				max_len = len;
		}

	return max_len;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLocalVariable::Erase
 *  機能概要：  指定された変数を消去します
 *
 *  実際に配列から消すわけではなく、空文字列を代入するだけです
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::Erase(const yaya::string_t &name)
{
	int	id, dp;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].Erase();
	}
}
