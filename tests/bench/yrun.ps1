<#
.SYNOPSIS
    yaya.exe（EXE 構成）を標準入出力で駆動し、1 回の load のあとに式を順に EVAL して時間を測る
.DESCRIPTION
    ベンチ用の辞書（bench.dic の request）が「?? 式」を EVAL して「!! 結果」で返す。
    結果は [pscustomobject]（Expr / Ms / Result）の配列で返す。Ms は request の往復の壁時計（ms）。
    -UserProfile を渡すと、その値を子プロセスの USERPROFILE にする（合成データの置き場所を差し替える）。
    出力を Select -First N などで打ち切ると、パイプラインごと止まって unload まで進まない。
    gprof のように unload 時に何かを書き出す EXE（tests/bench/profile）を使うときは注意すること。
#>
param(
    [string]$Exe,
    [Parameter(Mandatory = $true)][string]$Dir,
    [Parameter(Mandatory = $true)][string[]]$Exprs,
    [string]$UserProfile
)
$ErrorActionPreference = 'Stop'
# Windows PowerShell 5.1 は -File 起動だと param の既定値で $PSScriptRoot が空になるので、ここで補う
if (-not $Exe) { $Exe = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\..\Release_EXE\yaya.exe' }
$utf8 = New-Object System.Text.UTF8Encoding($false)
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = (Resolve-Path -LiteralPath $Exe).Path
$psi.WorkingDirectory = (Resolve-Path -LiteralPath $Dir).Path
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.CreateNoWindow = $true
if ($UserProfile) { $psi.EnvironmentVariables['USERPROFILE'] = $UserProfile }
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

$db = $utf8.GetBytes($psi.WorkingDirectory.TrimEnd('\') + '\' + "`r`n")
$sw = [Diagnostics.Stopwatch]::StartNew()
Send ('load:' + $db.Length) $db
$null = ReadLineRaw
$null = ReadBody 5
[pscustomobject]@{ Expr = '(load)'; Ms = $sw.ElapsedMilliseconds; Result = '' }

foreach ($e in $Exprs) {
    $rb = $utf8.GetBytes('?? ' + $e)
    $sw.Restart()
    Send ('request:' + $rb.Length) $rb
    $h = ReadLineRaw
    $n = [int]($h -replace '^request:', '')
    $body = ReadBody $n
    $ms = $sw.ElapsedMilliseconds
    $r = ($body -replace "[\r\n]+", ' ') -replace '^!! ', ''
    [pscustomobject]@{ Expr = $e; Ms = $ms; Result = $r }
}
Send 'unload:0' $null
if (-not $p.WaitForExit(180000)) { $p.Kill() }
