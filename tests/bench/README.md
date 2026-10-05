# YAYA ベンチマーク（Claudia「作業履歴」の頁より）

Claudia ゴースト（`..\claudia`）の「Claude Code は？」→「作業履歴」の頁は、YAYA にとってかなり意地の悪い負荷になっている。
`~/.claude/history.jsonl`（約 3MB・8 千行）を 1 行ずつ `FREAD` して文字列を切り出し、ハッシュに溜め、
数百件のファイルの存在を `FSIZE` で確かめ、会話の記録の末尾を読み、JSON を解く。
この処理の形をそのまま取り出して、ボトルネックを探るためのベンチにした。

## 構成

| ファイル | 内容 |
|---|---|
| `gen-data.ps1` | 実データと同じ形の合成データを作る（乱数固定）。履歴 7855 行 / 2423 セッション、会話の記録 493 件 |
| `bench.dic` | マイクロ・ベンチ。ループ、関数呼び出し、`STRSTR` / `SUBSTR` / `REPLACE`、ハッシュ、`FREAD` / `FSIZE`、`PARSEJSON` など要素ごと |
| `bench_scenario.dic` | シナリオ・ベンチ。`CC.HistScan` / `HistList` / `ReadTail` / `HistRow` / 頁 1 枚 / `CCS.Span` を元の形のまま写したもの |
| `yaya.txt` | ベンチ用の設定（`looplimit, 0`） |
| `yrun.ps1` | `yaya.exe` を標準入出力で駆動する。1 回の load のあとに式を順に EVAL して、時間と結果を返す |
| `run.ps1` | 上をまとめて走らせて表にする |
| `profile/` | サンプリング・プロファイラ入りの `yaya.exe` を作業コピーに作り、結果を集計する（後述） |

## 使い方

PowerShell 7（`pwsh`）で動かす。Windows PowerShell 5.1 の `powershell -File` では、`run.ps1` が `unload` の後で止まることがあった（原因は未解明）。

```powershell
# 既定は Release_EXE\yaya.exe（VC6 の Release EXE）。-Exe で別の EXE（MinGW 版など）に替えられる
pwsh -NoProfile -File tests/bench/run.ps1
pwsh -NoProfile -File tests/bench/run.ps1 -Set scenario     # 頁の処理だけ（6 秒ほど）
```

合成データは初回に `%TEMP%\yaya_bench\home` へ作る（2 分ほど）。辞書は `%TEMP%\yaya_bench\ghost` に写して走らせるので、リポジトリは汚れない。

### 測定の注意

- **ループは `looplimit`（既定 10000）で黙って打ち切られる。** `for` も `while` も 10000 回で止まる（実測）。ベンチは `looplimit, 0`。回数を増やしても時間が変わらないときはこれを疑うこと
- 時計は `GETTICKCOUNT`（約 15.6ms 刻み）。1 本が 200ms 以上になる回数にする。短い項目は 0.000 や 15.6ms の倍数に量子化される
- **初めて開くファイルは遅い。** リアルタイム保護が有効な Windows では、新しくコピーしたファイルの初回オープンが 1 件 2.5ms（2 回目は 66µs）かかる。合成データを作り直した直後は 1 回空打ちしてから測る
- EVAL の式の中に書いたループより、登録済みの関数の中に書いたループの方が測りやすい。ただし速度そのものは同じ

## 基準値（VC6 Release EXE、この PC。下の「対応済み」の修正を入れる前）

マイクロ（`run.ps1 -Set micro`。ループ 1 周の分を含む。1 回あたり µs）:

| 項目 | µs | 項目 | µs |
|---|---|---|---|
| 空ループ 1 周 | 0.66 | `FREAD`（1 行 / 404B） | **19.9** |
| 代入 `_x = _i + 1` | 0.91 | `FSIZE`（あるファイル） | **32.8** |
| `if / elseif / else` | 1.56 | `FSIZE`（無いファイル） | 18.8 |
| ユーザー関数呼び出し（引数なし / 1 個） | **4.5 / 6.6** | `FOPEN` + `FCLOSE` | 37.6 |
| `STRSTR` / `SUBSTR` / `STRLEN` / `TOINT` | 2.9 / 2.5 / 1.7 / 1.9 | `FSEEK` + `FREAD` 1 行 | 10.2 |
| `REPLACE` / `RE_REPLACE` / `SPLIT` | 3.0 / 7.3 / 3.0 | `GETENV` | 4.7 |
| `GETTIME` | 3.6 | `PARSEJSON`（292 字） | 25 |
| ハッシュ代入 / 参照 | 3.8〜4.1 / 3.0 | `PARSEJSON`（8008 字、日本語） | **703**（88ns/字） |
| `foreach` ハッシュ 1 要素 | 0.59 | `ASORT`（文字列 500 件） | 625〜740 |
| 配列 `,=` 整数 / ハッシュ | 1.0〜1.1 / 6〜8 | ハッシュ 800 件のコピー / 関数への受け渡し | 0.8 / 6.3（コピーオンライトで効いている） |

シナリオ（`run.ps1 -Set scenario`。1 回あたり ms）:

| 処理 | ms | 備考 |
|---|---|---|
| `HistScan`（773 セッション） | 169 | 履歴の末尾 2MB ほどを 5 千行読む |
| `HistList`（493 件） | 247 | `HistScan` + 773 件の `FSIZE` など |
| `ReadTail` 1 件 | 12 | 末尾 256KB を読んで JSON を解く |
| 頁 1 枚（`HistList` + `HistRow` × 6） | **309** | 実機の `CC.PageHist` は約 300ms で一致 |
| `Span`（7855 行） | **488** | 実機の `CCS.Span`（月の索引つき）は約 780ms |

## 対応済み（局所的な修正。同じデータ・同じ PC で旧 → 新）

| 修正 | 内容 |
|---|---|
| `ws_fgets`（`wsex.cpp`） | 暗号化なしの経路を `fgets` の塊読み（512 バイト）に。NUL を含む行も壊さないよう、バッファを 0xFF で埋めて終端 NUL の位置から長さを出す |
| `CFile1::Open`（`file1.cpp`） | Windows はサイズ取得の seek / `ftell` 3 往復（stdio のバッファが捨てられる）をやめ、ハンドルから `GetFileSize` |
| `FSIZE`（`sysfunc.cpp`、Windows） | `GetFileAttributesExW` で、開かずにサイズを取る。ディレクトリ・リパースポイント・取得失敗は従来の `CreateFile` 経路 |
| `Ccct_ConvUTF8ToUnicode`（`ccct.cpp`） | 1 文字ずつ `append(1, c)` をやめ、先に確保して直接書き込む |
| `ExecFunctionWithArgs` / `ExecSystemFunctionWithArgs`（`function.cpp`） | 引数ベクタを `reserve`。`&` 引数が無い呼び出しでは、書き戻し用の 3 本のベクタを作らない |
| `GETENV`（`sysfunc.cpp`、Windows） | `GetEnvironmentVariableW` で直接取る（`getenv` の全走査と文字コード変換 2 回をやめる）。値が ANSI に収まらない文字も欠けなくなる |

| 項目 | 旧 | 新 |
|---|---|---|
| `FREAD` 1 行（404B） | 18.0 µs | **5.9 µs** |
| `HistScan` | 156 ms | 88 ms |
| `HistList`（ファイルが温まっている） | 206 ms | 141 ms |
| `HistList`（初めて開く 493 ファイル。AV のスキャンあり） | 1594 ms | **172 ms** |
| `ReadTail` 1 件 | 9.4 ms | 3.0 ms |
| 頁 1 枚 | 294 ms | **166 ms**（−44%） |
| `Span`（7855 行） | 469 ms | 328 ms（−30%） |
| `GETENV` | 5.0 µs | 1.9 µs |
| `REPLACE` / `SPLIT`（引数 2〜3 個の呼び出し） | 3.3 / 3.0 µs | 2.0 / 2.3 µs |
| Claudia の全辞書の load | 約 141 ms | 約 131 ms（約 7%） |

検証: 改修前後の EXE に、境界ケースのファイル（改行なし・CRLF・NUL・0xFF・BOM・511 / 512 / 513 バイトの行・空ファイル・サロゲート・途切れた UTF-8・不正な UTF-8）の
`FREAD` / `FSIZE`、`&` 引数と配列引数の書き戻し、`GETENV` の各種名前を食わせ、46 ケースで比べた。差は `GETENV` が `é` を欠かなくなった 1 件だけ。
MinGW 版でも同じ 46 ケースが VC6 版と一致した。

## ボトルネックの所見（優先順。1〜2 と 4 の一部は上で対応済み）

### 1. `FREAD` の 1 バイトずつの `fgetc`（頁の約 24%、`FREAD` の約 65%）

`yaya::ws_fgets`（`wsex.cpp`）は 1 バイトずつ `fgetc` して `std::string` に足している。`Release` は `/MT`（静的リンクの
マルチスレッド CRT）なので、`fgetc` のたびに CRT のロック（クリティカルセクション）を出入りする。
サンプラーでは `FREAD` の包括時間の 65% が `fgetc` で、そのうち約 50% が `RtlEnter/LeaveCriticalSection`。

単独の C++ で同じ読み方を比べた（gcc -O2、同じ 3.1MB・7855 行）:

| 読み方 | 1 行あたり |
|---|---|
| 現行（`fgetc` + 倍々 `reserve` + 1 文字 `+=`） | 7.6 µs |
| `_getc_nolock`（1 バイトずつ、ロックなし） | 1.8 µs |
| `fgets`（512 バイトずつ、1 行につきロック 1 回） | **1.1 µs** |

`FREAD` 全体は 18〜20 µs/行で、`fgetc` を直しても 8〜10 µs/行は残ると見ていた。計測できた残りは `MbcsToUcs2Buf`（UTF-8 → UCS-2）の 13% で、
これは 1 文字ずつの `append` をやめて対応した。`fgets` 化と合わせて 18.0 → 5.9 µs/行。`ToFullPath`、`CFile::Read` のリスト検索、毎回の `std::string buf; buf.reserve(1000)` は
読んだだけの候補で未計測。`ws_fgets` は辞書の読み込み（`parser0.cpp`）にも使われるが、Claudia の全辞書の load は約 7% しか縮まなかった（辞書の読み込みは行読みが支配的ではない）。

### 2. `FSIZE` が毎回ファイルを開いている（頁の約 12%。初回アクセスでは支配的）

`CSystemFunction::FSIZE` は `CreateFileW` → `GetFileSizeEx` → `CloseHandle`。1 回 19〜33 µs（`FOPEN` + `FCLOSE` は 38 µs）。
それ以上に、リアルタイム保護が有効だと**初めて開くファイルに 2.5ms** かかる（493 ファイルの初回 1.2 秒、2 回目は 32ms）。
開かずに属性だけ見る `GetFileAttributesExW` なら、初回でも 170 µs（.NET の `FileInfo.Length` で測った。AV のオンアクセススキャンを受けない）。
Claude Code の会話の記録は書き換わり続けるので、再スキャンの対象になりやすい。さらに、開かないので他のプロセスが排他で開いていても
サイズが取れる。

### 3. インタプリタの 1 文あたりの基本コスト（頁の約 30% がヒープの確保/解放）

空ループ 1 周が 0.66 µs、ユーザー関数 1 回が 4.5 µs、システム関数 1 回が 2〜3 µs。gcc -O2 のビルド（サンプラー入り）では空ループ 0.31 µs / 関数呼び出し 1.7 µs と
約 2 倍速く、VC6 のコード生成の分もある。サンプラーで見える内訳:

- **ブロックに入るたびに `CSelecter output`**（`std::vector<CVecValue>` を持つ）を作って壊す。ループ 1 周ごとのヒープ確保/解放（`~CSelecter` が 12〜21%）
- **ローカル変数は名前の `std::wstring` 比較で線形探索**（`CLocalVariable::GetIndex` が末尾から全部走査）。ホット変数を先に宣言した関数は、
  後ろの変数の数に比例して遅くなる（実測、1 周あたり: 埋め草 0 個で 0.99 µs、10 個で 1.35、30 個で 2.08、60 個で 2.86。ホット変数を最後に宣言すれば 0.89 で一定）
- システム関数・ユーザー関数の呼び出しごとに、引数ベクタや `shared_ptr<CValue>` を確保/解放する（`GetFormulaAnswer` の呼び出し行に寄る分）

候補: 結果を使わないブロック（ループ本体など）の `CSelecter` を遅延生成する / 変数の位置（深さと番号）を `CCell` にキャッシュする / 呼び出しごとの確保を減らす。
どれもインタプリタの中心なので影響範囲が広い。

### 4. その他

- `FTELL`（`CFile1::FTell`）は `CCS.Span` で約 6〜10%。テキストモードの `ftell` は CRLF 変換の分を数え直すので遅く、位置を自分で数える手もあるが、
  テキストモードの CRLF 変換とずれないようにするのが難しい（未対応）
- ~~`GETENV` は 4.7 µs~~ → 対応済み（1.9 µs）
- `PARSEJSON` は 88 ns/字。会話の記録の末尾の行（4〜30KB）を解くと 0.4〜2.6 ms
- `ASORT` は文字列 500 件で 0.6〜0.7 ms（1 件あたり 1.3 µs。内訳は未調査）

## Claudia 側への所見（YAYA ではなく辞書の話）

- **`looplimit` が既定の 10000 のまま**（`system_config.txt` などに無い）。`CC.HistScan` と `CCS.Span` は履歴を `while 1` で読むので、
  読む行が 10000 を超えると**新しい側が黙って落ちる**はず（`while` / `for` が 10000 で止まることは実測。履歴が 1 万行を超えた場合の挙動は未検証）。
  今は 7855 行（350 日、1 日に 22 行ほど）で、`Span` が全期間を数えるなら 3 か月ほどで届く。`OnLoopLimit` を見るか、読む前に `looplimit` を上げて戻す（`lint.dic` がやっている）のが無難
- 頁を開くたびに履歴 2MB 前後を読み直している（約 170ms）。ファイルの大きさと更新時刻をキーに結果を覚えれば、2 回目からほぼ 0 になる
- `HistList` は 773 セッション全部に `FSIZE` をかけて、残る 493 件を使う。上の 2 を直すまでは、新しい順に 6 件分だけ確かめて次の頁で続きを確かめる手もある

## プロファイラ（`profile/`）

MinGW-w64（64bit）では gprof のサンプルが取れなかったので、本体スレッドを 1ms ごとに止めてスタックを採る小さなサンプラーを作った
（停止中はコンテキストとスタックのコピーだけ。巻き戻しは再開後に `StackWalk64`。デッドロックを避けるため）。

```powershell
pwsh -NoProfile -File tests/bench/profile/build.ps1          # %TEMP%\yaya_prof に作業コピーを作ってビルド（75 秒ほど）
$w = "$env:TEMP\yaya_bench\ghost"
pwsh -NoProfile -File tests/bench/yrun.ps1 -Exe "$env:TEMP\yaya_prof\yaya.exe" -Dir $w -UserProfile "$env:TEMP\yaya_bench\home" -Exprs 'S.RunPage(0, 5)'
pwsh -NoProfile -File tests/bench/profile/agg.ps1 -File "$w\samples.txt" -Exe "$env:TEMP\yaya_prof\yaya.exe" -Src "$env:TEMP\yaya_prof"
```

- request の実行中だけ採り、`unload` で作業ディレクトリに `samples.txt` を書く。**`yrun.ps1` の出力を `Select -First` などで打ち切ると `unload` まで進まず、ファイルができない**
- 集計は「最寄りの YAYA ソース行に寄せた時間」「葉の自己時間」「スタックに含まれる包括時間」。EXE 内のアドレスは `addr2line` で関数名と行にする
- 数字は gcc -O2 のビルドの分布。絶対時間は VC6 の EXE より小さいが、どこが重いかの順は同じだった
