/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "include/cef_app.h"
#include "GLProcessorEditor.h"
#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/**
 */
class GainProcessor :
    public juce::AudioProcessor, public CefBrowserProcessHandler
{
public:

    //==============================================================================
    GainProcessor()
        : juce::AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo())
                                           .withOutput ("Output", juce::AudioChannelSet::stereo()))
        , mBrowserManager(this)
    {
        addParameter(freq = new juce::AudioParameterFloat ("freq", "Freq", 20.0f, 20000.0f, 20.f));
        addParameter(gain = new juce::AudioParameterFloat("gain", "Gain", 0.0f, 1.0f, 0.5f));
        addParameter(q = new juce::AudioParameterFloat("q", "Q", 1.0f, 10.0f, 1.f));
    }

    ~GainProcessor() {}

    //==============================================================================
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        buffer.applyGain (*gain);
    }

    //==============================================================================

    juce::AudioProcessorEditor* createEditor() override { return new GLProcessorEditor(*this, &mBrowserManager); }
    bool hasEditor() const override               { return true;   }

    //==============================================================================
    const juce::String getName() const override         { return "CEF PlugIn"; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override             { return juce::String(); }
    void changeProgramName (int , const juce::String& ) override { }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream (destData, true).writeFloat (*gain);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        gain->setValueNotifyingHost (juce::MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat());
    }

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const juce::AudioChannelSet& mainInLayout  = layouts.getChannelSet (true,  0);
        const juce::AudioChannelSet& mainOutLayout = layouts.getChannelSet (false, 0);

        return (mainInLayout == mainOutLayout && (! mainInLayout.isDisabled()));
    }

private:
    //==============================================================================
    juce::AudioParameterFloat* freq;
    juce::AudioParameterFloat* gain;
    juce::AudioParameterFloat* q;

    enum { kVST2MaxChannels = 16 };

    BrowserManager mBrowserManager;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainProcessor)
    
private:
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(GainProcessor);
};

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GainProcessor();
}
