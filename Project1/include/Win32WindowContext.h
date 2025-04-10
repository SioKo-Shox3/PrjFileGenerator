#pragma once

#include "IWindowContext.h"
#include <unordered_map>
#include <windows.h>

/**
 * @brief Win32 APIを使用したウィンドウコンテキスト実装
 */
class Win32WindowContext : public IWindowContext
{
public:
    /**
     * @brief コンストラクタ
     * @param hInstance アプリケーションインスタンスハンドル
     */
    Win32WindowContext(HINSTANCE hInstance);

    /**
     * @brief デストラクタ
     */
    virtual ~Win32WindowContext();

    /**
     * @brief ウィンドウクラスの登録
     * @param className クラス名
     * @return 登録の成否
     */
    bool RegisterWindowClass(const std::wstring &className) override;

    /**
     * @brief ウィンドウの作成
     * @param className クラス名
     * @param title ウィンドウタイトル
     * @param width 幅
     * @param height 高さ
     * @param parentHandle 親ウィンドウハンドル
     * @param messageHandler メッセージハンドラ
     * @return 作成されたウィンドウハンドル
     */
    HWND CreateWindowInstance(
        const std::wstring &className,
        const std::wstring &title,
        int width,
        int height,
        HWND parentHandle,
        MessageHandler messageHandler) override;

    /**
     * @brief ウィンドウの破棄
     * @param hwnd ウィンドウハンドル
     */
    void DestroyWindowInstance(HWND hwnd) override;

    /**
     * @brief ウィンドウの表示
     * @param hwnd ウィンドウハンドル
     * @param showCommand 表示コマンド
     */
    void ShowWindow(HWND hwnd, int showCommand) override;

    /**
     * @brief メッセージループの実行
     * @return 終了コード
     */
    int RunMessageLoop() override;

private:
    HINSTANCE m_hInstance;
    std::unordered_map<HWND, MessageHandler> m_messageHandlers;
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
