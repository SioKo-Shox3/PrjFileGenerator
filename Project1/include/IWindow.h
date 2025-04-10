#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <windows.h>

class IWindowContext;
class IApplication;

/**
 * @brief ウィンドウ抽象インターフェース
 *
 * ウィンドウの生成・破棄・メッセージの処理を抽象化する
 */
class IWindow : public std::enable_shared_from_this<IWindow>
{
public:
    /**
     * @brief 仮想デストラクタ
     */
    virtual ~IWindow() = default;

    /**
     * @brief ウィンドウの作成
     * @param title ウィンドウタイトル
     * @param width 幅
     * @param height 高さ
     * @param parent 親ウィンドウ（オプション）
     * @return 作成の成否
     */
    virtual bool Create(const std::wstring &title, int width, int height, std::shared_ptr<IWindow> parent = nullptr) = 0;

    /**
     * @brief ウィンドウの表示
     * @param showCommand 表示コマンド
     */
    virtual void Show(int showCommand) = 0;

    /**
     * @brief ウィンドウの破棄
     */
    virtual void Destroy() = 0;

    /**
     * @brief ウィンドウハンドルの取得
     * @return ウィンドウハンドル
     */
    virtual HWND GetHandle() const = 0;

    /**
     * @brief 親ウィンドウの設定
     * @param parent 親ウィンドウ
     */
    virtual void SetParent(std::shared_ptr<IWindow> parent) = 0;

    /**
     * @brief 子ウィンドウの追加
     * @param child 子ウィンドウ
     */
    virtual void AddChild(std::shared_ptr<IWindow> child) = 0;

    /**
     * @brief 子ウィンドウの削除
     * @param child 子ウィンドウ
     */
    virtual void RemoveChild(std::shared_ptr<IWindow> child) = 0;

    /**
     * @brief メッセージ処理
     * @param msg Windowsメッセージ
     * @return 処理結果
     */
    virtual LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;

    /**
     * @brief アプリケーションの設定
     * @param app アプリケーション
     */
    virtual void SetApplication(IApplication *app) = 0;

    /**
     * @brief ウィンドウコンテキストの設定
     * @param context ウィンドウコンテキスト
     */
    virtual void SetContext(std::shared_ptr<IWindowContext> context) = 0;

    /**
     * @brief 毎フレームの更新
     * @param deltaTime フレーム間の経過時間
     */
    virtual void Update(float deltaTime) = 0;
};
