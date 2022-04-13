#########################
OMWFX Language Reference
#########################

Overview
########

Shaders are written in a OpenMW specific ``*.omwfx`` format. This is a light
wrapper around GLSL, so a basic understanding of GLSL should be aquired before
attempting to write any shaders. Every shader must be contained within a single
``*.omwfx`` file, ``#include`` directives are currently unsupported.

Reserved Keywords
#################

GLSL doesn't support namespaces, instead reserved prefixes are used. Do not
attempt to name anything starting with ``_`` or ``omw``, this will cause
name clashes.


Builtin Samplers
################

+-------------+-----------------------+-------------------------------------+
| GLSL Type   | Name                  | Description                         |
+=============+=======================+=====================================+
| sampler2D   | omw_SamplerLastShader | Color output of last shader         |
+-------------+-----------------------+-------------------------------------+
| sampler2D   | omw_SamplerLastPass   | Color output of last pass           |
+-------------+-----------------------+-------------------------------------+
| sampler2D   | omw_SamplerDepth      | Non-linear normalized depth         |
+-------------+-----------------------+-------------------------------------+

Builtin Uniforms
################

+-------------+--------------------------+--------------------------------------------------+
| GLSL Type   | Name                     | Description                                      |
+=============+==========================+==================================================+
| mat4        | omw.projectionMatrix     | Cameras projection matrix                        |
+-------------+--------------------------+--------------------------------------------------+
| mat4        | omw.invProjectionMatrix  | Inverse of cameras projection matrix             |
+-------------+--------------------------+--------------------------------------------------+
| mat4        | omw.viewMatrix           | Cameras view matrix                              |
+-------------+--------------------------+--------------------------------------------------+
| mat4        | omw.prevViewMatrix       | Cameras previous frame view matrix               |
+-------------+--------------------------+--------------------------------------------------+
| mat4        | omw.invViewMatrix        | Inverse of cameras view matrix                   |
+-------------+--------------------------+--------------------------------------------------+
| vec4        | omw.eyePos               | Cameras eye position                             |
+-------------+--------------------------+--------------------------------------------------+
| vec4        | omw.eyeVec               | Normalized cameras eye vector                    |
+-------------+--------------------------+--------------------------------------------------+
| vec4        | omw.fogColor             | RGBA color of fog                                |
+-------------+--------------------------+--------------------------------------------------+
| vec4        | omw.sunColor             | RGBA color of sun                                |
+-------------+--------------------------+--------------------------------------------------+
| vec4        | omw.sunPos               | Normalized sun direction                         |
|             |                          |                                                  |
|             |                          | When sun is set `omw.sunpos.z` is negated        |
+-------------+--------------------------+--------------------------------------------------+
| vec2        | omw.resolution           | Render target resolution                         |
+-------------+--------------------------+--------------------------------------------------+
| vec2        | omw.rcpResolution        | Reciprocal of render target resolution           |
+-------------+--------------------------+--------------------------------------------------+
| vec2        | omw.fogNear              | Units at which fog begins to render              |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.fogFar               | Units at which fog ends                          |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.near                 | Cameras near clip                                |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.far                  | Cameras far clip                                 |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.fov                  | Vertical field of view, in degrees               |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.gameHour             | Game hour in range [0,23]                        |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.sunVis               | Sun visibility between [0, 1]                    |
|             |                          |                                                  |
|             |                          | Influenced by types of weather                   |
|             |                          |                                                  |
|             |                          | Closer to zero during overcast weathers          |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.waterHeight          | Water height of current cell                     |
|             |                          |                                                  |
|             |                          | Exterior water level is always zero              |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.simulationTime       | Time in milliseconds since simulation began      |
+-------------+--------------------------+--------------------------------------------------+
| float       | omw.deltaSimulationTime  | Change in `omw.simulationTime` from last frame   |
+-------------+--------------------------+--------------------------------------------------+
| bool        | omw.isUnderwater         | True if player is submerged underwater           |
+-------------+--------------------------+--------------------------------------------------+
| bool        | omw.isInterior           | True if player is in an interior                 |
|             |                          |                                                  |
|             |                          | False for interiors that behave like exteriors   |
+-------------+--------------------------+--------------------------------------------------+


Builtin Macros
##############

+------------------+----------------+---------------------------------------------------------------------------+
| Macro            | Definition     | Description                                                               |
+==================+================+===========================================================================+
|  OMW_REVERSE_Z   | ``0`` or ``1`` | Whether a reversed depth buffer is in use.                                |
|                  |                |                                                                           |
|                  |                | ``0``  Depth sampler will be in range [1, 0]                              |
|                  |                |                                                                           |
|                  |                | ``1``  Depth sampler will be in range [0, 1]                              |
+------------------+----------------+---------------------------------------------------------------------------+
|  OMW_RADIAL_FOG  | ``0`` or ``1`` | Whether radial fog is in use                                              |
|                  |                |                                                                           |
|                  |                | ``0``  Fog is linear                                                      |
|                  |                |                                                                           |
|                  |                | ``1``  Fog is radial                                                      |
+------------------+----------------+---------------------------------------------------------------------------+

Builtin Functions
#################

omw_GetDepth(sampler2D, TexCoord)

User Configurable Types
#######################

...

Define types and available fields here...

Simple Example
##############

Let us go through a simple example in which we apply a simple desaturation
filter with a user-configurable factor.

Our first step is defining our user-configurable variable. In this case all we
want is a normalized value between 0 and 1 to influence the amount of
desaturation to apply to the scene. Here we setup a new variable of type
``float``, define a few basic properties, and give it a tooltip description. 

.. code-block:: none

    uniform_float desaturation_factor {
        default = 0.5;
        min = 0.0;
        max = 1.0;
        step = 0.05;
        description = "Desaturation factor. A value of 1.0 is full grayscale.";
    }

Now, we can setup our first pass. Remember a pass is just a pixel shader invocation.

.. code-block:: none

    fragment desaturate {
        IN vec2 uv;
        uniform sampler2D omw_SamplerLastShader;

        void main()
        {
            // fetch scene texture from last shader
            vec4 scene = texture2D(omw_SamplerLastShader, uv);

            // desaturate RGB component
            const vec3 luminance = vec3(0.299, 0.587, 0.144);
            float gray = dot(luminance, scene.rgb);

            COLOR = vec4(mix(scene.rgb, vec3(gray), desaturation_factor), scene.a);
        }
    }

Next we can define our ``technique`` block, which is in charge of glueing
together passes, setting up metadata, and setting up various flags.

.. code-block:: none

    technique {
        description = "Desaturates scene";
        passes = desaturate;
        version = "1.0";
        author = "Fargoth";
        passes = desaturate;
    }


Putting it all together we have this simple shader.

.. code-block:: none

    uniform_float desaturation_factor {
        default = 0.5;
        min = 0.0;
        max = 1.0;
        step = 0.05;
        description = "Desaturation factor. A value of 1.0 is full grayscale.";
    }

    fragment desaturate {
        IN vec2 uv;
        uniform sampler2D omw_SamplerLastShader;

        void main()
        {
            // fetch scene texture from last shader
            vec4 scene = texture2D(omw_SamplerLastShader, uv);

            // desaturate RGB component
            const vec3 luminance = vec3(0.299, 0.587, 0.144);
            float gray = dot(luminance, scene.rgb);

            COLOR = vec4(mix(scene.rgb, vec3(gray), desaturation_factor), scene.a);
        }
    }

    technique {
        description = "Desaturates scene";
        passes = desaturate;
        version = "1.0";
        author = "Fargoth";
        passes = desaturate;
    }
