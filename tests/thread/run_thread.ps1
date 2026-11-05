<#
.SYNOPSIS
    THREADSTART などワーカースレッドの試験
.DESCRIPTION
    EXE 構成の yaya.exe を標準入出力で駆動する（tests/bench/yrun.ps1 を使う）。
    request の間はEXEが標準入力を待つだけなので、その間にワーカーが進む。
    辞書は %TEMP%\yaya_thread_test に写して走らせるので、リポジトリは汚れない。
    1 つ目の走行で機能を、2 つ目の走行で「動いているワーカーがあってもunloadが終わる」ことを確かめる。
.EXAMPLE
    pwsh -NoProfile -File tests/thread/run_thread.ps1 -Exe Release_EXE\yaya.exe
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$WorkDir = (Join-Path $env:TEMP 'yaya_thread_test')
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$yrun = Join-Path $here '..\bench\yrun.ps1'

function Prepare-Dir {
    if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
    New-Item -ItemType Directory -Force $WorkDir | Out-Null
    Copy-Item (Join-Path $here 'thread.dic'), (Join-Path $here 'yaya.txt') $WorkDir -Force
    Copy-Item (Resolve-Path $Exe).Path (Join-Path $WorkDir 'yaya.exe') -Force
}

# 実行はしない関数を n 個足した辞書にする（ワーカーを繰り返し起動・終了しても、辞書の大きさに応じたメモリが増えないことを見るため）
function Add-Filler([int]$n) {
    $sb = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $n; $i++) {
        [void]$sb.Append("Fill.F$i`n{`n`t_a = $i`n`t_b = 'text $i ' + TOSTR(_a)`n`tif _a > 10 {`n`t`t_b = _b + 'big'`n`t}`n`telse {`n`t`t_b = _b + 'small'`n`t}`n`t_b`n}`n`n")
    }
    [IO.File]::WriteAllText((Join-Path $WorkDir 'filler.dic'), $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
    Add-Content (Join-Path $WorkDir 'yaya.txt') 'dic, filler.dic'
}

function Dic([string]$name) { return "`"$name { 1 }`"" }

# 式 / 期待する結果 / 応答時間の上限(ms。0なら見ない)
$cases = @(
    # 基本
    @('THREADSTART("TH.Add", 3, 4)', '1', 0),
    @('THREADJOIN(1)', '1', 0),
    @('THREADRESULT(1)', '7', 0),
    @('THREADSTATE(1)', '1', 0),
    @('THREADSTATE(99)', '-1', 0),
    @('THREADSTART("NoSuch")', '-1', 0),
    @('THREADCLOSE(1)', '0', 0),
    @('THREADSTATE(1)', '-1', 0),
    # 同じ関数をホストと2つのワーカーが同時に走らせる
    @('THREADSTART("TH.Shared", 30000)', '2', 0),
    @('THREADSTART("TH.Shared", 30000)', '3', 0),
    @('TH.Shared(100)', '4950', 0),
    @('TH.Shared(2000)', '1999000', 0),
    @('TH.Shared(100)', '4950', 0),
    @('THREADJOIN(2)', '1', 0),
    @('THREADJOIN(3)', '1', 0),
    @('THREADRESULT(2)', '449985000', 0),
    @('THREADRESULT(3)', '449985000', 0),
    # グローバル変数
    @('THREADSTART("TH.Count", 2000)', '4', 0),
    @('THREADJOIN(4)', '1', 0),
    @('TH.counter', '2000', 0),
    # 終わらないループを中断する。ホストの応答が遅れないこと
    @('THREADSTART("TH.Spin")', '5', 0),
    @('1+1', '2', 400),
    @('1+1', '2', 400),
    @('TH.Shared(500)', '124750', 400),
    @('THREADSTATE(5)', '0', 0),
    @('THREADSTOP(5)', '1', 0),
    @('THREADJOIN(5, 5000)', '1', 0),
    @('THREADSTATE(5)', '2', 0),
    @('TH.spin > 0', '1', 0),
    @('THREADSTOP(5)', '0', 0),
    # 入れ子の呼び出しの途中で止める
    @('THREADSTART("TH.SpinDeep")', '6', 0),
    @('1+1', '2', 400),
    @('THREADSTOP(6)', '1', 0),
    @('THREADJOIN(6, 5000)', '1', 0),
    @('THREADSTATE(6)', '2', 0),
    # SLEEP は中断で早く戻る。動いている間は辞書を変えられない
    @("APPEND_RUNTIME_DIC($(Dic 'TH.AppendA'))", '0', 0),
    @('THREADSTART("TH.Sleeper", 60000)', '7', 0),
    @('THREADSTATE(7)', '0', 0),
    @("APPEND_RUNTIME_DIC($(Dic 'TH.AppendB'))", '-1', 0),
    @('ISFUNC("TH.AppendB")', '0', 0),
    @('THREADSTOP(7)', '1', 0),
    @('THREADJOIN(7, 5000)', '1', 1500),
    @('THREADSTATE(7)', '2', 0),
    @('TH.slept', '', 0),
    @("APPEND_RUNTIME_DIC($(Dic 'TH.AppendC'))", '0', 0),
    # スレッドの中からスレッド、辞書変更の拒否、変数を作る
    @('THREADSTART("TH.Nest")', '8', 0),
    @('THREADJOIN(8)', '1', 0),
    @('THREADRESULT(8)', '30', 0),
    @('THREADSTART("TH.DicInThread")', '10', 0),
    @('THREADJOIN(10)', '1', 0),
    @('THREADRESULT(10)', '-1', 0),
    @('THREADSTART("TH.MakeVars", 300)', '11', 0),
    @('THREADJOIN(11)', '1', 0),
    @('THREADRESULT(11)', '300', 0),
    @('TH.made299', '299', 0),
    # 終わっているスレッドの待ちはすぐ戻る・待ちの時間切れ・閉じると中断される
    @('THREADJOIN(11, 0)', '1', 0),
    @('THREADSTART("TH.Sleeper", 60000)', '12', 0),
    @('THREADJOIN(12, 200)', '0', 1500),
    @('THREADCLOSE(12)', '1', 0),
    @('THREADJOIN(12, 5000)', '1', 1500),
    # システム関数の状態（正規表現の結果）はスレッドごと
    @('THREADSTART("TH.ReState")', '13', 0),
    @('RE_MATCH("q", "q")', '1', 0),
    @('THREADJOIN(13, 5000)', '1', 0),
    @('THREADRESULT(13)', 'abc', 0)
)

# 外部の遅い応答（SAORI-basic、EXECUTE_WAIT）を待つ間も、ホストのrequestや他のスレッドが止まらない（Windows）
if ($IsWindows) {
    $cases += @(
        @('THREADSTART("TH.SaoriReq", 3)', '14', 0),
        @('1+1', '2', 400),
        @('1+1', '2', 400),
        @('TH.Shared(500)', '124750', 400),
        # 呼び出し中のライブラリはUNLOADLIBで消えない（UNLOADLIBは常に空の値を返す。断られたことは警告W0013に出る）
        @('UNLOADLIB(GETENV(''SystemRoot'') + ''\System32\cmd.exe'')', '', 0),
        @('THREADJOIN(14, 10000)', '1', 0),
        @('THREADRESULT(14)', '1', 0),
        @('THREADSTART("TH.ExecWait", 4)', '15', 0),
        @('1+1', '2', 400),
        @('THREADSTOP(15)', '1', 0),
        @('THREADJOIN(15, 3000)', '1', 1500),
        @('THREADSTATE(15)', '2', 0),
        @('THREADSTART("TH.ExecWait", 2)', '16', 0),
        @('1+1', '2', 400),
        @('THREADJOIN(16, 10000)', '1', 0),
        @('THREADRESULT(16)', '0', 0),
        # ホストもSAORIを呼ぶ。ワーカーが同じライブラリを呼び出している間は順番を待つ
        @('THREADSTART("TH.SaoriReq", 3)', '17', 0),
        @('TH.SaoriReq(1)', '1', 0),
        @('THREADJOIN(17, 10000)', '1', 0),
        @('THREADRESULT(17)', '1', 0),
        # EXECUTE_CAPTURE も待つ間はほかを止めない。中断を求められたら子プロセスを終了させて戻る
        @('THREADSTART("TH.Capture", 30)', '18', 0),
        @('1+1', '2', 400),
        @('THREADSTOP(18)', '1', 0),
        @('THREADJOIN(18, 3000)', '1', 1500),
        @('THREADSTATE(18)', '2', 0),
        @('THREADSTART("TH.Capture", 2)', '19', 0),
        @('1+1', '2', 400),
        @('THREADJOIN(19, 10000)', '1', 0),
        @('THREADRESULT(19)', 'ok,0', 0),
        # 子が終わった後に孫が出力し続けていても（終了後の読み取り中でも）中断できる
        @('THREADSTART("TH.CaptureFlood")', '20', 0),
        @('1+1', '2', 400),
        @('THREADSTOP(20)', '1', 0),
        @('THREADJOIN(20, 3000)', '1', 1500),
        @('THREADSTATE(20)', '2', 0)
    )
}

# SLEEP（実行権を手放す）をまたいで、式の途中で同じ関数をホストとワーカーが同時に走らせる
# （呼び出し元が式の途中で持つ一時値を、同じ関数の別の実行が壊さない）
$midId = if ($IsWindows) { 21 } else { 14 }
$cases += @(
    @('THREADSTART("TH.MidSleep", 300, 5)', "$midId", 0),
    @('TH.MidSleep(10, 20)', '1030', 0),
    @('TH.MidSleep(0, 0)', '1000', 0),
    @("THREADJOIN($midId)", '1', 0),
    @("THREADRESULT($midId)", '1305', 0)
)

Prepare-Dir
$exprs = @($cases | ForEach-Object { $_[0] })
$rows = @(& $yrun -Exe (Join-Path $WorkDir 'yaya.exe') -Dir $WorkDir -Exprs $exprs) | Where-Object { $_.Expr -ne '(load)' }

$fail = 0
for ($i = 0; $i -lt $cases.Count; $i++) {
    $c = $cases[$i]
    $r = $rows[$i]
    $ok = ($r.Result -eq $c[1])
    $slow = ($c[2] -gt 0 -and $r.Ms -gt $c[2])
    if (-not $ok -or $slow) {
        $fail++
        '{0}: {1}  expected [{2}] got [{3}]  {4}ms{5}' -f 'FAIL', $c[0], $c[1], $r.Result, $r.Ms, $(if ($slow) { " (limit $($c[2])ms)" } else { '' })
    }
}
'{0} / {1} cases passed' -f ($cases.Count - $fail), $cases.Count

# ログに想定外の警告が無いか（W0034 = 強制停止、E = エラー）
$log = Join-Path $WorkDir 'yaya_log.txt'
if (Test-Path $log) {
    $bad = Select-String -Path $log -Pattern 'W0034|E0\d\d\d' -Encoding utf8 | Select-Object -First 10
    if ($bad) { $fail++; 'unexpected log lines:'; $bad | ForEach-Object { '  ' + $_.Line } }
}

# 負荷：ワーカーの起動と終了の繰り返し、複数ワーカーの同時中断、ホストとの文字列・配列の競合
Prepare-Dir
Add-Filler 3000
$stress = @(
    @('TH.Churn(60)', '60', 0),
    @('THREADSTART("TH.Mixed", 20000)', '61', 0),
    @('THREADSTART("TH.Mixed", 20000)', '62', 0),
    @('TH.Shared(3000)', '4498500', 0),
    @('TH.Churn(20)', '20', 0),
    @('THREADJOIN(61)', '1', 0),
    @('THREADJOIN(62)', '1', 0),
    @('THREADRESULT(61)', '', 0),
    @('TH.mixed', '40000', 0),
    @('TH.Burst(20)', '20', 0),
    @('1+1', '2', 400),
    @('TH.Shared(100)', '4950', 400),
    # THREADCLOSEしなくても、終わったワーカーが資源を持ち続けない（使い捨てのスレッドを繰り返してもメモリが増えない）
    @('TH.ChurnKeep(20)', '20', 0),
    @('1+1', '2', 0),
    @('TH.ChurnKeep(300)', '300', 0),
    @('1+1', '2', 0)
)
$rows2 = @(& $yrun -Exe (Join-Path $WorkDir 'yaya.exe') -Dir $WorkDir -Memory -Exprs @($stress | ForEach-Object { $_[0] })) | Where-Object { $_.Expr -ne '(load)' }
for ($i = 0; $i -lt $stress.Count; $i++) {
    $c = $stress[$i]; $r = $rows2[$i]
    if ($c[0] -eq 'THREADRESULT(61)') { continue }  # 配列の長さは見ない（落ちずに値が返ればよい）
    $slow = ($c[2] -gt 0 -and $r.Ms -gt $c[2])
    if ($r.Result -ne $c[1] -or $slow) { $fail++; 'FAIL(stress): {0}  expected [{1}] got [{2}]  {3}ms' -f $c[0], $c[1], $r.Result, $r.Ms }
}
$ws0 = $rows2[$stress.Count - 3].WS_MB
$ws1 = $rows2[$stress.Count - 1].WS_MB
'working set after 20 workers: {0} MB, after 320 workers: {1} MB' -f $ws0, $ws1
if ($ws1 - $ws0 -gt 150) { $fail++; 'FAIL(memory): finished workers keep their function tables' }
'stress done'

# unload：動いているワーカーがあっても、すぐ終わること
Prepare-Dir
$sw = [Diagnostics.Stopwatch]::StartNew()
$null = @(& $yrun -Exe (Join-Path $WorkDir 'yaya.exe') -Dir $WorkDir -Exprs @('THREADSTART("TH.Spin")', 'THREADSTART("TH.Sleeper", 60000)', 'THREADSTART("TH.SpinDeep")', '1+1'))
$sw.Stop()
'unload with 3 running workers: {0} ms' -f $sw.ElapsedMilliseconds
if ($sw.ElapsedMilliseconds -gt 6000) { $fail++; 'FAIL: unload took too long' }
if (Test-Path $log) {
    $bad = Select-String -Path $log -Pattern 'W0034' -Encoding utf8 | Select-Object -First 3
    if ($bad) { $fail++; 'FAIL: a worker had to be killed:'; $bad | ForEach-Object { '  ' + $_.Line } }
}

if ($fail) { exit 1 }
'ALL OK'
