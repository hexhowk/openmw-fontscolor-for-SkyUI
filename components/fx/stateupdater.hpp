#ifndef OPENMW_COMPONENTS_FX_STATEUPDATER_H
#define OPENMW_COMPONENTS_FX_STATEUPDATER_H

#include <osg/Vec3f>
#include <osg/Vec2f>
#include <osg/Vec4f>
#include <osg/Matrixf>
#include <osg/BufferTemplate>

#include <cstdint>

#include <components/sceneutil/statesetupdater.hpp>

namespace fx
{
    using std140_mat4 = osg::Matrixf;
    using std140_vec4 = osg::Vec4f;
    using std140_vec2 = osg::Vec2f;
    using std140_float = float;
    using std140_int = std::int32_t;
    using std140_bool = std::int32_t;

    // Warning: This follows the strict std140 layout. Do not edit this structure unless you are familiar with the spec or you will certainly break things!
    // This must match the layout we pass to shaders in pass.cpp
    struct UniformData
    {
        std140_mat4 projectionMatrix;
        std140_mat4 invProjectionMatrix;
        std140_mat4 viewMatrix;
        std140_mat4 prevViewMatrix;
        std140_mat4 invViewMatrix;
        std140_vec4 eyePos;
        std140_vec4 eyeVec;
        std140_vec4 fogColor;
        std140_vec4 sunColor;
        std140_vec4 sunPos;
        std140_vec2 resolution;
        std140_vec2 rcpResolution;
        std140_float fogNear;
        std140_float fogFar;
        std140_float near;
        std140_float far;
        std140_float fov;
        std140_float gameHour;
        std140_float sunVis;
        std140_float waterHeight;
        std140_bool isUnderwater;
        std140_bool isInterior;
        std140_float simulationTime;
        std140_float deltaSimulationTime;
    };

    class StateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        StateUpdater(bool useUBO);

        void setProjectionMatrix(const osg::Matrixf& matrix)
        {
            mData.projectionMatrix = matrix;
            mData.invProjectionMatrix = osg::Matrixf::inverse(matrix);
        }

        void setViewMatrix(const osg::Matrixf& matrix) { mData.viewMatrix = matrix; }

        void setInvViewMatrix(const osg::Matrixf& matrix) { mData.invViewMatrix = matrix; }

        void setPrevViewMatrix(const osg::Matrixf& matrix) { mData.prevViewMatrix = matrix;}

        void setEyePos(const osg::Vec3f& pos) { mData.eyePos = osg::Vec4f(pos, 0.f); }

        void setEyeVec(const osg::Vec3f& vec) { mData.eyeVec = osg::Vec4f(vec, 0.f); }

        void setFogColor(const osg::Vec4f& color) { mData.fogColor = color; }

        void setSunColor(const osg::Vec4f& color) { mData.sunColor = color; }

        void setSunPos(const osg::Vec4f& pos) { mData.sunPos = pos; }

        void setResolution(const osg::Vec2f& size) { mData.resolution = size; mData.rcpResolution = {1.f / size.x(), 1.f / size.y()}; }

        void setSunVis(float vis) { mData.sunVis = vis; if (vis <= 0.f) mData.sunPos.z() *= -1.f; }

        void setFogRange(float near, float far) { mData.fogNear = near; mData.fogFar = far; }

        void setNearFar(float near, float far) { mData.near = near; mData.far = far;  }

        void setIsUnderwater(bool underwater) { mData.isUnderwater = underwater; }

        void setIsInterior(bool interior) { mData.isInterior = interior; }

        void setFov(float fov) { mData.fov = fov; }

        void setGameHour(float hour) { mData.gameHour = hour / 24.f; }

        void setWaterHeight(float height) { mData.waterHeight = height; }

        void setSimulationTime(float time) { mData.simulationTime = time; }

        void setDeltaSimulationTime(float time) { mData.deltaSimulationTime = time; }

    private:
        using UBOData = osg::BufferTemplate<UniformData>;

        void setDefaults(osg::StateSet* stateset) override;
        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

        UniformData mData;

        bool mUseUBO;
    };
}

#endif