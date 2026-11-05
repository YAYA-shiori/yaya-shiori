# cl /FAs が出した .asm から、1 つの関数のスタック上の変数（一時変数 $T を含む）を出す
#   python tests/call/locals.py <function.asm> "?GetFormulaAnswer@CFunction"
# 数値はスタックの EBP からのオフセット。隣との差が、その変数の大きさ
import re, sys

asm = sys.argv[1]
pat = sys.argv[2]
txt = open(asm, encoding='cp932', errors='replace').read().splitlines()

start = None
for i, ln in enumerate(txt):
    if re.search(r'^\S+\s+PROC', ln) and pat in ln:
        start = i
        break
if start is None:
    print('not found'); sys.exit(1)

rows = []
j = start - 1
# EQU 行はPROC行の直前に並ぶ
while j >= 0 and (txt[j].strip() == '' or re.search(r'=\s*-?\d+', txt[j]) or txt[j].lstrip().startswith(';')):
    m = re.match(r'^(\S+)\s*=\s*(-?\d+)', txt[j])
    if m:
        rows.append((int(m.group(2)), m.group(1)))
    j -= 1
rows.sort()
for off, name in rows:
    print('%6d  %s' % (off, name))
print('total locals:', len(rows))
