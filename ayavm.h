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
#include "globaldef.h"

class CBasis;
class CFunction;
class CFunctionDef;
class CCallLimit;
class CSystemFunction;
class CGlobalVariable;
class CFile;
class CSqliteDB;
class CLib;
class CParser0;
class CParser1;
class CDefine;
class CThreadManager;
class CAyaVM;

class CAyaVM
{
private:
	std_shared_ptr<CBasis>					m_basis;

	std_shared_ptr<CFunctionDef>	m_function_parse;
	std_shared_ptr<CFunctionDef>	m_function_exec;
	std::vector< std_shared_ptr<CFunctionDef> >	m_function_destruct;	// 差し替えた古い関数表（実行中の関数がいるのでrequestの終わりまで残す）
	
	std_shared_ptr< std::vector<CDefine> >	m_gdefines;

	std_shared_ptr<CCallLimit>				m_call_limit;
	std_shared_ptr<CSystemFunction>			m_sysfunction;
	std_shared_ptr<CGlobalVariable>			m_variable;

	std_shared_ptr<CFile>					m_files;
	std_shared_ptr<CSqliteDB>				m_sqlite;
	std_shared_ptr<CLib>						m_libs;

	std_shared_ptr<CParser0>					m_parser0;
	std_shared_ptr<CParser1>					m_parser1;

	CLog	m_logger;

	MersenneTwister64 rs_sysfunc64;
	MersenneTwister64 rs_internal64;

	// ワーカースレッド（workthread.h）
	// 実行権を持つスレッドが切り替わるたびに、今の呼び出し制限と関数表がここに入る（NULLならホスト側のもの）
	std_shared_ptr<CThreadManager>			m_threads;
	CCallLimit*								m_active_call_limit;
	CFunctionDef*							m_active_funcs;
	volatile long							m_attention;	// 1なら文の境界でthread_attendを呼ぶ（ホストが待っている・中断された）

public:
	CAyaVM();
	CAyaVM(CAyaVM &vm);
	virtual ~CAyaVM() {}

	CAyaVM* get_a_deep_copy();

	void load(void);
	void unload(void);

	void request_before(void);
	void request_after(void);

	void func_parse_to_exec(void);
	void func_parse_destruct(void);
	void func_parse_new(void);
	void func_destruct_clear(void);
	bool func_swapped(void) const { return ! m_function_destruct.empty(); }

	size_t genrand_uint(size_t n);

	yaya::int_t genrand_sysfunc_ll(yaya::int_t n);

	void genrand_sysfunc_srand_ll(yaya::int_t n);
	void genrand_sysfunc_srand_array(const std::uint64_t a[],const int n);

	// 主制御
	CBasis&					basis();

	// 関数/システム関数/グローバル変数
	CFunctionDef&	function_parse(); //パース用
	CFunctionDef&	function_exec(); //実行用

	std::vector<CDefine>&	gdefines();

	CCallLimit&				call_limit();		// 今実行しているスレッドのもの
	CCallLimit&				host_call_limit();	// ホスト（load/request/unload）のもの
	CSystemFunction&		sysfunction();
	CGlobalVariable&		variable();

	// ファイルとデータベースと外部ライブラリ
	CFile&					files();
	CSqliteDB&				sqlite();
	CLib&					libs();

	// ワーカースレッド
	CThreadManager&			threads();
	void SetActiveContext(CCallLimit *call_limit, CFunctionDef *funcs) {
		m_active_call_limit = call_limit;
		m_active_funcs = funcs;
	}
	void SetAttention(bool value) { m_attention = value ? 1 : 0; }
	bool thread_attention() const { return m_attention != 0; }
	bool thread_attend();	// 今のワーカーが中断すべきならtrue（ホストが待っていれば実行権を譲る）

	// ロガー
	inline CLog& logger() {
		return m_logger;
	}

	// パーサ
	CParser0&				parser0();
	CParser1&				parser1();
};

#endif //AYAVM_H


