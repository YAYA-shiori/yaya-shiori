<#
.SYNOPSIS
    サンプリング・プロファイラ入りの yaya.exe（MinGW-w64）を、作業コピーに作る
.DESCRIPTION
    gprof は MinGW-w64（64bit）ではサンプルが取れないので、本体スレッドを 1ms ごとに止めてスタックを採る
    小さなサンプラー（sampler.cpp）を EXE の main に差し込む。リポジトリのファイルは変更しない。

      1. git ls-files --recurse-submodules のファイルを作業ディレクトリへ写す
      2. aya5.cpp（CP932）の main に sampler_start / sampler_enable / sampler_dump を差し込む
         request の実行中だけ採り、unload でカレントディレクトリの samples.txt に書き出す
      3. -O2 -g の makefile を作って exe ターゲットをビルドする（Strawberry Perl 同梱の gcc で確認）

    使い方: この EXE を tests/bench/yrun.ps1 -Exe で駆動し、最後に unload まで進める
    （出力を Select -First などで打ち切らないこと）。できた samples.txt を agg.ps1 で集計する。
.EXAMPLE
    pwsh -NoProfile -File tests/bench/profile/build.ps1
#>
param(
    [string]$Work = (Join-Path $env:TEMP 'yaya_prof'),
    [int]$Jobs = [Environment]::ProcessorCount
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = (Resolve-Path (Join-Path $here '..\..\..')).Path
$cp932 = [Text.Encoding]::GetEncoding(932)
$utf8 = New-Object Text.UTF8Encoding($false)

Write-Host "作業コピーを作ります: $Work"
if (Test-Path -LiteralPath $Work) { Remove-Item -LiteralPath $Work -Recurse -Force }
New-Item -ItemType Directory -Force $Work | Out-Null
Push-Location $repo
try {
    $files = & git ls-files --recurse-submodules
} finally {
    Pop-Location
}
foreach ($f in $files) {
    $dst = Join-Path $Work $f
    New-Item -ItemType Directory -Force (Split-Path -Parent $dst) | Out-Null
    Copy-Item -LiteralPath (Join-Path $repo $f) -Destination $dst
}
Copy-Item -LiteralPath (Join-Path $here 'sampler.cpp') -Destination (Join-Path $Work 'sampler.cpp')

# ---- aya5.cpp の main にフックを差し込む（CP932 のまま、改行も保つ）
$aya = Join-Path $Work 'aya5.cpp'
$t = [IO.File]::ReadAllText($aya, $cp932)
function Patch([string]$text, [string]$old, [string]$new) {
    if (-not $text.Contains($old)) { throw "aya5.cpp に目印が見つかりません: $old" }
    return $text.Replace($old, $new)
}
$t = Patch $t "`tAYA_InitModule(NULL);" "`tAYA_InitModule(NULL); sampler_start();"
$t = Patch $t 'yaya::global_t res = request(read_ptr,&size);' 'sampler_enable(1); yaya::global_t res = request(read_ptr,&size); sampler_enable(0);'
$t = Patch $t "`t`t`tunload();`r`n`r`n`t`t`tconst char* result = `"unload:5" "`t`t`tsampler_dump(`"samples.txt`"); unload();`r`n`r`n`t`t`tconst char* result = `"unload:5"
$t = 'extern "C" void sampler_start(void); extern "C" void sampler_enable(int); extern "C" void sampler_dump(const char*);' + "`r`n" + $t
[IO.File]::WriteAllText($aya, $t, $cp932)

# ---- makefile（-pg なし、-g 付き、sampler.o を足す）
$m = [IO.File]::ReadAllText((Join-Path $Work 'makefile.mingw32'), $utf8)
$m = [regex]::Replace($m, '(?m)^CXXFLAGS = .*$', 'CXXFLAGS = -O2 -g -Wall -std=c++11 -fpermissive -finput-charset=CP932 -fexec-charset=CP932')
$m = [regex]::Replace($m, '(?m)^CFLAGS = .*$', 'CFLAGS = -O2 -g -Wall')
$m = $m.Replace('-static -s $(LD_ADD)', '-static $(LD_ADD) -ldbghelp -lwinmm')
$m = [regex]::Replace($m, '(?m)^AYA5EXE_OBJ = .*$', '$0 sampler.o')
$m += "`nsampler.o: sampler.cpp`n`t`$(CXX) -O2 -g -std=c++11 -finput-charset=UTF-8 -DWIN32 -D_WINDOWS -o `$@ -c sampler.cpp`n"
[IO.File]::WriteAllText((Join-Path $Work 'makefile.prof'), $m, $utf8)

Write-Host "ビルドします（-j $Jobs）"
Push-Location $Work
try {
    & mingw32-make -j $Jobs -f makefile.prof exe 2>&1 | Where-Object { $_ -match ' error |\*\*\*' } | Write-Host
} finally {
    Pop-Location
}
$exe = Join-Path $Work 'yaya.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw 'yaya.exe ができませんでした' }
Write-Host "できました: $exe"
Write-Host "計測: pwsh -File tests/bench/yrun.ps1 -Exe $exe -Dir <辞書のあるディレクトリ> -Exprs <式...> -UserProfile <合成データ>"
Write-Host "集計: pwsh -File tests/bench/profile/agg.ps1 -File <辞書のあるディレクトリ>\samples.txt -Exe $exe -Src $Work"
