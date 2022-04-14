#include "pass.hpp"

#include <unordered_set>
#include <string>
#include <sstream>

#include <osg/Program>
#include <osg/Shader>
#include <osg/State>
#include <osg/StateSet>
#include <osg/BindImageTexture>
#include <osg/FrameBufferObject>

#include <components/misc/stringops.hpp>
#include <components/sceneutil/util.hpp>
#include <components/resource/scenemanager.hpp>

#include "technique.hpp"

namespace
{
    constexpr char s_DefaultVertex[] = R"GLSL(
#if OMW_USE_BINDINGS
    omw_In vec2 omw_Vertex;
#endif
omw_Out vec2 omw_TexCoord;

void main()
{
    omw_Position = vec4(omw_Vertex.xy, 0.0, 1.0);
    omw_TexCoord = omw_Position.xy * 0.5 + 0.5;
})GLSL";

    class ClearColor : public osg::StateAttribute
    {
    public:
        ClearColor() : mMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) {}
        ClearColor(const osg::Vec4f& color, GLbitfield mask) : mColor(color), mMask(mask) {}

        ClearColor(const ClearColor& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mColor(copy.mColor), mMask(copy.mMask) {}

        META_StateAttribute(fx, ClearColor, (osg::StateAttribute::Type)100)

        int compare(const StateAttribute& sa) const override
        {
            throw std::runtime_error("ClearColor::compare: unimplemented");
        }

        void apply(osg::State& state) const override
        {
            glClearColor(mColor[0], mColor[1], mColor[2], mColor[3]);
            glClear(mMask);
        }

    private:
        osg::Vec4f mColor;
        GLbitfield mMask;
    };

}

namespace fx
{
    Pass::Pass(Pass::Type type, Pass::Order order, bool ubo)
        : mCompiled(false)
        , mType(type)
        , mOrder(order)
        , mLegacyGLSL(true)
        , mUBO(ubo)
    {
    }

    std::string Pass::getPassHeader(Technique& technique, std::string_view preamble, bool fragOut)
    {
        std::string header = R"GLSL(
#version @version @profile
@extensions

struct _omw_data {
    mat4 projectionMatrix;
    mat4 invProjectionMatrix;
    mat4 viewMatrix;
    mat4 prevViewMatrix;
    mat4 invViewMatrix;
    vec4 eyePos;
    vec4 eyeVec;
    vec4 fogColor;
    vec4 sunColor;
    vec4 sunPos;
    vec2 resolution;
    vec2 rcpResolution;
    float fogNear;
    float fogFar;
    float near;
    float far;
    float fov;
    float gameHour;
    float sunVis;
    float waterHeight;
    bool isUnderwater;
    bool isInterior;
    float simulationTime;
    float deltaSimulationTime;
};

#define OMW_REVERSE_Z @reverseZ
#define OMW_RADIAL_FOG @radialFog
#define OMW_HDR @hdr
#define OMW_USE_BINDINGS @useBindings
#define omw_In @in
#define omw_Out @out
#define omw_Position @position
#define omw_Texture1D @texture1D
#define omw_Texture2D @texture2D
#define omw_Texture3D @texture3D
#define omw_Vertex @vertex
#define omw_FragColor @fragColor

@fragBinding

#if @ubo
    layout(std140) uniform _data { _omw_data omw; };
#else
    uniform _omw_data omw;
#endif
    float omw_GetDepth(sampler2D depthSampler, vec2 uv)
    {
        float depth = omw_Texture2D(depthSampler, uv).r;
#if OMW_REVERSE_Z
        return 1.0 - depth;
#else
        return depth;
#endif
    }

#if OMW_HDR
    uniform sampler2D omw_EyeAdaption;
#endif

    float omw_GetEyeAdaption()
    {
#if OMW_HDR
        return omw_Texture2D(omw_EyeAdaption, vec2(0.5, 0.5)).r;
#else
        return 1.0;
#endif
    }
)GLSL";

        std::stringstream extBlock;
        for (const auto& extension : technique.getGLSLExtensions())
            extBlock << "#ifdef " << extension << '\n' << "\t#extension " << extension << ": enable" << '\n' << "#endif" << '\n';

        const std::unordered_map<std::string, std::string> defines = {
            {"@version", technique.getGLSLVersion()},
            {"@profile", technique.getGLSLProfile()},
            {"@extensions", extBlock.str()},
            {"@reverseZ", SceneUtil::AutoDepth::isReversed() ? "1" : "0"},
            {"@radialFog", Settings::Manager::getBool("radial fog", "Shaders") ? "1" : "0"},
            {"@ubo", mUBO ? "1" : "0"},
            {"@hdr", technique.getHDR() ? "1" : "0"},
            {"@in", mLegacyGLSL ? "varying" : "in"},
            {"@out", mLegacyGLSL ? "varying" : "out"},
            {"@position", "gl_Position"},
            {"@texture1D", mLegacyGLSL ? "texture1D" : "texture"},
            {"@texture2D", mLegacyGLSL ? "texture2D" : "texture"},
            {"@texture3D", mLegacyGLSL ? "texture3D" : "texture"},
            {"@vertex", mLegacyGLSL ? "gl_Vertex" : "omw_Vertex"},
            {"@fragColor", mLegacyGLSL ? "gl_FragColor" : "omw_FragColor"},
            {"@useBindings", mLegacyGLSL ? "0" : "1"},
            {"@fragBinding", mLegacyGLSL ? "" : "out vec4 omw_FragColor;"},
        };

        for (const auto& [define, value]: defines)
            header.replace(header.find(define), define.size(), value);

        for (auto& uniform : technique.getUniformMap())
            if (auto glsl = uniform->getGLSL())
                header.append(glsl.value());

        header.append(preamble);

        return header;
    }

    void Pass::prepareStateSet(osg::StateSet* stateSet, const std::string& name) const
    {
        osg::ref_ptr<osg::Program> program = new osg::Program;
        if (mType == Type::Pixel)
        {
            program->addShader(new osg::Shader(*mVertex));
            program->addShader(new osg::Shader(*mFragment));
        }
        else if (mType == Type::Compute)
        {
            program->addShader(new osg::Shader(*mCompute));
        }

        if (mUBO)
            program->addBindUniformBlock("_data", static_cast<int>(Resource::SceneManager::UBOBinding::PostProcessor));

        program->setName(name);

        if (!mLegacyGLSL)
        {
            program->addBindFragDataLocation("omw_FragColor", 0);
            program->addBindAttribLocation("omw_Vertex", 0);
        }

        stateSet->setAttribute(program);

        if (mBlendSource && mBlendDest)
            stateSet->setAttribute(new osg::BlendFunc(mBlendSource.value(), mBlendDest.value()));

        if (mBlendEq)
            stateSet->setAttribute(new osg::BlendEquation(mBlendEq.value()));

        if (mClearColor)
            stateSet->setAttribute(new ClearColor(mClearColor.value(), GL_COLOR_BUFFER_BIT));
    }

    void Pass::dirty()
    {
        mVertex = nullptr;
        mFragment = nullptr;
        mCompute = nullptr;

        mCompiled = false;
    }

    void Pass::compile(Technique& technique, std::string_view preamble)
    {
        if (mCompiled)
            return;

        mLegacyGLSL = technique.getGLSLVersion() != "330";

        if (mType == Type::Pixel)
        {
            if (!mVertex)
                mVertex = new osg::Shader(osg::Shader::VERTEX, s_DefaultVertex);

            mVertex->setShaderSource(getPassHeader(technique, preamble).append(mVertex->getShaderSource()));
            mFragment->setShaderSource(getPassHeader(technique, preamble, true).append(mFragment->getShaderSource()));

            mVertex->setName(mName);
            mFragment->setName(mName);
        }
        else if (mType == Type::Compute)
        {
            mCompute->setShaderSource(getPassHeader(technique, preamble).append(mCompute->getShaderSource()));
            mCompute->setName(mName);
        }

        mCompiled = true;
    }

}
