#include "stateupdater.hpp"

#include <osg/BufferObject>
#include <osg/BufferIndexBinding>

#include <components/resource/scenemanager.hpp>

#include <cassert>

namespace fx
{
    StateUpdater::StateUpdater(bool useUBO) : mUseUBO(useUBO) {}

    void StateUpdater::setDefaults(osg::StateSet* stateset)
    {
        if (mUseUBO)
        {
            static_assert(std::is_standard_layout<UniformData>::value);

            osg::ref_ptr<osg::UniformBufferObject> ubo = new osg::UniformBufferObject;

            osg::ref_ptr<UBOData> data = new osg::BufferTemplate<UniformData>();
            data->setBufferObject(ubo);

            osg::ref_ptr<osg::UniformBufferBinding> ubb = new osg::UniformBufferBinding(static_cast<int>(Resource::SceneManager::UBOBinding::PostProcessor), data, 0, sizeof(UniformData));

            stateset->setAttributeAndModes(ubb, osg::StateAttribute::ON);
        }
        else
        {
            stateset->addUniform(new osg::Uniform("omw.projectionMatrix", mData.projectionMatrix));
            stateset->addUniform(new osg::Uniform("omw.invProjectionMatrix", mData.invProjectionMatrix));
            stateset->addUniform(new osg::Uniform("omw.viewMatrix", mData.viewMatrix));
            stateset->addUniform(new osg::Uniform("omw.prevViewMatrix", mData.prevViewMatrix));
            stateset->addUniform(new osg::Uniform("omw.invViewMatrix", mData.invViewMatrix));
            stateset->addUniform(new osg::Uniform("omw.eyePos", mData.eyePos));
            stateset->addUniform(new osg::Uniform("omw.eyeVec", mData.eyeVec));
            stateset->addUniform(new osg::Uniform("omw.fogColor", mData.fogColor));
            stateset->addUniform(new osg::Uniform("omw.sunColor", mData.sunColor));
            stateset->addUniform(new osg::Uniform("omw.sunPos", mData.sunPos));
            stateset->addUniform(new osg::Uniform("omw.resolution", mData.resolution));
            stateset->addUniform(new osg::Uniform("omw.rcpResolution", mData.rcpResolution));
            stateset->addUniform(new osg::Uniform("omw.fogNear", mData.fogNear));
            stateset->addUniform(new osg::Uniform("omw.fogFar", mData.fogFar));
            stateset->addUniform(new osg::Uniform("omw.near", mData.near));
            stateset->addUniform(new osg::Uniform("omw.far", mData.far));
            stateset->addUniform(new osg::Uniform("omw.fov", mData.fov));
            stateset->addUniform(new osg::Uniform("omw.gameHour", mData.gameHour));
            stateset->addUniform(new osg::Uniform("omw.sunVis", mData.sunVis));
            stateset->addUniform(new osg::Uniform("omw.waterHeight", mData.waterHeight));
            stateset->addUniform(new osg::Uniform("omw.isUnderwater", mData.isUnderwater));
            stateset->addUniform(new osg::Uniform("omw.isInterior", mData.isInterior));
            stateset->addUniform(new osg::Uniform("omw.simulationTime", mData.simulationTime));
            stateset->addUniform(new osg::Uniform("omw.deltaSimulationTime", mData.deltaSimulationTime));
        }
    }

    void StateUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        if (mUseUBO)
        {
            osg::UniformBufferBinding* ubb = dynamic_cast<osg::UniformBufferBinding*>(stateset->getAttribute(osg::StateAttribute::UNIFORMBUFFERBINDING, static_cast<int>(Resource::SceneManager::UBOBinding::PostProcessor)));
            UniformData& data = static_cast<UBOData*>(ubb->getBufferData())->getData();
            data = mData;

            ubb->getBufferData()->dirty();
        }
        else
        {
            stateset->getUniform("omw.projectionMatrix")->set(mData.projectionMatrix);
            stateset->getUniform("omw.invProjectionMatrix")->set(mData.invProjectionMatrix);
            stateset->getUniform("omw.viewMatrix")->set(mData.viewMatrix);
            stateset->getUniform("omw.prevViewMatrix")->set(mData.prevViewMatrix);
            stateset->getUniform("omw.invViewMatrix")->set(mData.viewMatrix);
            stateset->getUniform("omw.eyePos")->set(mData.eyePos);
            stateset->getUniform("omw.eyeVec")->set(mData.eyeVec);
            stateset->getUniform("omw.fogColor")->set(mData.fogColor);
            stateset->getUniform("omw.sunColor")->set(mData.sunColor);
            stateset->getUniform("omw.sunPos")->set(mData.sunPos);
            stateset->getUniform("omw.resolution")->set(mData.resolution);
            stateset->getUniform("omw.rcpResolution")->set(mData.rcpResolution);
            stateset->getUniform("omw.fogNear")->set(mData.fogNear);
            stateset->getUniform("omw.fogFar")->set(mData.fogFar);
            stateset->getUniform("omw.near")->set(mData.near);
            stateset->getUniform("omw.far")->set(mData.far);
            stateset->getUniform("omw.fov")->set(mData.fov);
            stateset->getUniform("omw.gameHour")->set(mData.gameHour);
            stateset->getUniform("omw.sunVis")->set(mData.sunVis);
            stateset->getUniform("omw.waterHeight")->set(mData.waterHeight);
            stateset->getUniform("omw.isUnderwater")->set(mData.isUnderwater);
            stateset->getUniform("omw.isInterior")->set(mData.isInterior);
            stateset->getUniform("omw.simulationTime")->set(mData.simulationTime);
            stateset->getUniform("omw.deltaSimulationTime")->set(mData.deltaSimulationTime);
        }
    }
}