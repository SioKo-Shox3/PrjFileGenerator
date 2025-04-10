#pragma once

#include <string>
#include <windows.h>
#include <functional>

/**
 * @brief ウィンドウコンテキスト抽象インターフェース
 *
 * プラットフォーム固有のウィンドウ生成・操作を抽象化する
 */
class IWindowContext
{
public:
    /**
     * @brief メッセージハンドラの型定義
     */
    using MessageHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

    /**
     * @brief 仮想デストラクタ
     */
    virtual ~IWindowContext() = default;

    /**
     * @brief ウィンドウクラスの登録
     * @param className クラス名
     * @return 登録の成否
     */
    virtual bool RegisterWindowClass(const std::wstring &className) = 0;

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
    virtual HWND CreateWindowInstance(
        const std::wstring &className,
        const std::wstring &title,
        int width,
        int height,
        HWND parentHandle,
        MessageHandler messageHandler) = 0;

    /**
     * @brief ウィンドウの破棄
     * @param hwnd ウィンドウハンドル
     */
    virtual void DestroyWindowInstance(HWND hwnd) = 0;

    /**
     * @brief ウィンドウの表示
     * @param hwnd ウィンドウハンドル
     * @param showCommand 表示コマンド
     */
    virtual void ShowWindow(HWND hwnd, int showCommand) = 0;

    /**
     * @brief メッセージループの実行
     * @return 終了コード
     */
    virtual int RunMessageLoop() = 0;
};
