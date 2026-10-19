#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
namespace {
struct Maqam {const char* name; const char* tradition; std::array<double,7> steps;};
constexpr Maqam db[] = {
 {"Bayati","Arabic",{0,150,300,500,700,800,1000}},
 {"Rast","Arabic",{0,200,350,500,700,900,1050}},
 {"Hijaz","Arabic",{0,100,400,500,700,800,1100}},
 {"Nahawand","Arabic",{0,200,300,500,700,800,1100}},
 {"Kurd","Arabic",{0,100,300,500,700,800,1000}},
 {"Ajam","Arabic",{0,200,400,500,700,900,1100}},
 {"Saba","Arabic",{0,150,300,400,700,800,1000}},
 {"Huseyni","Turkish",{0,200,300,500,700,900,1000}},
 {"Ussak","Turkish",{0,150,300,500,700,900,1000}},
 {"Hicaz","Turkish",{0,100,400,500,700,800,1100}},
 {"Segah","Persian / Azerbaijani",{0,150,350,500,700,850,1050}},
 {"Shur","Persian / Azerbaijani",{0,150,300,500,700,850,1000}},
 {"Mahur","Persian",{0,200,400,500,700,900,1100}},
 {"Chahargah","Persian / Azerbaijani",{0,150,400,500,700,850,1100}},
 {"Homayun","Persian",{0,100,400,500,700,800,1100}},
 {"Shushtar","Azerbaijani",{0,100,300,500,700,800,1100}},
 {"Shur Mugham","Azerbaijani",{0,150,300,500,700,850,1000}},
 {"Rast Mugham","Azerbaijani",{0,200,350,500,700,900,1050}}
};
}
DAMaqamAnalyzerAudioProcessor::DAMaqamAnalyzerAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                  .withOutput("Output",juce::AudioChannelSet::stereo(),true)) {}
void DAMaqamAnalyzerAudioProcessor::prepareToPlay(double sr,int) {
 sampleRate=sr;ring.fill(0);write=hopCounter=0; pitchHz.store(0);
}
void DAMaqamAnalyzerAudioProcessor::startLearning() {
 learning.store(false);frames.store(0);pitches.fill(0);
 {juce::ScopedLock l(resultLock);results={};}
 learning.store(true);
}
void DAMaqamAnalyzerAudioProcessor::stopLearning() {learning.store(false);classify();}
void DAMaqamAnalyzerAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {
 juce::ScopedNoDenormals noDenormals;
 if(!learning.load())return;
 const int channels=b.getNumChannels();
 for(int i=0;i<b.getNumSamples();++i) {
  float s=0;
  for(int c=0;c<channels;++c)s+=b.getReadPointer(c)[i];
  s/=juce::jmax(1,channels);
  ring[(size_t)write]=s;write=(write+1)%window;
  if(++hopCounter>=hop) {
   hopCounter=0;analyzeFrame();
  }
 }
}
void DAMaqamAnalyzerAudioProcessor::analyzeFrame() {
 std::array<float,window> snapshot{};
 double energy=0;
 for(int j=0;j<window;++j){float x=ring[(size_t)((write+j)%window)];snapshot[(size_t)j]=x;energy+=x*x;}
 if(energy/window<0.00001)return;
 const int minLag=juce::jmax(2,(int)(sampleRate/1000.0));
 const int maxLag=juce::jmin(window/2,(int)(sampleRate/75.0));
 double best=-1;int bestLag=0;
 for(int lag=minLag;lag<=maxLag;++lag) {
  double dot=0,a=0,b=0;
  for(int i=0;i<window/2;++i) {
   const double x=snapshot[(size_t)i],y=snapshot[(size_t)(i+lag)];
   dot+=x*y;a+=x*x;b+=y*y;
  }
  const double score=dot/std::sqrt(a*b+1e-12);
  if(score>best){best=score;bestLag=lag;}
 }
 if(best<0.82||bestLag==0)return;
 float hz=(float)(sampleRate/bestLag);pitchHz.store(hz);
 int n=frames.load();
 if(n<maxFrames){pitches[(size_t)n]=hz;frames.store(n+1);}
 else learning.store(false);
}
void DAMaqamAnalyzerAudioProcessor::classify() {
 const int n=frames.load();
 if(n<8)return;
 struct Candidate {double score;int dbi,root;};
 std::array<Candidate,3> best{{{1e30,0,0},{1e30,0,0},{1e30,0,0}}};
 for(int m=0;m<(int)std::size(db);++m)for(int root=0;root<12;++root) {
  double score=0;
  for(int i=0;i<n;++i) {
   double hz=pitches[(size_t)i];if(hz<=0)continue;
   double cents=1200*std::log2(hz/440.)+900-root*100;
   double pc=std::fmod(cents,1200.);if(pc<0)pc+=1200;
   double d=600;
   for(double step:db[m].steps) {
    double e=std::abs(pc-step);d=std::min(d,std::min(e,1200-e));
   }
   score+=std::min(d,150.)/n;
  }
  Candidate c{score,m,root};
  for(auto& slot:best)if(c.score<slot.score){std::swap(c,slot);}
 }
 const char* notes[]={"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
 juce::ScopedLock l(resultLock);
 for(int k=0;k<3;++k) {
  auto c=best[(size_t)k];
  results[(size_t)k]={db[c.dbi].name,db[c.dbi].tradition,notes[c.root],
                       100.0*std::exp(-c.score/60.)};
 }
}
std::array<DAMaqamAnalyzerAudioProcessor::Result,3> DAMaqamAnalyzerAudioProcessor::getResults() const {
 juce::ScopedLock l(resultLock);return results;
}
void DAMaqamAnalyzerAudioProcessor::getStateInformation(juce::MemoryBlock& b) {
 juce::MemoryOutputStream s(b,false);s.writeInt(1);
}
void DAMaqamAnalyzerAudioProcessor::setStateInformation(const void*,int) {}
juce::AudioProcessorEditor* DAMaqamAnalyzerAudioProcessor::createEditor() {
 return new DAMaqamAnalyzerAudioProcessorEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DAMaqamAnalyzerAudioProcessor();}
