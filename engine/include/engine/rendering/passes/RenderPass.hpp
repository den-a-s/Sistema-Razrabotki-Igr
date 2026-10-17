#pragma once

#include <engine/engine_export.hpp>

#include <engine/rendering/RenderingResources.hpp>
#include <engine/rendering/RenderWorld.hpp>


namespace engine {

struct ENGINE_API RenderPass {
    
    virtual ~RenderPass() = default;

    virtual void Render(RenderWorld & world, RenderingContext & context, RenderingResources & resources) = 0;
    
    virtual void Initialize(RenderingResourceManager & resourceManager, RenderingContext & context, RenderingResources & resources) = 0;
};


}
