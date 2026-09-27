//
// AYA version 5
//
// SQLiteのデータベースを扱うクラス　CSqliteDB　（SQLOPEN/SQLCLOSE/SQLEXEC/SQLQUERY）
//
// パラメータと値の対応
//   整数<->INTEGER、実数<->REAL、文字列<->TEXT、空（VOID）<->NULL
//   BLOBは16進数の文字列（SQLのhex()と同じ大文字）で返す
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>
#include <vector>
#include <map>

#include "sqlite/sqlite3.h"

#include "sqlitedb.h"
#include "jsonxml.h"
#include "globaldef.h"
#include "manifest.h"
#include "value.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

// ロックの解除を待つ最大時間（ミリ秒）。SHIORIの応答が止まるので長くしない
#define SQLDB_BUSY_TIMEOUT	1000

// 1つの接続でキャッシュする準備済みステートメント（SQL文字列単位）の上限。超えたら全部捨てる
#define SQLDB_CACHE_MAX	100

typedef std::vector<sqlite3_stmt *> SqliteStmtList;
typedef std::map<std::string, SqliteStmtList> SqliteStmtCache;

/* -----------------------------------------------------------------------
 *  クラス名：  CSqliteConn
 *  機能概要：  開いている1つのデータベースと、その準備済みステートメントを保持します
 *
 *  SQL文字列ごとに、含まれる文を先頭から準備したものをキャッシュします
 * -----------------------------------------------------------------------
 */
class	CSqliteConn
{
public:
	yaya::string_t	name;
	sqlite3			*db;
	SqliteStmtCache	cache;

	CSqliteConn(const yaya::string_t &n, sqlite3 *d) : name(n), db(d) {}
	~CSqliteConn(void)
	{
		ClearCache();
		sqlite3_close_v2(db);
	}

	void	ClearCache(void);
	SqliteStmtList	*FindCache(const std::string &sql);
	void	AddCache(const std::string &sql, SqliteStmtList &list);

private:
	CSqliteConn(const CSqliteConn &);				// 使わない
	CSqliteConn &operator=(const CSqliteConn &);	// 使わない
};

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteConn::ClearCache
 *  機能概要：  キャッシュした準備済みステートメントをすべて破棄します
 * -----------------------------------------------------------------------
 */
void	CSqliteConn::ClearCache(void)
{
	for ( SqliteStmtCache::iterator it = cache.begin(); it != cache.end(); ++it ) {
		for ( size_t i = 0; i < it->second.size(); ++i ) {
			sqlite3_finalize(it->second[i]);
		}
	}
	cache.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteConn::FindCache
 *  機能概要：  SQL文字列に対応する準備済みステートメントを探します（無ければNULL）
 * -----------------------------------------------------------------------
 */
SqliteStmtList	*CSqliteConn::FindCache(const std::string &sql)
{
	SqliteStmtCache::iterator it = cache.find(sql);
	if ( it == cache.end() ) {
		return NULL;
	}
	return &it->second;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteConn::AddCache
 *  機能概要：  準備済みステートメントをキャッシュします。listの中身は引き取る（listは空になる）
 * -----------------------------------------------------------------------
 */
void	CSqliteConn::AddCache(const std::string &sql, SqliteStmtList &list)
{
	if ( cache.size() >= SQLDB_CACHE_MAX ) {
		ClearCache();
	}
	cache[sql].swap(list);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SqliteBindValue
 *  機能概要：  値をステートメントのパラメータに設定します
 *
 *  返値　　：　SQLiteの結果コード。配列/ハッシュは設定できないので-1
 * -----------------------------------------------------------------------
 */
static int	SqliteBindValue(sqlite3_stmt *st, int idx, const CValue &v)
{
	switch ( v.GetType() ) {
	case F_TAG_INT:
		return sqlite3_bind_int64(st, idx, v.i_value);
	case F_TAG_DOUBLE:
		return sqlite3_bind_double(st, idx, v.d_value);
	case F_TAG_STRING:
		{
			std::string	u = WideToUtf8(v.s_value);
			return sqlite3_bind_text(st, idx, u.c_str(), static_cast<int>(u.size()), SQLITE_TRANSIENT);
		}
	case F_TAG_VOID:
		return sqlite3_bind_null(st, idx);
	default:
		return -1;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SqliteBindParams
 *  機能概要：  ステートメントのパラメータをすべて設定します
 *
 *  set==NULLなら args[argstart] 以降を ? の順に設定する
 *  setがハッシュなら :名前 @名前 $名前 の「名前」をキーにして値を探す
 *  setが配列なら要素を ? の順に設定する
 *  値が見つからないパラメータはNULLになる
 * -----------------------------------------------------------------------
 */
static int	SqliteBindParams(sqlite3_stmt *st, const CValue *set, const CValueArray &args, size_t argstart,
				sqlite3 *db, yaya::string_t &errstr)
{
	int	n = sqlite3_bind_parameter_count(st);

	for ( int i = 1; i <= n; ++i ) {
		const CValue	*v = NULL;
		size_t	pos = static_cast<size_t>(i - 1);

		if ( set == NULL ) {
			if ( argstart + pos < args.size() ) {
				v = &args[argstart + pos];
			}
		}
		else if ( set->IsHash() ) {
			const char	*pname = sqlite3_bind_parameter_name(st, i);
			if ( pname && (pname[0] == ':' || pname[0] == '@' || pname[0] == '$') ) {
				const CValueHash	&h = set->hash();
				CValueHash::const_iterator	it = h.find(CValue(Utf8ToWide(pname + 1)));
				if ( it != h.end() ) {
					v = &it->second;
				}
			}
		}
		else {
			const CValueArray	&a = set->array();
			if ( pos < a.size() ) {
				v = &a[pos];
			}
		}

		int	rc = v ? SqliteBindValue(st, i, *v) : sqlite3_bind_null(st, i);
		if ( rc == -1 ) {
			return SQLDB_BAD_PARAM;
		}
		if ( rc != SQLITE_OK ) {
			errstr = Utf8ToWide(sqlite3_errmsg(db));
			return SQLDB_ERROR;
		}
	}
	return SQLDB_OK;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SqliteColumnToValue
 *  機能概要：  結果の列の値をCValueにします
 * -----------------------------------------------------------------------
 */
static void	SqliteColumnToValue(sqlite3_stmt *st, int col, CValue &out)
{
	switch ( sqlite3_column_type(st, col) ) {
	case SQLITE_INTEGER:
		out = CValue(static_cast<yaya::int_t>(sqlite3_column_int64(st, col)));
		break;
	case SQLITE_FLOAT:
		out = CValue(sqlite3_column_double(st, col));
		break;
	case SQLITE_TEXT:
		out = CValue(Utf8ToWide(reinterpret_cast<const char *>(sqlite3_column_text(st, col))));
		break;
	case SQLITE_BLOB:
		{
			static const yaya::char_t	hexchar[] = L"0123456789ABCDEF";
			const unsigned char	*b = static_cast<const unsigned char *>(sqlite3_column_blob(st, col));
			int	len = sqlite3_column_bytes(st, col);
			yaya::string_t	s;
			s.reserve(static_cast<size_t>(len) * 2);
			for ( int i = 0; i < len; ++i ) {
				s.append(1, hexchar[b[i] >> 4]);
				s.append(1, hexchar[b[i] & 0x0f]);
			}
			out = CValue(s);
		}
		break;
	default: // SQLITE_NULL
		out = CValue();
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SqliteRunStatement
 *  機能概要：  準備済みの1文にパラメータを設定して最後まで実行します
 *
 *  rowsがNULLでなければ、結果の行を列名→値のハッシュにしてrows（配列）に追加する
 *  返値　　：　SQLDB_OK / SQLDB_BAD_PARAM / SQLDB_ERROR（errstrに詳細）
 * -----------------------------------------------------------------------
 */
static int	SqliteRunStatement(sqlite3_stmt *st, const CValue *set, const CValueArray &args, size_t argstart,
				sqlite3 *db, CValue *rows, yaya::string_t &errstr)
{
	int	result = SqliteBindParams(st, set, args, argstart, db, errstr);

	while ( result == SQLDB_OK ) {
		int	rc = sqlite3_step(st);
		if ( rc == SQLITE_ROW ) {
			if ( rows ) {
				rows->array().push_back(CValue(F_TAG_HASH, 0/*dmy*/));
				CValueHash	&h = rows->array().back().hash();
				int	ncol = sqlite3_column_count(st);
				for ( int c = 0; c < ncol; ++c ) {
					SqliteColumnToValue(st, c, h[CValue(Utf8ToWide(sqlite3_column_name(st, c)))]);
				}
			}
		}
		else if ( rc == SQLITE_DONE ) {
			break;
		}
		else {
			errstr = Utf8ToWide(sqlite3_errmsg(db));
			result = SQLDB_ERROR;
		}
	}

	// ロックを早く手放すため、実行し終えた文はすぐにリセットする
	sqlite3_reset(st);
	sqlite3_clear_bindings(st);
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteDB::Find
 *  機能概要：  名前（フルパス）から開いている接続を探します
 * -----------------------------------------------------------------------
 */
CSqliteConn	*CSqliteDB::Find(const yaya::string_t &name) const
{
	for ( size_t i = 0; i < connlist.size(); ++i ) {
		if ( connlist[i]->name == name ) {
			return connlist[i];
		}
	}
	return NULL;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteDB::Open
 *  機能概要：  データベースを開きます
 *
 *  modeは "rwc"（読み書き、無ければ作る。省略時）/ "rw"（読み書き）/ "r"（読み取り専用）
 *  返値　　：　SQLDB_OK / SQLDB_ALREADY_OPEN / SQLDB_BAD_MODE / SQLDB_ERROR（errstrに詳細）
 * -----------------------------------------------------------------------
 */
int	CSqliteDB::Open(const yaya::string_t &name, const yaya::string_t &mode, yaya::string_t &errstr)
{
	if ( Find(name) ) {
		return SQLDB_ALREADY_OPEN;
	}

	int	flags;
	if ( mode.empty() || mode == L"rwc" ) {
		flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	}
	else if ( mode == L"rw" ) {
		flags = SQLITE_OPEN_READWRITE;
	}
	else if ( mode == L"r" ) {
		flags = SQLITE_OPEN_READONLY;
	}
	else {
		return SQLDB_BAD_MODE;
	}

	sqlite3	*db = NULL;
	int	rc = sqlite3_open_v2(WideToUtf8(name).c_str(), &db, flags, NULL);
	if ( rc != SQLITE_OK ) {
		errstr = Utf8ToWide(db ? sqlite3_errmsg(db) : sqlite3_errstr(rc));
		sqlite3_close_v2(db);
		return SQLDB_ERROR;
	}
	sqlite3_busy_timeout(db, SQLDB_BUSY_TIMEOUT);

	connlist.push_back(new CSqliteConn(name, db));
	return SQLDB_OK;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteDB::Close
 *  機能概要：  データベースを閉じます。終わっていないトランザクションはロールバックされます
 *
 *  返値　　：　SQLDB_OK / SQLDB_NOT_OPEN
 * -----------------------------------------------------------------------
 */
int	CSqliteDB::Close(const yaya::string_t &name)
{
	for ( std::vector<CSqliteConn *>::iterator it = connlist.begin(); it != connlist.end(); ++it ) {
		if ( (*it)->name == name ) {
			delete *it;
			connlist.erase(it);
			return SQLDB_OK;
		}
	}
	return SQLDB_NOT_OPEN;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteDB::CloseAll
 *  機能概要：  すべてのデータベースを閉じます
 * -----------------------------------------------------------------------
 */
void	CSqliteDB::CloseAll(void)
{
	for ( size_t i = 0; i < connlist.size(); ++i ) {
		delete connlist[i];
	}
	connlist.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSqliteDB::Execute
 *  機能概要：  SQLを実行します
 *
 *  sqlに複数の文があれば順に実行する。args[argstart] 以降がパラメータ
 *    - すべてスカラーなら1回だけ実行し、? に順に設定する（パラメータなしも含む）
 *    - すべて配列/ハッシュなら、1つを1組として組の数だけ実行する
 *  rowsがNULLでなければ、結果の行を列名→値のハッシュにしてrows（配列）に追加する
 *  changesにはINSERT/UPDATE/DELETEで変更された行数の合計が入る
 *  成功したとき、nparamには各文のパラメータの数（?の最大の番号）の最大、
 *  nvalueには ? の順に設定する値の数（スカラーなら値の数、配列の組なら要素数の最大。ハッシュの組は0）が入る
 *  （nvalue > nparam なら使われなかった値がある）
 *  返値　　：　SQLDB_OK / SQLDB_NOT_OPEN / SQLDB_BAD_PARAM / SQLDB_ERROR（errstrに詳細）
 * -----------------------------------------------------------------------
 */
int	CSqliteDB::Execute(const yaya::string_t &name, const yaya::string_t &sql, const CValueArray &args, size_t argstart,
			CValue *rows, yaya::int_t &changes, size_t &nparam, size_t &nvalue, yaya::string_t &errstr)
{
	changes = 0;
	nparam = 0;
	nvalue = 0;

	CSqliteConn	*conn = Find(name);
	if ( ! conn ) {
		return SQLDB_NOT_OPEN;
	}

	size_t	nargs = args.size() > argstart ? args.size() - argstart : 0;
	size_t	nset = 0;
	for ( size_t i = 0; i < nargs; ++i ) {
		const CValue	&a = args[argstart + i];
		if ( a.IsArray() || a.IsHash() ) {
			++nset;
		}
	}
	if ( nset && nset != nargs ) {
		return SQLDB_BAD_PARAM;	// スカラーと配列/ハッシュが混ざっている
	}
	bool	byset = nset > 0;
	if ( ! byset ) {
		nset = 1;
	}

	std::string	utf8 = WideToUtf8(sql);
	SqliteStmtList	*cached = conn->FindCache(utf8);
	SqliteStmtList	fresh;

	sqlite3_int64	before = sqlite3_total_changes64(conn->db);
	int	result = SQLDB_OK;

	for ( size_t s = 0; s < nset && result == SQLDB_OK; ++s ) {
		const CValue	*set = byset ? &args[argstart + s] : NULL;

		if ( ! cached && s == 0 ) {
			// 初回は1文ずつ準備しては実行する
			// （前の文で作った表を次の文が参照していると、先にまとめて準備できないため）
			const char	*p = utf8.c_str();
			const char	*end = p + utf8.size();

			while ( p < end && result == SQLDB_OK ) {
				sqlite3_stmt	*st = NULL;
				const char	*tail = NULL;

				if ( sqlite3_prepare_v2(conn->db, p, static_cast<int>(end - p), &st, &tail) != SQLITE_OK ) {
					errstr = Utf8ToWide(sqlite3_errmsg(conn->db));
					result = SQLDB_ERROR;
					break;
				}
				if ( st ) {	// 空白やコメントだけの部分は文にならない
					fresh.push_back(st);
					result = SqliteRunStatement(st, set, args, argstart, conn->db, rows, errstr);
				}
				if ( tail == NULL || tail <= p ) {
					break;
				}
				p = tail;
			}
		}
		else {
			const SqliteStmtList	&list = cached ? *cached : fresh;
			for ( size_t k = 0; k < list.size() && result == SQLDB_OK; ++k ) {
				result = SqliteRunStatement(list[k], set, args, argstart, conn->db, rows, errstr);
			}
		}
	}

	if ( result == SQLDB_OK ) {
		const SqliteStmtList	&list = cached ? *cached : fresh;
		for ( size_t k = 0; k < list.size(); ++k ) {
			size_t	n = static_cast<size_t>(sqlite3_bind_parameter_count(list[k]));
			if ( nparam < n ) {
				nparam = n;
			}
		}
		if ( byset ) {
			for ( size_t i = 0; i < nargs; ++i ) {
				const CValue	&a = args[argstart + i];
				if ( a.IsArray() && nvalue < a.array().size() ) {
					nvalue = a.array().size();
				}
			}
		}
		else {
			nvalue = nargs;
		}
	}

	if ( ! cached ) {
		if ( result == SQLDB_OK ) {
			conn->AddCache(utf8, fresh);
		}
		else {
			for ( size_t i = 0; i < fresh.size(); ++i ) {
				sqlite3_finalize(fresh[i]);
			}
		}
	}

	changes = static_cast<yaya::int_t>(sqlite3_total_changes64(conn->db) - before);
	return result;
}
