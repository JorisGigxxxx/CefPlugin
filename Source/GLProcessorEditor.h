#pragma once

#include "BrowserManager.h"
#include "../JuceLibraryCode/JuceHeader.h"

class BrowserManager;

class GLProcessorEditor
    : public juce::AudioProcessorEditor
    , public juce::KeyListener
    , public juce::Slider::Listener
    , private juce::Timer
    , private juce::OpenGLRenderer
{
public:
    enum
    {
        kParamSliderHeight = 40,
        kParamLabelWidth = 80,
        kParamSliderWidth = 300
    };

    GLProcessorEditor(juce::AudioProcessor& parent, BrowserManager* inBrowserManager);
    virtual ~GLProcessorEditor();

public:
    void resized() override;
    void paint(juce::Graphics& g) override;

public:
    virtual void mouseMove(const juce::MouseEvent& event) override;
    virtual void mouseDown(const juce::MouseEvent& event) override;
    virtual void mouseDrag(const juce::MouseEvent& event) override;
    virtual void mouseUp(const juce::MouseEvent& event) override;
    virtual void mouseWheelMove(const juce::MouseEvent& event,
                                const juce::MouseWheelDetails& wheel) override;

public:
    virtual bool keyPressed(const juce::KeyPress& key,
                            juce::Component* originatingComponent) override;

public:
    void sliderValueChanged(juce::Slider* slider) override;
    void sliderDragStarted(juce::Slider* slider) override;
    void sliderDragEnded(juce::Slider* slider) override;

private:
    void timerCallback() override;
    juce::AudioProcessorParameter* getParameterForSlider(juce::Slider* slider);

private:
    void renderOpenGL() override;

    void newOpenGLContextCreated() override
    {
    }

    void openGLContextClosing() override
    {
    }

public:
    static const int                sWidth = 800;
    static const int                sHeigth = 600;
    static const int                sNumPixels = sWidth * sHeigth;

private:
    juce::Label                     noParameterLabel;
    juce::OwnedArray<juce::Slider>  paramSliders;
    juce::OwnedArray<juce::Label>   paramLabels;

private:
    BrowserManager*                 mBrowserManager;
    CefRefPtr<RenderHandler>        mRenderHandler;

private:
    juce::OpenGLContext             mOpenGLContext;
    uint32*                         mPixels;
};
