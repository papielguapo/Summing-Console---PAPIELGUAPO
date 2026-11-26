#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#define jj8nls legacy8
#include "LegacyDSP.h"
#undef jj8nls
#define jj8nls tone20
#include "ToneBaselineDSP.h"
#undef jj8nls
#define jj8nls scalar220
#include "ScalarReference220.h"
#undef jj8nls
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <new>
#include <atomic>
#include <limits>
#include <chrono>
#include <iomanip>
static std::atomic<size_t> allocations {0};
static thread_local bool measureAllocations = false;
void* operator new (size_t n) { if (measureAllocations) ++allocations; if (void* p=std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[] (size_t n) { return ::operator new(n); }
void operator delete (void* p) noexcept { std::free(p); }
void operator delete[] (void* p) noexcept { std::free(p); }
void operator delete (void* p, size_t) noexcept { std::free(p); }
void operator delete[] (void* p, size_t) noexcept { std::free(p); }
constexpr double fs = 48000.0;
constexpr int blockSize = 256;
int failures=0;
void check(bool ok,const char* text) { std::cout << (ok ? "PASS " : "FAIL ") << text << '\n'; if(!ok) ++failures; }

template<class Engine>
void capture(const std::filesystem::path& path, int factor, int variant, double tone, double drive,
             const std::string& signal, double amplitude, double frequency, bool ag=false, int stages=8)
{
    Engine engine;
    engine.setConsoleVariant(variant); engine.setBipolarTone(tone); engine.setBipolarDrive(drive);
    engine.setMasterNoiseEnabled(false); engine.setAutoPeakRmsBalance(false); engine.setAutoGainEnabled(ag);
    engine.setActiveStageCount(stages);
    engine.prepare(fs * (1<<factor));
    juce::dsp::Oversampling<float> os(2,static_cast<size_t>(factor),juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true);
    os.initProcessing(blockSize);
    juce::AudioBuffer<float> buffer(2,blockSize);
    const int length = signal=="impulse" ? 96000 : 48000;
    const int warm = signal=="impulse" ? 0 : 48000;
    std::vector<float> result(static_cast<size_t>(length)*2,std::numeric_limits<float>::quiet_NaN());
    double maxValue=0;
    for(int offset=-warm; offset<length;)
    {
        int count=std::min(blockSize,length-offset);
        if(offset<0) count=std::min(count,-offset);
        for(int n=0;n<count;++n)
        {
            const int i=offset+n;
            float x=signal=="impulse" ? (i==0 ? static_cast<float>(amplitude) : 0.0f)
                                      : static_cast<float>(amplitude*std::sin(2.0*jj8nls::kPi*frequency*i/fs));
            buffer.setSample(0,n,x); buffer.setSample(1,n,x);
        }
        juce::dsp::AudioBlock<float> all(buffer); auto base=all.getSubBlock(0,static_cast<size_t>(count));
        auto high=os.processSamplesUp(base);
        for(size_t n=0;n<high.getNumSamples();++n) engine.processSampleStereo(high.getChannelPointer(0)[n],high.getChannelPointer(1)[n]);
        os.processSamplesDown(base);
        if(offset>=0) for(int n=0;n<count;++n) for(int ch=0;ch<2;++ch)
        {
            const float y=buffer.getSample(ch,n);
            if(!std::isfinite(y)) ++failures;
            maxValue=std::max(maxValue,std::abs(static_cast<double>(y)));
            result[static_cast<size_t>(offset+n)*2+ch]=y;
        }
        offset += count;
    }
    check(std::all_of(result.begin(),result.end(),[](float x){return std::isfinite(x);}),"capture contains no gaps or nonfinite samples");
    std::ofstream file(path,std::ios::binary); file.write(reinterpret_cast<const char*>(result.data()),static_cast<std::streamsize>(result.size()*sizeof(float)));
    std::cout << path.filename().string() << " peak=" << maxValue << '\n';
}
void parameter(JJ8NLSAudioProcessor& p,const char* id,float value)
{ auto* par=p.getAPVTS().getParameter(id); par->setValueNotifyingHost(par->convertTo0to1(value)); }
void regressions()
{
    for(int mode : {0,2,3}) for(double rate : {44100.0,48000.0,96000.0,192000.0}) for(int channels : {1,2})
    {
        JJ8NLSAudioProcessor p;
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add(channels==1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
        layout.outputBuses=layout.inputBuses;
        check(p.setBusesLayout(layout),"mono/stereo layout");
        p.setOversamplingMode(mode); p.setAnalogNoiseEnabled(false); p.prepareToPlay(rate,64);
        check(mode!=0 || p.getLatencySamples()==0,"native mode reports zero latency");
        juce::AudioBuffer<float> b(channels,257); juce::MidiBuffer midi;
        for(int iteration=0;iteration<20;++iteration)
        {
            parameter(p,"drive",iteration%2 ? 10.0f : -10.0f); parameter(p,"sub_punch",iteration%2 ? -10.0f : 10.0f);
            parameter(p,"autogain",iteration%2 ? 1.0f : 0.0f); parameter(p,"bypass",iteration%2 ? 0.0f : 1.0f);
            p.setActiveStages(1+iteration%8); p.setAnalogNoiseEnabled(iteration%2==0);
            for(int ch=0;ch<channels;++ch) for(int n=0;n<b.getNumSamples();++n) b.setSample(ch,n,0.2f*std::sin(static_cast<float>(n)*0.17f));
            measureAllocations=true; p.processBlock(b,midi); measureAllocations=false;
            bool finite=true;
            for(int ch=0;ch<channels;++ch) for(int n=0;n<b.getNumSamples();++n) finite &= std::isfinite(b.getSample(ch,n)) && std::abs(b.getSample(ch,n))<4.0f;
            check(finite,"automation/oversized block remains bounded");
        }
        parameter(p,"bypass",1.0f); p.setAnalogNoiseEnabled(false);
        b.clear(); for(int k=0;k<20;++k) { b.clear(); p.processBlock(b,midi); }
        b.clear(); b.setSample(0,0,0.5f); p.processBlock(b,midi);
        bool exact=true;
        for(int n=0;n<b.getNumSamples();++n) exact &= std::abs(b.getSample(0,n)-(n==p.getLatencySamples()?0.5f:0.0f))<1.e-7f;
        check(exact,"bypass is an exact latency-aligned dry impulse");
        juce::MemoryBlock state; p.getStateInformation(state); p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        p.releaseResources();
    }
    {
        JJ8NLSAudioProcessor a,b;
        a.setAnalogNoiseEnabled(false); b.setAnalogNoiseEnabled(false);
        a.prepareToPlay(48000,64); b.prepareToPlay(48000,64);
        juce::AudioBuffer<float> inputA(2,4096),inputB(2,4096); juce::MidiBuffer midi;
        for(int ch=0;ch<2;++ch) for(int n=0;n<4096;++n)
            inputA.setSample(ch,n,0.3f*std::sin(static_cast<float>(n)*(ch==0?0.1f:0.17f)));
        inputB.makeCopyOf(inputA);
        a.processBlock(inputA,midi);
        for(int offset=0;offset<4096;offset+=13)
        {
            const int count=std::min(13,4096-offset);
            float* ptrs[]{inputB.getWritePointer(0,offset),inputB.getWritePointer(1,offset)};
            juce::AudioBuffer<float> view(ptrs,2,count); b.processBlock(view,midi);
        }
        double error=0;
        for(int ch=0;ch<2;++ch) for(int n=0;n<4096;++n)
            error=std::max(error,std::abs(static_cast<double>(inputA.getSample(ch,n)-inputB.getSample(ch,n))));
        check(error<1.e-6,"block partition invariance: 4096 samples versus 13-sample blocks");
    }
    {
        JJ8NLSAudioProcessor p;
        p.setAnalogNoiseEnabled(false); parameter(p,"autogain",0.0f); p.prepareToPlay(48000,64);
        const int highLatency=p.getLatencySamples();
        juce::AudioBuffer<float> b(2,64); juce::MidiBuffer midi;
        int sampleIndex=0; float previous=0; double largestStep=0;
        for(int mode : {0,2,3,0})
        {
            p.setOversamplingMode(mode);
            bool finite=true;
            for(int block=0;block<300;++block)
            {
                for(int n=0;n<64;++n)
                {
                    const float x=0.2f*std::sin(static_cast<float>(2.0*jj8nls::kPi*1000.0*sampleIndex++/48000.0));
                    b.setSample(0,n,x); b.setSample(1,n,x);
                }
                measureAllocations=true; p.processBlock(b,midi); measureAllocations=false;
                for(int n=0;n<64;++n)
                {
                    const float y=b.getSample(0,n); finite &= std::isfinite(y) && std::abs(y)<1.0f;
                    largestStep=std::max(largestStep,std::abs(static_cast<double>(y-previous)));previous=y;
                }
            }
            juce::Thread::sleep(30); juce::Timer::callPendingTimersSynchronously();
            check(finite,"live oversampling switch remains finite and bounded");
            check(p.getActiveOversamplingMode()==mode,"requested quality domain becomes active");
            check(p.getLatencySamples()==(mode==0?0:highLatency),"host PDC matches native/oversampled mode");
        }
        check(largestStep<0.06,"1 kHz quality transitions contain no abrupt amplitude discontinuity");
        juce::MemoryBlock state; p.getStateInformation(state);
        JJ8NLSAudioProcessor restored; restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        check(restored.getOversamplingMode()==0,"native quality survives state round-trip");
        b.clear(); for(int k=0;k<100;++k) { b.clear(); p.processBlock(b,midi); }
        b.clear(); b.setSample(0,0,0.1f); p.processBlock(b,midi);
        check(std::abs(b.getSample(0,0))>0.01f,"native wet impulse starts at sample zero");
    }
    check(allocations.load()==0,"zero operator new/new[] calls inside processBlock (including automation and chunks)");
}
void benchmark(const std::filesystem::path& out)
{
    std::ofstream csv(out/"cpu.csv");
    csv << "sample_rate,block_size,oversampling,setting,median_us,p95_us,max_us,median_deadline_percent,p95_deadline_percent\n";
    for(int mode : {0,2,3}) for(double rate : {48000.0,96000.0}) for(int size : {256}) for(int setting : {0})
    {
        JJ8NLSAudioProcessor p; p.setOversamplingMode(mode); p.setAnalogNoiseEnabled(true); p.prepareToPlay(rate,size);
        if(setting) { parameter(p,"drive",10); parameter(p,"sub_punch",10); }
        juce::AudioBuffer<float> b(2,size); juce::MidiBuffer midi;
        std::vector<double> timings; timings.reserve(300);
        const int warmBlocks=static_cast<int>(std::ceil(rate/static_cast<double>(size)));
        for(int run=0;run<warmBlocks+300;++run)
        {
            for(int ch=0;ch<2;++ch) for(int n=0;n<size;++n)
                b.setSample(ch,n,0.18f*std::sin(static_cast<float>(run*size+n)*0.131f));
            const auto start=std::chrono::steady_clock::now(); p.processBlock(b,midi);
            const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
            if(run>=warmBlocks) timings.push_back(us);
        }
        std::sort(timings.begin(),timings.end());
        const double deadline=size/rate*1.e6;
        csv << rate << ',' << size << ',' << (mode==0?0:(1<<mode)) << ',' << (setting?"HEAT_D10_T10":"Jaycen_default") << ','
            << timings[150] << ',' << timings[285] << ',' << timings.back() << ','
            << 100*timings[150]/deadline << ',' << 100*timings[285]/deadline << '\n';
    }
}

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    const std::filesystem::path out=argc>1 ? argv[1] : "measurements"; std::filesystem::create_directories(out);
    if(argc>2 && std::string(argv[2])=="--bench") { benchmark(out); return 0; }
    if (argc > 2 && std::string(argv[2]) == "--null")
    {
        for (int channels : {1, 2}) for (int variant = 1; variant < 5; ++variant)
        {
            jj8nls::JJ8NLSChainDSP optimized;
            scalar220::JJ8NLSChainDSP reference;
            auto setup = [&](auto& engine) {
                engine.setConsoleVariant(variant);
                engine.setBipolarDrive(1.0);
                engine.setMasterNoiseEnabled(false);
                engine.prepare(384000);
            };
            setup(optimized);
            setup(reference);
            bool identical = true;
            for (int n = 0; n < 120000; ++n)
            {
                if (n >= 20000 && n < 60000 && n % 1201 == 0)
                {
                    auto automate = [&](auto& engine) {
                        engine.setBipolarDrive(10.0 * std::sin(n * 0.013));
                        engine.setBipolarTone(10.0 * std::cos(n * 0.017));
                        engine.setSlotEnabled(2, n % 2 == 0);
                        engine.setActiveStageCount(1 + n % 8);
                    };
                    automate(optimized);
                    automate(reference);
                }
                float l = static_cast<float>(0.5 * std::sin(n * 0.03));
                float r = static_cast<float>(0.3 * std::cos(n * 0.11));
                float a = l, b = r;
                if (channels == 2 && (n < 17000 || n >= 18000)) {
                    optimized.processSampleStereo(l, r);
                    reference.processSampleStereo(a, b);
                } else {
                    optimized.processSampleMono(l);
                    reference.processSampleMono(a);
                }
                const bool same = std::memcmp(&l, &a, sizeof(float)) == 0
                          && std::memcmp(&r, &b, sizeof(float)) == 0;
                if (identical && !same)
                    std::cout << "First difference at sample " << n << ", variant " << variant << ", channels " << channels << std::endl;
                identical &= same;
            }
            check(identical, "scalar null with fixed controls and automated Drive/Tone/stages");
        }
        return failures ? 1 : 0;
    }
    {
        jj8nls::JJ8NLSChainDSP chain;
        for (int bank : {0, 4, 0})
        {
            chain.setConsoleVariant(bank);
            for (int count = 4; count <= 8; ++count)
            {
                chain.setActiveStageCount(count);
                for (int i = 0; i < 8; ++i)
                {
                    const auto& slot = chain.getSlotConfig(i);
                    const bool expected = bank == 0 ? (i < count-3 || i >= 5) : i < count;
                    check(chain.stageSelected(i) == expected, "physical slot selection");
                    check(slot.drive == (bank == 0 && i == 0 ? 0.5 : 0.0)
                        && slot.trimDb == (bank == 0 && i == 7 ? -2.0 : 0.0), "bank gain recall");
                }
            }
        }
    }
    if (argc > 2 && std::string(argv[2]) == "--jaycen")
    {
        JJ8NLSAudioProcessor p;
        check(p.getActiveStages()==8 && p.getMikeMask()==15, "default uses all eight stages");
        p.toggleMike(0); p.toggleMike(2);
        juce::MemoryBlock state; p.getStateInformation(state);
        JJ8NLSAudioProcessor restored;
        restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        check(restored.getActiveStages()==6 && restored.getMikeMask()==10, "noncontiguous Mike bypasses recalled");
        constexpr int length = 96000;
        juce::AudioBuffer<float> audio(2,256); juce::MidiBuffer midi;
        p.setActiveStages(8); p.prepareToPlay(48000,256);
        std::ofstream exact(out/"fresh-default.f32",std::ios::binary);
        for (int offset=0; offset<length; offset+=256)
        {
            for (int i=0;i<256;++i) for (int ch=0;ch<2;++ch)
                audio.setSample(ch,i,static_cast<float>(0.17803894*std::sin(2.0*jj8nls::kPi*1000.0*(offset+i)/48000.0)));
            p.processBlock(audio,midi);
            for (int i=0;i<256 && offset+i<length;++i) for (int ch=0;ch<2;++ch)
            {
                const float v=audio.getSample(ch,i); exact.write(reinterpret_cast<const char*>(&v),sizeof(v));
            }
        }
        for (int stages = 4; stages <= 8; ++stages)
        {
            const auto name = std::to_string(stages);
            capture<jj8nls::JJ8NLSChainDSP>(out/("impulse"+name+".f32"),3,0,0,0,"impulse",1.e-4,0,false,stages);
            capture<jj8nls::JJ8NLSChainDSP>(out/("sine"+name+".f32"),3,0,0,0,"sine",0.17803894,1000,false,stages);
        }
        capture<jj8nls::JJ8NLSChainDSP>(out/"default-linked.f32",3,0,0,0,"sine",0.17803894,1000,true);
        for (int tone : {-10,0,10})
            capture<jj8nls::JJ8NLSChainDSP>(out/("tone"+std::to_string(tone)+".f32"),3,0,tone,0,"impulse",1.e-4,0);
        for (int drive : {-10,0,5,10})
            for (int ag : {0,1})
                capture<jj8nls::JJ8NLSChainDSP>(out/("drive"+std::to_string(drive)+"ag"+std::to_string(ag)+".f32"),3,0,0,drive,"sine",0.70794578,1000,ag!=0);
        return failures ? 1 : 0;
    }
    regressions();
    if (argc>2 && std::string(argv[2])=="--regressions-only") return failures ? 1 : 0;
    if (argc>2 && std::string(argv[2])=="--drive-only")
    {
        for (int variant=0; variant<5; ++variant)
        {
            const auto name=std::to_string(variant);
            for (int drive : {-10,-8,-5,-2,0,1,5,10})
                capture<jj8nls::JJ8NLSChainDSP>(out/("drive211_"+name+"_"+std::to_string(drive)+".f32"),3,variant,0,drive,"sine",0.17803894,1000);
            for (int drive : {-10,-5,0,1,5,10})
                if (variant==4 || drive>=0)
                    capture<tone20::JJ8NLSChainDSP>(out/("drive_ref_"+name+"_"+std::to_string(drive)+".f32"),3,variant,0,drive,"sine",0.17803894,1000);
            if (variant<4) for (int drive : {-10,1})
                capture<jj8nls::JJ8NLSChainDSP>(out/("drive_fr_"+name+"_"+std::to_string(drive)+".f32"),3,variant,0,drive,"impulse",1.e-4,0);
        }
        std::cout << "FAILURES=" << failures << '\n';
        return failures ? 1 : 0;
    }
    if (argc>2 && std::string(argv[2])=="--tone-only")
    {
        for (int variant=0; variant<5; ++variant)
        {
            const auto name=std::to_string(variant);
            capture<tone20::JJ8NLSChainDSP>(out/("tone20_center_"+name+".f32"),3,variant,0,1,"impulse",1.e-4,0);
            for (int tone : {-10,-5,0,5,10})
            {
                const auto suffix=name+"_"+std::to_string(tone);
                capture<jj8nls::JJ8NLSChainDSP>(out/("tone21_fr_"+suffix+".f32"),3,variant,tone,1,"impulse",1.e-4,0);
                if (tone==-10 || tone==0 || tone==10)
                    capture<jj8nls::JJ8NLSChainDSP>(out/("tone21_ag_"+suffix+".f32"),3,variant,tone,1,"sine",0.17803894,1000,true);
            }
        }
        for (int factor : {0,2,3}) for (int tone : {-10,10})
            capture<jj8nls::JJ8NLSChainDSP>(out/("tone21_alias_"+std::to_string(1<<factor)+"_"+std::to_string(tone)+".f32"),factor,0,tone,10,"sine",0.70794578,11000);
        std::cout << "FAILURES=" << failures << '\n';
        return failures ? 1 : 0;
    }
    for(int variant=0;variant<5;++variant)
    {
        const auto name=std::to_string(variant);
        capture<jj8nls::JJ8NLSChainDSP>(out/("new_fr_"+name+".f32"),3,variant,0,1,"impulse",1.e-4,0);
        capture<legacy8::JJ8NLSChainDSP>(out/("old_fr_"+name+".f32"),2,variant,0,1,"impulse",1.e-4,0);
        for(auto spec : {std::pair<const char*,double>{"r18",std::sqrt(2.0)*std::pow(10.0,-18.0/20)}, {"p6",std::pow(10.0,-6.0/20)}, {"p3",std::pow(10.0,-3.0/20)}})
        {
            capture<jj8nls::JJ8NLSChainDSP>(out/("new_h_"+name+"_"+spec.first+".f32"),3,variant,0,1,"sine",spec.second,1000);
            capture<legacy8::JJ8NLSChainDSP>(out/("old_h_"+name+"_"+spec.first+".f32"),2,variant,0,1,"sine",spec.second,1000);
        }
        capture<jj8nls::JJ8NLSChainDSP>(out/("new_ag_"+name+".f32"),3,variant,0,1,"sine",0.17803894,1000,true);
    }
    for(int tone : {-10,10}) capture<jj8nls::JJ8NLSChainDSP>(out/("new_tone_"+std::to_string(tone)+".f32"),3,0,tone,1,"impulse",1.e-4,0);
    for(int drive : {-8,8}) capture<jj8nls::JJ8NLSChainDSP>(out/("new_heat_"+std::to_string(drive)+".f32"),3,4,10,drive,"sine",0.17803894,1000);
    for(int factor : {0,3,4}) capture<jj8nls::JJ8NLSChainDSP>(out/("alias_"+std::to_string(factor)+".f32"),factor,0,10,10,"sine",0.70794578,11000);
    std::cout << "FAILURES=" << failures << '\n'; return failures ? 1 : 0;
}
