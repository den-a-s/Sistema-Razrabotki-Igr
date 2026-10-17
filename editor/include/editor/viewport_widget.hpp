#pragma once

#include <QWidget>

#include <engine/core/Surface.hpp>

namespace editor {

/// The area the engine renders into. Owns a native window (HWND) that is
/// handed to the engine as its NativeSurface; Qt never paints it
class ViewportWidget : public QWidget {
    Q_OBJECT

public:
    explicit ViewportWidget(QWidget* parent = nullptr);

    [[nodiscard]] engine::NativeSurface surface();

    /// No Qt painting at all, so it can't draw over the swap chain.
    [[nodiscard]] QPaintEngine* paintEngine() const override;

signals:
    /// Size in physical pixels.
    void surfaceResized(quint32 width, quint32 height);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    [[nodiscard]] QSize pixelSize() const;
};

}  // namespace editor
