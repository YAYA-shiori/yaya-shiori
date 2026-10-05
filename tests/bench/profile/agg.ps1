# samples.txt（sampler.cpp の出力）を集計する。最寄りの YAYA ソース行への寄せ集め・自己時間（葉）・包括時間（スタックに含まれる）の上位を出す
# 使い方: pwsh -File agg.ps1 -File <samples.txt> -Exe <作業コピー>\yaya.exe -Src <作業コピー>
param(
    [string]$File = 'samples.txt',
    [int]$Top = 25,
    [string]$Filter = '',
    [Parameter(Mandatory = $true)][string]$Exe,        # サンプラー入りの yaya.exe（addr2line で関数名に戻す）
    [Parameter(Mandatory = $true)][string]$Src         # その EXE を作った作業コピー（リポジトリのソース名の判定に使う）
)
$lines = [IO.File]::ReadAllLines($File)
$n = $lines.Count
if ($n -eq 0) { 'no samples'; return }
$self = @{}
$incl = @{}
$names = New-Object System.Collections.Generic.HashSet[string]
foreach ($l in $lines) {
    $fr = $l -split ' <- '
    foreach ($f in $fr) { [void]$names.Add($f) }
}
# 名前を c++filt でまとめて戻す
$tmpIn = [IO.Path]::GetTempFileName()
[IO.File]::WriteAllLines($tmpIn, [string[]]$names)
$arr = @($names)
# EXE 内の "@オフセット" は addr2line で関数名にする（イメージベースは 0x140000000）
$exe = $Exe
$offs = @($arr | ? { $_ -match '^(?:[A-Za-z0-9_.]+!)?@[0-9a-f]+$' })
$resolved = @{}
for ($b = 0; $b -lt $offs.Count; $b += 250) {
    $chunk = $offs[$b..([Math]::Min($b + 249, $offs.Count - 1))]
    $addrs = $chunk | % { '0x{0:x}' -f (0x140000000 + [Convert]::ToInt64(($_ -replace '^.*@', ''), 16)) }
    $out = & addr2line -f -C -s -e $exe @addrs
    for ($k = 0; $k -lt $chunk.Count; $k++) { $resolved[$chunk[$k]] = ($out[2 * $k] -replace '\(.*$', '') + ' [' + $out[2 * $k + 1] + ']' }
}
$arr2 = $arr | % { if ($resolved.ContainsKey($_)) { $resolved[$_] } else { $_ } }
[IO.File]::WriteAllLines($tmpIn, [string[]]$arr2)
$dem = @(Get-Content $tmpIn | & c++filt)
Remove-Item $tmpIn
$map = @{}
for ($i = 0; $i -lt $arr.Count; $i++) {
    $d = $dem[$i]
    # 長い引数リストや std:: の飾りを短くする
    $d = $d -replace '^[A-Za-z0-9_]+!', ''
    $d = $d -replace '\(.*$', ''
    $d = $d -replace 'std::__cxx11::basic_string<[^>]*>', 'string'
    if ($d.Length -gt 90) { $d = $d.Substring(0, 90) }
    $map[$arr[$i]] = $d
}
foreach ($l in $lines) {
    $fr = $l -split ' <- '
    $leaf = $map[$fr[0]]
    $self[$leaf] = 1 + [int]$self[$leaf]
    $seen = @{}
    foreach ($f in $fr) {
        $m = $map[$f]
        if (-not $seen.ContainsKey($m)) { $seen[$m] = 1; $incl[$m] = 1 + [int]$incl[$m] }
    }
}
# 最寄りの YAYA 側フレーム（リポジトリのソースの行が付いたもの）に葉の時間を寄せる
$repoFiles = @{}
Get-ChildItem $Src -File | ? { $_.Extension -in '.cpp', '.h', '.c' } | % { $repoFiles[$_.Name] = 1 }
$near = @{}
$nearLine = @{}
foreach ($l in $lines) {
    $fr = $l -split ' <- '
    $hit = $null
    foreach ($f in $fr) {
        $m = $map[$f]
        if ($m -match '\[([^\]:]+):(\d+|\?)\]\s*$' -and $repoFiles.ContainsKey($matches[1])) { $hit = $m; break }
    }
    if ($null -eq $hit) { $hit = '(no yaya frame) ' + $map[$fr[0]] }
    $near[$hit] = 1 + [int]$near[$hit]
}
"samples: $n"
''
'--- time attributed to nearest yaya source line ---'
$near.GetEnumerator() | Sort-Object Value -Descending | Select -First $Top | % { '{0,6:N1}%  {1}' -f (100.0 * $_.Value / $n), $_.Key }
''
'--- self (leaf) ---'
$self.GetEnumerator() | Sort-Object Value -Descending | Select -First $Top | % { '{0,6:N1}%  {1}' -f (100.0 * $_.Value / $n), $_.Key }
''
'--- inclusive ---'
$incl.GetEnumerator() | Sort-Object Value -Descending | ? { $Filter -eq '' -or $_.Key -match $Filter } | Select -First $Top | % { '{0,6:N1}%  {1}' -f (100.0 * $_.Value / $n), $_.Key }
