<#
.SYNOPSIS
    EXECUTE_CAPTURE の試験（Windows）
.DESCRIPTION
    EXE 構成の yaya.exe を標準入出力で駆動する。辞書の無い空のディレクトリで load するとシェルモードになり、
    request の入力を EVAL した結果を返すので、辞書を用意せずに式を試せる（CLAUDE.md「動作確認とシェルモード」）。
    作業ディレクトリは %TEMP%\yaya_capture_test に作るので、リポジトリは汚れない。
    子プロセスには cmd.exe / findstr.exe / sort.exe / ping.exe（Windows 標準）を使う。
.EXAMPLE
    pwsh -NoProfile -File tests/exec/run_capture.ps1 -Exe Release_EXE\yaya.exe
#>
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$WorkDir = (Join-Path $env:TEMP 'yaya_capture_test')
)
$ErrorActionPreference = 'Stop'

if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
New-Item -ItemType Directory -Force $WorkDir | Out-Null
$exeCopy = Join-Path $WorkDir 'yaya.exe'
Copy-Item (Resolve-Path $Exe).Path $exeCopy -Force
# 辞書もログも置かない空のディレクトリで load するため、実行ファイルは別のディレクトリから動かす
$dicDir = Join-Path $WorkDir 'empty'
New-Item -ItemType Directory -Force $dicDir | Out-Null

# 式 / 期待する結果 / 応答時間の上限(ms。0なら見ない)
$nl = 'CHR(13,10)'
$big = '_s = ''abcdefghi'' + CHR(10); _i = 0; while _i < 18 { _s = _s + _s; _i = _i + 1 }'
$cases = @(
    # 標準出力の取り込みと終了コード
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello'); _r['stdout'] == 'hello' + $nl", '1', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello'); _r['exitcode']", '0', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello'); _r['status']", 'ok', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello'); _r['stderr']", '', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c exit 3'); _r['exitcode']", '3', 0),
    # 標準エラー出力: 別（既定）/ 混ぜる / 捨てる
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo err 1>&2'); STRLEN(_r['stdout']) + ',' + STRLEN(_r['stderr'])", '0,6', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo err 1>&2', '', IHASH('stderr', 'merge')); STRLEN(_r['stdout']) + ',' + STRLEN(_r['stderr'])", '6,0', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo out& echo err 1>&2', '', IHASH('stderr', 'discard')); STRLEN(_r['stdout']) + ',' + STRLEN(_r['stderr'])", '5,0', 0),
    # 標準入力を渡す。指定しないときはすぐ EOF になる（sort が待ち続けない）
    @("_r = EXECUTE_CAPTURE('findstr.exe', '`"^`"', '', IHASH('stdin', 'abc')); _r['stdout'] == 'abc' + $nl", '1', 0),
    @("_r = EXECUTE_CAPTURE('sort.exe', '', '', IHASH('stdin', 'b' + $nl + 'a' + $nl)); _r['stdout'] == 'a' + $nl + 'b' + $nl", '1', 0),
    @("_r = EXECUTE_CAPTURE('sort.exe'); _r['stdout'] + ',' + _r['exitcode']", ',0', 3000),
    @("_r = EXECUTE_CAPTURE('sort.exe', '', '', IHASH('stdin', '')); _r['exitcode']", '0', 3000),
    # 作業ディレクトリ
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c cd', GETENV('SystemRoot')); TOLOWER(_r['stdout']) == TOLOWER(GETENV('SystemRoot')) + $nl", '1', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c cd', 'no_such_dir_xyz'); _r['status'] + ',' + _r['exitcode']", 'failed,-1', 0),
    # 起動できない
    @("_r = EXECUTE_CAPTURE('no_such_command_xyz.exe'); _r['status'] + ',' + _r['exitcode']", 'failed,-1', 0),
    # 時間切れ（子プロセスは終了させる）
    @("_r = EXECUTE_CAPTURE('ping.exe', '-n 20 127.0.0.1', '', IHASH('timeout', 500)); _r['status'] + ',' + _r['exitcode']", 'timeout,-1', 3000),
    # 起動した子が終わった後も、孫が出力し続けるあいだ（終了後の読み取り）も時間切れで止まる。終了コードは返る
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c start /b cmd /c for /L %i in (1,1,2000000) do @echo xxxxxxxxxx', '', IHASH('timeout', 700)); _r['status'] + ',' + _r['exitcode']", 'timeout,0', 3000),
    # 上限を超えた出力
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c for /L %i in (1,1,100000) do @echo xxxxxxxxxx', '', IHASH('maxsize', 1000)); _r['status'] + ',' + STRLEN(_r['stdout'])", 'truncated,1000', 5000),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello', '', IHASH('maxsize', 7)); _r['status'] + ',' + STRLEN(_r['stdout'])", 'ok,7', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello', '', IHASH('maxsize', 6)); _r['status'] + ',' + STRLEN(_r['stdout'])", 'truncated,6', 0),
    # 大きな入力を流しながら出力を読む（パイプが一杯になって互いに待ち合わない）
    @("$big; _r = EXECUTE_CAPTURE('findstr.exe', '`"^`"', '', IHASH('stdin', _s)); _r['status'] + ',' + (STRLEN(_r['stdout']) > 2000000)", 'ok,1', 8000),
    # 標準エラー出力だけを大量に出す（標準出力だけを読んでいると詰まる）
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c for /L %i in (1,1,200000) do @echo yyyyyyyyy 1>&2'); _r['status'] + ',' + STRLEN(_r['stdout']) + ',' + (STRLEN(_r['stderr']) > 2000000)", 'ok,0,1', 8000),
    # 文字コード: UTF-8 で渡して同じ文字コードで受け取る
    @("_r = EXECUTE_CAPTURE('findstr.exe', '`"^`"', '', IHASH('stdin', CHR(12354), 'charset', 'UTF-8')); _r['stdout'] == CHR(12354) + $nl", '1', 0),
    # 不正なオプションは W と failed、知らないキーは警告だけで実行する
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello', '', IHASH('stderr', 'bogus')); _r['status']", 'failed', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello', '', IHASH('timeout', -1)); _r['status']", 'failed', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello', '', IHASH('timout', 5)); _r['status']", 'ok', 0),
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello', '', 1); _r['status']", 'failed', 0),
    @("_r = EXECUTE_CAPTURE(); _r['status']", 'failed', 0),
    # 結果のハッシュの形
    @("_r = EXECUTE_CAPTURE('cmd.exe', '/c echo hello'); HASH_SIZE(_r)", '4', 0)
)

# ---- yaya.exe を標準入出力で駆動する ----
$utf8 = New-Object System.Text.UTF8Encoding($false)
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exeCopy
$psi.WorkingDirectory = $dicDir
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.CreateNoWindow = $true
$p = [System.Diagnostics.Process]::Start($psi)
$in = $p.StandardInput.BaseStream
$out = $p.StandardOutput.BaseStream

function Send([string]$head, [byte[]]$body) {
    $h = $utf8.GetBytes($head + "`r`n")
    $in.Write($h, 0, $h.Length)
    if ($body) { $in.Write($body, 0, $body.Length) }
    $in.Flush()
}
function ReadLineRaw {
    $sb = New-Object System.Collections.Generic.List[byte]
    while ($true) {
        $b = $out.ReadByte()
        if ($b -lt 0) { break }
        $sb.Add([byte]$b)
        if ($sb.Count -ge 2 -and $sb[$sb.Count - 2] -eq 13 -and $sb[$sb.Count - 1] -eq 10) { break }
    }
    return $utf8.GetString($sb.ToArray()).TrimEnd("`r", "`n")
}
function ReadBody([int]$n) {
    $buf = New-Object byte[] $n
    $got = 0
    while ($got -lt $n) {
        $r = $out.Read($buf, $got, $n - $got)
        if ($r -le 0) { break }
        $got += $r
    }
    return $utf8.GetString($buf, 0, $got)
}

$db = $utf8.GetBytes($dicDir.TrimEnd('\') + '\' + "`r`n")
Send ('load:' + $db.Length) $db
$null = ReadLineRaw
$null = ReadBody 5

$fail = 0
$sw = [Diagnostics.Stopwatch]::new()
foreach ($c in $cases) {
    $rb = $utf8.GetBytes($c[0])
    $sw.Restart()
    Send ('request:' + $rb.Length) $rb
    $h = ReadLineRaw
    $n = [int]($h -replace '^request:', '')
    $body = ReadBody $n
    $ms = $sw.ElapsedMilliseconds
    $got = ($body -replace "[\r\n]+", ' ')
    $slow = ($c[2] -gt 0 -and $ms -gt $c[2])
    if ($got -ne $c[1] -or $slow) {
        $fail++
        'FAIL: {0}  expected [{1}] got [{2}]  {3}ms{4}' -f $c[0], $c[1], $got, $ms, $(if ($slow) { " (limit $($c[2])ms)" } else { '' })
    }
}
Send 'unload:0' $null
$null = $p.WaitForExit(5000)
if (-not $p.HasExited) { $p.Kill() }
'{0} / {1} cases passed' -f ($cases.Count - $fail), $cases.Count
if ($fail) { exit 1 }
