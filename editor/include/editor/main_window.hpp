#pragma once

#include <QMainWindow>

#include <memory>

#include <engine/engine.hpp>

namespace ads {
class CDockManager;
}

class QLabel;
class QPlainTextEdit;
class QTimer;

namespace spdlog::sinks {
class sink;
}

namespace editor {

class EditorContext;
class ModuleLoader;
class ViewportWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    /// Saves the window geometry and dock layout.
    void closeEvent(QCloseEvent* event) override;

private slots:
    void startEngine();
    void tickEngine();

private:
    void buildMenus();
    void buildLogDock();
    void setupDefaultScene();
    void restoreLayout();
    void refreshStatus();
    void appendLog(const QString& level, const QString& text);

    // Destroyed bottom-up: modules, then the context (and its panels), then the engine.
    engine::Engine engine_;
    std::unique_ptr<EditorContext> context_;
    std::unique_ptr<ModuleLoader> modules_;
    std::shared_ptr<spdlog::sinks::sink> log_sink_;

    ads::CDockManager* dock_manager_ = nullptr;
    ViewportWidget* viewport_ = nullptr;
    QPlainTextEdit* log_view_ = nullptr;
    QLabel* status_label_ = nullptr;
    QTimer* tick_timer_ = nullptr;
};

}  // namespace editor
