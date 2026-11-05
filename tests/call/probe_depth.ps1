<#
.SYNOPSIS
    再帰がどの深さまで耐えるかを、二分探索で測る
.DESCRIPTION
    deep/deep.dic の再帰関数（D.Plain / D.Nest / D.Expr / D.Many / D.NoArgStart）を、fncdepth, 0（無制限）で
    呼び、'ok' が返る最大の深さを探す。EXE 構成の既定スタックは 1MB。
    スタック検査（CCallLimit）が入っていると、落ちる代わりに E0097 で止まるので、検査が働く深さが出る。
    1 段あたりのスタック消費を変える変更（フレームの大きさ、再帰の経路に載る関数）の前後で比べる。
.EXAMPLE
    pwsh -NoProfile -File tests/call/probe_depth.ps1 -Exe Release_EXE\yaya.exe
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string[]]$Funcs = @('D.Plain', 'D.Nest', 'D.Expr', 'D.Many', 'D.NoArgStart'),
    [int]$Max = 30000,
    [string]$WorkDir = (Join-Path $env:TEMP 'yaya_call_test\deep')
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$yrun = Join-Path $here '..\bench\yrun.ps1'

if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
New-Item -ItemType Directory -Force $WorkDir | Out-Null
Copy-Item (Join-Path $here 'deep\*') $WorkDir -Force

function Test-Depth([string]$expr) {
    try {
        $rows = @(& $yrun -Exe $Exe -Dir $WorkDir -Exprs @($expr) 2>$null)
        $r = $rows | Where-Object { $_.Expr -ne '(load)' } | Select-Object -First 1
        return ($r -and $r.Result -match 'ok$')
    }
    catch { return $false }
}

foreach ($f in $Funcs) {
    $lo = 100; $hi = $Max
    if (-not (Test-Depth "$f($lo)")) { '{0}: fails even at {1}' -f $f, $lo; continue }
    if (Test-Depth "$f($hi)") { '{0}: survives {1}' -f $f, $hi; continue }
    while ($hi - $lo -gt [math]::Max(8, [int]($lo * 0.02))) {
        $mid = [int](($lo + $hi) / 2)
        if (Test-Depth "$f($mid)") { $lo = $mid } else { $hi = $mid }
    }
    '{0}: max depth ~ {1}' -f $f, $lo
}
