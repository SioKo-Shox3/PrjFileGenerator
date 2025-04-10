#include "../include/WinApplication.h"
#include "../include/Win32WindowContext.h"
#include "../include/WindowRoutines.h"
#include "../include/TWindow.h"
#include <chrono>
#include <thread>
#include <clocale> // setlocale、LC_ALLのために必要
#include <locale>  // std::locale のために必要

WinApplication::WinApplication()
    : m_hInstance(GetModuleHandle(NULL)), m_nCmdShow(SW_SHOWDEFAULT), m_bIsRunning(false)
{
    // COM初期化
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // 日本語文字コードサポートを設定
    setlocale(LC_ALL, "Japanese");
    std::locale::global(std::locale(""));

    // ウィンドウコンテキストを作成
    m_windowContext = std::make_shared<Win32WindowContext>(m_hInstance);
}

WinApplication::WinApplication(HINSTANCE hInstance, int nCmdShow)
    : m_hInstance(hInstance), m_nCmdShow(nCmdShow), m_bIsRunning(false)
{
    // COM初期化
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // 日本語文字コードサポートを設定
    setlocale(LC_ALL, "Japanese");
    std::locale::global(std::locale(""));

    // ウィンドウコンテキストを作成
    m_windowContext = std::make_shared<Win32WindowContext>(hInstance);
}

WinApplication::~WinApplication()
{
    Shutdown();

    // COM終了処理
    CoUninitialize();
}

bool WinApplication::Initialize(const std::vector<std::wstring> &args)
{
    // コマンドライン引数の処理（必要であれば）

    return true;
}

bool WinApplication::Initialize(HINSTANCE hInstance, const std::wstring &windowTitle, int width, int height, int nCmdShow)
{
    // インスタンスハンドルと表示モードを設定
    m_hInstance = hInstance;
    m_nCmdShow = nCmdShow;

    // ウィンドウコンテキストがなければ作成
    if (!m_windowContext)
    {
        m_windowContext = std::make_shared<Win32WindowContext>(hInstance);
    }

    // メインウィンドウのルーチンを作成
    auto mainWindowRoutine = std::make_shared<MainWindowRoutine>();

    // メインウィンドウを作成
    auto mainWindow = std::make_shared<TWindow<MainWindowRoutine>>(mainWindowRoutine, m_windowContext);

    // アプリケーションにウィンドウを登録
    RegisterWindow(mainWindow);

    // ウィンドウの作成
    if (!mainWindow->Create(windowTitle, width, height))
    {
        return false;
    }

    return true;
}

int WinApplication::Run()
{
    m_bIsRunning = true;

    // 前回の更新時間を記録
    auto lastUpdateTime = std::chrono::high_resolution_clock::now();

    // メインウィンドウが作成されていない場合
    if (!m_mainWindow || !m_mainWindow->GetHandle())
    {
        return -1;
    }

    // メインウィンドウを表示
    m_mainWindow->Show(m_nCmdShow);

    // メッセージループ
    MSG msg = {};

    while (m_bIsRunning)
    {
        // Windowsメッセージを処理
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_bIsRunning = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // デルタタイム計算（秒単位）
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
            lastUpdateTime = currentTime;

            // アプリケーションの更新
            if (m_mainWindow)
            {
                m_mainWindow->Update(deltaTime);
            }

            // CPUを占有しないように少し待機
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return (int)msg.wParam;
}

void WinApplication::Shutdown()
{
    m_bIsRunning = false;

    // ウィンドウをすべて削除
    for (auto window : m_windows)
    {
        if (window)
        {
            window->Destroy();
        }
    }
    m_windows.clear();

    m_mainWindow.reset();
}

IWindow *WinApplication::GetMainWindow()
{
    return m_mainWindow.get();
}

void WinApplication::RegisterWindow(std::shared_ptr<IWindow> window)
{
    if (window)
    {
        m_windows.insert(window);
        window->SetApplication(this);

        // メインウィンドウがまだ設定されていない場合は設定
        if (!m_mainWindow)
        {
            m_mainWindow = window;
        }
    }
}

void WinApplication::UnregisterWindow(std::shared_ptr<IWindow> window)
{
    if (window)
    {
        m_windows.erase(window);

        // メインウィンドウが削除された場合
        if (m_mainWindow == window)
        {
            m_mainWindow.reset();

            // 別のメインウィンドウを探す
            if (!m_windows.empty())
            {
                m_mainWindow = *m_windows.begin();
            }
        }
    }
}

HINSTANCE WinApplication::GetInstance() const
{
    return m_hInstance;
}

int WinApplication::GetCmdShow() const
{
    return m_nCmdShow;
}
