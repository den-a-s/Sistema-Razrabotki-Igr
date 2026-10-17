#include <editor/viewport_widget.hpp>

#include <QResizeEvent>

#include <cmath>

namespace editor {

ViewportWidget::ViewportWidget(QWidget* parent) : QWidget(parent) {
    // A real native child window, so the renderer can create a swap chain on it.
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(64, 64);
}

engine::NativeSurface ViewportWidget::surface() {
    const QSize size = pixelSize();
    return engine::NativeSurface{
        .windowHandle = reinterpret_cast<void*>(winId()),
        .width = static_cast<std::uint32_t>(size.width()),
        .height = static_cast<std::uint32_t>(size.height()),
    };
}

QPaintEngine* ViewportWidget::paintEngine() const {
    return nullptr;
}

void ViewportWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    const QSize size = pixelSize();
    emit surfaceResized(static_cast<quint32>(size.width()), static_cast<quint32>(size.height()));
}

QSize ViewportWidget::pixelSize() const {
    const qreal ratio = devicePixelRatioF();
    return {static_cast<int>(std::lround(width() * ratio)), static_cast<int>(std::lround(height() * ratio))};
}

}  // namespace editor
