# ユーザー関数呼び出しの試験と計測

ユーザー関数の呼び出し（`CFunction::Execute` から `ExecuteInBrace` / `GetFormulaAnswer` / `ExecFunctionWithArgs` に至る再帰の経路）を変えたときに、挙動が変わっていないことと、1 段あたりのスタック消費が増えていないことを確かめるための道具。

PowerShell 7（`pwsh`）で動かす。`yaya.exe` は EXE 構成のもの（`Release_EXE\yaya.exe` など）を `-Exe` に渡す。辞書は `%TEMP%\yaya_call_test` に写して走らせるので、リポジトリは汚れない。

## 構成

| ファイル | 内容 |
|---|---|
| `reg/reg.dic` | 回帰試験の辞書。再帰、引数と `&` 引数の書き戻し、返り値、出力候補（`--` / `melt` / `pool` / `switch` / `array` / `sequential` / `nonoverlap`）、ループ、`foreach`、`EVAL`、watcher / setter / destorier、`RE_*` の関数解決、`APPEND_RUNTIME_DIC` など |
| `run_reg.ps1` | 上の式を実行して結果を行ごとに出す。`-Baseline` に以前の出力を渡すと、違う行を表示して差があれば終了コード 1 |
| `deep/deep.dic` | 深い再帰の辞書（`fncdepth, 0`）。素直な再帰、`if` / `while` の入れ子を通る再帰、式の途中の再帰、引数なしの再帰、局所変数の多い再帰 |
| `probe_depth.ps1` | 上の再帰が耐える最大の深さを二分探索で測る |
| `stack_guard.ps1` | 深すぎる再帰でも落ちず、次の request も動くこと（スタック検査）を、`shiori.OnCallLimit` なし / あり / ハンドラ自身も再帰する、の 3 通りで確かめる |
| `frames.py` | `dumpbin /disasm` の出力から、関数ごとのフレームの大きさ（`sub esp,N` + 退避したレジスタ）を出す |
| `locals.py` | `cl /FAs` が出した `.asm` から、1 つの関数のスタック上の変数（一時変数を含む）を、スタック上の位置の順に出す |

## 回帰試験

```powershell
# 変更前に基準を取る（基準用の EXE は変更前のコミットでビルドしておく）
pwsh -NoProfile -File tests/call/run_reg.ps1 -Exe C:\work\base\yaya.exe -Out base.txt
# 変更後に比べる
pwsh -NoProfile -File tests/call/run_reg.ps1 -Exe Release_EXE\yaya.exe -Baseline base.txt
```

- `T.Rnd*` / `T.NonOv*` は乱数で選ぶので、実行のたびに変わる。`-Baseline` の比較からは外してある
- VC6 の EXE と MinGW の EXE のように、別の処理系同士でも同じ出力になる（`T.Cs` の呼び出し履歴や、浮動小数の表記も含めて）
- Debug EXE でも通すこと。デバッグヒープが解放済みの領域を埋めるので、値の受け渡し（swap）の誤りが表に出やすい

### 乱数の経路

出力候補から選ぶ処理（`CSelecter::Output` / `OutputConsume`）が、変更の前後で同じ乱数を同じ順に消費することを厳密に確かめたいときは、選択用の乱数の種を固定する。選択用の乱数（`CAyaVM::rs_internal64`）は `CAyaVM::load` で時刻から種を作る。`SRAND` が触るのは別系統（`rs_sysfunc64`）なので、辞書からは固定できない。

1. `ayavm.cpp` の `CAyaVM::load` に、環境変数で種を上書きする一時パッチを入れる（`getenv("YAYA_TEST_SEED")` を `dwSeed` に入れる。**検証後に必ず外す**）
2. 基準側は、変更を `git stash push -- <変更したファイル>` で退避して、同じパッチだけでビルドする
3. `$env:YAYA_TEST_SEED = '1'` のようにして `run_reg.ps1` を両方で流し、`T.Rnd*` / `T.NonOv*` を含めて出力を比べる。種を複数（1, 2, 3, 12345 など）で試す

## スタックの消費

```powershell
pwsh -NoProfile -File tests/call/probe_depth.ps1 -Exe Release_EXE\yaya.exe
pwsh -NoProfile -File tests/call/stack_guard.ps1 -Exe Release_EXE\yaya.exe
```

- EXE 構成の既定スタックは 1MB。深さの数字は、この条件での値
- フレームの大きさは、Release EXE をビルドしたあとに次で見る（`dumpbin` と `cl` は VC6 のもの）

  ```powershell
  python tests/call/frames.py Release_EXE\function.obj
  ```

  内訳は、同じフラグで `.asm` を出して読む。フラグは `aya5.dsp` の `Release EXE` の `ADD CPP` と同じにして、`/FAs /Fa<出力先>\ /Fo<出力先>\` を足す。

  ```powershell
  python tests/call/locals.py <出力先>\function.asm "?GetFormulaAnswer@CFunction"
  ```

- VC6 は、同じ型の一時変数でも領域を共有しない。`ExecuteInBrace` / `GetFormulaAnswer` / `ExecFunctionWithArgs` に `CValue` の一時変数や局所変数を足すと、再帰 1 段ごとに積まれるフレームがその分だけ増える。重い処理は再帰の経路に載らない別関数に出し、`YAYA_NOINLINE` を付けて `#pragma auto_inline(off)` で囲む（CLAUDE.md の「実行エンジン」を参照）

## 性能

呼び出しの速さは `tests/bench` の `B.Call0` / `B.Call1` で測る。再帰（フィボナッチなど）は、`bench.dic` に自分で関数を足して `GETTICKCOUNT` で測ること。
