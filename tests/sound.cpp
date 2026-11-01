#include "../fx.cpp"
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdint>
#define REQUIRE(c) do{if(!(c)){std::printf("FAIL line %d\n",__LINE__);return 1;}}while(false)
using opxy::i32;
const double pi=3.141592653589793;
static std::vector<double> render(i32* parameters, int kind){
 formantdist::State s{};formantdist::initialize(s);i32 scratch[128];for(auto& x:scratch)x=0x76543210;
 std::vector<double> out;opxy::Buffer buffer{scratch,128};i32 l[64],r[64];
 for(unsigned block=0;block<512;++block){
  for(unsigned i=0;i<64;++i){unsigned n=block*64+i;double x=0;
   if(kind==0 || kind==2)x=(kind==2?.3:.1)*std::sin(2*pi*1000*n/44100);
   if(kind==1)for(unsigned b=0;b<12;++b){double f=140*std::pow(2.,b/2.);double gain=.006+.05*std::exp(-.5*std::pow((double(b)-4)/1.5,2));x+=gain*std::sin(2*pi*f*n/44100);}
   l[i]=r[i]=static_cast<i32>(x*2147483647.);
  }
  formantdist::process(s,buffer,l,r,parameters);
  for(unsigned i=0;i<64;++i){if(l[i]!=r[i])return {};out.push_back(l[i]/2147483648.);}
 }
 return out;
}
static double amplitude(const std::vector<double>& v,double hz){double a=0,b=0;for(unsigned n=8192;n<v.size();++n){a+=v[n]*std::cos(2*pi*hz*n/44100);b+=v[n]*std::sin(2*pi*hz*n/44100);}return std::sqrt(a*a+b*b)/(v.size()-8192);}
int main(){
 REQUIRE(sizeof(formantdist::State)<=472);
 i32 p[8]={1073741824,0,1073741824,0,2147483647,0,1073741824,1073741824};
 auto neutral=render(p,0);REQUIRE(neutral.size()==32768);REQUIRE(amplitude(neutral,1000)>.049);
 for(unsigned n=15;n<neutral.size();++n)REQUIRE(std::abs(neutral[n]-.1*std::sin(2*pi*1000*(n-15)/44100))<.000002);
 p[2]=1503238553;auto up=render(p,0);double hz=1000.+(p[2]/65536-16384)*1189.*44100./4294967296.;
 std::printf("SSB desired %.5f image %.5f rejection %.1f dB\n",amplitude(up,hz),amplitude(up,2000-hz),20*std::log10(amplitude(up,hz)/amplitude(up,2000-hz))); REQUIRE(amplitude(up,hz)>.035);REQUIRE(amplitude(up,2000-hz)<amplitude(up,hz)*.3);
 p[2]=644245094;auto down=render(p,0);hz=1000.+(p[2]/65536-16384)*1189.*44100./4294967296.;REQUIRE(amplitude(down,hz)>.035);
 p[2]=1073741824;auto original=render(p,1);p[0]=2147483647;auto shifted=render(p,1);
 double before=amplitude(original,1120)/amplitude(original,560),after=amplitude(shifted,1120)/amplitude(shifted,560);
 unsigned source_peak=0, shifted_peak=0;
 for(unsigned b=1;b<8;++b){double f=280*std::pow(2.,b/2.);if(amplitude(original,f)>amplitude(original,280*std::pow(2.,source_peak/2.)))source_peak=b;if(amplitude(shifted,f)>amplitude(shifted,280*std::pow(2.,shifted_peak/2.)))shifted_peak=b;}
 std::printf("spectral maximum %.1f -> %.1f Hz\n",280*std::pow(2.,source_peak/2.),280*std::pow(2.,shifted_peak/2.));
 REQUIRE(source_peak==2 && shifted_peak>source_peak);
 std::printf("formant 1120/560 ratio %.3f -> %.3f\n",before,after);REQUIRE(after>before*2);
 p[0]=1073741824;p[1]=2147483647;auto folded=render(p,2);REQUIRE(amplitude(folded,3000)>.001);
 p[1]=0;p[4]=0;auto filtered=render(p,0);REQUIRE(amplitude(filtered,1000)<.01);
 p[3]=2147483647;auto opened=render(p,0);REQUIRE(amplitude(opened,1000)>amplitude(filtered,1000)*2);
 formantdist::State s{};formantdist::initialize(s);i32 scratch[128],l[64],r[64];std::uint32_t random=7;
 for(unsigned block=0;block<2048;++block){for(unsigned j=0;j<8;++j){random=random*1664525u+1013904223u;p[j]=random&0x7fffffffu;}for(unsigned i=0;i<64;++i){random=random*1664525u+1013904223u;l[i]=r[i]=static_cast<i32>(random);}formantdist::process(s,{scratch,128},l,r,p);for(unsigned i=0;i<64;++i)REQUIRE(l[i]==r[i]);}
 formantdist::initialize(s);for(auto& x:scratch)x=123456789;for(unsigned i=0;i<64;++i)l[i]=r[i]=0;formantdist::process(s,{scratch,128},l,r,p);for(unsigned i=0;i<64;++i)REQUIRE(l[i]==0&&r[i]==0);
 for(unsigned i=0;i<64;++i)l[i]=r[i]=100;formantdist::process(s,{nullptr,0},l,r,p);for(unsigned i=0;i<64;++i)REQUIRE(l[i]==0&&r[i]==0);
 // Losing a scratch allocation invalidates all ring history before a new one arrives.
 for(auto& x:scratch)x=123456789;for(unsigned i=0;i<64;++i)l[i]=r[i]=0;
 formantdist::process(s,{scratch,128},l,r,p);for(unsigned i=0;i<64;++i)REQUIRE(l[i]==0&&r[i]==0);
 std::puts("formant: delay, spectral warp, signed sidebands, fold, dynamics, reset and extreme controls passed");
}
