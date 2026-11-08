#include "client/renderer/RenderHelper.h"

#include "client/renderer/DxStateManager.h"
#include "render/gl/GL.h"
#include "util/Vec3.h"

namespace mc {

namespace {

const Vec3& light0Pos()
{
    static const Vec3 pos = Vec3(0.20000000298023224, 1.0, -0.699999988079071).normalize();
    return pos;
}

const Vec3& light1Pos()
{
    static const Vec3 pos = Vec3(-0.20000000298023224, 1.0, 0.699999988079071).normalize();
    return pos;
}

void setLight(int light, int pname, float a, float b, float c, float d)
{
    const float values[4] { a, b, c, d };
    GL::glLightfv(light, pname, values);
}

} // namespace

void RenderHelper::disableStandardItemLighting()
{
    DxStateManager::disableLighting();
    DxStateManager::disableLight(0);
    DxStateManager::disableLight(1);
    DxStateManager::disableColorMaterial();
}

void RenderHelper::enableStandardItemLighting()
{
    DxStateManager::enableLighting();
    DxStateManager::enableLight(0);
    DxStateManager::enableLight(1);
    DxStateManager::enableColorMaterial();
    DxStateManager::colorMaterial(1032, 5634);
    const float f = 0.4f;
    const float f1 = 0.6f;
    const float f2 = 0.0f;
    const Vec3& l0 = light0Pos();
    const Vec3& l1 = light1Pos();
    setLight(GL::GL_LIGHT0, GL::GL_POSITION, static_cast<float>(l0.xCoord), static_cast<float>(l0.yCoord), static_cast<float>(l0.zCoord), 0.0f);
    setLight(GL::GL_LIGHT0, GL::GL_DIFFUSE, f1, f1, f1, 1.0f);
    setLight(GL::GL_LIGHT0, GL::GL_AMBIENT, 0.0f, 0.0f, 0.0f, 1.0f);
    setLight(GL::GL_LIGHT0, GL::GL_SPECULAR, f2, f2, f2, 1.0f);
    setLight(GL::GL_LIGHT1, GL::GL_POSITION, static_cast<float>(l1.xCoord), static_cast<float>(l1.yCoord), static_cast<float>(l1.zCoord), 0.0f);
    setLight(GL::GL_LIGHT1, GL::GL_DIFFUSE, f1, f1, f1, 1.0f);
    setLight(GL::GL_LIGHT1, GL::GL_AMBIENT, 0.0f, 0.0f, 0.0f, 1.0f);
    setLight(GL::GL_LIGHT1, GL::GL_SPECULAR, f2, f2, f2, 1.0f);
    DxStateManager::shadeModel(7424);
    const float ambient[4] { f, f, f, 1.0f };
    GL::glLightModelfv(GL::GL_LIGHT_MODEL_AMBIENT, ambient);
}

void RenderHelper::enableGUIStandardItemLighting()
{
    DxStateManager::pushMatrix();
    DxStateManager::rotate(-30.0f, 0.0f, 1.0f, 0.0f);
    DxStateManager::rotate(165.0f, 1.0f, 0.0f, 0.0f);
    enableStandardItemLighting();
    DxStateManager::popMatrix();
}

} // namespace mc
