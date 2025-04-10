#include "WinApplication.h"
#include "Win32WindowContext.h"
#include "WindowRoutines.h"
#include "TWindow.h"
#include <chrono>
#include <thread>
#include <clocale> // setlocale、LC_ALLのために必要
#include <locale>  // std::locale のために必要

/**
 * @brief デフォルトコンストラクタ
 *
 * 現在のモジュールのインスタンスハンドルを取得し、
 * COMを初期化し、日本語サポートを設定します。
 */
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

/**
 * @brief 指定されたインスタンスハンドルと表示コマンドを使用してアプリケーションを初期化
 *
 * @param hInstance アプリケーションインスタンスハンドル
 * @param nCmdShow ウィンドウ表示コマンド
 */
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

/**
 * @brief デストラクタ
 *
 * アプリケーションのシャットダウンを実行し、COMを終了します
 */
WinApplication::~WinApplication()
{
    Shutdown();

    // COM終了処理
    CoUninitialize();
}

/**
 * @brief コマンドライン引数でアプリケーションを初期化
 *
 * @param args コマンドライン引数のリスト
 * @return 初期化が成功した場合はtrue
 */
bool WinApplication::Initialize(const std::vector<std::wstring> &args)
{
    // コマンドライン引数の処理（必要であれば）

    return true;
}

/**
 * @brief ウィンドウパラメータでアプリケーションを初期化
 *
 * @param hInstance アプリケーションインスタンスハンドル
 * @param windowTitle メインウィンドウのタイトル
 * @param width ウィンドウの幅
 * @param height ウィンドウの高さ
 * @param nCmdShow ウィンドウ表示モード
 * @return 初期化が成功した場合はtrue
 */
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
    std::shared_ptr<MainWindowRoutine> mainWindowRoutine = std::make_shared<MainWindowRoutine>();

    // メインウィンドウを作成
    std::shared_ptr<TWindow<MainWindowRoutine>> mainWindow = std::make_shared<TWindow<MainWindowRoutine>>(mainWindowRoutine, m_windowContext);

    // アプリケーションにウィンドウを登録
    RegisterWindow(mainWindow);

    // ウィンドウの作成
    if (!mainWindow->Create(windowTitle, width, height))
    {
        return false;
    }

    return true;
}

/**
 * @brief アプリケーションのメインループを実行
 *
 * ウィンドウメッセージの処理とアプリケーションの更新を行います
 *
 * @return 終了コード
 */
int WinApplication::Run()
{
    m_bIsRunning = true;

    // 前回の更新時間を記録
    std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();

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
            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
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

/**
 * @brief アプリケーションを終了し、すべてのウィンドウを破棄
 */
void WinApplication::Shutdown()
{
    m_bIsRunning = false;

    // ウィンドウをすべて削除
    for (std::shared_ptr<IWindow> window : m_windows)
    {
        if (window)
        {
            window->Destroy();
        }
    }
    m_windows.clear();

    m_mainWindow.reset();
}

/**
 * @brief メインウィンドウを取得
 *
 * @return メインウィンドウへのポインタ
 */
IWindow *WinApplication::GetMainWindow()
{
    return m_mainWindow.get();
}

/**
 * @brief アプリケーションにウィンドウを登録
 *
 * @param window 登録するウィンドウ
 */
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

/**
 * @brief ウィンドウの登録を解除
 *
 * @param window 解除するウィンドウ
 */
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

/**
 * @brief インスタンスハンドルを取得
 *
 * @return アプリケーションのインスタンスハンドル
 */
HINSTANCE WinApplication::GetInstance() const
{
    return m_hInstance;
}

/**
 * @brief 表示コマンドを取得
 *
 * @return ウィンドウ表示コマンド
 */
int WinApplication::GetCmdShow() const
{
    return m_nCmdShow;
}
