#ifndef OPENMW_MWRENDER_POSTPROCESSOR_H
#define OPENMW_MWRENDER_POSTPROCESSOR_H

#include <array>
#include <vector>
#include <string>
#include <unordered_map>

#include <filesystem>

#include <osg/Texture2D>
#include <osg/Group>
#include <osg/FrameBufferObject>
#include <osg/Camera>

#include <osgViewer/Viewer>

#include <components/fx/stateupdater.hpp>
#include <components/fx/technique.hpp>
#include <components/debug/debuglog.hpp>

#include "pingpongcanvas.hpp"
#include "transparentpass.hpp"

namespace osgViewer
{
    class Viewer;
}

namespace VFS
{
    class Manager;
}

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class RenderingManager;
    class PingPongCull;
    class PingPongCanvas;
    class TransparentDepthBinCallback;

    // We resolve depth early, so we must explicitly set the resolve framebuffer at right time during cull traversals.
    // This is **very** important, depth buffer blits can be incredibly costly with MSAA and floating point formats.
    class ResolveFboInterceptor : public SceneUtil::NodeCallback<ResolveFboInterceptor, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        void operator()(osg::Node* node, osgUtil::CullVisitor* cv);
        void setFbo(size_t frameId, osg::ref_ptr<osg::FrameBufferObject> fbo);

    private:
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mFbos;
    };

    class PostProcessor : public osg::Referenced
    {
    public:
        using FBOArray = std::array<osg::ref_ptr<osg::FrameBufferObject>, 4>;
        using TextureArray = std::array<osg::ref_ptr<osg::Texture2D>, 4>;
        using TechniqueList = std::vector<std::shared_ptr<fx::Technique>>;

        enum TextureIndex
        {
            Tex_Scene,
            Tex_Scene_LDR,
            Tex_Depth,
            Tex_OpaqueDepth
        };

        enum FBOIndex
        {
            FBO_Primary,
            FBO_Multisample,
            FBO_FirstPerson,
            FBO_OpaqueDepth
        };

        enum TextureUnits
        {
            Unit_LastShader = 0,
            Unit_LastPass,
            Unit_Depth,
            Unit_EyeAdaptation,
            Unit_NextFree
        };

        PostProcessor(RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode, const VFS::Manager* vfs);

        ~PostProcessor();

        osg::ref_ptr<osg::FrameBufferObject> getFbo(FBOIndex index, unsigned int frameId) { return mFbos[frameId][index]; }

        osg::ref_ptr<osg::Texture2D> getTexture(TextureIndex index, unsigned int frameId) { return mTextures[frameId][index]; }

        osg::ref_ptr<osg::FrameBufferObject> getPrimaryFbo(unsigned int frameId) { return mFbos[frameId][FBO_Multisample] ? mFbos[frameId][FBO_Multisample] : mFbos[frameId][FBO_Primary]; }

        osg::ref_ptr<fx::StateUpdater> getStateUpdater() { return mStateUpdater; }

        const TechniqueList& getTechniques() { return mTechniques; }

        const TechniqueList& getTemplates() const { return mTemplates; }

        osg::ref_ptr<PingPongCanvas> getCanvas() { return mPingPongCanvas; }

        int omw_GetDepthFormat() { return mDepthFormat; }

        const auto& getTechniqueMap() const { return mTechniqueFileMap; }

        void update();

        void resize(int width, int height, bool resizeAttachments = false);

        bool enableTechnique(std::shared_ptr<fx::Technique> technique, std::optional<int> location = std::nullopt);

        bool disableTechnique(std::shared_ptr<fx::Technique> technique, bool dirty = true);

        template <class T>
        void setUniform(std::shared_ptr<fx::Technique> technique, const std::string& name, const T& value)
        {
            if (!isEnabled())
                return;

            auto it = technique->findUniform(name);

            if (it == technique->getUniformMap().end())
                return;

            if ((*it)->mStatic)
            {
                Log(Debug::Warning) << "Attempting to set a configration variable [" << name << "] as a uniform";
                return;
            }

            (*it)->setValue(value);
        }

        bool isTechniqueEnabled(const std::shared_ptr<fx::Technique>& technique) const;

        void setExteriorFlag(bool exterior) { mExteriorFlag = exterior; }

        void setUnderwaterFlag(bool underwater) { mUnderwater = underwater; }

        void toggleMode();

        std::shared_ptr<fx::Technique> loadTechnique(const std::string& name, bool insert=true);

        void addTemplate(std::shared_ptr<fx::Technique> technique);

        bool isEnabled() const { return mUsePostProcessing && mEnabled; }

        bool softParticlesEnabled() const {return mSoftParticles; }

        bool getHDR() const { return mHDR; }

    private:
        int width() const { return mViewer->getCamera()->getViewport()->width(); }

        int height() const { return mViewer->getCamera()->getViewport()->height(); }

        size_t frame() const { return mViewer->getFrameStamp()->getFrameNumber(); }

        void createObjectsForFrame(size_t frameId, int width, int height);

        void createTexturesAndCamera(int width, int height);

        void reloadTechniques();

        void reloadMainPass(fx::Technique& technique);

        void dirtyTechniques();

        osg::ref_ptr<ResolveFboInterceptor> mResolveCullCallback;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Camera> mHUDCamera;

        std::array<TextureArray, 2> mTextures;
        std::array<FBOArray, 2> mFbos;

        TechniqueList mTechniques;
        TechniqueList mTemplates;

        std::unordered_map<std::string, std::filesystem::path> mTechniqueFileMap;

        int mDepthFormat;
        int mSamples;

        bool mDirty;
        size_t mDirtyFrameId;

        RenderingManager& mRendering;
        osgViewer::Viewer* mViewer;
        const VFS::Manager* mVFS;

        bool mReload;
        bool mEnabled;
        bool mUsePostProcessing;
        bool mSoftParticles;
        bool mDisableDepthPasses;

        bool mExteriorFlag;
        bool mUnderwater;
        bool mHDR;
        bool mUBO;
        int mGLSLVersion;

        osg::ref_ptr<osg::Texture2D> mMainTemplate;

        osg::ref_ptr<fx::StateUpdater> mStateUpdater;
        osg::ref_ptr<PingPongCull> mPingPongCull;
        osg::ref_ptr<PingPongCanvas> mPingPongCanvas;
        osg::ref_ptr<TransparentDepthBinCallback> mTransparentDepthPostPass;
    };
}

#endif
