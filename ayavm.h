// 
// AYA version 5
//
// AYAの1インスタンスを保持するクラスAYAVM
// written by the Maintenance Shop/C.Ponapalt 2006
// 
// CAyaVMをたくさん作ると複数のAYAを1つのプロセス/スレッド/モジュール内で走らせることができます。
// 

#ifndef AYAVM_H
#define AYAVM_H

#include <vector>
#include <map>
#include <memory>
#include "log.h"
#include "mt19937ar.h"

class CBasis;
class CFunction;
class CCallDepth;
class CSystemFunction;
class CGlobalVariable;
class CFile;
class CLib;
class CParser0;
class CParser1;

class CAyaVM
{
private:
	std::shared_ptr<CBasis>					m_basis;

	std::shared_ptr< std::vector<CFunction> >	m_function;
	std::shared_ptr< yaya::indexmap > m_functionmap;

	std::shared_ptr<CCallDepth>				m_calldepth;
	std::shared_ptr<CSystemFunction>			m_sysfunction;
	std::shared_ptr<CGlobalVariable>			m_variable;

	std::shared_ptr<CFile>					m_files;
	std::shared_ptr<CLib>						m_libs;

	std::shared_ptr<CParser0>					m_parser0;
	std::shared_ptr<CParser1>					m_parser1;

	CLog	m_logger;

public:
	void Load(void);
	void Unload(void);

	unsigned int genrand(void);
	int genrand_int(int n);

	// 主制御
	CBasis&					basis();

	// 関数/システム関数/グローバル変数
	std::vector<CFunction>&	function();
	yaya::indexmap& functionmap();

	CCallDepth&				calldepth();
	CSystemFunction&		sysfunction();
	CGlobalVariable&		variable();

	// ファイルと外部ライブラリ
	CFile&					files();
	CLib&					libs();

	// ロガー
	inline CLog& logger() {
		return m_logger;
	}

	// パーサ
	CParser0&				parser0();
	CParser1&				parser1();
};

#endif //AYAVM_H


