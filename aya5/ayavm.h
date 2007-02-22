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
#include <boost/shared_ptr.hpp>
#include "log.h"

class CWordMatch;
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
	boost::shared_ptr<CWordMatch>				m_formulatag_wm;
	boost::shared_ptr<CWordMatch>				m_function_wm;

	boost::shared_ptr<CBasis>					m_basis;

	boost::shared_ptr< std::vector<CFunction> >	m_function;
	boost::shared_ptr<CCallDepth>				m_calldepth;
	boost::shared_ptr<CSystemFunction>			m_sysfunction;
	boost::shared_ptr<CGlobalVariable>			m_variable;

	boost::shared_ptr<CFile>					m_files;
	boost::shared_ptr<CLib>						m_libs;

	boost::shared_ptr<CParser0>					m_parser0;
	boost::shared_ptr<CParser1>					m_parser1;

	CLog	m_logger;

public:
	// 演算子・関数の検索マップ
	CWordMatch&				formulatag_wm();
	CWordMatch&				function_wm();

	// 主制御
	CBasis&					basis();

	// 関数/システム関数/グローバル変数
	std::vector<CFunction>&	function();
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

