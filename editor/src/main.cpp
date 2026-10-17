#include <QApplication>

#include <editor/main_window.hpp>

int main(int argc, char* argv[]) {
    // The viewport is a native window; without this Qt makes all of its siblings native
    // too, which the docking system's splitters and drag overlays handle badly.
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("PEN_I_SOSNA"));
    QCoreApplication::setApplicationName(QStringLiteral("SRI Editor"));

    editor::MainWindow window;
    window.show();

    return QApplication::exec();
}
