<#
.SYNOPSIS
    作業履歴の頁をもとにしたベンチ（マイクロ + シナリオ）を yaya.exe で走らせる
.DESCRIPTION
    1. 合成データ（history.jsonl と会話の記録）が無ければ gen-data.ps1 で作る（2 分ほど）
    2. bench の辞書を作業用ディレクトリに写す（yaya_variable.cfg でリポジトリを汚さないため）
    3. yaya.exe に USERPROFILE=<DataRoot> を渡して load し、式を順に EVAL して結果を表にする

    -Exe は EXE 構成の yaya.exe（既定は Release_EXE\yaya.exe。VC6 の Release EXE）。
    -Set micro（要素ごと）/ scenario（頁の処理）/ all。
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File tests/bench/run.ps1
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File tests/bench/run.ps1 -Set scenario -Exe C:\work\gcc\yaya.exe
#>
param(
    [string]$Exe,
    [string]$DataRoot = (Join-Path $env:TEMP 'yaya_bench\home'),
    [string]$WorkDir = (Join-Path $env:TEMP 'yaya_bench\ghost'),
    [ValidateSet('micro', 'scenario', 'all')][string]$Set = 'all'
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path   # 5.1 の -File 起動では param の既定値で $PSScriptRoot が使えない
if (-not $Exe) { $Exe = Join-Path $here '..\..\Release_EXE\yaya.exe' }

if (-not (Test-Path -LiteralPath (Join-Path $DataRoot '.claude\history.jsonl'))) {
    Write-Host "合成データを作ります: $DataRoot"
    & (Join-Path $here 'gen-data.ps1') -Root $DataRoot | Write-Host
}
New-Item -ItemType Directory -Force $WorkDir | Out-Null
foreach ($f in 'yaya.txt', 'bench.dic', 'bench_scenario.dic') {
    Copy-Item -LiteralPath (Join-Path $here $f) -Destination (Join-Path $WorkDir $f) -Force
}

# 回数は VC6 の Release EXE で 1 本あたり 0.2〜0.5 秒になるよう選んである
$micro = @(
    'B.Line',
    'B.Loop(500000)', 'B.Assign(500000)', 'B.If(300000)', 'B.Call0(100000)', 'B.Call1(100000)',
    'B.Strstr(150000)', 'B.StrstrShort(150000)', 'B.Substr(150000)', 'B.Strlen(250000)', 'B.Toint(250000)',
    'B.Replace(100000)', 'B.ReReplace(30000)', 'B.Split(100000)', 'B.Getenv(50000)', 'B.Gettime(100000)',
    'B.Concat(40000)',
    'B.ArrayAppend(100000)', 'B.ArrayAppendHash(20000)', 'B.ArrayIndexHash(100000)',
    'B.HashSetNew(100000)', 'B.HashSetSame(100000)', 'B.HashGet(100000)', 'B.HashForeach(500)',
    'B.HashCopy(20000)', 'B.HashReturn(10000)', 'B.HashNested(20000)', 'B.Asort(400)',
    'B.Fsize(10000)', 'B.FsizeMissing(10000)', 'B.FopenClose(5000)', 'B.Seek(20000)',
    'B.FreadAll', 'B.ScanNoHash', 'B.ScanFull',
    'B.ParseJson(10000)', 'B.ParseJsonBig(1000)'
)
$scenario = @(
    'S.RunHistScan(0, 5)', 'S.RunHistList(0, 5)', 'S.RunReadTail(0, 5)', 'S.RunPage(0, 5)', 'S.RunSpan(0, 5)'
)
$exprs = @()
if ($Set -ne 'scenario') { $exprs += $micro }
if ($Set -ne 'micro') { $exprs += $scenario }

$rows = & (Join-Path $here 'yrun.ps1') -Exe $Exe -Dir $WorkDir -Exprs $exprs -UserProfile $DataRoot
Write-Host ('exe: ' + (Resolve-Path -LiteralPath $Exe).Path)
foreach ($r in $rows) {
    if ($r.Expr -eq '(load)') { Write-Host ('load: {0} ms' -f $r.Ms); continue }
    if ($r.Result -match '^[A-Za-z0-9_.]+\(.*\)$|^\d+$') { continue }   # B.Line の戻り値など
    Write-Host $r.Result
}
