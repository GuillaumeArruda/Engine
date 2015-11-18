#pragma once
#include "RenderTree\Node\CompositeNode.h"
#include "Utility\3D\Model3D.h"
#include "RenderTree\Node\Skybox.h"
class RenderTree 
    : public CompositeNode
{
public:
    RenderTree(AbstractNode* parent);
    virtual ~RenderTree();

    virtual void Update(double deltaT);
    virtual void Render(glm::mat4 model, const glm::mat4& view, const glm::mat4& projection, Environment* environnement, const glm::vec4& clipPlane = glm::vec4(0));
    virtual void RenderFirstPass(glm::mat4 model, const glm::mat4& view, const glm::mat4& projection);
    virtual void RenderShadowMap(glm::mat4 model, const glm::mat4& view, const glm::mat4& projection);
    virtual void RenderReflection(glm::mat4 model, const glm::mat4& view, const glm::mat4& projection, Environment* environnement, const glm::vec4& clipPlane);

private:
    Skybox* skybox_;
};

