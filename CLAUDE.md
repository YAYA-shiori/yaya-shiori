このディレクトリは伺かのSHIORI "YAYA" のコードです。
C++98/旧いVC++でコンパイルできるように製作されており、文字コードShift JISなので、Sjis_ではじまる編集・探索ツール群を使う必要があります。
コード管理ツールはgitです。
仕様は C:\D_DRIVE\MyDocuments\GitHub\yaya-docs にあります。

## ビルド方法

VC++6 (`msdev.exe`) がインストール済みで PATH が通っているため、`aya5.dsw` をコマンドラインからビルドできる。

```powershell
msdev aya5.dsw /MAKE "aya5 - Win32 Debug" /OUT "$env:TEMP\claude\yaya_build.log"
Get-Content "$env:TEMP\claude\yaya_build.log" -Encoding oem
```

- 構成名: `aya5 - Win32 Release` / `aya5 - Win32 Debug` / `aya5 - Win32 Release EXE` / `aya5 - Win32 Debug EXE` / `aya5 - Win32 ReleaseLangSep`（全構成は `aya5 - ALL`）
- 出力先: `Release/` / `Debug/` / `ReleaseLangSep/` 等（いずれも `.gitignore` 済み）。DLL名は `yaya.dll`
- `/MAKE` は差分ビルド。全再ビルドは `/REBUILD` を付ける
- `msdev.exe` はGUIアプリのためコンソールに出力されない。結果は `/OUT` のログで確認する（ログはShift JIS なので `-Encoding oem` で読む）
- 成功時はログ末尾が `yaya.dll - ｴﾗｰ 0、警告 N`。VC6 の STL 由来の警告 C4786 は無視してよい
- Git Bash から実行する場合は `/MAKE` がパス変換されるため `MSYS_NO_PATHCONV=1` を前置する
- 配布用zipの作成は `make_aya.bat`（Release / ReleaseLangSep のビルド後に実行）
- JSON/XML の解析に使う parson（`parson/`）と tinyxml2（`tinyxml2/`、ponapalt のフォーク）、SQL* 関数に使う SQLite（`sqlite/`、ponapalt のフォーク sqlite-amalgamation-new）は git サブモジュール。クローン直後は `git submodule update --init` が必要。いずれも UTF-8 のソースなので Sjis_ 系ツールで編集しない（そもそも本体側では編集せず、tinyxml2 / SQLite の修正はフォーク側（`../tinyxml2` / `../sqlite-amalgamation-new` にクローンあり）で行ってから参照を更新する）
- サブモジュールを使う処理（JSON/XML、SQL* 関数）に手を入れる前に、`git submodule update --init --remote parson tinyxml2 sqlite` で参照を最新にしてから作業する
- SQLite のフォークは upstream の amalgamation に VC6 / 古い SDK 向けの修正を1コミット載せたもの。64bit のリテラルは `INT64_C()` / `UINT64_C()` で書く（`LL` は VC6 が、`i64` は gcc が読めない）。修正したら VC6 と gcc（Strawberry Perl 同梱の `gcc`）の両方で `sqlite3.c` 単体がコンパイルできることを確かめる
- VC6 以外のビルドは makefile。`makefile.linux` が基準で、`freebsd`（clang）/ `posix`（macOS、`.bundle`）/ `emscripten` はオブジェクト一覧と規則を linux と同じにしてある。ソースを増やしたら全部の一覧に足す（`makefile.mingw32` は `posix_utils.o` に加えて `aya5_res.o` も持つ）。POSIX 系の makefile は `.cpp` を iconv で UTF-8 にしてからコンパイルする
- `makefile.mingw32` は Windows 用の yaya.dll を MinGW-w64 で作る（`-finput-charset=CP932 -fexec-charset=CP932` で VC++ と同じく文字列を CP932 のまま扱う）。Strawberry Perl 同梱の gcc（x86_64、64bit の DLL になる）で確かめられる。リポジトリを汚さないよう、`git ls-files --recurse-submodules` のファイルを作業用ディレクトリに写してから `mingw32-make -f makefile.mingw32`（動作確認用の EXE は `exe` ターゲット）
  - MinGW は `_WINDOWS` だが `_MSC_VER` ではない。SEH（`__try`）や `i64` リテラルなど MSVC 専用の書き方は `_MSC_VER` で分け、64bit の整数リテラルは `LL_DEF()` / `ULL_DEF()` で書く
- ソースは Shift JIS（CP932）で統一する。UTF-8 のファイルが混ざると `-finput-charset=CP932` の MinGW ビルドが通らない
- `messagetxt/*.txt` は `aya5.rc` からリソースとして埋め込まれるが、`/MAKE` はその変更を検知しない。messagetxt を変えたら `/REBUILD` するか、`Release/aya5.res` などを消してからビルドする

## リリース手順

1. バージョンを上げる（例: `Tc600-2`）。どちらも Shift JIS かつ CRLF なので Sjis_ 系ツールで編集する（Git Bash の `sed -i` は CRLF を LF に変えてしまう）
   - `manifest.cpp` の `aya_version`
   - `aya5.rc` の `FILEVERSION 6,00,2,0` と `VALUE "FileVersion", "6, 00, 2, 0\0"`（`TcXYY-N` → `X,YY,N,0`）
2. サブモジュール（`parson` / `tinyxml2` / `sqlite`）の参照を最新に更新する
   ```powershell
   git submodule update --init --remote parson tinyxml2 sqlite
   git submodule status
   ```
   - 参照が変わったら `git diff --submodule` で取り込まれるコミットを確認し、リリースのコミットに含める
   - ライセンス文の年や著作権者が変わっていたら `sysfunc.cpp` の `LICENSE` 関数の parson / TinyXML-2 の部分も合わせる（SQLite はパブリックドメイン）
3. Release と ReleaseLangSep をリビルドし、ログ末尾が `ｴﾗｰ 0` であることを確認する
   ```powershell
   msdev aya5.dsw /MAKE "aya5 - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\yaya_build_Release.log"
   msdev aya5.dsw /MAKE "aya5 - Win32 ReleaseLangSep" /REBUILD /OUT "$env:TEMP\claude\yaya_build_ReleaseLangSep.log"
   ```
4. `make_aya.bat` を実行して `tmp/yaya.zip` と `tmp/yaya_lang_sep.zip` を作る
   - PowerShell から `cmd /c .\make_aya.bat` で実行する（Git Bash の `cmd //c make_aya.bat` はバッチが見つからず失敗する）
   - zip の更新日時が新しくなったこと、中の `yaya.dll` が `Release/` `ReleaseLangSep/` のものと一致することを確認する
5. コミットして push する。コミットメッセージの1行目は `変更内容 / TcXYY-N`、本文に変更点の箇条書き
6. GitHub のリリースを作る。タグ名とタイトルはバージョン、対象は現在のブランチ、2つの zip を添付、本文は変更点の日本語の箇条書き
   ```powershell
   gh release create Tc600-2 tmp/yaya.zip tmp/yaya_lang_sep.zip --target 600 --title Tc600-2 --notes-file <本文ファイル>
   ```
   - beta（`--prerelease`）を付けるかどうかは毎回ユーザーに確認する

## シェルモード

- 通常モードで読む辞書が1つも無い（設定ファイルが無い、dic/dicif/dicdir の結果が0個）と、`CParser0::ParseShellDictionary` が `load{}unload{}request{EVAL(_argv[0])}` だけの組み込み辞書（辞書名 `_SHELL_DIC_`）を作り、request の入力を EVAL した結果を返す。ログには注記 N0002 が出る
- 設定ファイルが無いときは変数の自動保存・復元をしない。緊急モード（`yaya_emerg.txt`）はシェルモードにならず、従来どおり設定ファイルが無ければ抑止する
- 辞書を用意しなくても、空のディレクトリを load して `request:長さ\r\n<式や文>` を送れば EXE 構成の `yaya.exe` で動作確認できる

## パーサの構造

辞書の読み込みと解析は2フェーズに分かれている。

### フェーズ1: LoadDictionary1（ファイルごとに順次実行）
- ソースを読み込み、関数・ステートメントを登録する
- `StructFormula` で数式をセル列に分解し、演算子の整形を行う（この時点では識別子は全て `F_TAG_NOP` のまま）
- **全ファイルの読み込みが終わるまで関数名の解決は行わない**

### フェーズ2: ParseAfterLoad（全ファイル読み込み後、ファイルごとに実行）
1. `AddSimpleIfBrace` – 省略波括弧の補完
2. `SetCellType` → `SetCellType1` – `F_TAG_NOP` セルの型付け
   - ユーザー関数名 → `F_TAG_USERFUNC`
   - システム関数名 → `F_TAG_SYSFUNC`
   - 整数/実数/文字列リテラル → 各リテラル型
   - それ以外（グローバル変数名）→ `F_TAG_VARIABLE`
   - `_` 始まり → `F_TAG_LOCALVARIABLE`
3. `MakeCompleteFormula` – 埋め込み文字列の展開・演算順序の決定（`CheckDepthAndSerialize1`）
4. `CheckExecutionCode` – 最終検証
   - `CheckFunctionArgument` – `F_TAG_FUNCPARAM` の前がUSERFUNC/SYSFUNCでなければE0071
   - `ReindexUserFunctions` – `F_TAG_USERFUNC` セルの関数インデックス解決

### StructFormula の主な処理
- `identifier(` → 直前が `F_TAG_NOP` なら `F_TAG_FUNCPARAM` を挿入
- `()` 空カッコ → `(` `)` を消去し、直前の `F_TAG_FUNCPARAM` も消去
- `identifier[` → `F_TAG_ARRAYORDER` を挿入

### 関数1個版のパース処理と EVAL の文の並び
- ParseAfterLoad の各段階（`AddSimpleIfBrace` / `SetCellType` / `MakeCompleteFormula` 配下）と parser1 の構文検査（`SetBreakJumpNo` 〜 `SetIfJumpNo`、`CheckExecutionCode`）には、辞書ファイル名で関数表を回す版と `CFunction&` を受け取る関数1個版のオーバーロードがある。辞書ファイル名版は関数1個版をループで呼ぶだけなので、処理を変えるときは関数1個版を直す
- `EVAL` / `ISEVALUABLE` は `CParser0::IsEvalBlock`（クォート外の `;` `{` `}` 改行、または先頭が制御文のキーワード）で文の並びかを判定する。文の並びなら `ParseEvalBlock` で関数表に登録しない一時 `CFunction` を作り、`CFunction::ExecuteEval` で呼び出し元の `lvar` のもとで実行する（`_argv` などは上書きしない）。関数表の deep copy や差し替え（`func_parse_new` / `func_parse_to_exec`）は起こさない
- `StoreInternalStatement` / `MakeStatement` は関数表の番号ではなく `CFunction&` を受け取る

### CCell の主要フィールド
- `m_type` / `value_GetType()` – セルの種別（F_TAG_*）
- `value` – リテラル値（識別子名は `value.s_value` に格納）
- `name` – 関数名・変数名（型付け後に設定）
- `index` – 関数テーブルや変数テーブルへのインデックス
- `depth` – ローカル変数のスコープ深さ用フィールド（実際には未使用、フラグ用途に流用可）
- `ansv` – 実行時の演算結果

### 演算順序の決定（CheckDepthAndSerialize1）
- `(` で深さ+20、`)` で深さ-20（括弧内の優先度を大幅に上げる）
- `F_TAG_FUNCPARAM` の優先度は11（括弧内より低い）
- 演算子は左から右に結合（C/C++と異なり右結合ではない）
- `F_TAG_FUNCPARAM` は右辺のオペランドが必須（右辺がない場合E0022）

## 実行エンジンの構造

### ステートメント種別と処理の対応
- `ST_*` 定数は `manifest.h` で定義。`ST_FORMULA`(=6)以上のステートメントのみセルを持つ
- `parser0.cpp:StoreInternalStatement` – 行をステートメントに変換する入口
  - 引数なしキーワード（`break`/`continue`/`{`/`}`/`--` など）: セルなし `CStatement` を直接生成
  - 引数ありキーワード（`if`/`while`/`return expr` など）: `MakeStatement` → `StructFormula` でセルを生成
- `SetFormulaType` は `ST_FORMULA` のみ `ST_FORMULA_OUT_FORMULA` / `ST_FORMULA_SUBST` に再分類する。他の型（`ST_IF` / `ST_RETURN` 等）は変更しない
- `SetCellType` は `type >= ST_FORMULA` のステートメント全てのセルを型付けするため、`ST_RETURN`(=18) にセルを持たせても自動的に型解決される

### 実行フロー（function.cpp）
- `CFunction::Execute` → `Execute_SEHhelper` → `ExecuteInBrace(1, ...)` でトップレベル `{}` を実行
- `ExecuteInBrace` はローカル `CSelecter output` に返り値候補を蓄積し、`ExecutionInBraceResult` を返す
- 返り値候補の蓄積: `ST_FORMULA_OUT_FORMULA` / ネスト `{}` / if-else / ループ各イテレーション の結果が `output.Append()` される
- `ST_FORMULA_SUBST`（代入）・`ST_VOID` は `output` に追加しない

### CSelecter（selecter.h / selecter.cpp）
- 返り値候補を「エリア」単位で管理。`--` が来るたびに `AddArea()` で新エリアを追加
- `Output()` で最終値を決定: デフォルトはランダム選択。`duplctl` の `choicetype_t` で挙動変化
- `clear()` でエリア・候補を全リセット（`areanum=0`、`values` クリア後1エリア追加）

### 制御フローの伝搬（exitcode）
- `exitcode`（int参照）は `ExecuteInBrace` の再帰チェーン全体を貫通する
- `break`/`continue`: 直近のループが `exitcode = ST_NOP` にリセット → それ以上伝搬しない
- `return`: 誰もリセットしない → 全ネストを貫通してトップまで伝搬する
- `return expr`: `CValue* pReturnExprResult` も同様に全レベルを貫通させ、各 `ExecuteInBrace` の終端で `output.clear()` + `output.Append(*pReturnExprResult)` して候補を上書き

## 値の構造（value.h / value.cpp）

- **`CValueSub` は廃止済み**（Tc600-1）。配列・ハッシュの要素も `CValue` なので入れ子にできる。旧コードや600-utf8から移植する際は `CValueSub(...)` を `CValue(...)` に置き換える
- VC6 は `CValue` が不完全型の間に `std::map<CValue,...>` を実体化できないため、ハッシュ関連のメンバ関数はクラス内に書かず、クラス外で inline 定義する
- `CValue::Less` は配列/ハッシュを扱えない。map のキーや集合要素の比較には `CValueLess` を使う（使わないと配列/ハッシュが全て同一視される）
- 多次元代入 `a[x][y] = v` はパース側（`parser1.cpp:CheckSubstSyntax`）と実行側（`CFunction::SubstToArray` → `FindUpperArrayOrder`）の両方で `]` を遡って処理している。配列序数まわりを変更する際は両方を揃える
  - 参照渡し `F(&a[x][y])` の書き戻し（`ExecFunctionWithArgs`）も同じ `SubstToArray` を使う。`FindFeedbackArrayOrder` で `&` が指す配列序数セルを求め、`RefreshUpperArrayOrder` で手前の次元を関数実行後の値に読み直してから渡す

## JSON/XML/YAML/TOML の入出力

- `FREAD*` / `PARSE*` / `FWRITE*` / `DUMP*` は `sysfunc.cpp` の共通処理（`FReadDataFile` / `ParseDataString` / `ParseUtf8Data` / `FWriteDataFile` / `DumpData` / `ValueToData`）に形式 `DATAFMT_*`（`jsonxml.h`）を渡して振り分ける。解析側はいったん UTF-8 の `std::string` にしてから各形式の関数に渡す
- JSON は parson、XML は tinyxml2（`jsonxml.cpp`）。YAML と TOML は自前（`yamltoml.cpp`）。YAML は1文書だけのサブセット（複数文書・複合キー・字下げのタブは非対応）
- 実数の書式（`AppendFiniteDouble`）、UTF-8 変換、`CNumericLocaleGuard`（解析・出力の間だけ `LC_NUMERIC` を C にする）は `jsonxml.h` で共有している
- VC6 の最適化は `inf - inf` を 0 に畳むので、NaN はビット列から作る（`yamltoml.cpp:MakeNan`）。NaN の判定も `d != d` ではなく `_isnan` を使う
- `yamltoml.cpp` のようにソース中に `\uXXXX` を書くファイルは `sjis_write` / `sjis_edit` で書くとエスケープが文字に展開される。UTF-8 で書いてから CP932・CRLF に変換する

## SQLite（SQLOPEN / SQLCLOSE / SQLEXEC / SQLQUERY）

- SQLite 本体は `sqlite3_yaya.c` がコンパイルオプション（`SQLITE_OMIT_LOAD_EXTENSION` など）を定義してから `sqlite/sqlite3.c` を `#include` する。オプションはここだけで決め、`.dsp` や makefile には書かない。C としてコンパイルすること（makefile では `$(CC)` の専用ルール。g++ に `.c` を渡すと C++ 扱いで通らない）
- 接続の管理と実行は `sqlitedb.cpp` の `CSqliteDB`（`vm.sqlite()`）、引数の検査とログは `sysfunc.cpp`（`SqlExecute` など）。`CFile` と同じくデータベースはパスの文字列（`ToFullPath` の結果、`":memory:"` はそのまま）で識別する。`./` などは正規化しないので、書き方が違えば別の接続になる
- `CSqliteDB` のコピーコンストラクタは接続を引き継がない（`CAyaVM` のディープコピーで同じ `sqlite3*` を二重に閉じないため）。unload では `CBasis::Termination` が `CloseAll` する
- 準備済みステートメントは接続ごとに SQL 文字列をキーにしてキャッシュする。初回は1文ずつ prepare → 実行する（`CREATE TABLE t...; CREATE INDEX ... ON t` のように前の文が作った表を次の文が参照すると、まとめて prepare できないため）
- パラメータは `args[2]` 以降。すべてスカラーなら1回実行、すべて配列/ハッシュなら1つを1回分として繰り返す（関数呼び出しで外側の配列が展開されるのを利用している）。値の対応は 整数/実数/文字列/VOID ↔ INTEGER/REAL/TEXT/NULL、BLOB は16進数の文字列
- 動作確認は EXE 構成の `yaya.exe` を標準入出力で動かす（`load:長さ\r\n<パス>` → `request:長さ\r\n<要求>` → `unload:0`）。辞書で戻り値を捨てる呼び出しは `_d = SQLEXEC(...)` のように代入しないと、関数の返り値の候補に混ざる
