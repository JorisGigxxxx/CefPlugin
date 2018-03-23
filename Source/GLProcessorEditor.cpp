#include "GLProcessorEditor.h"

GLProcessorEditor::GLProcessorEditor (juce::AudioProcessor& parent, BrowserManager *inBrowserManager)
    : AudioProcessorEditor (parent)
    , noParameterLabel ("noparam", "No parameters available")
    , mBrowserManager(inBrowserManager)
    , mPixels(new uint32[1920 * 1080])
{
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    setResizable(true, true);
    setResizeLimits(200, 150, 1920, 1080);

    const juce::OwnedArray<juce::AudioProcessorParameter>& params = parent.getParameters();
    for (int i = 0; i < params.size(); ++i)
    {
        if (const juce::AudioParameterFloat* param = dynamic_cast<juce::AudioParameterFloat*>(params[i]))
        {
            const bool isLevelMeter = (((param->category & 0xffff0000) >> 16) == 2);
            if (isLevelMeter)
                continue;

            juce::Slider* aSlider;

            paramSliders.add (aSlider = new juce::Slider (param->name));
            aSlider->setRange (param->range.start, param->range.end);
            aSlider->setSliderStyle (juce::Slider::LinearHorizontal);
            aSlider->setValue (dynamic_cast<const juce::AudioProcessorParameter*>(param)->getValue());

            aSlider->addListener (this);
            //addAndMakeVisible (aSlider);

            juce::Label* aLabel;
            paramLabels.add (aLabel = new juce::Label (param->name, param->name));
            //addAndMakeVisible (aLabel);
        }
    }

    noParameterLabel.setJustificationType (juce::Justification::horizontallyCentred | juce::Justification::verticallyCentred);
    noParameterLabel.setFont (noParameterLabel.getFont().withStyle (juce::Font::italic));

    setSize(kParamSliderWidth + kParamLabelWidth,
            juce::jmax (1, kParamSliderHeight * paramSliders.size()));

    if (paramSliders.size() == 0)
        addAndMakeVisible (noParameterLabel);
    else
        startTimer (100);

    setSize(sWidth, sHeigth);
    
    mOpenGLContext.setRenderer(this);
    mOpenGLContext.attachTo(*this);
    mOpenGLContext.setContinuousRepainting(false);
    
    mRenderHandler = mBrowserManager->getRenderHandler();
    mRenderHandler->setOpenGLContext(&mOpenGLContext);
}

GLProcessorEditor::~GLProcessorEditor()
{
    //mBrowserClient->GetBrower()->GetHost()->CloseBrowser(false);
    mRenderHandler->setOpenGLContext(nullptr);
    removeKeyListener(this);
    free(mPixels);
}

// ----------------------------------------------------------------------------

void GLProcessorEditor::resized()
{
    if (mRenderHandler != nullptr)
    {
        mRenderHandler->resize(getWidth(), getHeight());
    }

    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return;
    }
    browser->GetHost()->WasResized();

    juce::Rectangle<int> r = getLocalBounds();
    noParameterLabel.setBounds (r);

    for (int i = 0; i < paramSliders.size(); ++i)
    {
        juce::Rectangle<int> paramBounds = r.removeFromTop (kParamSliderHeight);
        juce::Rectangle<int> labelBounds = paramBounds.removeFromLeft (kParamLabelWidth);

        paramLabels[i]->setBounds (labelBounds);
        paramSliders[i]->setBounds (paramBounds);
    }
}

void GLProcessorEditor::paint (juce::Graphics& g)
{
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

// ----------------------------------------------------------------------------

void GLProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return;
    }
    CefMouseEvent cefEvent;
    cefEvent.x = event.getPosition().getX();
    cefEvent.y = event.getPosition().getY();
    browser->GetHost()->SendMouseMoveEvent(cefEvent, false);
}

void GLProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return;
    }
    CefMouseEvent cefEvent;
    cefEvent.x = event.getPosition().getX();
    cefEvent.y = event.getPosition().getY();

    cef_mouse_button_type_t type;
    if (event.mods.isLeftButtonDown())
    {
        type = MBT_LEFT;
    }
    else if (event.mods.isMiddleButtonDown())
    {
        type = MBT_MIDDLE;
    }
    else if (event.mods.isRightButtonDown())
    {
        type = MBT_RIGHT;
    }
    browser->GetHost()->SendMouseClickEvent(cefEvent, type, false, event.getNumberOfClicks());
}

void GLProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return;
    }
    CefMouseEvent cefEvent;
    cefEvent.x = event.getPosition().getX();
    cefEvent.y = event.getPosition().getY();
    browser->GetHost()->SendMouseMoveEvent(cefEvent, false);
}

void GLProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return;
    }
    CefMouseEvent cefEvent;
    cefEvent.x = event.getPosition().getX();
    cefEvent.y = event.getPosition().getY();
    
    cef_mouse_button_type_t type;
    if (event.mods.isLeftButtonDown())
    {
        type = MBT_LEFT;
    }
    else if (event.mods.isMiddleButtonDown())
    {
        type = MBT_MIDDLE;
    }
    else if (event.mods.isRightButtonDown())
    {
        type = MBT_RIGHT;
    }
    browser->GetHost()->SendMouseClickEvent(cefEvent, type, true, event.getNumberOfClicks());
}

void GLProcessorEditor::mouseWheelMove(const juce::MouseEvent& event,
                                       const juce::MouseWheelDetails& wheel)
{
    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return;
    }

    CefMouseEvent cefEvent;
    cefEvent.x = event.getPosition().getX();
    cefEvent.y = event.getPosition().getY();

    int deltaY = static_cast<int>(wheel.deltaY * 100.f);

    //browser->GetHost()->SendMouseWheelEvent(cefEvent, 0, deltaY);
}

// ----------------------------------------------------------------------------

bool GLProcessorEditor::keyPressed(const juce::KeyPress& key,
                                   juce::Component* originatingComponent)
{
    CefRefPtr<CefBrowser> browser = mBrowserManager->getBrowser();
    if (browser == nullptr)
    {
        return false;
    }

    CefKeyEvent cefKey;
    cefKey.type = KEYEVENT_CHAR;
    cefKey.modifiers = 0;
    cefKey.windows_key_code = key.getTextCharacter();

    browser->GetHost()->SendKeyEvent(cefKey);

    return true;
}

// ----------------------------------------------------------------------------

void GLProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (juce::AudioProcessorParameter* param = getParameterForSlider (slider))
    {
        if (slider->isMouseButtonDown())
            param->setValueNotifyingHost ((float) slider->getValue());
        else
            param->setValue ((float) slider->getValue());
    }
}

void GLProcessorEditor::sliderDragStarted (juce::Slider* slider)
{
    if (juce::AudioProcessorParameter* param = getParameterForSlider (slider))
        param->beginChangeGesture();
}

void GLProcessorEditor::sliderDragEnded (juce::Slider* slider)
{
    if (juce::AudioProcessorParameter* param = getParameterForSlider (slider))
        param->endChangeGesture();
}

void GLProcessorEditor::timerCallback()
{
    //CefDoMessageLoopWork();
    //const juce::OwnedArray<juce::AudioProcessorParameter>& params = getAudioProcessor()->getParameters();
    //for (int i = 0; i < params.size(); ++i)
    //{
    //    if (const juce::AudioProcessorParameter* param = params[i])
    //    {
    //        if (i < paramSliders.size())
    //            paramSliders[i]->setValue (param->getValue());
    //    }
    //}
}

juce::AudioProcessorParameter* GLProcessorEditor::getParameterForSlider (juce::Slider* slider)
{
    const juce::OwnedArray<juce::AudioProcessorParameter>& params = getAudioProcessor()->getParameters();
    return params[paramSliders.indexOf (slider)];
}

// ----------------------------------------------------------------------------

void GLProcessorEditor::renderOpenGL()
{
    jassert(juce::OpenGLHelpers::isContextActive());
    //juce::OpenGLHelpers::clear(juce::Colours::red);


    int bufferWidth, bufferHeight;
    uint32 *pBuffer = mRenderHandler->getBuffer(bufferWidth, bufferHeight);
    uint32 *pBufferPtr = pBuffer;

    //int width = bufferWidth;
    //int height = bufferHeight;
    int width = getWidth();
    int height = getHeight();

    char *pixelsPtr = (char*)mPixels;

    for (int i = height - 1; i >= 0; i--)
    {
        char* line = (char*)&pBufferPtr[i * width];
        for (int j = 0; j < width; j++)
        {
            // BGRA to RGBA !
            *pixelsPtr++ = line[2];
            *pixelsPtr++ = line[1];
            *pixelsPtr++ = line[0];
            *pixelsPtr++ = line[3];

            line += 4;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, mPixels);
    glEnable(GL_DEPTH_TEST);
}
