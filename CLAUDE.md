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

- parson / tinyxml2 / uka_regexp / sqlite / gumbo は git サブモジュール。クローン直後は `git submodule update --init` が必要
- 本体側では編集しない。tinyxml2 / uka_regexp / SQLite の修正は `../tinyxml2` / `../uka_regexp` / `../sqlite-amalgamation-new` のフォーク側で行ってから参照を更新する。tinyxml2 / SQLite は UTF-8 なので Sjis_ 系ツールで触らない（uka_regexp は ASCII のみ）
- サブモジュールを使う処理（JSON/XML、SQL* 関数、HTML、RE_* 関数）に手を入れる前に `git submodule update --init --remote parson tinyxml2 uka_regexp sqlite gumbo` で参照を最新にする
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
   - `aya5.rc` の `FILEVERSION 6,8,2,0` と `VALUE "FileVersion", "6, 8, 2, 0\0"`（`TcXYY-N` → `X,Y,N,0`。`YY` は先頭の0を付けない整数にする。`08` は windres が8進数として読んで警告し、MinGW のバージョン資源が不正確になる）
2. サブモジュール（`parson` / `tinyxml2` / `uka_regexp` / `sqlite` / `gumbo`）の参照を最新に更新する
   ```powershell
   git submodule update --init --remote parson tinyxml2 uka_regexp sqlite gumbo
   git submodule status
   ```
   - 参照が変わったら `git diff --submodule` で取り込まれるコミットを確認し、リリースのコミットに含める
   - ライセンス文の年や著作権者が変わっていたら `sysfunc.cpp` の `LICENSE` 関数の該当部分も合わせる（SQLite はパブリックドメイン。uka_regexp は Unlicense なので表記不要）。`README.md` の「同梱しているサードパーティのコード」の表も合わせる
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
- 制御フローは `exitcode`（int 参照）が `ExecuteInBrace` の再帰チェーンを貫通して伝わる。`break` / `continue` は直近のループが `ST_NOP` に戻して止める。`return` は誰もリセットせずトップまで伝わる。`return expr` は `CValue* pReturnExprResult` も全レベルを貫通させ、各 `ExecuteInBrace` の終端で `output.clear()` + `output.Append(*pReturnExprResult)` して候補を上書きする（この終端処理は `FinishBlock`）

### 再帰とスタック（ユーザー関数呼び出し）

- ユーザー関数 1 回の呼び出しで C++ のスタックに積まれるのは `ExecFunctionWithArgs` → `Execute`（`ExecuteToValue` / `ExecuteInto`）→ `Execute_SEHbody` → `Execute_SEHhelper` → `ExecuteInBrace`（`{}` / `if` / `while` の入れ子ごとに `ExecuteSubBlock` を挟んで 1 段）→ `GetFormulaAnswer` の鎖（各段に実行フレーム `CExecFrame &fr` が引数で渡る）。EXE 構成は既定の 1MB スタックで、`fncdepth, 0` の再帰が数百段で尽きる（実行フレーム導入で、素直な再帰が 648 段から 640 段、`if` の入れ子を通る再帰が 385 段から 370 段になった。引数が1つ増えた分で、許容した）
- **この鎖のフレームを大きくしない。** VC6 は同じ型の一時変数（約 56B の `CValue` など）でも領域を共有しないので、`ExecuteInBrace` / `GetFormulaAnswer` / `ExecFunctionWithArgs` に `CValue` の一時変数や局所変数を足すと、再帰 1 段ごとにその分だけ積まれる（演算ごとの一時変数でフレームが 1KB 超になっていた）。再帰の経路に載らない処理（ループ上限の処理、`melt` / `pool` の後処理、二項演算、`&` 引数の組み立てと書き戻し、値の受け渡し）は別の関数に出し、定義に `YAYA_NOINLINE` を付け、`#pragma auto_inline(off)` で囲んだ範囲に置く。VC6 には関数単位の `noinline` が無く、`/O2` は小さな関数を呼び出し元に展開して、分けたのにフレームが元に戻る
- フレームは Release EXE の `function.obj` を `python tests/call/frames.py` にかけて測る（`sub esp,N`）。深さは `tests/call/probe_depth.ps1`、回帰は `tests/call/run_reg.ps1`（使い方は `tests/call/README.md`）
- 返り値のコピーを増やさない。`CSelecter` は `STEAL` で候補を奪って作り（奪われた側は破棄しか許されない）、`OutputConsume` / `AppendTake` / `CValue::swap` で選んだ値を移す。`ExecuteToValue` が辞書内の呼び出しすべての入口で、`Execute(...)` の戻り値（`ExecutionResult`）の複製を避けている。式の最後の値は、最後の演算の項が持つ一時値で、他と共有されていないときだけ奪える（変数やリテラルの値、`emptyvalue` は壊さない）
- 乱数で選ぶ経路（`OutputConsume` の高速経路）は、`Output` と同じ回数だけ乱数を使い、`SetLso` も同じにすること。変えたら `tests/call/README.md` の「乱数の経路」で基準と厳密に比べる
- `CCallLimit::AddCall` は `fncdepth` だけでなく、C++ のスタックの残りが `CCALLLIMIT_STACK_MARGIN`（128KB。スタックが小さいときは残りの 1/4）を下回ったときも止める。`InitCall`（load / request / unload の入口）でスレッドのスタックの下端を取る（`GetCurrentThreadStackLow`。Windows は `VirtualQuery`、Linux / FreeBSD / macOS は pthread、Emscripten は取れないので検査なし）。止めたときは `fncdepth` 超過と同じ経路（E0097、`shiori.OnCallLimit`）で、スタック不足で呼んだハンドラの中でさらに足りなくなったときはハンドラを呼ばずログだけ出す（際限なく再帰しないため）。`sp` が記録した範囲の外にあるとき（`InitCall` を通らない別スレッド）は検査しない

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

## 時刻とタイムゾーン

- `GETTIME` / `GETSECCOUNT` / ログの時刻は `misc.cpp` の `EpochTimeToTM` / `TMToEpochTime`（`CTimeZone` でローカルか固定オフセットかを指定）を通す。年月日の計算は 64bit の暦計算（`DaysFromCivil` / `CivilFromDays`）で自前で行い、`localtime` / `mktime` / Win32 の時刻変換 API で年月日は求めない（範囲外で NULL や失敗になる、VC6 の `time_t` が 32bit、`mktime` の夏時間の判定が環境ごとに違う、といった差を避けるため）
- OS から借りるのは `GetLocalOffset`（あるUTC時刻でのローカルタイムのUTCオフセットと夏時間か）だけ。Windows は `SystemTimeToTzSpecificLocalTimeEx`（Windows 7 以降、年ごとの夏時間の規則も反映）→ `SystemTimeToTzSpecificLocalTime` → `GetTimeZoneInformation` の順に `GetProcAddress` で探して使い、夏時間かどうかはその年の1月と7月のオフセットの小さいほうを標準時とみなして決める。POSIX は `tzset` + `localtime_r` の `tm_gmtoff`。`FileTimeToLocalFileTime` / `LocalFileTimeToFileTime` は現在のバイアスしか使わないので、過去・未来の日時に使わない
- ローカル時刻 → EPOCH は、その時刻でのオフセットを求め、それで引いた時刻のオフセットを使い直す（夏時間の境目対策）。夏時間が戻る日の重複する1時間は、どちらか一方になる（`mktime` と同じく実装依存）
- `tm_yday` は0始まり（`localtime` と同じ）。タイムゾーン引数は文字列（`UTC` / `+09:00` / `JST` など）か整数（秒、東が正）で、`GETTIME` の2番目、`GETSECCOUNT` の8番目
- 日付文字列（`GETSECCOUNT(text)`）の読み取りは `sysfunc.cpp` の `Utils_ISO8601ToTM`（先に試す。0=ISO 8601 ではない / 1=成功 / -1=ISO 8601 の形だが値が不正で、-1 のときは HTTP 形式に回さない）→ `Utils_HTTPToTM`。HTTP 形式では `-` は英数字に続くときだけ日付の区切りとして扱う（タイムゾーンの符号を潰さないため）。どちらもタイムゾーンが書かれていなければローカルタイム、読めなければ W0012 と `-1`
- `GETTIMEZONE` / `GETSETTING("coreinfo.timezone")` は `misc.cpp` の `GetLocalTimeZoneInfo` / `GetLocalTimeZoneId`。IANA 名は、POSIX は `TZ` → `/etc/localtime` のリンク先 → `/etc/timezone`。Windows は `GetDynamicTimeZoneInformation` の `TimeZoneKeyName` を `icu.dll`（Windows 10 1903 以降。`LoadLibraryExW` に `LOAD_LIBRARY_SEARCH_SYSTEM32` を付けて読む）の `ucal_getTimeZoneIDForWindowsID` で変換する。`ucal_getDefaultTimeZone` は最初に決めた値を ICU が覚えてしまい実行中の設定変更に追従しないので使わない
- UTC や固定オフセットの指定は OS のタイムゾーンによらないので、スモークテストの共通ケースに入れてある。ローカルの夏時間は POSIX 専用の `tz_cases`（`TZ` をテスト側で設定）で確かめる

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

## 正規表現（RE_* 関数）

- エンジンは `uka_regexp`（`wchar_t` 専用、PCRE2 寄り）。RE_SEARCH / RE_MATCH / RE_ASEARCH(EX) / RE_GREP / RE_SPLIT / RE_REPLACE / RE_REPLACEEX は呼び出しごとに `CRegexpW` をコンパイルする（キャッシュなし）。`RE_OPTION` の `m s x i` は `CRegexpW::MULTILINE` などのフラグ
- RE_MATCH は `MatchExact`（先頭から末尾まで届く一致）。RE_GREP / RE_SPLIT は `GLOBAL` を付けてコンパイルし、同じ `CRegexpContext` で `Match` を繰り返す（空一致で止まらない）。RE_REPLACEEX は個数指定つきの `Replace` で、`RE_GETSTR` 系には最後に置換した部分（置換後の文字列中の位置）を返す
- 例外は `RegexErrorCode` で警告番号にする（パターンの誤り `ERR_SYNTAX` は W0016、それ以外は W0017）
- パターン内の未定義のサブルーチン名 `(?&name)` `\g<name>` などは `ReResolveSubroutine` が解決する。探す順は ローカル変数 `_name`（名前が `_` で始まるときはそのまま）→ グローバル変数 `name` → 関数 `name`（引数なしで実行した戻り値）。文字列と数値だけが使え、VOID・配列・ハッシュは次の候補へ回る。全部だめなら W0016。コンパイルのたびに呼ぶので、関数は副作用に注意

## SAORI-basic（LOADLIB / REQUESTLIB）

- `CLib1::IsBasicName` が拡張子で SAORI-basic（実行ファイル）かを判定する（Windows は `.dll` と拡張子なし以外、POSIX は `.dll` `.so` `.dylib` `.bundle` 以外）。SAORI-basic なら DLL は扱わず、`Load` / `Unload` / `Request` は `LoadBasic` / 何もしない / `RequestBasic` に分かれる
- `RequestBasic` は SAORI/1.0 の要求を解析し、`GET Version` には本体が応答する。`EXECUTE` は ArgumentN を1つずつの引数にして `RunBasic`（Windows は `CreateProcessW`、POSIX は fork → execv。シェルは通さない）で起動し、標準出力を Result（行を文字の `\r\n` でつなぐ）と ValueN（1行ずつ）にする。10秒・16MB で打ち切って 500
- システム辞書の `FUNCTIONEX` は `LOADLIB` / `REQUESTLIB` を呼ぶだけなので、辞書側は変えずに SAORI-basic を直接呼べる
- 出力を読むときは容量を倍々で確保する（`AppendOutput`）。VC6 の `std::string` は足りない分しか確保し直さず、大きな出力で二乗の時間がかかる

## ワーカースレッド（THREADSTART / THREADSTATE / THREADSTOP / THREADJOIN / THREADRESULT / THREADCLOSE）

`workthread.h/.cpp` の `CThreadManager`（VM ごとに1つ。`CAyaVM::threads()`）が、ワーカースレッドと、インタプリタ全体のロック（GIL）を持つ。辞書の関数を実行できるのは GIL を持つスレッドだけで、本当に並列に動くのは GIL を手放した区間（SLEEP、DIRECTSSTP の通信、THREADJOIN の待ち）だけ。

- **ホストは割り込まれない**。load / request / unload（`aya5.cpp` の `CHostScope`）は優先して GIL を取り、ワーカーは文の境界（`ExecuteInBrace` の先頭と `while` / `for` の周回）で `pvm->thread_attention()` を見て実行権を譲る。ホスト自身は途中で GIL を手放さない（THREADJOIN だけは、ほかのホストを入れずにワーカーへ譲る = `m_host_parked`）。ホストの再入（同じスレッドからの入れ子の request）は深さで数える
- **実行権が移るたびに差し替えるもの**（`AcquireLocked` / `ReleaseLocked`）: `CAyaVM` の「今の呼び出し制限」（`call_limit()` が返す。ホストでは NULL でホスト側のもの。関数表は共有なので差し替えない）、`CSystemFunction` の直前の呼び出しの状態（`CSysFuncState`。`GETLASTERROR`、`LSO`、`RE_GETSTR` など）、`CLog` のコールバックの繰り延べ。ローカル変数は `CLocalVariable` が関数ごとのインスタンスなので何もしなくてよい。`CSysFuncState` の正規表現の結果は `F_TAG_ARRAY` で初期化すること（空の値だと配列として使えなくなる）
- **関数表（中間コード）は全スレッドで共有し、実行中は読み取り専用**。式の評価中の一時値（`CCellTmp` = `ansv` / `order` / `emb_ansv`）は `CCell` に持たず、関数の1回の実行ごとの実行フレーム `CExecFrame`（`function.h`）に持つ。`ExecuteInto` / `ExecuteEval` が `CFrameHolder`（RAII）で借り、内部の関数（`ExecuteInBrace` / `GetFormulaAnswer` / `GetValueRefForCalc` など）には第1引数 `CExecFrame &fr` で渡す。再帰でも別スレッドからの同時実行でも、実行ごとにフレームが別なので一時値が衝突しない。以前あった `CFunctionReentryGuard`（再帰のたびにセルの一時値を退避する）、`MakePrivate`・`shared_ast`（スレッドごとの関数表のコピー）は無い。2万関数・2.5MB の辞書で、ワーカー起動は 18ms・13MB から 2ms・0.1MB 未満になった（Tc608-2）
  - 一時値の位置は `CStatement::tmpbase`（文の最初の項の位置。`CFunction::CompleteSetting` が決め、`framesize` が全体の大きさ）と、文の中の項の位置の和（`CExecFrame::at(st, cell)`）。**`st` と `fr` は必ず対応させる**。関数に属さない一時の文（`SolveEmbedCell` の `%` 埋め込み、`EVAL` の式 = `EvalTempStatement`）は、`CExecFrame t(st.cell_size())` の専用フレームで評価する（`tmpbase` は0のまま）。項の数を変えたら `CompleteSetting` を呼び直す
  - 使い終わったフレームは `CFunction::frame_cache`（`CFrameCache`）が1つだけ取っておき、非再帰の呼び出しで使い回す（一時値の確保をやり直さない）。**`CFrameCache` はコピーしても中身を引き継がない**（`CFunctionDef` のコピーや VM の複製で同じフレームを複数の関数が持つと、同時に使ってしまう）。借りる・返す（`AcquireFrame` / `ReleaseFrame`）はGILを持つ間に行う。大きな値は返すときに手放す（`tmpdata_cleanup`）
  - システム関数に渡す `pcellarg`（`CCell*`）は静的な情報（`index` / `name` / `value`）だけを持つ。評価済みの値は `valuearg` から取る（項の一時値は読めない。システム関数はフレームに依存しない）
- **中断は協調式**。`THREADSTOP` は `cancel` を立てて待機を起こす。ワーカーは `ThreadCancelStep` で `exitcode = ST_RETURN` にして `FeedLineToTail` で今の `{}` を抜ける（`return` 文と同じ経路。呼び出し元も次の文の境目で同じ確認に掛かって抜ける）。C++ の例外は使わない（`Execute_SEHbody` が `catch(...)` ですべて `memory_error` にする）。長いシステム関数1回の中では止まらない
- **GIL を手放している間は、`CValue`・`std::string`・`shared_ptr` に触れない**。`sharedptr.h` の参照カウントはスレッドセーフではなく、VC6 の `std::string` はコピーオンライトで参照カウントが非アトミック。ブロックする処理（`DIRECTSSTP` の送信など）は、引数を先に `std::string` / 整数に取り出し、`CGilRelease` の範囲では通信だけを行い、戻ってから `CValue` を作る
- **ログ**: `SetLoghandler` のコールバックとチェックツールへの `WM_COPYDATA` は、ワーカーが動いている間は `CLog::Call_loghandler` が溜め（`defer_callbacks`）、ホストが GIL を取った直後・手放す直前・待ちから戻った直後に `FlushDeferred` で送る（SHIORI のコールバックは load / request / unload を呼んだスレッドで呼ぶ約束。ワーカーが SSP の UI スレッドを待つと、UI スレッドが request の GIL 待ちで止まって固まる）。`FlushDeferred` は `mon` を持たずに呼ぶ（コールバックが request を呼びうる）。ファイルへの書き出しはその場で行う
- **辞書の変更**: ワーカーが1つでも動いている間は `DICLOAD` / `DICUNLOAD` / `APPEND_RUNTIME_DIC` / `UNDEFFUNC` を W0032 で断る（`CParser0::Dynamic*` の先頭）。`EVAL` は一時の `CFunction` を作るだけで関数表を触らないので許す
- **unload**: `CBasis::Termination` の先頭で `Shutdown`。全ワーカーに中断を求め、ホストが GIL を手放して5秒待つ。終わらないものは強制停止する（Windows は `TerminateThread`。GIL を持つスレッドに使うとヒープのロックを残しうる。POSIX は `pthread_cancel` が強制 unwind で `catch(...)` や `CGilRelease` のデストラクタと噛み合わないため、見捨てて `detach` し、GIL を持つものは待ち続ける）。`HostEnterForUnload` は GIL が10秒空かなければ持ち主を強制停止して奪う。`DllMain` の `DLL_PROCESS_DETACH` で `g_yaya_detaching` を立て、そのとき（ローダーロックを持っているので待てない）は待たず、ロックも取らない
- **外部の遅い応答を待つ関数**（`DIRECTSSTP`、`SLEEP`、`REQUESTLIB`、`EXECUTE_WAIT`）は、待つ間 GIL を手放す（`CGilRelease`）。ワーカーからなら常に、ホストからは動いているワーカーがあるときだけ（`alsoHost`。ホストが手放す間は別のホストを入れない）。`REQUESTLIB` は `CLib::Request` が、要求を別のバッファにコピーし、ログを `errlog` に溜めて（`CLib1::Request` / `RequestBasic` はその間 vm に触れない）、`CLib1::busy` を数えてから手放す。呼び出し中のライブラリは `UNLOADLIB` で消えない（`CLib::Delete` が 0 を返し W0013）。SAORI は同時に1つのスレッドからしか呼べないものとして、`LockLib(ライブラリ)` でライブラリごとに直列にする（**GIL を手放してから**取る。持ったまま待つと、持ち主が GIL を待って止まる）。`EXECUTE_WAIT` は Windows ではプロセスを 200ms ごとに見て中断を調べ、POSIX は終わるまで待つ。SAORI-basic の子プロセスの待ち（最大10秒）は中断で途中では戻らない
- **限界**: `multi_load` で複数の VM を作ると、別の VM のホストとワーカーはロックを共有せず並列に動くので、プロセス全体の静的変数は競合しうる。SAORI の DLL がスレッド親和性を持つ（COM、ウィンドウ）場合、ワーカーのスレッドから呼ぶと動かないことがある。Emscripten は非対応（`THREADSTART` は -5）
- **試験**: `tests/thread/run_thread.ps1`（EXE 構成。機能、同じ関数をホストと同時に実行、中断、辞書変更の拒否、unload、負荷）、`tests/thread/dll_host.c`（VC6 の `cl` でビルドする。DLL を `LoadLibrary` して unload / FreeLibrary / ExitProcess / ログのコールバックのスレッドを見る）、`tests/posix_smoke.c`（シェルモードの `request` を `THREADSTART` で呼ぶ3件）。**`makefile.mingw32` などはヘッダの依存を持たない**ので、ヘッダを変えたあとは `.o` を消して全部作り直す（古い `.o` が混ざると起動直後に落ちる）
