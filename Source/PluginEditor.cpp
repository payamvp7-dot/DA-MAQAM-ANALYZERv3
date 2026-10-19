#include "PluginEditor.h"
#include <cmath>
namespace {
const auto background=juce::Colour::fromRGB(8,16,23);
const auto surface=juce::Colour::fromRGB(17,29,38);
const auto surface2=juce::Colour::fromRGB(22,38,47);
const auto gold=juce::Colour::fromRGB(218,171,91);
const auto ivory=juce::Colour::fromRGB(239,229,206);
const auto muted=juce::Colour::fromRGB(144,160,164);
void text(juce::Graphics& g,juce::String s,juce::Rectangle<float> r,float sz,juce::Colour c,bool heavy=false,
          juce::Justification align=juce::Justification::centredLeft) {
 g.setColour(c);g.setFont(juce::Font(juce::FontOptions(sz).withStyle(heavy?"Bold":"Regular")));
 g.drawFittedText(s,r.toNearestInt(),align,1);
}
juce::String noteFor(float hz) {
 if(hz<40 || !std::isfinite(hz))return "--";
 int midi=(int)std::round(69+12*std::log2(hz/440.0f));
 const char* names[]={"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
 return juce::String(names[(midi%12+12)%12])+juce::String(midi/12-1);
}
}
DAMaqamAnalyzerAudioProcessorEditor::DAMaqamAnalyzerAudioProcessorEditor(DAMaqamAnalyzerAudioProcessor& p)
: AudioProcessorEditor(&p),processor(p) {
 setSize(1100,710);
 setResizable(true,true);setResizeLimits(950,630,1550,1000);
 history.fill(0);
 for(auto* b:{&learn,&stop,&copy,&exportGuide}) {
  addAndMakeVisible(*b);
  b->setColour(juce::TextButton::buttonColourId,surface2);
  b->setColour(juce::TextButton::textColourOffId,ivory);
 }
 learn.setColour(juce::TextButton::buttonColourId,gold);
 learn.setColour(juce::TextButton::textColourOffId,background);
 learn.onClick=[this]{history.fill(0);head=0;processor.startLearning();repaint();};
 stop.onClick=[this]{processor.stopLearning();repaint();};
 copy.onClick=[this]{
  auto results=processor.getResults();juce::String s="Rank,Mode,Root,Tradition,Heuristic fit\n";
  for(int i=0;i<3;++i){auto& r=results[(size_t)i];s+=juce::String(i+1)+","+r.name+","+r.root+","+r.tradition+","+juce::String(r.fit,1)+"\n";}
  juce::SystemClipboard::copyTextToClipboard(s);
 };
 addAndMakeVisible(culture);
 culture.addItem("ALL TRADITIONS",1);culture.addItem("PERSIAN",2);culture.addItem("AZERBAIJANI",3);
 culture.addItem("TURKISH",4);culture.addItem("ARABIC",5);culture.setSelectedId(1,juce::dontSendNotification);
 culture.setColour(juce::ComboBox::backgroundColourId,surface2);
 culture.setColour(juce::ComboBox::textColourId,ivory);
 culture.setColour(juce::ComboBox::outlineColourId,gold.withAlpha(.4f));
 culture.onChange=[this]{
  modeLibrary.clear(juce::dontSendNotification);
  const int selected=culture.getSelectedId();
  struct Entry {const char* name;int culture;};
  const Entry entries[]={ {"Bayati",5},{"Rast",5},{"Hijaz",5},{"Nahawand",5},{"Kurd",5},{"Ajam",5},{"Saba",5},
    {"Huseyni",4},{"Ussak",4},{"Hicaz",4},{"Segah",2},{"Shur",2},{"Mahur",2},{"Chahargah",2},
    {"Homayun",2},{"Shushtar",3},{"Shur Mugham",3},{"Rast Mugham",3} };
  int id=1;
  for(const auto& e:entries)if(selected==1||selected==e.culture||
    (selected==3&&(juce::String(e.name)=="Segah"||juce::String(e.name)=="Shur"||juce::String(e.name)=="Chahargah")))
    modeLibrary.addItem(e.name,id++);
  modeLibrary.setSelectedId(id>1?1:0,juce::sendNotification);
  repaint();
 };
 addAndMakeVisible(modeLibrary);
 modeLibrary.setColour(juce::ComboBox::backgroundColourId,surface2);
 modeLibrary.setColour(juce::ComboBox::textColourId,ivory);
 modeLibrary.setColour(juce::ComboBox::outlineColourId,gold.withAlpha(.4f));
 addAndMakeVisible(modeDescription);
 modeDescription.setColour(juce::Label::textColourId,muted);
 modeDescription.setJustificationType(juce::Justification::centredLeft);
 modeLibrary.onChange=[this]{
  modeDescription.setText("Reference: "+modeLibrary.getText()+" | seed scale profile only; not a verified full modal description",juce::dontSendNotification);
  repaint();
 };
 culture.onChange();
 exportGuide.onClick=[this]{
  auto r=processor.getResults();
  juce::String t="D.A MAQAM ANALYZER | EXPERIMENTAL REFERENCE\n";
  t+="Best match: "+r[0].name+" on "+r[0].root+"\n";
  t+="Tradition: "+r[0].tradition+"\n";
  t+="This is a heuristic scale match, NOT a verified Auto-Tune scale or cents table.\n";
  t+="Confirm tonic, melodic behavior and cents manually before tuning.\n";
  juce::SystemClipboard::copyTextToClipboard(t);
 };
 addAndMakeVisible(footer);
 footer.setText("EXPERIMENTAL CLASSIFIER  |  FIT IS NOT A PROBABILITY  |  NO AUTOMATIC TUNING",juce::dontSendNotification);
 footer.setColour(juce::Label::textColourId,muted);
 footer.setJustificationType(juce::Justification::centred);
 startTimerHz(12);
}
void DAMaqamAnalyzerAudioProcessorEditor::resized(){
 auto w=getWidth(),h=getHeight();
 culture.setBounds(27,104,210,35);
 learn.setBounds(w-432,101,132,39);
 stop.setBounds(w-292,101,145,39);
 copy.setBounds(w-139,101,112,39);
 footer.setBounds(20,h-42,w-40,25);
 const float top=157.0f,bottom=(float)h-57.0f,available=bottom-top;
 const float left=juce::jmax(255.f,w*.255f);
 modeLibrary.setBounds(36,(int)(top+available*.68f),juce::jmax(180,(int)left-62),34);
 modeDescription.setBounds(36,(int)(top+available*.76f),juce::jmax(180,(int)left-62),50);
 exportGuide.setBounds(w-292,h-106,263,32);
}
void DAMaqamAnalyzerAudioProcessorEditor::timerCallback(){
 auto hz=processor.getPitch();history[(size_t)head]=processor.isLearning()?hz:0;
 head=(head+1)%(int)history.size();repaint();
}
void DAMaqamAnalyzerAudioProcessorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> r,const juce::String& heading) const {
 g.setColour(surface);g.fillRoundedRectangle(r,12);
 g.setColour(gold.withAlpha(.30f));g.drawRoundedRectangle(r,12,1);
 g.setColour(gold);g.fillRect(r.getX()+18,r.getY()+44,r.getWidth()-36,1.0f);
 text(g,heading,{r.getX()+19,r.getY()+12,r.getWidth()-38,25},14,gold,true);
}
void DAMaqamAnalyzerAudioProcessorEditor::drawOrnament(juce::Graphics& g,juce::Rectangle<float> r) const {
 auto center=r.getCentre();float radius=juce::jmin(r.getWidth(),r.getHeight())*.32f;
 g.setColour(gold.withAlpha(.12f));g.fillEllipse(center.x-radius*1.25f,center.y-radius*1.25f,radius*2.5f,radius*2.5f);
 g.setColour(gold.withAlpha(.75f));
 for(int i=0;i<12;++i){
  float a=juce::MathConstants<float>::twoPi*i/12.0f;
  auto x=center.x+radius*.65f*std::cos(a),y=center.y+radius*.65f*std::sin(a);
  g.drawEllipse(x-radius*.45f,y-radius*.45f,radius*.9f,radius*.9f,1.2f);
 }
 g.setColour(gold);g.drawEllipse(center.x-radius,center.y-radius,radius*2,radius*2,1.5f);
 g.drawEllipse(center.x-radius*.26f,center.y-radius*.26f,radius*.52f,radius*.52f,2.0f);
}
void DAMaqamAnalyzerAudioProcessorEditor::drawGraph(juce::Graphics& g,juce::Rectangle<float> r) const {
 g.setColour(surface2);g.fillRoundedRectangle(r,7);
 for(int i=1;i<5;++i){
  auto y=r.getY()+r.getHeight()*i/5.0f;
  g.setColour(gold.withAlpha(.10f));g.drawHorizontalLine((int)y,r.getX()+8,r.getRight()-8);
 }
 juce::Path line;bool started=false;
 for(size_t i=0;i<history.size();++i){
  auto hz=history[(size_t)((head+(int)i)%history.size())];
  if(hz<60||hz>1300){started=false;continue;}
  float semis=69+12*std::log2(hz/440.0f);
  float y=juce::jmap(juce::jlimit(36.f,84.f,semis),36.f,84.f,r.getBottom()-12,r.getY()+12);
  float x=r.getX()+8+(r.getWidth()-16)*(float)i/(history.size()-1);
  if(!started){line.startNewSubPath(x,y);started=true;}else line.lineTo(x,y);
 }
 g.setColour(gold);g.strokePath(line,juce::PathStrokeType(2.2f));
 text(g,"LIVE MONOPHONIC PITCH TRACE",r.reduced(13).removeFromTop(22),11,muted);
}
void DAMaqamAnalyzerAudioProcessorEditor::paint(juce::Graphics& g){
 g.fillAll(background);
 auto w=(float)getWidth(),h=(float)getHeight();
 g.setColour(juce::Colour::fromRGB(12,25,33));g.fillRect(0,0,getWidth(),82);
 g.setColour(gold);g.fillRect(0,80,getWidth(),2);
 text(g,"D.A",{27,13,75,47},32,gold,true);
 text(g,"MAQAM ANALYZER",{103,16,500,37},28,ivory,true);
 text(g,"D.A PRODUCTION  /  MUSIC INTELLIGENCE",{w-362,26,337,24},12,gold,false,juce::Justification::centredRight);
 text(g,"TRADITION",{27,86,220,17},10,muted,true);
 text(g,processor.isLearning()?"●  LISTENING TO MELODY":"○  READY / ANALYSIS STOPPED",
      {257,103,360,37},15,processor.isLearning()?gold:muted,true);
 float top=157,bottom=h-57,available=bottom-top;
 float left=juce::jmax(255.f,w*.255f),right=juce::jmax(260.f,w*.275f),gap=12;
 auto a=juce::Rectangle<float>(20,top,left-26,available);
 auto b=juce::Rectangle<float>(a.getRight()+gap,top,w-left-right-gap-18,available);
 auto c=juce::Rectangle<float>(b.getRight()+gap,top,right-14,available);
 drawPanel(g,a,"MAQAM LIBRARY");drawPanel(g,b,"MELODY & MODE DETECTION");drawPanel(g,c,"TUNING REFERENCE");
 drawOrnament(g,{a.getX()+20,a.getY()+55,a.getWidth()-40,155});
 text(g,"FOUR MUSICAL TRADITIONS",{a.getX()+17,a.getY()+218,a.getWidth()-34,26},13,ivory,true);
 const char* entries[]={"PERSIAN  /  DASTGAH","AZERBAIJANI  /  MUGHAM","TURKISH  /  MAKAM","ARABIC  /  MAQAM"};
 for(int i=0;i<4;++i){auto y=a.getY()+255+i*43;
  g.setColour(surface2);g.fillRoundedRectangle(a.getX()+14,y,a.getWidth()-28,35,5);
  text(g,entries[i],{a.getX()+24,y+3,a.getWidth()-46,28},12,ivory);
 }
 text(g,"SELECT REFERENCE MODE",{a.getX()+17,a.getY()+available*.62f,a.getWidth()-34,24},12,gold,true);
 text(g,"Reference motifs only; not a complete library.",
      {a.getX()+17,a.getBottom()-49,a.getWidth()-34,37},11,muted);
 auto hz=processor.getPitch();
 text(g,"CURRENT NOTE",{b.getX()+19,b.getY()+61,180,20},11,muted,true);
 text(g,noteFor(hz),{b.getX()+18,b.getY()+83,190,62},48,gold,true);
 if(hz>40 && std::isfinite(hz)) {
  double midi=69.0+12.0*std::log2(hz/440.0);
  int nearest=(int)std::round(midi);
  double deviation=(midi-nearest)*100.0;
  text(g,"Deviation "+juce::String(deviation>=0?"+":"")+juce::String(deviation,1)+" cents",
       {b.getX()+185,b.getY()+133,b.getWidth()-205,22},12,muted,false,juce::Justification::centredRight);
 }
 text(g,hz>0?juce::String(hz,1)+" Hz":"NO PITCH",{b.getRight()-175,b.getY()+94,154,38},19,ivory,true,juce::Justification::centredRight);
 drawGraph(g,{b.getX()+16,b.getY()+160,b.getWidth()-32,juce::jmax(120.f,available*.33f)});
 auto results=processor.getResults();
 text(g,"TOP MODE MATCHES",{b.getX()+18,b.getY()+available*.64f,b.getWidth()-36,28},14,gold,true);
 for(int i=0;i<3;++i){
  const auto& x=results[(size_t)i];float y=b.getY()+available*.71f+i*40;
  g.setColour(surface2);g.fillRoundedRectangle(b.getX()+15,y,b.getWidth()-30,35,5);
  text(g,juce::String(i+1)+"   "+(x.name.isEmpty()?"Awaiting analysis":x.name+"  /  "+x.root),
       {b.getX()+24,y+2,b.getWidth()-150,30},13,ivory,true);
  if(!x.name.isEmpty())text(g,juce::String(x.fit,1)+" fit",{b.getRight()-119,y+2,95,30},13,gold,true,juce::Justification::centredRight);
 }
 text(g,"MODE",{c.getX()+18,c.getY()+61,c.getWidth()-36,20},11,muted,true);
 text(g,results[0].name.isEmpty()?"NOT ANALYZED":results[0].name,
      {c.getX()+18,c.getY()+83,c.getWidth()-36,44},27,gold,true);
 text(g,"ROOT",{c.getX()+18,c.getY()+137,140,20},11,muted,true);
 text(g,results[0].root.isEmpty()?"--":results[0].root,{c.getX()+18,c.getY()+160,c.getWidth()-36,38},27,ivory,true);
 g.setColour(gold.withAlpha(.35f));g.drawLine(c.getX()+18,c.getY()+210,c.getRight()-18,c.getY()+210,1);
 text(g,"AUTO-TUNE SETUP GUIDE",{c.getX()+18,c.getY()+223,c.getWidth()-36,30},14,gold,true);
 text(g,"1  Identify the tonic and mode",{c.getX()+18,c.getY()+263,c.getWidth()-36,30},13,ivory);
 text(g,"2  Verify intervals by ear",{c.getX()+18,c.getY()+298,c.getWidth()-36,30},13,ivory);
 text(g,"3  Enter cents in a compatible tuner",{c.getX()+18,c.getY()+333,c.getWidth()-36,42},12,ivory);
 text(g,"Copy guide below. Exact cents offsets and direct Auto-Tune integration are not available.\nDo not treat a match as a tuning preset.",
      {c.getX()+18,c.getBottom()-91,c.getWidth()-36,78},12,muted);
}
