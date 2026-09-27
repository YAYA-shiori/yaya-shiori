//
// AYA version 5
//
// SQLiteのデータベースを扱うクラス　CSqliteDB　（SQLOPEN/SQLCLOSE/SQLEXEC/SQLQUERY）
// SQLiteはamalgamation（sqlite/sqlite3.c）をsqlite3_yaya.cから組み込んでいます。
//
// CFileと同じく、データベースは開いたときのパス（フルパス）で識別します。
//

#ifndef	SQLITEDB_H
#define	SQLITEDB_H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "globaldef.h"
#include "value.h"

//----

// CSqliteDBの各関数の返値
enum {
	SQLDB_ERROR = 0,		// SQLiteのエラー（errstrに詳細）
	SQLDB_OK,				// 成功
	SQLDB_ALREADY_OPEN,		// Open：既に開いている
	SQLDB_NOT_OPEN,			// Close/Execute：開いていない
	SQLDB_BAD_MODE,			// Open：モードの指定が不正
	SQLDB_BAD_PARAM			// Execute：パラメータの形が不正
};

class	CSqliteConn;

class	CSqliteDB
{
protected:
	std::vector<CSqliteConn *>	connlist;

	CSqliteConn	*Find(const yaya::string_t &name) const;

private:
	CSqliteDB &operator=(const CSqliteDB &);	// 使わない

public:
	CSqliteDB(void) {}
	// CAyaVMのコピー用。接続は引き継がず、空の状態で作る（同じ接続を二重に閉じないため）
	CSqliteDB(const CSqliteDB &) {}
	~CSqliteDB(void) { CloseAll(); }

	int		Open(const yaya::string_t &name, const yaya::string_t &mode, yaya::string_t &errstr);
	int		Close(const yaya::string_t &name);
	void	CloseAll(void);
	int		Execute(const yaya::string_t &name, const yaya::string_t &sql, const CValueArray &args, size_t argstart,
				CValue *rows, yaya::int_t &changes, size_t &nparam, size_t &nvalue, yaya::string_t &errstr);
};

//----

#endif
