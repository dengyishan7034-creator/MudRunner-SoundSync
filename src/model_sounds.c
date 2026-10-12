/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 dengyishan7034-creator
 * Small bounded reader for the game's multi-root XML-like definitions. */
#include "sync.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
typedef struct { const char*attrs,*attrsEnd,*body,*end,*next; char name[64]; } Node;
typedef struct { char idle[256],high[256]; } Sounds;
static int space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
static const char*tagEnd(const char*p,const char*end){char quote=0;for(;p<end;p++){if(quote){if(*p==quote)quote=0;}else if(*p=='"'||*p==39)quote=*p;else if(*p=='>')return p;}return NULL;}
static const char*skipSpecial(const char*p,const char*end){const char*q;if(end-p>=4&&!memcmp(p,"<!--",4)){q=strstr(p+4,"-->");return q&&q+3<=end?q+3:end;}if(end-p>=2&&p[1]=='?'){q=strstr(p+2,"?>");return q&&q+2<=end?q+2:end;}if(end-p>=2&&p[1]=='!'){q=tagEnd(p+2,end);return q?q+1:end;}return p;}
static int nextNode(const char**cursor,const char*end,Node*n){const char*p=*cursor,*q,*name,*scan,*close=NULL;size_t len;int depth=1;
 for(;;){while(p<end&&*p!='<')p++;if(p>=end)return 0;q=skipSpecial(p,end);if(q!=p){p=q;continue;}if(p+1>=end||p[1]=='/')return 0;break;}
 name=p+1;q=name;while(q<end&&!space(*q)&&*q!='/'&&*q!='>')q++;len=q-name;if(!len||len>=sizeof(n->name))return 0;memcpy(n->name,name,len);n->name[len]=0;n->attrs=q;n->attrsEnd=tagEnd(q,end);if(!n->attrsEnd)return 0;n->body=n->attrsEnd+1;
 q=n->attrsEnd;while(q>n->attrs&&space(q[-1]))q--;if(q>n->attrs&&q[-1]=='/'){n->end=n->body;n->next=n->body;*cursor=n->next;return 1;}
 scan=n->body;while(scan<end){while(scan<end&&*scan!='<')scan++;if(scan>=end)break;q=skipSpecial(scan,end);if(q!=scan){scan=q;continue;}q=tagEnd(scan+1,end);if(!q)return 0;if(scan[1]=='/'){if(--depth==0){close=scan;n->next=q+1;break;}}else{const char*t=q;while(t>scan&&space(t[-1]))t--;if(t[-1]!='/')depth++;}scan=q+1;}
 if(!close)return 0;n->end=close;*cursor=n->next;return 1;}
static int findNode(const char*start,const char*end,const char*name,Node*out){Node n;while(nextNode(&start,end,&n))if(!strcmp(n.name,name)){*out=n;return 1;}return 0;}
static int attribute(const Node*n,const char*key,char*out,size_t cap){const char*p=n->attrs,*q,*value;size_t kl=strlen(key),len;while(p<n->attrsEnd){while(p<n->attrsEnd&&space(*p))p++;q=p;while(q<n->attrsEnd&&!space(*q)&&*q!='='&&*q!='/')q++;if(q==p){p++;continue;}len=q-p;while(q<n->attrsEnd&&space(*q))q++;if(q==n->attrsEnd||*q!='='){p=q;continue;}q++;while(q<n->attrsEnd&&space(*q))q++;if(q==n->attrsEnd||(*q!='"'&&*q!=39)){p=q+1;continue;}{char quote=*q++;value=q;while(q<n->attrsEnd&&*q!=quote)q++;if(len==kl&&!memcmp(p,key,kl)){size_t size=q-value;if(q==n->attrsEnd||size>=cap)return 0;memcpy(out,value,size);out[size]=0;return 1;}}p=q<n->attrsEnd?q+1:q;}return 0;}
static void directSounds(const Node*n,Sounds*out){Node v;char s[256];if(findNode(n->body,n->end,"EngineIdle",&v)&&attribute(&v,"Sound",s,sizeof(s)))strcpy(out->idle,s);if(findNode(n->body,n->end,"EngineHigh",&v)&&attribute(&v,"Sound",s,sizeof(s)))strcpy(out->high,s);}
static void overlay(Sounds*out,const Sounds*in){if(in->idle[0])strcpy(out->idle,in->idle);if(in->high[0])strcpy(out->high,in->high);}
static int templateSounds(const char*doc,size_t size,const char*wanted,Sounds*out,int depth){Node templates,group,item;char include[1024],parent[64];const char*p;Sounds found={0};int ok=0;if(depth>16||!findNode(doc,doc+size,"_templates",&templates))return 0;
 if(attribute(&templates,"Include",include,sizeof(include))){p=include;while(*p){char name[128],path[256];size_t len=0,n;unsigned char*base;Sounds inherited={0};while(*p&&(space(*p)||*p==','||*p==';'))p++;while(*p&&!space(*p)&&*p!=','&&*p!=';'){if(len+1<sizeof(name))name[len++]=*p;p++;}if(!len)continue;name[len]=0;snprintf(path,sizeof(path),"_templates/%s.xml",name);base=GameAudioReadResource(path,&n);if(base){if(templateSounds((char*)base,n,wanted,&inherited,depth+1)){overlay(&found,&inherited);ok=1;}GameAudioFree(base);}}}
 if(findNode(templates.body,templates.end,"Sounds",&group)&&findNode(group.body,group.end,wanted,&item)){Sounds inherited={0};if(attribute(&item,"_template",parent,sizeof(parent))&&strcmp(parent,wanted)&&templateSounds(doc,size,parent,&inherited,depth+1))overlay(&found,&inherited);directSounds(&item,&found);ok=1;}overlay(out,&found);return ok;}
static int truckSounds(const char*model,Sounds*out,int depth){char path[256],parent[64],name[64],noinherit[16];size_t n;unsigned char*data;Node node,truck,td,sounds;Sounds base={0};if(depth>16||strchr(model,'/')||strchr(model,'\\')||strchr(model,':'))return 0;snprintf(path,sizeof(path),"classes/trucks/%s.xml",model);data=GameAudioReadResource(path,&n);if(!data)return 0;
 if(findNode((char*)data,(char*)data+n,"_parent",&node)&&attribute(&node,"File",parent,sizeof(parent)))truckSounds(parent,&base,depth+1);
 if(findNode((char*)data,(char*)data+n,"Truck",&truck)&&findNode(truck.body,truck.end,"TruckData",&td)&&findNode(td.body,td.end,"Sounds",&sounds)){
 if(attribute(&sounds,"_noinherit",noinherit,sizeof(noinherit))&&!strcmp(noinherit,"true"))memset(&base,0,sizeof(base));
 if(attribute(&sounds,"_template",name,sizeof(name))){Sounds t={0};templateSounds((char*)data,n,name,&t,0);overlay(&base,&t);}directSounds(&sounds,&base);}
 overlay(out,&base);GameAudioFree(data);return 1;}
static int aliasInNodes(const char*start,const char*end,char*out,int depth){Node n;if(depth>16)return 0;while(nextNode(&start,end,&n)){if(attribute(&n,"Name",out,256)||attribute(&n,"File",out,256)||attribute(&n,"Sound",out,256))return 1;if(aliasInNodes(n.body,n.end,out,depth+1))return 1;}return 0;}
static int soundPath(const char*name,char*out,size_t cap,int depth){char path[512],alias[256];unsigned char*data;size_t n,len;if(!*name||depth>16||strstr(name,"..")||strchr(name,':')||name[0]=='/'||name[0]=='\\')return 0;len=strlen(name);if(len>=256)return 0;strcpy(alias,name);if(len>4&&!strcmp(alias+len-4,".wav"))alias[len-4]=0;
 snprintf(path,sizeof(path),"sounds/%s.wav",alias);data=GameAudioReadResource(path,&n);if(data){GameAudioFree(data);if(strlen(path)+6>cap)return 0;snprintf(out,cap,"game:%s",path);return 1;}
 snprintf(path,sizeof(path),"classes/sounds/%s.xml",alias);data=GameAudioReadResource(path,&n);if(!data)return 0;alias[0]=0;aliasInNodes((char*)data,(char*)data+n,alias,0);GameAudioFree(data);return soundPath(alias,out,cap,depth+1);}
int ResolveModelSounds(const char*model,char*idle,char*high,size_t cap){Sounds s={0},defaults={0};unsigned char*data;size_t n;idle[0]=high[0]=0;if(!truckSounds(model,&s,0))return 0;data=GameAudioReadResource("_templates/trucks.xml",&n);if(data){templateSounds((char*)data,n,"Default",&defaults,0);GameAudioFree(data);}if(!s.idle[0])strcpy(s.idle,defaults.idle);if(!s.high[0])strcpy(s.high,defaults.high);return soundPath(s.idle,idle,cap,0)&&soundPath(s.high,high,cap,0);}
