# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 dengyishan7034-creator
import argparse,ctypes as C,pathlib,zlib,random,io,wave,struct,zipfile,tempfile,json
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[1]/'build');a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
L=C.CDLL(str(a.build_dir/'standalone_test.dll'))
class Sample(C.Structure):_fields_=[('samples',C.c_void_p),('count',C.c_uint32),('rate',C.c_uint32),('fade',C.c_uint32),('loaded',C.c_uint32)]
L.TestInflate.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t];L.TestInflate.restype=C.c_int;L.TestRoot.argtypes=[C.c_wchar_p];L.TestWave.argtypes=[C.c_char_p,C.POINTER(Sample)];L.TestWave.restype=C.c_int;L.TestFree.argtypes=[C.c_void_p]
r=random.Random(123);cases=0
for raw in [b'',b'a',b'abc'*10000,b'\0'*150000,bytes(range(256))*500,bytes(r.randrange(256) for _ in range(100000))]:
 for level in [0,1,6,9]:
  for strategy in [zlib.Z_DEFAULT_STRATEGY,zlib.Z_FIXED,zlib.Z_HUFFMAN_ONLY]:
   co=zlib.compressobj(level,zlib.DEFLATED,-15,8,strategy);data=co.compress(raw)+co.flush();buf=C.create_string_buffer(data);out=C.create_string_buffer(len(raw)+16)
   assert L.TestInflate(buf,len(data),out,len(raw))==1 and out.raw[:len(raw)]==raw;cases+=1
assert L.TestInflate(C.create_string_buffer(b'\x07'),1,C.create_string_buffer(16),16)==0
raw=b'abc'*100;co=zlib.compressobj(6,zlib.DEFLATED,-15);data=co.compress(raw)+co.flush()
for n in range(len(data)):
 assert L.TestInflate(C.create_string_buffer(data),n,C.create_string_buffer(len(raw)+1),len(raw))==0
out=C.create_string_buffer(b'X'*(len(raw)+16));assert L.TestInflate(C.create_string_buffer(data),len(data),out,len(raw)-1)==0;assert out.raw[len(raw)-1:len(raw)+15]==b'X'*16
mono=struct.pack('<4h',1000,-1000,2000,-2000);stereo=struct.pack('<8h',1000,3000,-1000,-3000,500,-500,2000,0)
def wav(pcm,ch):
 b=io.BytesIO()
 with wave.open(b,'wb') as w:w.setnchannels(ch);w.setsampwidth(2);w.setframerate(22050);w.writeframes(pcm)
 return b.getvalue()
def load(name):
 s=Sample();assert L.TestWave(name.encode(),C.byref(s))==1;data=C.string_at(s.samples,s.count*2);L.TestFree(s.samples);return s,data
with tempfile.TemporaryDirectory(dir=a.build_dir) as t:
 d=pathlib.Path(t);(d/'Config.xml').write_text('<Config><MediaPath Path="base.zip"/><MediaPath MountPoint="_t/" Path="ignored.zip"/></Config>')
 with zipfile.ZipFile(d/'base.zip','w',zipfile.ZIP_DEFLATED) as z:z.writestr('sounds/engine/uaz_idle.wav',wav(mono,1))
 L.TestRoot(str(d));s,b=load('game:sounds/engine/uaz_idle.wav');assert b==mono and s.count==4 and s.rate==22050
 s,b=load('F:/_mp_sync/sounds/engine/uaz_idle.wav');assert b==mono
 assert L.TestWave(b'game:../not-allowed.wav',C.byref(Sample()))==0
 L.TestRoot('')
 with zipfile.ZipFile(d/'override.zip','w',zipfile.ZIP_STORED) as z:z.writestr('SOUNDS/ENGINE/UAZ_IDLE.WAV',wav(stereo,2))
 (d/'Config.xml').write_text('<Config><MediaPath Path="base.zip"/><MediaPath Path="override.zip"/></Config>');L.TestRoot(str(d));s,b=load('game:sounds/engine/uaz_idle.wav');assert b==struct.pack('<4h',2000,-2000,0,1000)
 L.TestRoot('');loose=d/'loose/sounds/engine';loose.mkdir(parents=True);(loose/'uaz_idle.wav').write_bytes(wav(mono,1));(d/'Config.xml').write_text('<Config><MediaPath Path="override.zip"/><MediaPath Path="loose"/></Config>');L.TestRoot(str(d));s,b=load('game:sounds/engine/uaz_idle.wav');assert b==mono
 L.TestRoot('');broken=bytearray((d/'base.zip').read_bytes());idx=broken.index(b'PK\x01\x02');broken[idx+16]^=1;(d/'broken.zip').write_bytes(broken);(d/'Config.xml').write_text('<Config><MediaPath Path="broken.zip"/></Config>');L.TestRoot(str(d));assert L.TestWave(b'game:sounds/engine/uaz_idle.wav',C.byref(Sample()))==0;L.TestRoot('')
result={'deflate_cases':cases,'truncated_input_checks':len(data),'output_bound_check':True,'zip_deflate':True,'zip_stored':True,'case_insensitive_names':True,'directory_mount':True,'stereo_downmix':True,'bootstrap_path_redirect':True,'crc_rejection':True,'game_files_required':False}
(a.build_dir/'game-audio-test-results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
