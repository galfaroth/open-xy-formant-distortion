#include "core.h"
#include <opxy/q31.hpp>
#include "tables.h"
namespace formantdist {
namespace {
// Q23 audio leaves headroom for overlapping bands and folding.
const opxy::i32 full=8388608;
opxy::i32 clamp(opxy::i32 x,opxy::i32 lo,opxy::i32 hi){return x<lo?lo:(x>hi?hi:x);}
opxy::i32 absolute(opxy::i32 x){return x<0?-x:x;}
opxy::i32 mul(opxy::i32 a,opxy::i32 b){return opxy::q31::multiply(a,b);}
opxy::i32 smooth(opxy::i32 a,opxy::i32 b,unsigned shift){const opxy::i32 d=b-a;return a+(d<0?-(((-d)+(1<<shift)-1)>>shift):((d+(1<<shift)-1)>>shift));}
opxy::i32 sin_phase(opxy::u32 phase){
 unsigned quadrant=phase>>30;opxy::u32 p=phase&0x3fffffffu;
 if(quadrant&1)p=0x40000000u-p;
 const unsigned index=p>>24;const opxy::i32 value=index==64?sine[64]:sine[index]+mul(sine[index+1]-sine[index],static_cast<opxy::i32>((p&0xffffffu)<<7));
 return quadrant>=2?-value:value;
}
opxy::i32 fold(opxy::i32 x,opxy::i32 amount){
 if(!amount)return x;
 const opxy::i32 a=absolute(x);const opxy::i32 driven=a+mul(a,amount*65536)*7;
 const opxy::i32 u=(driven+full)%(4*full), y=u<=2*full?u-full:3*full-u;
 return x<0?-y:y;
}
opxy::i32 tap(opxy::i32* ring,unsigned cursor,unsigned written,unsigned age){return age<written?ring[(cursor-age)&63]:0;}
}
void initialize(State& s){s=State{};for(unsigned i=0;i<8;++i)s.gain[i]=8192;}
void process(State& s,opxy::Buffer buffer,opxy::Channel left,opxy::Channel right,opxy::Parameters parameters){
 if(!buffer.words || buffer.count<128){initialize(s);for(unsigned i=0;i<opxy::block_frames;++i)left[i]=right[i]=0;return;}
 opxy::i32 target[8];for(unsigned i=0;i<8;++i)target[i]=parameters[i]<0?0:parameters[i]>>16;
 // Q15 controls: the normalized midpoint is exactly 16384.
 if(!s.started){for(unsigned i=0;i<8;++i)s.control[i]=target[i];s.started=1;}
 // Remap measured input envelopes; clamp ratios before parallel-band correction.
 opxy::i32 desired[8];const opxy::i32 offset=(s.control[0]-16384)/32;
 for(unsigned b=0;b<8;++b){
  const opxy::i32 pos=clamp(static_cast<opxy::i32>(b)*256-offset,0,7*256);unsigned index=pos/256;
  opxy::i32 env=s.envelope[index];if(index<7)env+=mul(s.envelope[index+1]-env,(pos%256)*8388608);
  desired[b]=offset?clamp((env/512)*8192/(s.envelope[b]/512+1),2048,32768):8192;
 }
 opxy::i32* history=static_cast<opxy::i32*>(buffer.words);
 for(unsigned p=0;p<8;++p)s.control[p]=smooth(s.control[p],target[p],1);
 for(unsigned b=0;b<8;++b)s.gain[b]=smooth(s.gain[b],desired[b],1);
 for(unsigned n=0;n<opxy::block_frames;++n){
  opxy::i32 input[2]={left[n]/256,right[n]/256}, warped[2]={input[0],input[1]};
  for(unsigned b=0;b<8;++b){
   opxy::i32 peak=0;
   for(unsigned c=0;c<2;++c){
    opxy::i32* z=s.band[c][b];
    const opxy::i32 y=clamp(mul(input[c]-s.input[c][1],bands[b][0])+2*mul(z[0],bands[b][1])-mul(z[1],bands[b][2]),-4*full,4*full);
    z[1]=z[0];z[0]=y;const opxy::i32 a=absolute(y);if(a>peak)peak=a;
    warped[c]=opxy::q31::add(warped[c],mul(y,(s.gain[b]-8192)*65536)*4);
   }
   s.envelope[b]=smooth(s.envelope[b],peak,peak>s.envelope[b]?5:11);
  }
  opxy::i32 peak=absolute(input[0]);if(absolute(input[1])>peak)peak=absolute(input[1]);
  s.detector=smooth(s.detector,peak,peak>s.detector?4u:static_cast<unsigned>(7+s.control[6]/8192));
  const opxy::i32 opening=mul(s.detector,s.control[3]*65536)/256;
  const opxy::i32 cutoff=clamp(s.control[4]+opening,0,32767);
  // Stable one-pole lowpass; Emphasis blends the complementary highpass.
  const opxy::i32 alpha=16777216+cutoff*65023;
  const opxy::i32 cosine=sin_phase(s.phase+0x40000000u),sine_value=sin_phase(s.phase);
  if(s.written<64)++s.written;
  for(unsigned c=0;c<2;++c){
   s.input[c][1]=s.input[c][0];s.input[c][0]=input[c];
   opxy::i32* ring=history+64*c;ring[s.cursor]=fold(clamp(warped[c],-4*full,4*full),s.control[1]);
   // Antisymmetric FIR pairs produce quadrature; both branches share 15 samples of delay.
   opxy::i32 imaginary=0;
   for(unsigned k=0;k<8;++k){const unsigned distance=2*k+1;imaginary=opxy::q31::add(imaginary,mul(tap(ring,s.cursor,s.written,15+distance)-tap(ring,s.cursor,s.written,15-distance),hilbert[k]));}
   const opxy::i32 real=tap(ring,s.cursor,s.written,15);
   opxy::i32 x=mul(real,cosine)-mul(imaginary,sine_value);
   s.low[c]+=mul(x-s.low[c],alpha);
   if(cutoff<32767)x=s.low[c]+mul(x-s.low[c],s.control[5]*65536);
   x=mul(clamp(x,-8*full,8*full),s.control[7]*65536)*2;
   x=clamp(x,-full,full-1);if(c==0)left[n]=x*256;else right[n]=x*256;
  }
  s.phase+=static_cast<opxy::u32>((s.control[2]-16384)*1189);
  s.cursor=(s.cursor+1)&63;
 }
}
}
