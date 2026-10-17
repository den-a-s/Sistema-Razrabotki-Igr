#pragma once

#include <string_view>

namespace editor {

class EditorContext;

/// We want editor to load dll modules to speed up iteration and builds, so yeah...
class IEditorModule {
public:
    virtual ~IEditorModule() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    virtual void startup(EditorContext& context) = 0;
    /// Undo whatever startup() hooked up outside the module's own widgets.
    virtual void shutdown() {}
};

/// Exported function the editor looks up in every module DLL.
/// needs to be the same with the macro below
inline constexpr const char* ModuleEntryPoint = "sri_create_editor_module";

}  // namespace editor

#define SRI_EDITOR_MODULE(Type)                                                      \
    extern "C" Q_DECL_EXPORT ::editor::IEditorModule* sri_create_editor_module() { \
        return new Type();                                                           \
    }
