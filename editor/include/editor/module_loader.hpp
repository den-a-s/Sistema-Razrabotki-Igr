#pragma once

#include <QString>

#include <memory>
#include <vector>

class QLibrary;

namespace editor {

class EditorContext;
class IEditorModule;

/// Editor parts are modules for faster iteration
class ModuleLoader {
public:
    ModuleLoader();
    ~ModuleLoader();

    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader& operator=(const ModuleLoader&) = delete;

    /// A DLL that fails to load or has no entry point is logged and skipped.
    void loadAll(const QString& directory, EditorContext& context);

private:
    struct Loaded {
        std::unique_ptr<QLibrary> library;
        std::unique_ptr<IEditorModule> module;
    };
    std::vector<Loaded> modules_;
};

}  // namespace editor
