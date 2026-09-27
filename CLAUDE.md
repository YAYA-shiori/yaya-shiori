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
- `messagetxt/*.txt` は `aya5.rc` からリソースとして埋め込まれるが、`/MAKE` はその変更を検知しない。messagetxt を変えたら `/REBUILD` するか、`Release/aya5.res` などを消してからビルドする

## リリース手順

1. バージョンを上げる（例: `Tc600-2`）。どちらも Shift JIS かつ CRLF なので Sjis_ 系ツールで編集する（Git Bash の `sed -i` は CRLF を LF に変えてしまう）
   - `manifest.cpp` の `aya_version`
   - `aya5.rc` の `FILEVERSION 6,00,2,0` と `VALUE "FileVersion", "6, 00, 2, 0\0"`（`TcXYY-N` → `X,YY,N,0`）
2. Release と ReleaseLangSep をリビルドし、ログ末尾が `ｴﾗｰ 0` であることを確認する
   ```powershell
   msdev aya5.dsw /MAKE "aya5 - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\yaya_build_Release.log"
   msdev aya5.dsw /MAKE "aya5 - Win32 ReleaseLangSep" /REBUILD /OUT "$env:TEMP\claude\yaya_build_ReleaseLangSep.log"
   ```
3. `make_aya.bat` を実行して `tmp/yaya.zip` と `tmp/yaya_lang_sep.zip` を作る
   - PowerShell から `cmd /c .\make_aya.bat` で実行する（Git Bash の `cmd //c make_aya.bat` はバッチが見つからず失敗する）
   - zip の更新日時が新しくなったこと、中の `yaya.dll` が `Release/` `ReleaseLangSep/` のものと一致することを確認する
4. コミットして push する。コミットメッセージの1行目は `変更内容 / TcXYY-N`、本文に変更点の箇条書き
5. GitHub のリリースを作る。タグ名とタイトルはバージョン、対象は現在のブランチ、2つの zip を添付、本文は変更点の日本語の箇条書き
   ```powershell
   gh release create Tc600-2 tmp/yaya.zip tmp/yaya_lang_sep.zip --target 600 --title Tc600-2 --notes-file <本文ファイル>
   ```
   - beta（`--prerelease`）を付けるかどうかは毎回ユーザーに確認する

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
