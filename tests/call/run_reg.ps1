<#
.SYNOPSIS
    ユーザー関数呼び出しの回帰試験: reg/reg.dic の式を yaya.exe（EXE 構成）で実行し、結果を行ごとに出す
.DESCRIPTION
    再帰・引数・&引数の書き戻し・返り値・出力候補（--/melt/pool/switch/array/sequential）・
    ループ・EVAL・watcher/setter・APPEND_RUNTIME_DIC など、関数呼び出しの経路を通る処理を並べてある。
    変更の前後（または VC6 と MinGW など）で出力を比べて、挙動が変わっていないことを確かめる。
    -Baseline に以前の出力を渡すと、違う行だけを表示して、差があれば終了コード 1 にする。

    乱数で選ぶ行（T.Rnd* / T.NonOv*）は、実行のたびに変わるので、-Baseline との比較からは外す。
    厳密に比べたいときは README.md の「乱数の経路」を見ること。
.EXAMPLE
    pwsh -NoProfile -File tests/call/run_reg.ps1 -Exe Release_EXE\yaya.exe -Out before.txt
    pwsh -NoProfile -File tests/call/run_reg.ps1 -Exe Release_EXE\yaya.exe -Baseline before.txt
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$Out,
    [string]$Baseline,
    [string]$WorkDir = (Join-Path $env:TEMP 'yaya_call_test\reg')
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$yrun = Join-Path $here '..\bench\yrun.ps1'

# 辞書は作業用ディレクトリに写して走らせる（ログなどでリポジトリを汚さないため）
if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
New-Item -ItemType Directory -Force $WorkDir | Out-Null
Copy-Item (Join-Path $here 'reg\*') $WorkDir -Force

$exprs = @(
    'T.Sum(10)', 'T.Sum(0)', 'T.Fib(15)', 'T.W(8)', 'T.Ev(7)', 'T.Od(7)', 'T.Ev(10)',
    'T.Str(6, ">")', 'T.Deep(5)', 'T.Deep(14)', 'T.Deep(20)', 'T.Deep(40)',
    'T.Fb1()', 'T.Fb2()', 'T.Fb3()', 'T.Fb4()', 'T.Fb5()', 'T.NoFb()',
    'T.Argc()', 'T.Args()', 'T.ArgsArr()', 'T.Local()',
    'T.Ret(1)', 'T.Ret(0)', 'T.RetLoop(3)', 'T.RetLoop(30)', 'T.RetNested()', 'T.RetVoid()',
    'T.RetArrUse()', 'T.HashUse()', 'T.VoidUse()',
    'T.Area()', 'T.All()', 'T.Last()', 'T.ArrUse()', 'T.SeqUse()', 'T.MeltUse()', 'T.PoolUse()', 'T.SwUse()',
    'T.Loops()', 'T.LoopBreak()', 'T.LoopLimit()', 'T.Void2()',
    'T.Cs(0)', 'T.Cs(3)', 'T.Ev2(5)', 'T.Fx()', 'T.Cmp()', 'T.Nest()', 'T.NestFn()', 'T.Pass()', 'T.Big()',
    'T.Fe()', 'T.FeRec(6)',
    'T.RndUse()', 'T.RndAreaUse()', 'T.RndNestUse()', 'T.NonOvUse()', 'T.RndRecUse()',
    'T.OutN()', 'T.Watch()', 'T.WatchErase()', 'T.EvalBlock()', 'T.EvalRec(3)', 'T.ReFn()', 'T.GrowUse()',
    'T.Sum(5)', 'T.NAStart(8)', 'T.FbRecUse()', 'T.Ops()', 'T.Short()', 'T.Sum(10)', 'T.Fib(15)',
    'T.HistUse()', 'T.FbRecAUse()', 'T.FbIdxUse()', 'T.DelimUse()'
)
$rows = & $yrun -Exe $Exe -Dir $WorkDir -Exprs $exprs
$lines = @(foreach ($r in $rows) { if ($r.Expr -ne '(load)') { '{0} => {1}' -f $r.Expr, $r.Result } })

if ($Out) {
    [IO.File]::WriteAllLines($Out, [string[]]$lines, (New-Object Text.UTF8Encoding($false)))
}

if ($Baseline) {
    $skip = '^T\.(Rnd|NonOv)'
    $a = @(Get-Content $Baseline -Encoding utf8 | Where-Object { $_ -notmatch $skip })
    $b = @($lines | Where-Object { $_ -notmatch $skip })
    $diff = 0
    for ($i = 0; $i -lt [math]::Max($a.Count, $b.Count); $i++) {
        if ($a[$i] -ne $b[$i]) { $diff++; Write-Host ('BASE: ' + $a[$i]); Write-Host ('NEW : ' + $b[$i]) }
    }
    Write-Host ('compared {0} lines (random lines excluded): {1} differences' -f $a.Count, $diff)
    if ($diff) { exit 1 }
}
else {
    $lines
}
