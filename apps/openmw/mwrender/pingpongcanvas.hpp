#ifndef OPENMW_MWRENDER_PINGPONGCANVAS_H
#define OPENMW_MWRENDER_PINGPONGCANVAS_H

#include <array>

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

        void dirty(unsigned int frame) { mBufferData[frame % 2].dirty = true; }

        const fx::DispatchArray& getCurrentFrameData(unsigned int frame) { return mBufferData[frame % 2].data; }

        void setCurrentFrameData(unsigned int frame, fx::DispatchArray&& data);

        void setMask(unsigned int frame, bool underwater, bool exterior);

        void setFallbackFbo(unsigned int frame, osg::ref_ptr<osg::FrameBufferObject> fbo) { mBufferData[frame % 2].fallbackFbo = fbo; }

        void setSceneTextures(osg::ref_ptr<osg::Texture2D> texture, osg::ref_ptr<osg::Texture2D> textureLDR) { mSceneTex = texture; mSceneTexLDR = textureLDR;}

        void setDepthTexture(osg::ref_ptr<osg::Texture2D> texture) { mDepthTex = texture; }

        void setHDR(bool hdr) { mHDR = hdr; }

        size_t getMaxMipmapLevel() const { return mMaxMipMapLevel; }

        const osg::ref_ptr<osg::Texture2D>& getSceneTexture() const { return mSceneTex; }

        void drawGeometry(osg::RenderInfo& renderInfo) const;

    private:
        void copyNewFrameData(unsigned int frameId) const;

        mutable bool mLoggedErrorLastFrame;
        const bool mUsePostProcessing;

        bool mHDR;
        HDRDriver mHDRDriver;

        mutable size_t mMaxMipMapLevel;

        osg::ref_ptr<osg::Program> mFallbackProgram;

        osg::ref_ptr<osg::Texture2D> mSceneTex;
        osg::ref_ptr<osg::Texture2D> mSceneTexLDR;
        osg::ref_ptr<osg::Texture2D> mDepthTex;

        struct BufferData
        {
            bool dirty = false;
            std::optional<fx::DispatchArray> nextFrameData = std::nullopt;
            fx::DispatchArray data;

            fx::FlagsType mask;

            osg::ref_ptr<osg::FrameBufferObject> destination;
            osg::ref_ptr<osg::FrameBufferObject> fallbackFbo;
        };

        mutable std::array<BufferData, 2> mBufferData;
        mutable std::array<osg::ref_ptr<osg::FrameBufferObject>, 3> mFbos;
    };
}

#endif
