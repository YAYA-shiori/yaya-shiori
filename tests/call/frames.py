# dumpbin /disasm の出力から、関数ごとのスタックフレームの大きさ（バイト）を出す
#   python tests/call/frames.py <.obj> [関数名に含まれる文字列...]
# 表示: 合計 (sub esp,N + 先頭で退避した push 数) 関数名
# 文字列を省くと、ユーザー関数の呼び出しの経路にある関数を出す
import re, sys, subprocess

obj = sys.argv[1]
pats = sys.argv[2:] or ['ExecuteInBrace', 'GetFormulaAnswer', 'ExecFunctionWithArgs', 'Execute@CFunction', 'Execute_SEH', 'Foreach', 'GetValueRefForCalc', 'Subst@CFunction', 'Comma@CFunction', 'Array@CFunction']
out = subprocess.run(['dumpbin', '/disasm', obj], capture_output=True, text=True, errors='replace').stdout
lines = out.splitlines()

funcs = []
cur = None
for ln in lines:
    if ln and not ln.startswith(' ') and ln.rstrip().endswith(':') and '(' in ln:
        cur = {'name': ln.split(' ')[0], 'sub': None, 'push': 0, 'n': 0}
        funcs.append(cur)
        continue
    if cur is None:
        continue
    cur['n'] += 1
    if cur['n'] > 40:
        continue
    m = re.search(r'\bsub\s+esp,([0-9A-F]+)h', ln)
    if m and cur['sub'] is None:
        cur['sub'] = int(m.group(1), 16)
    m = re.search(r'mov\s+eax,([0-9A-F]+)h', ln)
    if m:
        cur['_eax'] = int(m.group(1), 16)
    if '__chkstk' in ln and cur['sub'] is None and cur.get('_eax'):
        cur['sub'] = cur['_eax']
    if cur['sub'] is None and re.search(r'\bpush\b', ln):
        cur['push'] += 1

for f in funcs:
    if any(p in f['name'] for p in pats):
        sub = f['sub'] or 0
        total = sub + 4 * f['push'] + 4
        print('%6d (sub %5d + push %2d)  %s' % (total, sub, f['push'], f['name'][:90]))
