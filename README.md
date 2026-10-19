# YAYA (Yet Another AYA)

伺か（ukagaka）の SHIORI「YAYA」のソースコードです。umeici 氏の「AYA Ver.5」を原型に、整備班が引き継いで発展させています。

```
OnBoot
{
	"\0\s[0]こんにちは。\e"
}
```

## マニュアル

**https://yaya-shiori.github.io/yaya-docs/**

文法、組み込み関数、システム変数、設定ファイルなどをまとめています。マニュアルのソースは [YAYA-shiori/yaya-docs](https://github.com/YAYA-shiori/yaya-docs) です。

## ダウンロード

[Releases](https://github.com/YAYA-shiori/yaya-shiori/releases) から入手してください。

| ファイル | 内容 |
|----------|------|
| `yaya.zip` | `yaya.dll` 本体。メッセージ（ログの文言）を DLL に埋め込んだ通常版 |
| `yaya_lang_sep.zip` | メッセージを `messagetxt\*.txt` に分離した版（文言を差し替えたい場合に使う） |

どちらにも `README.md`（このファイル）、`LICENSE`、`readme-original.txt`（AYA Ver.5 のオリジナルの使用許諾）が入っています。

バージョンは `TcXYY-N` の形で、X.YY が系列を表します。

| 系列 | バージョン | ブランチ |
|------|------------|----------|
| 6.xx 系 | `Tc6YY-N`（現行は `Tc606-N`） | `600` |
| 5.xx 系 | `Tc5YY-N`（現行は `Tc574-N`） | `500` |

新機能は主に `600` に入ります。変更履歴は各リリースのノートと、マニュアルの更新履歴を参照してください。

## 動作環境

| 環境 | 成果物 | 備考 |
|------|--------|------|
| Windows | `yaya.dll` | Visual C++ 6.0 でビルド（Windows 2000 以降を対象） |
| Windows（MinGW-w64） | `yaya.dll` | `makefile.mingw32` |
| Linux / FreeBSD | `libaya5.so` | `makefile.linux` / `makefile.freebsd` |
| macOS | `libyaya.bundle` | `makefile.posix` |
| WebAssembly | | `makefile.emscripten`（保守は最低限） |

Linux と macOS は手動実行の GitHub Actions（`posix-build.yml`）でビルドし、`tests/posix_smoke.c` で dlopen から unload までの動作を確かめています。リリースとして配布しているのは Windows 版のみです。

## 主な特徴

- 関数単位で辞書を書く、C 言語に近い文法のスクリプト（`if` / `while` / `for` / `foreach` / `switch` など）と、複数の出力候補から選ぶ独自の返り値の仕組み
- 配列・ハッシュ（入れ子可）、正規表現（`RE_*`、[uka_regexp](https://github.com/ukatech/uka_regexp)）、SQLite（`SQL*`）
- JSON / XML / YAML / TOML / HTML の読み書き（`FREAD*` / `PARSE*` / `FWRITE*` / `DUMP*`）
- SAORI の読み込み（`LOADLIB` / `REQUESTLIB`）と、実行ファイルを直接呼ぶ SAORI-basic
- 時刻・タイムゾーン・ロケールを OS 設定に依らず扱う処理
- 辞書を用意しなくても式を評価できるシェルモード（`yaya.exe` を標準入出力で動かして確認できる）

## リポジトリの構成

| パス | 内容 |
|------|------|
| `*.cpp` `*.h` | YAYA 本体（パーサ、実行エンジン、システム関数、SHIORI/SAORI の入出力など） |
| `messagetxt/` | ログ・エラーメッセージの文言（言語別） |
| `aya5.dsw` `aya5.dsp` | VC6 のワークスペース（基準のビルド） |
| `aya5.sln` `aya5.vcxproj` `VisualC7.1/` | Visual Studio 用のプロジェクト |
| `makefile.*` | Windows（MinGW）・Linux・FreeBSD・macOS・Emscripten 用 |
| `make_aya.bat` | 配布用 zip（`tmp/yaya.zip` `tmp/yaya_lang_sep.zip`）の作成 |
| `tests/` | POSIX 向けのスモークテストとベンチマーク |
| `OriginalAya/` | AYA Ver.5 オリジナル（umeici 氏）の配布物 |
| `digest.cpp` `mt19937ar.cpp` | ハッシュ（MD5・SHA-1・SHA-2・SHA-3・Adler-32・各種 CRC）・乱数の実装 |
| `parson/` `tinyxml2/` `uka_regexp/` `sqlite/` `gumbo/` | git サブモジュール |

## ビルド

クローンしたら、サブモジュールを取得してください。

```
git clone https://github.com/YAYA-shiori/yaya-shiori.git
cd yaya-shiori
git submodule update --init
```

Visual C++ 6.0 では、`msdev` で `aya5.dsw` をビルドします（C++98 の範囲で書かれています）。

```
msdev aya5.dsw /MAKE "aya5 - Win32 Release" /REBUILD
msdev aya5.dsw /MAKE "aya5 - Win32 ReleaseLangSep" /REBUILD
```

構成は `Release` / `Debug`（DLL）、`Release EXE` / `Debug EXE`（標準入出力で動かせる `yaya.exe`）、`ReleaseLangSep` です。ビルド後に `make_aya.bat` を実行すると `tmp\yaya.zip` と `tmp\yaya_lang_sep.zip` ができます（`zip` コマンドが必要です）。

Linux などでは `make -f makefile.linux`（macOS は `makefile.posix`）です。詳しい手順や注意点は [CLAUDE.md](CLAUDE.md) にあります。

ソースの文字コードは Shift_JIS（CP932）、改行は CRLF です。編集するときはこれを保ってください（POSIX 系の makefile は、ビルド時に iconv で UTF-8 へ変換します）。

## ライセンス

YAYA は 2 つのライセンスのどちらかで利用できます。

1. **伺かのゴースト、およびその周辺プログラムに組み込んで利用する場合**
   AYA Ver.5 の元のライセンスが適用されます。使途を限定せず自由に利用でき、配布物に含まれるすべてのファイル（またはその一部）を自由に扱えます。改変した版を作って配布することもでき、その際は配布者が独自のライセンスを適用できます。このプログラムの使用によって損害が生じても、作者は責任を負いません。原文は [`readme-original.txt`](readme-original.txt) にあります。
   ゴーストやそれに関係するユーティリティに組み込んで使う場合は、これまでと何も変わりません。
2. **それ以外の場合**
   [BSD 3-Clause License](LICENSE) が適用されます。ソースコード形式かバイナリ形式かを問わず、著作権表示・条件一覧・免責条項を含めること、貢献者の名前を宣伝に使わないことを条件に、再頒布と使用ができます。

```
AYA Ver.5: umeici
Copyright (c) 2007-, 整備班 (SEIBIHAN)
```

### 同梱しているサードパーティのコード

| 名前 | 用途 | ライセンス | 場所 |
|------|------|------------|------|
| [Mersenne Twister (MT19937)](http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/emt.html) | 乱数 | BSD 3-Clause（Makoto Matsumoto, Takuji Nishimura） | `mt19937ar.cpp` |
| [parson](https://github.com/kgabis/parson) | JSON | MIT License（Krzysztof Gabis） | `parson/` |
| [TinyXML-2](https://github.com/leethomason/tinyxml2) | XML | zlib License（Lee Thomason） | `tinyxml2/` |
| [uka_regexp](https://github.com/ukatech/uka_regexp) | 正規表現 | Unlicense（パブリックドメイン） | `uka_regexp/` |
| [SQLite](https://www.sqlite.org/) | SQL | パブリックドメイン | `sqlite/` |
| [Gumbo](https://github.com/google/gumbo-parser) | HTML | Apache License 2.0（Google Inc.）、UTF-8 デコーダは MIT License（Bjoern Hoehrmann） | `gumbo/` |

- `STRDIGEST` / `FDIGEST` のダイジェスト（MD5・SHA-1・SHA-2・SHA-3・Adler-32・各種 CRC）は、公開されている仕様（RFC 1321・RFC 1950・FIPS 180-4・FIPS 202・CRC のカタログ）をもとに `digest.cpp` で実装したもので、外部のコードを含みません。
- バイナリ（`yaya.dll`）に含まれるサードパーティのライセンス全文のうち、MT19937・parson・TinyXML-2・Gumbo・SQLite の分は、システム関数 `LICENSE` で取得できます。
- サブモジュールの本体は、それぞれのリポジトリのライセンスに従います（`tinyxml2` `sqlite` `gumbo` は、VC6・古い SDK 向けの修正を載せたフォークです）。

## 開発

- 現在の開発: [YAYA-shiori](https://github.com/YAYA-shiori) / 整備班（<http://ms.shillest.net/yaya.xhtml>）
- オリジナルの AYA Ver.5: umeici 氏

現行の 6.xx 系は、[ponapalt](https://github.com/ponapalt) と [Claudia](https://ponadocs.shillest.net/claudia/)（Claude）が中心になって保守しています。多くの方からの報告・パッチに支えられています。
