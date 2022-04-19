#include "stateupdater.hpp"

#include <osg/BufferObject>
#include <osg/BufferIndexBinding>

#include <components/resource/scenemanager.hpp>

namespace fx
{
    StateUpdater::StateUpdater(bool useUBO) : mUseUBO(useUBO) {}

    void StateUpdater::setDefaults(osg::StateSet* stateset)
    {
        if (mUseUBO)
        {
            osg::ref_ptr<osg::UniformBufferObject> ubo = new osg::UniformBufferObject;

            osg::ref_ptr<osg::BufferTemplate<UniformData::BufferType>> data = new osg::BufferTemplate<UniformData::BufferType>();
            data->setBufferObject(ubo);

            osg::ref_ptr<osg::UniformBufferBinding> ubb = new osg::UniformBufferBinding(static_cast<int>(Resource::SceneManager::UBOBinding::PostProcessor), data, 0, mData.getGPUSize());

            stateset->setAttributeAndModes(ubb, osg::StateAttribute::ON);
        }
        else
        {
            stateset->addUniform(new osg::Uniform("omw.projectionMatrix", mData.get<ProjectionMatrix>()));
            stateset->addUniform(new osg::Uniform("omw.invProjectionMatrix", mData.get<InvProjectionMatrix>()));
            stateset->addUniform(new osg::Uniform("omw.viewMatrix", mData.get<ViewMatrix>()));
            stateset->addUniform(new osg::Uniform("omw.prevViewMatrix", mData.get<PrevViewMatrix>()));
            stateset->addUniform(new osg::Uniform("omw.invViewMatrix", mData.get<InvViewMatrix>()));
            stateset->addUniform(new osg::Uniform("omw.eyePos", mData.get<EyePos>()));
            stateset->addUniform(new osg::Uniform("omw.eyeVec", mData.get<EyeVec>()));
            stateset->addUniform(new osg::Uniform("omw.fogColor", mData.get<FogColor>()));
            stateset->addUniform(new osg::Uniform("omw.sunColor", mData.get<SunColor>()));
            stateset->addUniform(new osg::Uniform("omw.sunPos", mData.get<SunPos>()));
            stateset->addUniform(new osg::Uniform("omw.resolution", mData.get<Resolution>()));
            stateset->addUniform(new osg::Uniform("omw.rcpResolution", mData.get<RcpResolution>()));
            stateset->addUniform(new osg::Uniform("omw.fogNear", mData.get<FogNear>()));
            stateset->addUniform(new osg::Uniform("omw.fogFar", mData.get<FogFar>()));
            stateset->addUniform(new osg::Uniform("omw.near", mData.get<Near>()));
            stateset->addUniform(new osg::Uniform("omw.far", mData.get<Far>()));
            stateset->addUniform(new osg::Uniform("omw.fov", mData.get<Fov>()));
            stateset->addUniform(new osg::Uniform("omw.gameHour", mData.get<GameHour>()));
            stateset->addUniform(new osg::Uniform("omw.sunVis", mData.get<SunVis>()));
            stateset->addUniform(new osg::Uniform("omw.waterHeight", mData.get<WaterHeight>()));
            stateset->addUniform(new osg::Uniform("omw.isUnderwater", mData.get<IsUnderwater>()));
            stateset->addUniform(new osg::Uniform("omw.isInterior", mData.get<IsInterior>()));
            stateset->addUniform(new osg::Uniform("omw.simulationTime", mData.get<SimulationTime>()));
            stateset->addUniform(new osg::Uniform("omw.deltaSimulationTime", mData.get<DeltaSimulationTime>()));
        }
    }

    void StateUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        if (mUseUBO)
        {
            osg::UniformBufferBinding* ubb = dynamic_cast<osg::UniformBufferBinding*>(stateset->getAttribute(osg::StateAttribute::UNIFORMBUFFERBINDING, static_cast<int>(Resource::SceneManager::UBOBinding::PostProcessor)));

            auto& dest = static_cast<osg::BufferTemplate<UniformData::BufferType>*>(ubb->getBufferData())->getData();
            mData.copyTo(dest);

            ubb->getBufferData()->dirty();
        }
        else
        {
            stateset->getUniform("omw.projectionMatrix")->set(mData.get<ProjectionMatrix>());
            stateset->getUniform("omw.invProjectionMatrix")->set(mData.get<InvProjectionMatrix>());
            stateset->getUniform("omw.viewMatrix")->set(mData.get<ViewMatrix>());
            stateset->getUniform("omw.prevViewMatrix")->set(mData.get<PrevViewMatrix>());
            stateset->getUniform("omw.invViewMatrix")->set(mData.get<ViewMatrix>());
            stateset->getUniform("omw.eyePos")->set(mData.get<EyePos>());
            stateset->getUniform("omw.eyeVec")->set(mData.get<EyeVec>());
            stateset->getUniform("omw.fogColor")->set(mData.get<FogColor>());
            stateset->getUniform("omw.sunColor")->set(mData.get<SunColor>());
            stateset->getUniform("omw.sunPos")->set(mData.get<SunPos>());
            stateset->getUniform("omw.resolution")->set(mData.get<Resolution>());
            stateset->getUniform("omw.rcpResolution")->set(mData.get<RcpResolution>());
            stateset->getUniform("omw.fogNear")->set(mData.get<FogNear>());
            stateset->getUniform("omw.fogFar")->set(mData.get<FogFar>());
            stateset->getUniform("omw.near")->set(mData.get<Near>());
            stateset->getUniform("omw.far")->set(mData.get<Far>());
            stateset->getUniform("omw.fov")->set(mData.get<Fov>());
            stateset->getUniform("omw.gameHour")->set(mData.get<GameHour>());
            stateset->getUniform("omw.sunVis")->set(mData.get<SunVis>());
            stateset->getUniform("omw.waterHeight")->set(mData.get<WaterHeight>());
            stateset->getUniform("omw.isUnderwater")->set(mData.get<IsUnderwater>());
            stateset->getUniform("omw.isInterior")->set(mData.get<IsInterior>());
            stateset->getUniform("omw.simulationTime")->set(mData.get<SimulationTime>());
            stateset->getUniform("omw.deltaSimulationTime")->set(mData.get<DeltaSimulationTime>());
        }
    }
}