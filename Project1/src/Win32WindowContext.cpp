#include "Win32WindowContext.h"

/**
 * @brief コンストラクタ
 *
 * インスタンスハンドルを初期化します
 *
 * @param hInstance アプリケーションインスタンスハンドル
 */
Win32WindowContext::Win32WindowContext(HINSTANCE hInstance)
    : m_hInstance(hInstance)
{
}

/**
 * @brief デストラクタ
 *
 * メッセージハンドラの登録を解除します
 */
Win32WindowContext::~Win32WindowContext()
{
    m_messageHandlers.clear();
}

/**
 * @brief ウィンドウクラスの登録
 *
 * Win32 APIを使用してウィンドウクラスを登録します
 *
 * @param className 登録するウィンドウクラス名
 * @return 登録に成功した場合はtrue、失敗した場合はfalse
 */
bool Win32WindowContext::RegisterWindowClass(const std::wstring &className)
{
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = Win32WindowContext::StaticWindowProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = className.c_str();
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    return (RegisterClassW(&wc) != 0);
}

/**
 * @brief ウィンドウの作成
 *
 * Win32 APIを使用してウィンドウを作成し、メッセージハンドラを登録します
 *
 * @param className ウィンドウクラス名
 * @param title ウィンドウのタイトル
 * @param width ウィンドウの幅
 * @param height ウィンドウの高さ
 * @param parentHandle 親ウィンドウのハンドル（子ウィンドウの場合）
 * @param messageHandler ウィンドウメッセージを処理するハンドラ関数
 * @return 作成されたウィンドウのハンドル、失敗した場合はNULL
 */
HWND Win32WindowContext::CreateWindowInstance(
    const std::wstring &className,
    const std::wstring &title,
    int width,
    int height,
    HWND parentHandle,
    MessageHandler messageHandler)
{
    // ウィンドウスタイルを設定
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (parentHandle)
    {
        style = WS_CHILD | WS_VISIBLE;
    }

    // ウィンドウを作成
    HWND hwnd = CreateWindowExW(
        0,                            // 拡張スタイル
        className.c_str(),            // ウィンドウクラス名
        title.c_str(),                // ウィンドウタイトル
        style,                        // ウィンドウスタイル
        CW_USEDEFAULT, CW_USEDEFAULT, // 位置
        width, height,                // サイズ
        parentHandle,                 // 親ウィンドウ
        NULL,                         // メニュー
        m_hInstance,                  // インスタンスハンドル
        this                          // 追加のアプリケーションデータ
    );

    if (hwnd)
    {
        // メッセージハンドラを登録
        m_messageHandlers[hwnd] = messageHandler;
    }

    return hwnd;
}

/**
 * @brief ウィンドウの破棄
 *
 * ウィンドウを破棄し、関連するメッセージハンドラの登録を解除します
 *
 * @param hwnd 破棄するウィンドウのハンドル
 */
void Win32WindowContext::DestroyWindowInstance(HWND hwnd)
{
    if (hwnd)
    {
        m_messageHandlers.erase(hwnd);
        ::DestroyWindow(hwnd);
    }
}

/**
 * @brief ウィンドウの表示
 *
 * 指定されたウィンドウを表示し、更新します
 *
 * @param hwnd 表示するウィンドウのハンドル
 * @param showCommand 表示コマンド（例：SW_SHOW, SW_HIDE など）
 */
void Win32WindowContext::ShowWindow(HWND hwnd, int showCommand)
{
    if (hwnd)
    {
        ::ShowWindow(hwnd, showCommand);
        UpdateWindow(hwnd);
    }
}

/**
 * @brief メッセージループの実行
 *
 * Windowsメッセージループを実行し、アプリケーションの終了まで処理を続けます
 *
 * @return アプリケーションの終了コード
 */
int Win32WindowContext::RunMessageLoop()
{
    MSG msg = {};

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

/**
 * @brief 静的ウィンドウプロシージャ
 *
 * ウィンドウメッセージを処理する静的関数です。
 * ウィンドウコンテキストを取得し、適切なメッセージハンドラを呼び出します。
 *
 * @param hwnd メッセージを受け取るウィンドウのハンドル
 * @param msg メッセージの種類
 * @param wParam メッセージの追加情報（メッセージによって異なる）
 * @param lParam メッセージの追加情報（メッセージによって異なる）
 * @return メッセージ処理の結果
 */
LRESULT CALLBACK Win32WindowContext::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // メッセージがWM_NCREATEの場合、ウィンドウコンテキストを取得して設定
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
        Win32WindowContext *context = reinterpret_cast<Win32WindowContext *>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(context));
    }

    // ウィンドウコンテキストを取得
    Win32WindowContext *context = reinterpret_cast<Win32WindowContext *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // コンテキストがあれば、対応するメッセージハンドラを呼び出す
    if (context && context->m_messageHandlers.count(hwnd) > 0)
    {
        return context->m_messageHandlers[hwnd](hwnd, msg, wParam, lParam);
    }

    // デフォルトの処理
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
