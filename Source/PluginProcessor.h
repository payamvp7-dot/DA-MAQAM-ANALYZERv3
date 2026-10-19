#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
class DAMaqamAnalyzerAudioProcessor : public juce::AudioProcessor {
public:
 DAMaqamAnalyzerAudioProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "D.A MAQAM ANALYZER";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 void startLearning();
 void stopLearning();
 struct Result {juce::String name, tradition, root; double fit=0;};
 std::array<Result,3> getResults() const;
 float getPitch() const {return pitchHz.load();}
 bool isLearning() const {return learning.load();}
 int getFrames() const {return frames.load();}
private:
 static constexpr int window=4096, hop=2048, maxFrames=2048;
 std::array<float,window> ring{};
 std::array<float,maxFrames> pitches{};
 std::atomic<int> frames{0};
 std::atomic<float> pitchHz{0};
 std::atomic<bool> learning{false};
 int write=0,hopCounter=0;
 double sampleRate=44100;
 mutable juce::CriticalSection resultLock;
 std::array<Result,3> results{};
 void analyzeFrame();
 void classify();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DAMaqamAnalyzerAudioProcessor)
};
