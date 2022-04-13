#ifndef OPENMW_MWRENDER_TRANSPARENTPASS_H
#define OPENMW_MWRENDER_TRANSPARENTPASS_H

#include <array>

#include <osg/FrameBufferObject>
#include <osg/StateSet>

#include <osgUtil/RenderBin>

#include "postprocessor.hpp"

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class TransparentDepthBinCallback : public osgUtil::RenderBin::DrawCallback
    {
    public:
        TransparentDepthBinCallback(Shader::ShaderManager& shaderManager, const std::array<PostProcessor::FBOArray, 2>& fbos, bool postPass);

        void drawImplementation(osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override;

    private:
        std::array<PostProcessor::FBOArray, 2> mFbos;
        osg::ref_ptr<osg::StateSet> mStateSet;
        bool mPostPass;
    };

}

#endif