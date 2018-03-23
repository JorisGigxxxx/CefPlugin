#pragma once

#include <include/cef_app.h>
#include <include/cef_client.h>
#include "../JuceLibraryCode/JuceHeader.h"

class RenderHandler
    : public CefRenderHandler
{
public:
    RenderHandler(int w, int h)
        : mWidth(w)
        , mHeight(h)
        , mBuffer(nullptr)
        , mOpenGLContext(nullptr)
    {
        resize(w, h);
    }

    ~RenderHandler()
    {
        mOpenGLContext->detach();
        free(mBuffer);
    }

    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
    {
        //OutputDebugStringW(L"GetViewRect()\n");
        rect = CefRect(0, 0, mWidth, mHeight);
        return true;
    }

    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void * buffer, int w, int h)
    {
        //OutputDebugStringW(L"OnPaint()\n");
        //if (w != mWidth || h != mHeight)
        //{
        //    resize(w, h);
        //}

        mBufferWidth = std::min(mWidth, w);
        mBufferHeight = std::min(mHeight, h);

        memcpy(mBuffer, buffer, mBufferWidth * mBufferHeight << 2);
        if (mOpenGLContext != nullptr)
        {
            mOpenGLContext->triggerRepaint();
        }
    }

    void resize(int w, int h)
    {
        mWidth = w;
        mHeight = h;
        if (mBuffer != nullptr)
        {
            free(mBuffer);
        }
        mBuffer = new uint32[w * h];
    }

    void render()
    {
    }

public:
    void setOpenGLContext(juce::OpenGLContext* inOpenGLContext)
    {
        mOpenGLContext = inOpenGLContext;
    }
    uint32* getBuffer(int& outWidth, int& outHeight)
    {
        outWidth = mBufferWidth;
        outHeight = mBufferHeight;
        return mBuffer;
    }

private:
    int mWidth;
    int mHeight;

    juce::OpenGLContext* mOpenGLContext;
    uint32* mBuffer;
    int mBufferWidth;
    int mBufferHeight;

    IMPLEMENT_REFCOUNTING(RenderHandler);
};

class RequestContextHandler :public CefRequestContextHandler
{
public:
    RequestContextHandler()
    {
        mCookieManager = CefCookieManager::CreateManager("D:/CefCookie", FALSE, nullptr);
    };
    virtual ~RequestContextHandler()
    {
    };
    virtual CefRefPtr<CefCookieManager> GetCookieManager() override
    {
        return mCookieManager;
    }
    
private:
    CefRefPtr<CefCookieManager> mCookieManager;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(RequestContextHandler);
};

class BrowserClient
    : public CefClient
    , public CefLifeSpanHandler
    , public CefLoadHandler
{
public:
    BrowserClient(CefRefPtr<CefRenderHandler> ptr)
        : mRenderHandler(ptr)
    {
    }

public: // CefClient
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler()
    {
        return this;
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler()
    {
        return this;
    }

    virtual CefRefPtr<CefRenderHandler> GetRenderHandler()
    {
        return mRenderHandler;
    }

public: // CefLifeSpanHandler
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        // Must be executed on the UI thread.
        //CEF_REQUIRE_UI_THREAD();

        OutputDebugStringW(L"OnAfterCreated\n");
        mBrowser = browser;
    }

    virtual bool DoClose(CefRefPtr<CefBrowser> browser) override
    {
        // Must be executed on the UI thread.
        //CEF_REQUIRE_UI_THREAD();

        // Closing the main window requires special handling. See the DoClose()
        // documentation in the CEF header for a detailed description of this
        // process.
        if (browser->GetIdentifier() == mBrowser->GetIdentifier())
        {
            // Set a flag to indicate that the window close should be allowed.
            mClosing = true;
        }

        // Allow the close. For windowed browsers this will result in the OS close
        // event being sent.
        return false;
    }

    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
    {
        OutputDebugStringW(L"OnBeforeClose()\n");
    }


    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefLoadHandler::ErrorCode errorCode,
                                const CefString & errorText,
                                const CefString & failedUrl) override
    {
        OutputDebugStringW(L"OnLoadError()\n");
        mLoaded = true;
    }

    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                        bool isLoading,
                                        bool canGoBack,
                                        bool canGoForward) override
    {
        OutputDebugStringW(L"OnLoadingStateChange()\n");
    }

    virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                TransitionType transition_type) override
    {
        OutputDebugStringW(L"OnLoadStart()\n");
    }

    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame,
                            int httpStatusCode) override
    {
        OutputDebugStringW(L"OnLoadEnd()\n");
        std::cout << "OnLoadEnd(" << httpStatusCode << ")" << std::endl;
        mLoaded = true;
    }
public:
    bool closeAllowed() const
    {
        return mClosing;
    }

    bool isLoaded() const
    {
        return mLoaded;
    }

    CefRefPtr<CefBrowser> GetBrower() {return mBrowser;}

private:
    bool mClosing = false;
    bool mLoaded = false;
    CefRefPtr<CefRenderHandler> mRenderHandler;
    CefRefPtr<CefBrowser> mBrowser;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

class App
    : public CefApp
    , public CefRenderProcessHandler
    , public CefV8Interceptor
{
public:
    App(juce::AudioProcessor* inAudioProcessor)
        : mAudioProcessor(inAudioProcessor)
        , mParams(inAudioProcessor->getParameters())
        , mGain(1)
    {
    }

public: // CefApp
    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() {
        return this;
    }

public: // CefRenderProcessHandler
    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context)
    {
        //// Retrieve the context's window object.
        CefRefPtr<CefV8Value> window = context->GetGlobal();
        //
        //// Create a new V8 string value. See the "Basic JS Types" section below.
        //CefRefPtr<CefV8Value> str = CefV8Value::CreateString("My Value!");
        //
        //// Add the string to the window object as "window.myval". See the "JS Objects" section below.
        //object->SetValue("myval", str, V8_PROPERTY_ATTRIBUTE_NONE);

        CefRefPtr<CefV8Value> object = CefV8Value::CreateObject(nullptr, this);

        CefString objName("parameters");

        object->SetValue(objName, V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
        window->SetValue(objName, object, V8_PROPERTY_ATTRIBUTE_NONE);
     }

public: // CefV8Interceptor
    virtual bool Get(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     CefRefPtr<CefV8Value>& retval,
                     CefString& exception) override
    {
        for (const auto& param : mParams)
        {
            if (param->getName(name.length()) == juce::String(name))
            {
                retval = CefV8Value::CreateDouble(param->getValue());
                return true;
            }
        }
        return false;
    }

    virtual bool Get(int index,
                     const CefRefPtr<CefV8Value> object,
                     CefRefPtr<CefV8Value>& retval,
                     CefString& exception) override
    {
        return true;
    }


    virtual bool Set(const CefString& name,
                     const CefRefPtr<CefV8Value> object,
                     const CefRefPtr<CefV8Value> value,
                     CefString& exception) override
    {
        for (const auto& param : mParams)
        {
            if (param->getName(name.length()) == juce::String(name))
            {
                jassert(value->IsDouble());
                param->setValueNotifyingHost((float)value->GetDoubleValue());
                return true;
            }
        }

        return false;
    }

    virtual bool Set(int index,
                     const CefRefPtr<CefV8Value> object,
                     const CefRefPtr<CefV8Value> value,
                     CefString& exception) override
    {
        return true;
    }

public:
    double mGain; 
    juce::AudioProcessor* mAudioProcessor;
    const juce::OwnedArray<juce::AudioProcessorParameter>& mParams;

public:
    IMPLEMENT_REFCOUNTING(App);
};

class BrowserManager
{
private:
    static BrowserManager* sInstance;
    static const char* sTestUrl;

public:
    BrowserManager(juce::AudioProcessor* inAudioProcessor)
    {    
        // init CEF
        CefMainArgs args;
        CefRefPtr<CefApp> app = new App(inAudioProcessor);

#if 1
        {
            int result = CefExecuteProcess(args, app, nullptr);
            // checkout CefApp, derive it and set it as second parameter, for more control on
            // command args and resources.
            if (result >= 0) // child proccess has endend, so exit.
            {
                return;
            }
            else if (result == -1)
            {
                // we are here in the father proccess.
            }
        }
#endif

        {
            CefSettings settings;

            // CefString(&settings.resources_dir_path).FromASCII("");
            // checkout detailed settings options:
            // http://magpcss.org/ceforum/apidocs/projects/%28default%29/_cef_settings_t.html
            // nearly all the settings can be set via args too.
            settings.single_process = true;
            settings.multi_threaded_message_loop = true; // not supported, except windows
            // settings.single_process = true; // not supported, except windows
            // settings.remote_debugging_port = 8090;
            // CefString(&settings.browser_subprocess_path).FromASCII("sub_proccess path, by default uses and starts this executeable as child");
            // CefString(&settings.cache_path).FromASCII("");
            // CefString(&settings.log_file).FromASCII("");
            // settings.log_severity = LOGSEVERITY_DEFAULT;

            bool result = CefInitialize(args, settings, app, nullptr);
            // CefInitialize creates a sub-proccess and executes the same executeable,
            // as calling CefInitialize, if not set different in settings.browser_subprocess_path
            // if you create an extra program just for the childproccess you only have to call CefExecuteProcess(...) in it.
            if (!result)
            {
                // handle error
                return;
            }
        }

        mRenderHandler = new RenderHandler(800, 600);
        mBrowserClient = new BrowserClient(mRenderHandler);

        CefWindowInfo window_info;
        CefBrowserSettings browserSettings;

        // browserSettings.windowless_frame_rate = 60; // 30 is default

        //window_info.SetAsWindowless((HWND) getWindowHandle());
        window_info.SetAsWindowless(NULL);

        CefRequestContextSettings reqsettings;
        CefRefPtr<CefRequestContext> rc = CefRequestContext::GetGlobalContext();

        CefBrowserHost::CreateBrowser(window_info, mBrowserClient.get(), sTestUrl, browserSettings, rc);
    }

public:
    //static BrowserManager* getInstance()
    //{
    //    if (sInstance == nullptr)
    //    {
    //        sInstance = new BrowserManager();
    //    }
    //
    //    return sInstance;
    //}

    CefRefPtr<RenderHandler> getRenderHandler()
    {
        return mRenderHandler;
    }

    CefRefPtr<CefBrowser> getBrowser()
    {
        return mBrowserClient->GetBrower();
    }


private:
    CefRefPtr<RenderHandler> mRenderHandler;
    CefRefPtr<BrowserClient> mBrowserClient;
};
