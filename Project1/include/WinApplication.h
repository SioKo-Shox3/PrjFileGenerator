#pragma once

#include "IApplication.h"
#include "IWindow.h"
#include "IWindowContext.h"
#include <windows.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>

/**
 * @brief Windows環境でのアプリケーション実装
 */
class WinApplication : public IApplication
{
public:
    /**
     * @brief デフォルトコンストラクタ
     */
    WinApplication();

    /**
     * @brief コンストラクタ
     * @param hInstance アプリケーションインスタンスハンドル
     * @param nCmdShow 表示コマンド
     */
    WinApplication(HINSTANCE hInstance, int nCmdShow);

    /**
     * @brief デストラクタ
     */
    virtual ~WinApplication();

    /**
     * @brief アプリケーションの初期化
     * @param args コマンドライン引数
     * @return 初期化の成否
     */
    bool Initialize(const std::vector<std::wstring> &args) override;

    /**
     * @brief アプリケーションの初期化（拡張版）
     * @param hInstance アプリケーションインスタンスハンドル
     * @param windowTitle メインウィンドウのタイトル
     * @param width ウィンドウの幅
     * @param height ウィンドウの高さ
     * @param nCmdShow ウィンドウ表示モード
     * @return 初期化の成否
     */
    bool Initialize(HINSTANCE hInstance, const std::wstring &windowTitle, int width, int height, int nCmdShow);

    /**
     * @brief アプリケーションの実行
     * @return 終了コード
     */
    int Run() override;

    /**
     * @brief アプリケーションの終了
     */
    void Shutdown() override;

    /**
     * @brief メインウィンドウの取得
     * @return メインウィンドウへの参照
     */
    IWindow *GetMainWindow() override;

    /**
     * @brief アプリケーションにウィンドウを登録
     * @param window 登録するウィンドウ
     */
    void RegisterWindow(std::shared_ptr<IWindow> window) override;

    /**
     * @brief ウィンドウの登録解除
     * @param window 解除するウィンドウ
     */
    void UnregisterWindow(std::shared_ptr<IWindow> window) override;

    /**
     * @brief インスタンスハンドルの取得
     * @return インスタンスハンドル
     */
    HINSTANCE GetInstance() const;

    /**
     * @brief 表示コマンドの取得
     * @return 表示コマンド
     */
    int GetCmdShow() const;

private:
    HINSTANCE m_hInstance;
    int m_nCmdShow;
    std::shared_ptr<IWindowContext> m_windowContext;
    std::shared_ptr<IWindow> m_mainWindow;
    std::unordered_set<std::shared_ptr<IWindow>> m_windows;
    bool m_bIsRunning;
};
