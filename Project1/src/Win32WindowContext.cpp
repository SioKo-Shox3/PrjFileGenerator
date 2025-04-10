#include "../include/Win32WindowContext.h"

Win32WindowContext::Win32WindowContext(HINSTANCE hInstance)
    : m_hInstance(hInstance)
{
}

Win32WindowContext::~Win32WindowContext()
{
    m_messageHandlers.clear();
}

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

void Win32WindowContext::DestroyWindowInstance(HWND hwnd)
{
    if (hwnd)
    {
        m_messageHandlers.erase(hwnd);
        ::DestroyWindow(hwnd);
    }
}

void Win32WindowContext::ShowWindow(HWND hwnd, int showCommand)
{
    if (hwnd)
    {
        ::ShowWindow(hwnd, showCommand);
        UpdateWindow(hwnd);
    }
}

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
