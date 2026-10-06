このディレクトリは伺かのSHIORI "YAYA" のコードです。
C++98/旧いVC++でコンパイルできるように製作されており、文字コードShift JISなので、Sjis_ではじまる編集・探索ツール群を使う必要があります。
コード管理ツールはgitです。
仕様は C:\D_DRIVE\MyDocuments\GitHub\yaya-docs にあります。

## ビルド

### VC++6

VC++6 (`msdev.exe`) が PATH に通っているため、`aya5.dsw` をコマンドラインからビルドできる。

```powershell
msdev aya5.dsw /MAKE "aya5 - Win32 Debug" /OUT "$env:TEMP\claude\yaya_build.log"
Get-Content "$env:TEMP\claude\yaya_build.log" -Encoding oem
```

- 構成は `aya5 - Win32 Release` / `Debug` / `Release EXE` / `Debug EXE` / `ReleaseLangSep`（全部は `aya5 - ALL`）。`/MAKE` は差分ビルドで、全再ビルドは `/REBUILD`
- `msdev.exe` はGUIアプリのためコンソールに何も出ない。結果は `/OUT` のログで見る（Shift JIS なので `-Encoding oem`）。成功時の末尾は `yaya.dll - ｴﾗｰ 0、警告 N`。VC6 の STL 由来の警告 C4786 は無視してよい
- Git Bash から実行すると `/MAKE` がパス変換されるので `MSYS_NO_PATHCONV=1` を前置する
- `messagetxt/*.txt` は `aya5.rc` からリソースとして埋め込まれるが、`/MAKE` は変更を検知しない。変えたら `/REBUILD` するか `Release/aya5.res` などを消す

### サブモジュール

- parson / tinyxml2 / deelx / sqlite / gumbo は git サブモジュール。クローン直後は `git submodule update --init` が必要
- 本体側では編集しない。tinyxml2 / deelx / SQLite の修正は `../tinyxml2` / `../deelx` / `../sqlite-amalgamation-new` のフォーク側で行ってから参照を更新する。いずれも UTF-8 なので Sjis_ 系ツールで触らない
- サブモジュールを使う処理（JSON/XML、SQL* 関数、HTML）に手を入れる前に `git submodule update --init --remote parson tinyxml2 deelx sqlite gumbo` で参照を最新にする
- SQLite のフォークは upstream の amalgamation に VC6 / 古い SDK 向けの修正を1コミット載せたもの。64bit のリテラルは `INT64_C()` / `UINT64_C()`（`LL` は VC6 が、`i64` は gcc が読めない）。修正したら VC6 と gcc（Strawberry Perl 同梱）の両方で `sqlite3.c` 単体がコンパイルできることを確かめる

### VC6 以外（makefile / GitHub Actions）

- `makefile.linux` が基準で、`freebsd` / `posix`（macOS、`.bundle`）/ `emscripten` はオブジェクト一覧と規則を linux と同じにしてある。ソースを増やしたら全部の一覧に足す（`makefile.mingw32` は `posix_utils.o` に加えて `aya5_res.o` も持つ）
- ソースは Shift JIS（CP932）で統一する。POSIX 系の makefile は `.cpp` を iconv で UTF-8 にしてからコンパイルし（変換できない文字があれば止まる。`#line` で元のファイルを指す）、MinGW は `-finput-charset=CP932` なので、UTF-8 のファイルが混ざると通らない。POSIX ではヘッダは変換しないので、ヘッダの非 ASCII はコメントだけにする
- `makefile.mingw32` は Windows 用の yaya.dll を MinGW-w64 で作る（文字列を VC++ と同じく CP932 のまま扱う）。Strawberry Perl 同梱の gcc（x86_64）で確かめられる。リポジトリを汚さないよう `git ls-files --recurse-submodules` のファイルを作業用ディレクトリに写してから `mingw32-make -f makefile.mingw32`（動作確認用の EXE は `exe` ターゲット）
  - MinGW は `_WINDOWS` だが `_MSC_VER` ではない。SEH（`__try`）など MSVC 専用の書き方は `_MSC_VER` で分け、64bit の整数リテラルは `LL_DEF()` / `ULL_DEF()` で書く
- Linux / macOS は手動実行の `.github/workflows/posix-build.yml`（`gh workflow run posix-build.yml --ref 600`）でビルドし、`tests/posix_smoke.c` で dlopen → load → request → unload → dlclose まで通す。スモークテストは空ディレクトリを load してシェルモードにし、式を EVAL した結果を比べる（期待値を足すときは `yaya.exe` で先に確かめる）
  - gcc で通って clang（macOS）で落ちる典型: `yaya::int_t`（`std::int64_t`）は Linux が `long`、macOS が `long long` なので、`ptrdiff_t` / `size_t` から `CValue(...)` を作ると曖昧になる（`static_cast<yaya::int_t>` を付ける）。他の翻訳単位から呼ぶ関数は1ファイルだけで `inline` 定義しない（gcc はたまたまリンクできるが clang は実体を出さない）。macOS の `<fcntl.h>` は `FREAD` / `FWRITE` をマクロにする（`sysfunc.h` で `#undef`）。`basename` には `<libgen.h>` が要る
  - POSIX 版 `CBasis::ExecuteRequest` は成功時に入力バッファを `free` しない（エラー時は `free` する）。テストも成功時は解放しない
  - POSIX のパスは UTF-8。内部の文字列からは `posix_path()`（UTF-8 化と `fix_filepath`）で作り、OS から受け取った名前は `Ccct::MbcsToUcs2Buf(..., CHARSET_UTF8)` で戻す（旧 `narrow` / `widen` は廃止）。文字コード変換は iconv（macOS は `-liconv`）で、OS デフォルトは UTF-8 扱い
  - 公開するのは `DLLEXPORT` の関数だけ（`-fvisibility=hidden`）。スモークテストの共通ケースは Windows の `yaya.exe` で期待値を確かめてから足す。非 ASCII は UTF-8 の `\x` エスケープで書き、`\x` の直後に16進の文字が続くときは文字列を区切る
  - Emscripten は CI の対象外（最新の emsdk では `-shared` が `-fPIC` 必須の SIDE_MODULE になりリンクが通らない）。MinGW / VC6 も CI には載せていない

## リリース手順

1. バージョンを上げる（例: `Tc600-2`）。どちらも Shift JIS かつ CRLF なので Sjis_ 系ツールで編集する（Git Bash の `sed -i` は CRLF を LF に変えてしまう）
   - `manifest.cpp` の `aya_version`
   - `aya5.rc` の `FILEVERSION 6,00,2,0` と `VALUE "FileVersion", "6, 00, 2, 0\0"`（`TcXYY-N` → `X,YY,N,0`）
2. サブモジュール（`parson` / `tinyxml2` / `deelx` / `sqlite` / `gumbo`）の参照を最新に更新する
   ```powershell
   git submodule update --init --remote parson tinyxml2 deelx sqlite gumbo
   git submodule status
   ```
   - 参照が変わったら `git diff --submodule` で取り込まれるコミットを確認し、リリースのコミットに含める
   - ライセンス文の年や著作権者が変わっていたら `sysfunc.cpp` の `LICENSE` 関数の該当部分も合わせる（SQLite はパブリックドメイン）
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

## 動作確認とシェルモード

- 通常モードで読む辞書が1つも無い（設定ファイルが無い、dic/dicif/dicdir の結果が0個）と、`CParser0::ParseShellDictionary` が `load{}unload{}request{EVAL(_argv[0])}` だけの組み込み辞書（`_SHELL_DIC_`）を作り、request の入力を EVAL した結果を返す。ログには注記 N0002 が出る
- 設定ファイルが無いときは変数の自動保存・復元をしない。緊急モード（`yaya_emerg.txt`）はシェルモードにならず、従来どおり設定ファイルが無ければ抑止する
- そのため辞書を用意しなくても、EXE 構成の `yaya.exe` を標準入出力で動かして確認できる: `load:長さ\r\n<空のディレクトリ>` → `request:長さ\r\n<式や文>` → `unload:0`。関数の戻り値を捨てる呼び出しは `_d = SQLEXEC(...)` のように代入しないと、返り値の候補に混ざる

## パーサ

- 読み込みは2フェーズ。フェーズ1（`LoadDictionary1`、ファイルごと）は識別子を全て `F_TAG_NOP` のまま `StructFormula` でセル列にするだけで、**全ファイルの読み込みが終わるまで関数名の解決はしない**。フェーズ2（`ParseAfterLoad`）で型付け（`_` 始まりはローカル変数、それ以外の未解決名はグローバル変数）と演算順序の決定を行う
- `ParseAfterLoad` の各段階と parser1 の構文検査（`SetBreakJumpNo` 〜 `SetIfJumpNo`、`CheckExecutionCode`）には、辞書ファイル名で関数表を回す版と `CFunction&` を受け取る関数1個版がある。辞書ファイル名版は関数1個版をループで呼ぶだけなので、処理を変えるときは関数1個版を直す。`StoreInternalStatement` / `MakeStatement` も `CFunction&` を受け取る
- `EVAL` / `ISEVALUABLE` は `CParser0::IsEvalBlock`（クォート外の `;` `{` `}` 改行、または先頭が制御文のキーワード）で文の並びかを判定する。文の並びなら `ParseEvalBlock` で関数表に登録しない一時 `CFunction` を作り、`CFunction::ExecuteEval` で呼び出し元の `lvar` のもとで実行する（`_argv` などは上書きしない）。関数表の deep copy や差し替え（`func_parse_new` / `func_parse_to_exec`）は起こさない
- 演算子は左から右に結合する（C/C++ と違い右結合ではない）。`F_TAG_FUNCPARAM` は右辺のオペランドが必須（無ければ E0022）。`CCell::depth` はローカル変数のスコープ深さ用だが実際には未使用で、フラグ用途に流用されている

### %埋め込み（括弧なし）と embed.lazy

- `"..%name.."` の括弧なし部分は `F_TAG_STRING_EMBED` の項になり、既定（`embed.lazy, on`）では実行時に `CFunction::SolveEmbedCell` がその時点の変数・関数の最長一致で名前を探す
- `embed.lazy, off` のときは `CParser0::FixEmbedName` が読み込み後に名前を確定し、項の `depth`（種別 1/2/3=変数/関数/システム関数）と `index`（一致した長さ、0 なら文字列のまま）に記録する。`index >= 0` が確定済みの印。変数は `CVariable::in_code`（`SetCellType1` が式に書かれた名前に立てる）が立ったものだけが対象。ローカル変数（`_` 始まり）は実行時のまま
- 全辞書の変数名が出揃ってから確定するため、`Parse` では `ParseAfterLoad` のループの後に呼ぶ。`DICLOAD` / `APPEND_RUNTIME_DIC` / `EVAL`（`ParseEmbedString` / `ParseEvalBlock`）でもそれぞれの解析の後に呼ぶ
- `%[n]` は `F_TAG_STRING_EMBED` の項を数えるので、文字列のまま残るものも型は変えない

## 実行エンジン

- `ST_FORMULA`(=6)以上のステートメントだけがセルを持つ。`SetCellType` はそれ全てを型付けするので、`ST_RETURN` にセルを持たせても自動的に型解決される。`SetFormulaType` は `ST_FORMULA` だけを `ST_FORMULA_OUT_FORMULA` / `ST_FORMULA_SUBST` に再分類し、`ST_IF` / `ST_RETURN` 等は変えない
- 引数なしキーワード（`break` / `continue` / `{` / `}` / `--` など）は `StoreInternalStatement` がセルなしの `CStatement` を直接作り、引数ありのもの（`if` / `while` / `return expr` など）は `MakeStatement` → `StructFormula` でセルを作る
- `ExecuteInBrace` はローカルの `CSelecter output` に返り値候補を蓄積する。候補になるのは `ST_FORMULA_OUT_FORMULA`・ネスト `{}`・if-else・ループ各イテレーションの結果で、`ST_FORMULA_SUBST`（代入）と `ST_VOID` は含めない。`CSelecter` は `--` ごとに「エリア」を分け、`Output()` の既定はランダム選択（`duplctl` で変わる）
- 制御フローは `exitcode`（int 参照）が `ExecuteInBrace` の再帰チェーンを貫通して伝わる。`break` / `continue` は直近のループが `ST_NOP` に戻して止める。`return` は誰もリセットせずトップまで伝わる。`return expr` は `CValue* pReturnExprResult` も全レベルを貫通させ、各 `ExecuteInBrace` の終端で `output.clear()` + `output.Append(*pReturnExprResult)` して候補を上書きする

## 値（value.h / value.cpp）

- **`CValueSub` は廃止済み**（Tc600-1）。配列・ハッシュの要素も `CValue` なので入れ子にできる。旧コードや600-utf8から移植する際は `CValueSub(...)` を `CValue(...)` に置き換える
- VC6 は `CValue` が不完全型の間に `std::map<CValue,...>` を実体化できないため、ハッシュ関連のメンバ関数はクラス内に書かず、クラス外で inline 定義する
- `CValue::Less` は配列/ハッシュを扱えない。map のキーや集合要素の比較には `CValueLess` を使う（使わないと配列/ハッシュが全て同一視される）
- 多次元代入 `a[x][y] = v` はパース側（`parser1.cpp:CheckSubstSyntax`）と実行側（`CFunction::SubstToArray` → `FindUpperArrayOrder`）の両方で `]` を遡って処理している。配列序数まわりを変更する際は両方を揃える
  - 参照渡し `F(&a[x][y])` の書き戻し（`ExecFunctionWithArgs`）も同じ `SubstToArray` を使う。`FindFeedbackArrayOrder` で `&` が指す配列序数セルを求め、`RefreshUpperArrayOrder` で手前の次元を関数実行後の値に読み直してから渡す

## ロケール

- `setlocale` はプロセス全体に効く（POSIX ではホストにも）ので、一時的に切り替える書き方はしない。実数の文字列化・解析は `wsex.cpp` の `ws_atof` / `ws_ftoa` / `ws_decimal_point_to_dot`（`localeconv()` の小数点を `.` に直す）を通す。`wcstod` / `snprintf("%f")` を直接使うと、小数点が `,` のロケール（ドイツ語など）で `0.25 * 4` が `0,000000` になる。`STRFORM` の実数もこれを通している
- Windows の `AYA_InitModule` は `setlocale(LC_ALL, "")` の後に `LC_NUMERIC` だけ `"C"` へ戻す。確かめるときは、そこに `setlocale(LC_NUMERIC, "German_Germany")` を一時的に足して EXE 構成をビルドし、`yaya.exe` に `0.25 * 4` や `TOREAL("1.5")*2` を EVAL させる（終わったら必ず外す）。POSIX は `setlocale` を呼ばないのでホストのロケールのまま
- `TOUPPER` / `TOLOWER` のロケールは `Ccct::MapCase`。Windows は `LCMapStringEx`（`LocaleNameToLCID` が `0x1000` を返す名前は未知とみなし、CRT のロケール名として `setlocale` 経由に回す）、POSIX は `newlocale` + `towupper_l` / `towlower_l`（UTF-8 版を先に探す）。使えない名前は W0012 を出して C モードで変換する。`LCMapStringEx` は未知の `xx-ZZ` でも成功してしまうので、名前の検証は必須
- OS のロケール名は `Ccct::GetOsLocaleName`（`GETSETTING("coreinfo.locale" / "coreinfo.uilocale")`）。POSIX は環境変数だけを見る

## JSON/XML/YAML/TOML/HTML の入出力

- `FREAD*` / `PARSE*` / `FWRITE*` / `DUMP*` は `sysfunc.cpp` の共通処理（`FReadDataFile` / `ParseDataString` / `FWriteDataFile` / `DumpData` など）に形式 `DATAFMT_*`（`jsonxml.h`）を渡して振り分ける。解析側はいったん UTF-8 の `std::string` にしてから各形式の関数に渡す
- JSON は parson、XML は tinyxml2（`jsonxml.cpp`）。YAML と TOML は自前（`yamltoml.cpp`）で、YAML は1文書だけのサブセット（複数文書・複合キー・字下げのタブは非対応）
- 実数の書式（`AppendFiniteDouble`）、UTF-8 変換、`CNumericLocaleGuard`（解析・出力の間だけ `LC_NUMERIC` を C にする）は `jsonxml.h` で共有している
- VC6 の最適化は `inf - inf` を 0 に畳むので、NaN はビット列から作る（`yamltoml.cpp:MakeNan`）。NaN の判定も `d != d` ではなく `_isnan` を使う
- `yamltoml.cpp` のようにソース中に `\uXXXX` を書くファイルは `sjis_write` / `sjis_edit` で書くとエスケープが文字に展開される。UTF-8 で書いてから CP932・CRLF に変換する

## SQLite（SQLOPEN / SQLCLOSE / SQLEXEC / SQLQUERY）

- SQLite 本体は `sqlite3_yaya.c` がコンパイルオプション（`SQLITE_OMIT_LOAD_EXTENSION` など）を定義してから `sqlite/sqlite3.c` を `#include` する。オプションはここだけで決め、`.dsp` や makefile には書かない。C としてコンパイルする（makefile では `$(CC)` の専用ルール。g++ に `.c` を渡すと C++ 扱いで通らない）
- データベースは `CFile` と同じくパスの文字列（`ToFullPath` の結果、`":memory:"` はそのまま）で識別する。`./` などは正規化しないので、書き方が違えば別の接続になる
- `CSqliteDB` のコピーコンストラクタは接続を引き継がない（`CAyaVM` のディープコピーで同じ `sqlite3*` を二重に閉じないため）。unload では `CBasis::Termination` が `CloseAll` する
- SQL の中に書いたパス（`ATTACH DATABASE`、`VACUUM INTO` など）は SQLite にそのまま渡る。相対パスを `base_path` から解決するため、`CSqliteDB::Open` は既定の VFS を写して `xFullPathname` だけ差し替えた VFS（`SqliteBaseVfs`）を接続ごとに登録する。POSIX ではここで `fix_filepath` も通す（`ToFullPath` の結果は `\` 区切りのため）
- 準備済みステートメントは接続ごとに SQL 文字列をキーにしてキャッシュする。初回は1文ずつ prepare → 実行する（`CREATE TABLE t...; CREATE INDEX ... ON t` のように前の文が作った表を次の文が参照すると、まとめて prepare できないため）
- パラメータは `args[2]` 以降。すべてスカラーなら1回実行、すべて配列/ハッシュなら1つを1回分として繰り返す（関数呼び出しで外側の配列が展開されるのを利用している）。BLOB は16進数の文字列

## SAORI-basic（LOADLIB / REQUESTLIB）

- `CLib1::IsBasicName` が拡張子で SAORI-basic（実行ファイル）かを判定する（Windows は `.dll` と拡張子なし以外、POSIX は `.dll` `.so` `.dylib` `.bundle` 以外）。SAORI-basic なら DLL は扱わず、`Load` / `Unload` / `Request` は `LoadBasic` / 何もしない / `RequestBasic` に分かれる
- `RequestBasic` は SAORI/1.0 の要求を解析し、`GET Version` には本体が応答する。`EXECUTE` は ArgumentN を1つずつの引数にして `RunBasic`（Windows は `CreateProcessW`、POSIX は fork → execv。シェルは通さない）で起動し、標準出力を Result（行を文字の `\r\n` でつなぐ）と ValueN（1行ずつ）にする。10秒・16MB で打ち切って 500
- システム辞書の `FUNCTIONEX` は `LOADLIB` / `REQUESTLIB` を呼ぶだけなので、辞書側は変えずに SAORI-basic を直接呼べる
- 出力を読むときは容量を倍々で確保する（`AppendOutput`）。VC6 の `std::string` は足りない分しか確保し直さず、大きな出力で二乗の時間がかかる
