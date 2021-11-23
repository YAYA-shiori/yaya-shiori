// stdafx.h : 標準のシステム インクルード ファイル、
//            または参照回数が多く、かつあまり変更されない
//            プロジェクト専用のインクルード ファイルを記述します。
//

#if !defined(AFX_STDAFX_H__308D3A4E_B685_4328_B501_D274F42D9159__INCLUDED_)
#define AFX_STDAFX_H__308D3A4E_B685_4328_B501_D274F42D9159__INCLUDED_

#if _MSC_VER > 1000
# pragma once
#endif // _MSC_VER > 1000

// STL使用時にデバッグビルドで発生する無駄なワーニングを抑止
#if defined(_MSC_VER)
# pragma warning (disable: 4786)
#endif

// BoostがCE系で通らないことの対策
#if defined(_WIN32_WCE)
# define BOOST_USE_WINDOWS_H
# define BOOST_NO_STD_LOCALE
#endif

// VC8 Secure C 関数対策
#if (_MSC_VER >= 1400)
# pragma warning(disable : 4996)
#endif

#define WIN32_LEAN_AND_MEAN		// Windows ヘッダーから殆ど使用されないスタッフを除外します
#define NOMINMAX

#include <windows.h>
#include <winreg.h>
#include <Winperf.h>

#include "winbase.h"
#include <winnls.h>
#include <assert.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_STDAFX_H__308D3A4E_B685_4328_B501_D274F42D9159__INCLUDED_)
