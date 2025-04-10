#pragma once

#include "IWindow.h"
#include "IWindowContext.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief ウィンドウルーチンのインターフェース
 */
class IWindowRoutine
{
public:
    virtual ~IWindowRoutine() = default;
    virtual void OnCreate(IWindow *window) = 0;
    virtual void OnDestroy(IWindow *window) = 0;
    virtual void OnUpdate(IWindow *window, float deltaTime) = 0;
    virtual LRESULT OnMessage(IWindow *window, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};

/**
 * @brief テンプレート化されたウィンドウ実装
 * @tparam TRoutine ウィンドウルーチン型
 */
template <typename TRoutine>
class TWindow : public IWindow
{
public:
    /**
     * @brief コンストラクタ
     * @param routine ウィンドウルーチン
     * @param context ウィンドウコンテキスト
     */
    TWindow(std::shared_ptr<TRoutine> routine, std::shared_ptr<IWindowContext> context)
        : m_routine(routine), m_context(context), m_handle(nullptr), m_application(nullptr) {}

    /**
     * @brief デストラクタ
     */
    virtual ~TWindow()
    {
        Destroy();
    }

    /**
     * @brief ウィンドウの作成
     * @param title ウィンドウタイトル
     * @param width 幅
     * @param height 高さ
     * @param parent 親ウィンドウ（オプション）
     * @return 作成の成否
     */
    bool Create(const std::wstring &title, int width, int height, std::shared_ptr<IWindow> parent = nullptr) override
    {
        m_parent = parent;

        // ウィンドウクラス名を設定（一意であること）
        std::wstring className = L"TWindowClass_" + std::to_wstring(reinterpret_cast<uintptr_t>(this));

        if (!m_context->RegisterWindowClass(className))
        {
            return false;
        }

        HWND parentHandle = parent ? parent->GetHandle() : nullptr;

        // ウィンドウを作成
        m_handle = m_context->CreateWindowInstance(
            className,
            title,
            width,
            height,
            parentHandle,
            [this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
            {
                return this->ProcessMessage(msg, wParam, lParam);
            });

        if (m_handle)
        {
            if (m_routine)
            {
                m_routine->OnCreate(this);
            }

            // 親がある場合は子として追加
            if (parent)
            {
                parent->AddChild(shared_from_this());
            }

            // アプリケーションに登録
            if (m_application)
            {
                m_application->RegisterWindow(shared_from_this());
            }

            return true;
        }

        return false;
    }

    /**
     * @brief ウィンドウの表示
     * @param showCommand 表示コマンド
     */
    void Show(int showCommand) override
    {
        if (m_handle)
        {
            m_context->ShowWindow(m_handle, showCommand);
        }
    }

    /**
     * @brief ウィンドウの破棄
     */
    void Destroy() override
    {
        if (m_handle)
        {
            if (m_routine)
            {
                m_routine->OnDestroy(this);
            }

            // 子ウィンドウをすべて削除
            for (auto &child : m_children)
            {
                child->Destroy();
            }
            m_children.clear();

            // 親から削除
            if (m_parent)
            {
                m_parent->RemoveChild(shared_from_this());
                m_parent.reset();
            }

            // アプリケーションから登録解除
            if (m_application)
            {
                m_application->UnregisterWindow(shared_from_this());
            }

            m_context->DestroyWindowInstance(m_handle);
            m_handle = nullptr;
        }
    }

    /**
     * @brief ウィンドウハンドルの取得
     * @return ウィンドウハンドル
     */
    HWND GetHandle() const override
    {
        return m_handle;
    }

    /**
     * @brief 親ウィンドウの設定
     * @param parent 親ウィンドウ
     */
    void SetParent(std::shared_ptr<IWindow> parent) override
    {
        if (m_parent)
        {
            m_parent->RemoveChild(shared_from_this());
        }

        m_parent = parent;

        if (m_parent)
        {
            m_parent->AddChild(shared_from_this());
        }
    }

    /**
     * @brief 子ウィンドウの追加
     * @param child 子ウィンドウ
     */
    void AddChild(std::shared_ptr<IWindow> child) override
    {
        m_children.push_back(child);
    }

    /**
     * @brief 子ウィンドウの削除
     * @param child 子ウィンドウ
     */
    void RemoveChild(std::shared_ptr<IWindow> child) override
    {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end())
        {
            m_children.erase(it);
        }
    }

    /**
     * @brief メッセージ処理
     * @param msg Windowsメッセージ
     * @param wParam wParam
     * @param lParam lParam
     * @return 処理結果
     */
    LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
    {
        if (m_routine)
        {
            return m_routine->OnMessage(this, msg, wParam, lParam);
        }
        return DefWindowProc(m_handle, msg, wParam, lParam);
    }

    /**
     * @brief アプリケーションの設定
     * @param app アプリケーション
     */
    void SetApplication(IApplication *app) override
    {
        m_application = app;
    }

    /**
     * @brief ウィンドウコンテキストの設定
     * @param context ウィンドウコンテキスト
     */
    void SetContext(std::shared_ptr<IWindowContext> context) override
    {
        m_context = context;
    }

    /**
     * @brief 毎フレームの更新
     * @param deltaTime フレーム間の経過時間
     */
    void Update(float deltaTime) override
    {
        if (m_routine)
        {
            m_routine->OnUpdate(this, deltaTime);
        }

        // 子ウィンドウも更新
        for (auto &child : m_children)
        {
            child->Update(deltaTime);
        }
    }

private:
    std::shared_ptr<TRoutine> m_routine;
    std::shared_ptr<IWindowContext> m_context;
    IApplication *m_application;
    HWND m_handle;
    std::shared_ptr<IWindow> m_parent;
    std::vector<std::shared_ptr<IWindow>> m_children;
};
