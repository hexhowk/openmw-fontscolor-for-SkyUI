#include "pingpongcull.hpp"

#include <array>

#include <osg/Camera>
#include <osg/FrameBufferObject>
#include <osgUtil/CullVisitor>

#include <components/fx/technique.hpp>
#include <components/fx/pass.hpp>

#include <components/settings/shadermanager.hpp>
#include <components/settings/settings.hpp>
#include <components/debug/debuglog.hpp>

#include "postprocessor.hpp"
#include "pingpongcanvas.hpp"

namespace MWRender
{
    PingPongCull::PingPongCull()
        : mLastFrameNumber(0)
        , mLastSimulationTime(0.f)
    { }

    void PingPongCull::operator()(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        osgUtil::RenderStage* renderStage = cv->getCurrentRenderStage();

        size_t frame = cv->getTraversalNumber();
        size_t frameId = frame % 2;

        MWRender::PostProcessor* postProcessor = dynamic_cast<MWRender::PostProcessor*>(cv->getCurrentCamera()->getUserData());

        if (!postProcessor)
        {
            renderStage->setMultisampleResolveFramebufferObject(nullptr);
            renderStage->setFrameBufferObject(nullptr);
            traverse(node, cv);
            return;
        }

        if (!postProcessor->getFbo(PostProcessor::FBO_Multisample, frameId))
        {
            renderStage->setFrameBufferObject(postProcessor->getFbo(PostProcessor::FBO_Primary, frameId));
        }
        else
        {
            renderStage->setMultisampleResolveFramebufferObject(postProcessor->getFbo(PostProcessor::FBO_Primary, frameId));
            renderStage->setFrameBufferObject(postProcessor->getFbo(PostProcessor::FBO_Multisample, frameId));
        }

        if (!postProcessor->isEnabled())
        {
            traverse(node, cv);
            return;
        }

        // per-view data
        postProcessor->getStateUpdater()->setViewMatrix(cv->getCurrentCamera()->getViewMatrix());
        postProcessor->getStateUpdater()->setInvViewMatrix(cv->getCurrentCamera()->getInverseViewMatrix());
        postProcessor->getStateUpdater()->setPrevViewMatrix(mLastViewMatrix);
        mLastViewMatrix = cv->getCurrentCamera()->getViewMatrix();
        postProcessor->getStateUpdater()->setEyePos(cv->getEyePoint());
        postProcessor->getStateUpdater()->setResolution(osg::Vec2f(cv->getViewport()->width(), cv->getViewport()->height()));
        postProcessor->getStateUpdater()->setEyeVec(cv->getLookVectorLocal());

        // per-frame data
        if (frame != mLastFrameNumber)
        {
            mLastFrameNumber = frame;

            auto stamp = cv->getFrameStamp();

            postProcessor->getStateUpdater()->setSimulationTime(static_cast<float>(stamp->getSimulationTime()));
            postProcessor->getStateUpdater()->setDeltaSimulationTime(static_cast<float>(stamp->getSimulationTime() - mLastSimulationTime));
            mLastSimulationTime = stamp->getSimulationTime();

            for (const auto& dispatchNode : postProcessor->getCanvas()->getCurrentFrameData(frame))
            {
                for (auto& uniform : dispatchNode.mHandle->getUniformMap())
                {
                    if (uniform->getType().has_value() && !uniform->mSamplerType)
                        if (auto* u = dispatchNode.mRootStateSet->getUniform(uniform->mName))
                            uniform->setUniform(u);
                }
            }
        }

        traverse(node, cv);
    }
}
