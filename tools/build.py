# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 dengyishan7034-creator
"""Build every plugin component from source; no prior mod DLL or game file is used."""
import argparse,pathlib,subprocess,sys
R=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tcc',required=True);p.add_argument('--output',type=pathlib.Path,default=R/'build');p.add_argument('--tests',action='store_true');a=p.parse_args()
if sys.platform!='win32':p.error('Windows x64 is required')
a.output.mkdir(parents=True,exist_ok=True);sources=[str(x) for x in sorted((R/'src').glob('*.c'))]
variants=[('dinput8.dll',[])]
if a.tests:variants.append(('standalone_test.dll',['-DTEST_BUILD']))
for name,opts in variants:subprocess.run([a.tcc,'-shared','-Wall',*opts,'-o',str(a.output/name),*sources],check=True)
print('Built',', '.join(x[0] for x in variants),'from',len(sources),'source files')
