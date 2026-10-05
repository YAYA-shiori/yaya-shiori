# 実データ（~/.claude の history.jsonl と会話の記録）と同じ形の合成データを作る。乱数は固定
#   <Root>\.claude\history.jsonl          7855 行 / 350 日 / 2423 セッション / 92 作業フォルダ
#   <Root>\.claude\projects\<cwd>\<sid>.jsonl   最近の 493 セッション。新しい 12 件だけ末尾が重い記録、残りは小さい記録
#   <Root>\.claude\sessions\*.json        動いているセッション 1 件
param(
    [Parameter(Mandatory = $true)][string]$Root,
    [int]$Lines = 7855,
    [int]$Sessions = 2423,
    [int]$Projects = 92,
    [int]$Days = 350,
    [int]$Recent = 493,
    [int]$Heavy = 12
)
$ErrorActionPreference = 'Stop'
$utf8 = New-Object System.Text.UTF8Encoding($false)
$rnd = New-Object System.Random(20261005)
$nowMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()

function Hex([int]$n) { $s = ''; for ($i = 0; $i -lt $n; $i++) { $s += '0123456789abcdef'[$rnd.Next(16)] }; $s }
function NewGuid { (Hex 8) + '-' + (Hex 4) + '-' + (Hex 4) + '-' + (Hex 4) + '-' + (Hex 12) }
# 日本語まじりの文章（バイト数で長さを決める）。JSON のエスケープ（\n と \"）も混ぜる
$kana = -join (0x3041..0x3093 | % { [char]$_ })
$kanji = '作業履歴確認実装修正関数辞書文字列配列読込書込速度改善調査原因対応設定起動終了表示追加削除変更検索結果処理記録会話続行'
function Text([int]$bytes) {
    $sb = New-Object System.Text.StringBuilder
    $b = 0
    while ($b -lt $bytes) {
        $r = $rnd.Next(100)
        if ($r -lt 45) { $c = $kana[$rnd.Next($kana.Length)]; $b += 3 }
        elseif ($r -lt 70) { $c = $kanji[$rnd.Next($kanji.Length)]; $b += 3 }
        elseif ($r -lt 96) { $c = [char](97 + $rnd.Next(26)); $b += 1 }
        elseif ($r -lt 98) { $sb.Append('\n') | Out-Null; $b += 2; continue }
        else { $sb.Append('\"') | Out-Null; $b += 2; continue }
        $sb.Append($c) | Out-Null
    }
    $sb.ToString()
}
# 中央値 median バイト、対数正規
function LogN([double]$median, [double]$sigma, [int]$lo, [int]$hi) {
    $u1 = 1.0 - $rnd.NextDouble(); $u2 = $rnd.NextDouble()
    $z = [math]::Sqrt(-2.0 * [math]::Log($u1)) * [math]::Cos(2.0 * [math]::PI * $u2)
    [int][math]::Min($hi, [math]::Max($lo, $median * [math]::Exp($sigma * $z)))
}

# ---- 作業フォルダ（1 つが全体の 6 割）
$projs = @()
for ($i = 0; $i -lt $Projects; $i++) { $projs += ('C:\\Users\\bench\\work\\proj{0:D2}' -f $i) }
function PickProj { if ($rnd.NextDouble() -lt 0.61) { $projs[0] } else { $projs[1 + $rnd.Next($Projects - 1)] } }

# ---- セッション。開始は新しいほど多め。1 セッションの依頼は 1〜21 件、数分おき
$sess = New-Object System.Collections.Generic.List[object]
for ($i = 0; $i -lt $Sessions; $i++) {
    $age = [math]::Pow($rnd.NextDouble(), 1.6) * $Days * 86400000   # 0 に寄せる
    $sess.Add([pscustomobject]@{ id = NewGuid; start = $nowMs - [int64]$age; proj = PickProj; n = 0 })
}
for ($k = 0; $k -lt $Lines; $k++) { $sess[$rnd.Next($Sessions)].n++ }
$rows = New-Object System.Collections.Generic.List[object]
foreach ($s in $sess) {
    $t = $s.start
    for ($j = 0; $j -lt $s.n; $j++) {
        $t += [int64]($rnd.Next(60, 1800) * 1000)
        if ($t -gt $nowMs) { $t = $nowMs - $rnd.Next(1000, 60000) }
        $bytes = LogN 267 0.8 40 14000
        $rows.Add([pscustomobject]@{ ts = $t; s = $s; len = $bytes })
    }
}
$sorted = $rows | Sort-Object ts
$cl = Join-Path $Root '.claude'
New-Item -ItemType Directory -Force (Join-Path $cl 'projects'), (Join-Path $cl 'sessions') | Out-Null
$sb = New-Object System.Text.StringBuilder
foreach ($r in $sorted) {
    $body = 130  # {"display":"" + pastedContents + timestamp + project + sessionId の分を引く
    $sb.Append('{"display":"').Append((Text ([math]::Max(10, $r.len - $body)))).Append('","pastedContents":{},"timestamp":').Append($r.ts).Append(',"project":"').Append($r.s.proj).Append('","sessionId":"').Append($r.s.id).Append("`"}`n") | Out-Null
}
[IO.File]::WriteAllText((Join-Path $cl 'history.jsonl'), $sb.ToString(), $utf8)

# ---- 会話の記録。最後の依頼が新しい順の Recent 件
$last = @{}
foreach ($r in $sorted) { $last[$r.s.id] = $r }
$newest = $last.Values | Sort-Object ts -Descending | Select -First $Recent
$idx = 0
foreach ($r in $newest) {
    $s = $r.s
    $dir = Join-Path (Join-Path $cl 'projects') (($s.proj -replace '\\\\', '\') -replace '[^A-Za-z0-9]', '-')
    New-Item -ItemType Directory -Force $dir | Out-Null
    $tsText = [DateTimeOffset]::FromUnixTimeMilliseconds($r.ts).UtcDateTime.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
    $w = New-Object System.Text.StringBuilder
    $nlines = if ($idx -lt $Heavy) { 1400 } else { 3 }
    for ($i = 0; $i -lt $nlines; $i++) {
        $role = if ($i % 2 -eq 0) { 'user' } else { 'assistant' }
        $r2 = $rnd.Next(100)
        if ($r2 -lt 4) { $w.Append('{"type":"ai-title","aiTitle":"').Append((Text 30)).Append('","sessionId":"').Append($s.id).Append("`"}`n") | Out-Null }
        elseif ($r2 -lt 6) { $w.Append('{"type":"agent-name","agentName":"bench-').Append((Hex 2)).Append('","sessionId":"').Append($s.id).Append("`"}`n") | Out-Null }
        elseif ($r2 -lt 14) { $w.Append('{"type":"cost-state","sessionId":"').Append($s.id).Append('","totalCostUsd":').Append($rnd.Next(100)).Append(".5}`n") | Out-Null }
        else {
            $sz = if ($rnd.Next(100) -lt 5) { LogN 20000 0.6 3000 60000 } else { LogN 560 0.6 200 4000 }
            $w.Append('{"parentUuid":"').Append((NewGuid)).Append('","isSidechain":false,"type":"').Append($role).Append('","message":{"role":"').Append($role).Append('","content":[{"type":"text","text":"').Append((Text $sz)).Append('"}]},"uuid":"').Append((NewGuid)).Append('","timestamp":"').Append($tsText).Append('","sessionId":"').Append($s.id).Append("`"}`n") | Out-Null
        }
    }
    [IO.File]::WriteAllText((Join-Path $dir ($s.id + '.jsonl')), $w.ToString(), $utf8)
    $idx++
}
$active = $newest | Select -First 1
$sj = '{"pid":4242,"sessionId":"' + $active.s.id + '","cwd":"' + $active.s.proj + '","status":"waiting","waitingFor":"input needed","kind":"interactive","name":"bench-main","statusUpdatedAt":' + $nowMs + '}'
[IO.File]::WriteAllText((Join-Path $cl 'sessions\4242.json'), $sj, $utf8)
$hi = Get-Item (Join-Path $cl 'history.jsonl')
'generated: history {0:N0} B / {1} lines, transcripts {2}' -f $hi.Length, $sorted.Count, @($newest).Count
