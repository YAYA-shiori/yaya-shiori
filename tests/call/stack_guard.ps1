<#
.SYNOPSIS
    スタック検査（CCallLimit）の試験: 深すぎる再帰でも落ちず、次の request も動くことを確かめる
.DESCRIPTION
    fncdepth, 0（無制限）で深い再帰を呼び、次の 3 通りで確かめる。
      plain    shiori.OnCallLimit なし   -> E0097 のログが出る
      handler  shiori.OnCallLimit あり   -> ハンドラが呼ばれる（D.Hits の値が増える）。ログは出ない
      nested   ハンドラ自身も深く再帰する -> ハンドラが連鎖せず、ログだけ出して止まる
    どれも、途中で D.Plain(100) が 'ok' を返し続けること（プロセスが落ちていない）と、
    OnMemoryError（E0100）が出ていないことを見る。失敗があれば終了コード 1。
.EXAMPLE
    pwsh -NoProfile -File tests/call/stack_guard.ps1 -Exe Release_EXE\yaya.exe
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$yrun = Join-Path $here '..\bench\yrun.ps1'
$root = Join-Path $env:TEMP 'yaya_call_test\guard'

$handler = @'

shiori.OnCallLimit
{
	G.hit = G.hit + 1
	'handled'
}

D.Hits
{
	G.hit
}
'@
$nested = @'

shiori.OnCallLimit
{
	G.hit = G.hit + 1
	_x = D.Plain(100000)
	'handled'
}

D.Hits
{
	G.hit
}
'@
$cases = @(
    @{ Name = 'plain';   Extra = '' },
    @{ Name = 'handler'; Extra = $handler },
    @{ Name = 'nested';  Extra = $nested }
)
$exprs = @('D.Plain(100)', 'D.Plain(5000)', 'D.Plain(100)', 'D.Nest(3000)', 'D.NoArgStart(5000)', 'D.Fat(5000)', 'D.Plain(100)', 'D.Hits()')

$failed = 0
foreach ($c in $cases) {
    $dir = Join-Path $root $c.Name
    if (Test-Path $dir) { Remove-Item $dir -Recurse -Force }
    New-Item -ItemType Directory -Force $dir | Out-Null
    Copy-Item (Join-Path $here 'deep\*') $dir -Force
    if ($c.Extra) { Add-Content (Join-Path $dir 'deep.dic') $c.Extra -Encoding utf8 }

    # プロセスが落ちると yrun が例外を投げる。そのときは答えた行だけが残る（FAIL になる）
    $rows = @()
    try { $rows = @(& $yrun -Exe $Exe -Dir $dir -Exprs $exprs 2>$null | Where-Object { $_.Expr -ne '(load)' }) }
    catch { }
    $log = @(Get-Content (Join-Path $dir 'yaya_log.txt') -Encoding utf8 -ErrorAction SilentlyContinue)
    $e0097 = @($log | Select-String 'E0097').Count
    $crash = @($log | Select-String 'E0100|OnMemoryError').Count

    $ok = ($rows.Count -eq $exprs.Count)                                  # 全部の request に答えた（落ちていない）
    if (-not $ok) { '{0,-8} FAIL  rows={1}/{2} (process died or request unanswered)' -f $c.Name, $rows.Count, $exprs.Count; $failed++; continue }
    foreach ($i in 0, 2, 6) { if ($rows[$i].Result -ne 'ok') { $ok = $false } }   # 浅い再帰は常に正しい
    if ($crash) { $ok = $false }
    if ($c.Name -eq 'handler' -and ($e0097 -ne 0 -or [int]$rows[7].Result -le 0)) { $ok = $false }
    if ($c.Name -ne 'handler' -and $e0097 -eq 0) { $ok = $false }
    if ($c.Name -eq 'nested' -and [int]$rows[7].Result -le 0) { $ok = $false }

    '{0,-8} {1}  rows={2}/{3} E0097={4} memory-error={5} hits={6}' -f $c.Name, $(if ($ok) { 'PASS' } else { 'FAIL' }), $rows.Count, $exprs.Count, $e0097, $crash, $rows[-1].Result
    if (-not $ok) { $failed++ }
}
if ($failed) { exit 1 }
