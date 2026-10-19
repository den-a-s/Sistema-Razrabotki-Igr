#include <QAction>
#include <QMenu>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <cstdint>
#include <string>
#include <string_view>

#include <engine/assets/AssetSystem.hpp>
#include <engine/engine.hpp>
#include <engine/rendering/RenderComponents.hpp>

#include <editor/editor_context.hpp>
#include <editor/editor_module.hpp>

#include "inspector_panel.hpp"

namespace {

/// Lists the scene's entities as a tree, rebuilt whenever the scene's revision changes.
/// The current item is the selection; each item carries its entity.
class HierarchyPanel : public QTreeWidget {
public:
    static entt::entity entityOf(const QTreeWidgetItem* item) {
        return item != nullptr ? static_cast<entt::entity>(item->data(0, Qt::UserRole).toUInt()) : entt::null;
    }

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
        // Rebuilding isn't a selection change: keep the selected entity selected, quietly.
        const entt::entity selected = entityOf(currentItem());
        QSignalBlocker blocker(this);
        clear();
        for (const entt::entity root : scene_.GetRootEntities()) {
            addTopLevelItem(makeItem(root));
        }
        expandAll();
        if (QTreeWidgetItem* item = findItem(selected)) {
            setCurrentItem(item);
        } else if (selected != entt::null) {
            blocker.unblock();
            emit currentItemChanged(nullptr, nullptr);  // destroyed
        }
    }

    void select(const entt::entity entity) {
        refresh();
        setCurrentItem(findItem(entity));
    }

private:
    QTreeWidgetItem* makeItem(const entt::entity entity) {
        auto* item = new QTreeWidgetItem({QString::fromStdString(scene_.GetName(entity))});
        item->setData(0, Qt::UserRole, entt::to_integral(entity));
        for (const entt::entity child : scene_.GetChildren(entity)) {
            item->addChild(makeItem(child));
        }
        return item;
    }

    QTreeWidgetItem* findItem(const entt::entity entity) const {
        if (entity == entt::null) {
            return nullptr;
        }
        for (QTreeWidgetItemIterator it(const_cast<HierarchyPanel*>(this)); *it != nullptr; ++it) {
            if (entityOf(*it) == entity) {
                return *it;
            }
        }
        return nullptr;
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

        auto* inspector = new InspectorPanel(context.scene());
        context.addPanel(QStringLiteral("InspectorPanel"), QObject::tr("Inspector"), inspector, Qt::RightDockWidgetArea);
        QObject::connect(hierarchy_, &QTreeWidget::currentItemChanged, inspector,
                         [inspector](QTreeWidgetItem* current) { inspector->setEntity(HierarchyPanel::entityOf(current)); });

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
        hierarchy_->select(cube);
    }

    editor::EditorContext* context_ = nullptr;
    HierarchyPanel* hierarchy_ = nullptr;  // owned by its dock
    int cube_count_ = 0;
};

}  // namespace

SRI_EDITOR_MODULE(SceneModule)
