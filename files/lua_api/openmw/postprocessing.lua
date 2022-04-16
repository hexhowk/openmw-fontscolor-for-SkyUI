---
-- `openmw.postprocessing` is an interface to postprocessing shaders.
-- Can be used only by local scripts, that are attached to a player.
-- @module shader
-- @usage local postprocessing = require('openmw.postprocessing')



---
-- Load a shader and return its handle.
-- @function [parent=#postprocessing] load
-- @param #string name Name of the shader without its extension
-- @return #Shader
-- @usage
-- If the shader exists and compiles, the shader will still be off by default.
-- It must be enabled to see its effect.
-- local vignetteShader = postprocessing.load('vignette')

---
-- Enable the shader. Has no effect if the shader is already enabled or does
-- not exist. Will not apply until the next frame.
-- @function [parent=#Shader] enable Enable the shader
-- @param self
-- @param #number position optional position to place the shader. If left out the shader will be inserted at the end of the chain.
-- @usage
-- -- Load shader
-- local vignetteShader = postprocessing.load('vignette')
-- -- Toggle shader on
-- vignetteShader:enable()

---
-- Deactivate the shader. Has no effect if the shader is already deactivated or does not exist.
-- Will not apply until the next frame.
-- @function [parent=#Shader] disable Disable the shader
-- @param self
-- @usage
-- local vignetteShader = shader.postprocessing('vignette')
-- vignetteShader:disable() -- shader will be toggled off

---
-- Check if the shader is enabled.
-- @function [parent=#Shader] isEnabled
-- @param self
-- @return #boolean True if shader is enabled and was compiled successfully.
-- @usage
-- local vignetteShader = shader.postprocessing('vignette')
-- vignetteShader:enable() -- shader will be toggled on

---
-- Set a non static shader variable.
-- @function [parent=#Shader] setUniform
-- @param self
-- @param #string name Name of uniform
-- @param #any value Value of uniform. No type conversions take place, ensure correct type is used.
-- @usage
-- local postprocessing = require('openmw.postprocessing')
-- local util = require('openmw.util')
--
-- -- Load and activate a shader
-- local vignetteShader = shader.postprocessing('vignette')
-- vignetteShader:enable()
--
-- -- Ensure you pass correct uniform type.
-- vignetteShader.setUniform('myBool', true)
-- vignetteShader.setUniform('myFloat', 1.0)
-- vignetteShader.setUniform('myInt', 1)
-- vignetteShader.setUniform('myVector2', util.vector2(1, 1))
-- vignetteShader.setUniform('myVector3', util.vector3(1, 1, 1))
-- vignetteShader.setUniform('myVector4', util.vector4(1, 1, 1, 1))

return nil
