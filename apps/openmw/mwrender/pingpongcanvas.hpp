#ifndef OPENMW_MWRENDER_PINGPONGCANVAS_H
#define OPENMW_MWRENDER_PINGPONGCANVAS_H

#include <array>
#include <optional>

#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <components/fx/technique.hpp>

#include "postprocessor.hpp"
#include "hdr.hpp"

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class PingPongCanvas : public osg::Geometry
    {
    public:
        PingPongCanvas(bool usePostProcessing, Shader::ShaderManager& shaderManager);

        void drawImplementation(osg::RenderInfo& renderInfo) const override;

        void dirty(size_t frameId) { mBufferData[frameId].dirty = true; }

        const fx::DispatchArray& getCurrentFrameData(size_t frame) { return mBufferData[frame % 2].data; }

        // Sets current frame pass data and stores copy of dispatch array to apply to next frame data
        void setCurrentFrameData(size_t frameId, fx::DispatchArray&& data);

        void setMask(size_t frameId, bool underwater, bool exterior);

        void setFallbackFbo(size_t frameId, osg::ref_ptr<osg::FrameBufferObject> fbo) { mBufferData[frameId].fallbackFbo = fbo; }

        void setSceneTexture(size_t frameId, osg::ref_ptr<osg::Texture2D> tex) { mBufferData[frameId].sceneTex = tex; }

        void setLDRSceneTexture(size_t frameId, osg::ref_ptr<osg::Texture2D> tex) { mBufferData[frameId].sceneTexLDR = tex; }

        void setDepthTexture(size_t frameId, osg::ref_ptr<osg::Texture2D> tex) { mBufferData[frameId].depthTex = tex; }

        void setHDR(size_t frameId, bool hdr) { mBufferData[frameId].hdr = hdr; }

        const osg::ref_ptr<osg::Texture2D>& getSceneTexture(size_t frameId) const { return mBufferData[frameId].sceneTex; }

        void drawGeometry(osg::RenderInfo& renderInfo) const;

    private:
        void copyNewFrameData(size_t frameId) const;

        mutable bool mLoggedErrorLastFrame;
        const bool mUsePostProcessing;

        mutable HDRDriver mHDRDriver;

        osg::ref_ptr<osg::Program> mFallbackProgram;

        struct BufferData
        {
            bool dirty = false;
            bool hdr = false;

            fx::DispatchArray data;
            fx::FlagsType mask;

            osg::ref_ptr<osg::FrameBufferObject> destination;
            osg::ref_ptr<osg::FrameBufferObject> fallbackFbo;

            osg::ref_ptr<osg::Texture2D> sceneTex;
            osg::ref_ptr<osg::Texture2D> sceneTexLDR;
            osg::ref_ptr<osg::Texture2D> depthTex;
        };

        mutable std::array<BufferData, 2> mBufferData;
        mutable std::array<osg::ref_ptr<osg::FrameBufferObject>, 3> mFbos;

        mutable std::optional<fx::DispatchArray> mQueuedDispatchArray;
        mutable size_t mQueuedDispatchFrameId;
    };
}

#endif
