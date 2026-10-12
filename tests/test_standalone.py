# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 dengyishan7034-creator
import argparse,ctypes as C,pathlib,struct,json,math,tempfile,zipfile,io,wave,time
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[1]/'build');a=p.parse_args();a.build_dir=a.build_dir.resolve();L=C.CDLL(str(a.build_dir/'standalone_test.dll'))
class Packet(C.Structure):
 _pack_=1
 _fields_=[('magic',C.c_uint32),('version',C.c_uint16),('engine',C.c_uint8),('valid',C.c_uint8),('session',C.c_uint32),('seq',C.c_uint32),('entity',C.c_uint32),('pitch',C.c_float),('x',C.c_float),('y',C.c_float),('z',C.c_float),('model',C.c_char*64)]
class Listener(C.Structure):_fields_=[('position',C.c_float*3),('right',C.c_float*3),('up',C.c_float*3),('forward',C.c_float*3),('tick',C.c_uint32),('valid',C.c_int)]
L.TestSnapshot.argtypes=[C.c_size_t,C.POINTER(Packet)];L.TestGuard.argtypes=[C.c_size_t];L.TestPacket.argtypes=[C.POINTER(Packet),C.c_uint32];L.TestCamera.argtypes=[C.POINTER(C.c_float),C.POINTER(Listener)]
L.TestRoot.argtypes=[C.c_wchar_p];L.TestResolve.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_uint32];L.TestReceive.argtypes=[C.c_uint64,C.POINTER(Packet),C.c_uint32,C.c_uint32];L.TestMix.argtypes=[C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(Listener)];L.TestAttachNet.argtypes=[C.c_void_p];L.TestStep.argtypes=[C.c_size_t,C.c_uint32]
checks=[]
def check(name,ok):
 assert ok,name
 checks.append(name)
# Simulate an overlay resolver that returns this proxy for the Windows input export.
# Forwarding must ignore that name-based interception and use the native export.
L.TestPatch.argtypes=[C.POINTER(C.c_void_p),C.c_void_p,C.POINTER(C.c_void_p)];L.TestSystemInputExport.restype=C.c_void_p
base=L._handle;headers=C.string_at(base,4096);nt=struct.unpack_from('<I',headers,0x3c)[0];imports=struct.unpack_from('<I',headers,nt+24+120)[0];entry=base+imports;slot=None
while C.c_uint32.from_address(entry+12).value:
 oft=C.c_uint32.from_address(entry).value;ft=C.c_uint32.from_address(entry+16).value;i=0
 while oft and C.c_uint64.from_address(base+oft+i*8).value:
  name=C.c_uint64.from_address(base+oft+i*8).value
  if not name>>63 and C.string_at(base+name+2)==b'GetProcAddress':slot=C.c_void_p.from_address(base+ft+i*8)
  i+=1
 entry+=20
assert slot is not None
k=C.WinDLL('kernel32');k.GetProcAddress.argtypes=[C.c_void_p,C.c_char_p];k.GetProcAddress.restype=C.c_void_p;k.GetModuleHandleW.argtypes=[C.c_wchar_p];k.GetModuleHandleW.restype=C.c_void_p
PROC=C.WINFUNCTYPE(C.c_void_p,C.c_void_p,C.c_char_p);resolutions=[]
@PROC
def overlay(module,name):
 if name==b'DirectInput8Create':resolutions.append(name);return C.cast(L.DirectInput8Create,C.c_void_p).value
 return k.GetProcAddress(module,name)
old=C.c_void_p();assert L.TestPatch(C.byref(slot),C.cast(overlay,C.c_void_p),C.byref(old))==1
native=L.TestSystemInputExport();check('native input export differs from proxy',native and native!=C.cast(L.DirectInput8Create,C.c_void_p).value)
guid=(C.c_ubyte*16).from_buffer_copy(struct.pack('<IHH8B',0xbf798031,0x483a,0x4da2,0xaa,0x99,0x5d,0x64,0xed,0x36,0x97,0x00));obj=C.c_void_p()
L.DirectInput8Create.argtypes=[C.c_void_p,C.c_uint32,C.c_void_p,C.POINTER(C.c_void_p),C.c_void_p];L.DirectInput8Create.restype=C.c_long
check('input proxy works with hostile overlay resolver',L.DirectInput8Create(k.GetModuleHandleW(None),0x800,guid,C.byref(obj),None)>=0 and bool(obj.value))
check('input forwarding bypasses overlay name resolution',not resolutions)
if obj.value:
 table=C.cast(obj,C.POINTER(C.POINTER(C.c_void_p))).contents;C.WINFUNCTYPE(C.c_uint32,C.c_void_p)(table[2])(obj)
assert L.TestPatch(C.byref(slot),old,None)==1

def put(addr,fmt,*vals):C.memmove(addr,struct.pack(fmt,*vals),struct.calcsize(fmt))
def memory(size):return C.create_string_buffer(size)
game=memory(0xbc0000);g=C.addressof(game);ctrl=memory(0x100);c=C.addressof(ctrl);truck=memory(0x800);t=C.addressof(truck);model=memory(0x500);m=C.addressof(model);body=memory(0x350);b=C.addressof(body)
put(g+0xbb81c8,'<Q',c);put(c,'<Q',g+0xa49870);put(c+8,'<Q',t);put(t,'<Q',g+0xa4f6d8);put(t+0x10,'<Q',m);put(t+0x24,'<I',125);put(t+0x5b0,'<I',1);put(m+0x1a0,'<Q',b);put(b+0x1a0,'<3f',11,12,13);put(m+0x410,'<f',0.6);C.memmove(m+0x3a8,b'uaz469\0',7);put(m+0x3b8,'<Q',6);put(m+0x3c0,'<Q',15)
pkt=Packet();check('native vehicle/type/position/engine',L.TestSnapshot(g,C.byref(pkt))==1 and pkt.model==b'uaz469' and pkt.entity==125 and pkt.engine==1 and [pkt.x,pkt.y,pkt.z]==[11,12,13]);check('normalized engine pitch',abs(pkt.pitch-1.3)<1e-6)
put(t+0x5b0,'<I',0);check('stationary engine OFF',L.TestSnapshot(g,C.byref(pkt))==1 and pkt.engine==0)
heap=memory(64);C.memmove(C.addressof(heap),b'long_vehicle_name\0',18);put(m+0x3a8,'<Q',C.addressof(heap));put(m+0x3b8,'<Q',17);put(m+0x3c0,'<Q',31);check('heap model string',L.TestSnapshot(g,C.byref(pkt))==1 and pkt.model==b'long_vehicle_name');C.memmove(m+0x3a8,b'uaz469\0',7);put(m+0x3b8,'<Q',6);put(m+0x3c0,'<Q',15)
put(c+8,'<Q',0);check('leaving truck',L.TestSnapshot(g,C.byref(pkt))==0 and pkt.valid==0);put(c+8,'<Q',t);put(t,'<Q',0);check('wrong vtable',L.TestSnapshot(g,C.byref(pkt))==-1);put(t,'<Q',g+0xa4f6d8)
put(m+0x1a0,'<Q',1);check('bad body pointer',L.TestSnapshot(g,C.byref(pkt))==-1);put(m+0x1a0,'<Q',b);put(b+0x1a0,'<f',float('nan'));check('invalid physical coordinates',L.TestSnapshot(g,C.byref(pkt))==-1);put(b+0x1a0,'<3f',11,12,13)
put(g,'<H',0x5a4d);put(g+0x3c,'<I',0x80);put(g+0x80,'<I',0x4550);put(g+0x84,'<H',0x8664);C.memmove(g+0xbda0,bytes.fromhex('488d0521c4ba00c3'),8);C.memmove(g+0x89edb2,bytes.fromhex('f686b005000001'),7);C.memmove(g+0x8ca309,bytes.fromhex('4881c1a8030000'),7);C.memmove(g+0x1d6e0,bytes.fromhex('488d0539abb900c3'),8);check('supported game guard',L.TestGuard(g)==1);put(g+0xbda0,'<B',0);check('unsupported game guard',L.TestGuard(g)==0);put(g+0xbda0,'<B',0x48)
def packet(seq=1,engine=1):return Packet(0x3341524d,3,engine,1,77,seq,125,1,20,0,0,b'uaz469')
pkt=packet();check('wire size',C.sizeof(Packet)==100);check('valid packet',L.TestPacket(C.byref(pkt),100)==1)
for field,value in [('magic',0),('version',4),('engine',2),('valid',2),('session',0),('pitch',float('nan')),('pitch',4),('x',float('inf')),('y',100001),('z',float('nan')),('model',b'../bad'),('model',b'')]:
 q=packet();setattr(q,field,value);check('reject '+field+' '+str(value),L.TestPacket(C.byref(q),100)==0)
check('reject truncated wire',L.TestPacket(C.byref(pkt),99)==0);q=packet();q.valid=0;q.model=b'';check('no vehicle packet',L.TestPacket(C.byref(q),100)==1)
mat=(C.c_float*16)(1,0,0,0,0,1,0,0,0,0,1,0,-10,-20,-30,1);listener=Listener();check('camera inverse',L.TestCamera(mat,C.byref(listener))==1 and list(listener.position)==[10,20,30] and list(listener.right)==[1,0,0]);mat[3]=1;check('reject projection as camera',L.TestCamera(mat,C.byref(listener))==0);mat[3]=0
# Native renderer camera: all data comes from checked game memory, no graphics device.
engine=memory(0x100);renderCamera=memory(0x180);e=C.addressof(engine);rc=C.addressof(renderCamera);put(g+0xbb8220,'<Q',e);put(e,'<Q',g+0xa4a528);put(e+8,'<Q',rc);C.memmove(rc,mat,C.sizeof(mat));nativeListener=Listener()
L.TestNativeCamera.argtypes=[C.c_size_t,C.POINTER(Listener)]
check('native renderer camera and position',L.TestNativeCamera(g,C.byref(nativeListener))==1 and list(nativeListener.position)==[10,20,30])
put(e,'<Q',0);check('wrong renderer object rejected',L.TestNativeCamera(g,C.byref(nativeListener))==0);put(e,'<Q',g+0xa4a528)
put(e+8,'<Q',1);check('invalid native camera pointer rejected',L.TestNativeCamera(g,C.byref(nativeListener))==0);put(e+8,'<Q',rc)
put(rc,'<f',float('nan'));check('invalid native camera matrix rejected',L.TestNativeCamera(g,C.byref(nativeListener))==0);C.memmove(rc,mat,C.sizeof(mat))

# A rotated view distinguishes row axes from column axes.
rot=(C.c_float*16)(0,0,-1,0,0,1,0,0,1,0,0,0,0,0,0,1);check('camera right from view column',L.TestCamera(rot,C.byref(listener))==1 and list(listener.right)==[0,0,1]);listener=Listener((0,0,0),(1,0,0),(0,1,0),(0,0,1),0,1)
def wav(value,ch=1):
 out=io.BytesIO()
 with wave.open(out,'wb') as w:w.setnchannels(ch);w.setsampwidth(2);w.setframerate(22050);w.writeframes(struct.pack('<'+('h'*512*ch),*([value]*(512*ch))))
 return out.getvalue()
def resolve(name):
 x=C.create_string_buffer(256);y=C.create_string_buffer(256);ok=L.TestResolve(name.encode(),x,y,256);return ok,x.value.decode(),y.value.decode()
def render(now,rounds=12):
 out=(C.c_int16*2048)()
 for _ in range(rounds):L.TestMix(out,1024,now,C.byref(listener))
 return list(out)
with tempfile.TemporaryDirectory(dir=a.build_dir) as temp:
 d=pathlib.Path(temp);(d/'Config.xml').write_text('<Config><MediaPath Path="resources.zip"/></Config>')
 with zipfile.ZipFile(d/'resources.zip','w',zipfile.ZIP_DEFLATED) as z:
  z.writestr('_templates/trucks.xml','<_templates><Sounds><Default><EngineIdle Sound="engine/uaz_idle"/><EngineHigh Sound="engine/uaz_high"/></Default></Sounds></_templates>')
  z.writestr('_templates/custom.xml','<_templates><Sounds><Imported><EngineIdle Sound="engine/uaz_idle"/><EngineHigh Sound="engine/uaz_high"/></Imported></Sounds></_templates>')
  z.writestr('classes/trucks/uaz469.xml','<_templates Include="custom"><Sounds><Local _template="Imported"/></Sounds></_templates><Truck><TruckData><Sounds _template="Local"/></TruckData></Truck>')
  z.writestr('classes/trucks/child.xml','<_parent File="uaz469"/><Truck><TruckData><Sounds><EngineHigh Sound="alias"/></Sounds></TruckData></Truck>')
  z.writestr('classes/trucks/fresh_mod.xml','<!-- <Truck><TruckData><Sounds><EngineIdle Sound="wrong"/></Sounds></TruckData></Truck> --><Truck><TruckData><Sounds><EngineIdle Sound="engine/uaz_idle"/><EngineHigh Sound="engine/uaz_high"/></Sounds></TruckData></Truck>')
  z.writestr('classes/sounds/alias.xml','<Sound Name="engine/uaz_high"/>');z.writestr('sounds/engine/uaz_idle.wav',wav(10000));z.writestr('sounds/engine/uaz_high.wav',wav(20000))
 L.TestRoot(str(d));expected=('game:sounds/engine/uaz_idle.wav','game:sounds/engine/uaz_high.wav');check('included/local sound templates',resolve('uaz469')==(1,*expected));check('parent and sound alias',resolve('child')==(1,*expected));check('unindexed new mod model and XML comments',resolve('fresh_mod')==(1,*expected));check('unknown model rejected',resolve('missing')[0]==0);check('model traversal rejected',resolve('../uaz469')[0]==0)
 L.TestClearMixer();L.TestResetSync();q=packet();check('receive valid snapshot',L.TestReceive(42,C.byref(q),100,1000)==1);check('duplicate packet ignored',L.TestReceive(42,C.byref(q),100,1001)==0);q.seq=0;check('older packet ignored',L.TestReceive(42,C.byref(q),100,1002)==0);q.seq=2;check('newer packet accepted',L.TestReceive(42,C.byref(q),100,1003)==1);q.session=88;q.seq=1;check('new session accepted',L.TestReceive(42,C.byref(q),100,1004)==1)
 samples=render(1010);check('source on right uses right channel',sum(abs(x) for x in samples[1::2])>100 and sum(abs(x) for x in samples[::2])==0)
 q.engine=0;q.seq=2;L.TestReceive(42,C.byref(q),100,1020);check('stationary OFF fades to silence',all(x==0 for x in render(1020,35)))
 q.engine=1;q.seq=3;L.TestReceive(42,C.byref(q),100,1030);render(1030);check('stale peer fades to silence',all(x==0 for x in render(5000,35)))
 L.TestClearMixer();L.TestResetSync();q=packet();q.x=-20;L.TestReceive(43,C.byref(q),100,1000);samples=render(1010);check('source on left uses left channel',sum(abs(x) for x in samples[::2])>100 and sum(abs(x) for x in samples[1::2])==0)
 q.seq=2;q.x=-201;L.TestReceive(43,C.byref(q),100,1010);check('out of range silent',all(x==0 for x in render(2000,60)))
 L.TestClearMixer();L.TestResetSync();q=packet();q.x=0;q.z=20;L.TestReceive(44,C.byref(q),100,1000);samples=render(1010);check('forward source centered',samples[::2]==samples[1::2] and any(samples));q.seq=2;q.valid=0;L.TestReceive(44,C.byref(q),100,1020);check('leaving remote vehicle silent',all(x==0 for x in render(1020,35)))
 check('bounded cached memory',0<L.TestCacheBytes()<=64*1024*1024)
 L.TestResetSync();q=packet(0xfffffffe,0);check('sequence near wrap accepted',L.TestReceive(77,C.byref(q),100,1000)==1);q.seq=0xffffffff;check('sequence wrap last accepted',L.TestReceive(77,C.byref(q),100,1001)==1);q.seq=0;check('sequence wrapped to zero accepted',L.TestReceive(77,C.byref(q),100,1002)==1);q.seq=0xfffffffe;check('old pre-wrap sequence rejected',L.TestReceive(77,C.byref(q),100,1003)==0)
 L.TestClearMixer();L.TestResetSync()
 for id in range(100,108):
  q=packet();q.pitch=1.5;q.x=q.y=q.z=0;L.TestReceive(id,C.byref(q),100,1000)
 samples=render(1000,35);check('eight sources saturate safely in stereo',max(samples)==32767 and samples[::2]==samples[1::2])
 L.TestClearMixer();L.TestResetSync()

 # Actual SDK-style vtable callbacks; game payloads must pass through unchanged.
 sendRecords=[];queue=[]
 SEND=C.CFUNCTYPE(C.c_ubyte,C.c_void_p,C.c_uint64,C.c_void_p,C.c_uint32,C.c_int,C.c_int);AVAIL=C.CFUNCTYPE(C.c_ubyte,C.c_void_p,C.POINTER(C.c_uint32),C.c_int);READ=C.CFUNCTYPE(C.c_ubyte,C.c_void_p,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint64),C.c_int)
 @SEND
 def send(self,id,data,size,mode,channel):sendRecords.append((id,C.string_at(data,size),mode,channel));return 73
 @AVAIL
 def avail(self,size,channel):
  if not queue:return 0
  size[0]=len(queue[0][1]);return 1
 @READ
 def read(self,out,cap,size,id,channel):
  if channel!=7:data=b'game-data';remote=99
  elif queue:remote,data=queue.pop(0)
  else:return 0
  if cap<len(data):return 0
  C.memmove(out,data,len(data));size[0]=len(data);id[0]=remote;return 1
 table=(C.c_void_p*21)();table[0]=C.cast(send,C.c_void_p);table[1]=C.cast(avail,C.c_void_p);table[2]=C.cast(read,C.c_void_p);obj=(C.c_void_p*1)(C.addressof(table));L.TestAttachNet(obj)
 payload=C.create_string_buffer(b'game-payload');wrapped=SEND(table[0]);check('game SendP2P result preserved',wrapped(obj,99,payload,12,2,0)==73);check('game SendP2P bytes/mode/channel preserved',sendRecords[-1]==(99,b'game-payload',2,0))
 out=memory(128);count=C.c_uint32();remote=C.c_uint64();wrappedRead=READ(table[2]);check('game ReadP2P forwarded',wrappedRead(obj,out,128,C.byref(count),C.byref(remote),0)==1 and out.raw[:count.value]==b'game-data' and remote.value==99)
 now=C.windll.kernel32.GetTickCount()&0xffffffff;L.TestStep(g,now+200);check('autodiscovered interface sends state',any(r[3]==7 and len(r[1])==100 for r in sendRecords));tx=next(Packet.from_buffer_copy(r[1]) for r in sendRecords if r[3]==7);check('transmitted native vehicle without legacy DLL',tx.model==b'uaz469' and tx.entity==125 and tx.engine==0)
 q=packet(10,0);queue.append((99,bytes(q)));queue.append((99,b'bad'));L.TestStep(g,now+210);check('custom queue consumed and malformed packet rejected',not queue)
 L.TestClearMixer();L.TestRoot('')
# Verify Windows output buffers are queued and completed (silent test).
check('Windows audio output opens',L.TestAudioOpen()==1);time.sleep(0.15);written=C.c_long();failed=C.c_long();L.TestAudioStats(C.byref(written),C.byref(failed));L.TestAudioClose();check('audio buffers complete without errors',written.value>6 and failed.value==0)
result={'passed':len(checks),'checks':checks,'uses_original_dll':False,'uses_game_files':False,'windows_audio_blocks':written.value,'windows_audio_failures':failed.value,'live_multiplayer_test':False}
(a.build_dir/'standalone-test-results.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result))
