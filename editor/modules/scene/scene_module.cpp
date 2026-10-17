#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>

#include <cstdint>
#include <string>
#include <string_view>

#include <engine/assets/AssetSystem.hpp>
#include <engine/engine.hpp>
#include <engine/rendering/RenderComponents.hpp>

#include <editor/editor_context.hpp>
#include <editor/editor_module.hpp>

namespace {

/// Lists the scene's entities as a tree, rebuilt whenever the scene's revision changes.
class HierarchyPanel : public QTreeWidget {
public:
    explicit HierarchyPanel(engine::Scene& scene) : scene_(scene) {
        setHeaderHidden(true);
        // Scene has no change signal, only a revision counter: poll it.
        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &HierarchyPanel::refresh);
        timer->start(200);
        refresh();
    }

    void refresh() {
        if (scene_.GetRevision() == shown_revision_) {
            return;
        }
        shown_revision_ = scene_.GetRevision();
        clear();
        for (const entt::entity root : scene_.GetRootEntities()) {
            addTopLevelItem(makeItem(root));
        }
        expandAll();
    }

private:
    QTreeWidgetItem* makeItem(const entt::entity entity) {
        auto* item = new QTreeWidgetItem({QString::fromStdString(scene_.GetName(entity))});
        for (const entt::entity child : scene_.GetChildren(entity)) {
            item->addChild(makeItem(child));
        }
        return item;
    }

    engine::Scene& scene_;
    std::uint64_t shown_revision_ = ~std::uint64_t{0};
};

class SceneModule final : public editor::IEditorModule {
public:
    [[nodiscard]] std::string_view name() const override {
        return "Scene";
    }

    void startup(editor::EditorContext& context) override {
        context_ = &context;
        hierarchy_ = new HierarchyPanel(context.scene());
        context.addPanel(QStringLiteral("HierarchyPanel"), QObject::tr("Hierarchy"), hierarchy_, Qt::LeftDockWidgetArea);

        auto* add_cube = context.menu(QObject::tr("&Scene"))->addAction(QObject::tr("Add &Cube"));
        context.toolBar()->addAction(add_cube);
        QObject::connect(add_cube, &QAction::triggered, hierarchy_, [this] { addCube(); });
    }

private:
    void addCube() {
        engine::Scene& scene = context_->scene();
        const int index = cube_count_++;
        const entt::entity cube = scene.CreateEntity("Cube " + std::to_string(index + 1));
        // Rows of five in front of the camera, so new cubes don't overlap.
        scene.GetTransform(cube).position = DirectX::SimpleMath::Vector3{
            static_cast<float>(index % 5 - 2) * 1.5f, 0.5f, -static_cast<float>(index / 5) * 1.5f};
        // Asynchronous: the cube shows up once the mesh is loaded (immediately after the first one).
        scene.GetRegistry().emplace<engine::MeshRenderer>(
            cube, context_->engine().GetAssets().RequestMesh(engine::BuiltinAssets::CubeMesh));
        hierarchy_->refresh();
    }

    editor::EditorContext* context_ = nullptr;
    HierarchyPanel* hierarchy_ = nullptr;  // owned by its dock
    int cube_count_ = 0;
};

}  // namespace

SRI_EDITOR_MODULE(SceneModule)
