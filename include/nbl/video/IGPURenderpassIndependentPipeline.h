// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#ifndef __NBL_VIDEO_I_GPU_RENDERPASS_INDEPENDENT_PIPELINE_H_INCLUDED__
#define __NBL_VIDEO_I_GPU_RENDERPASS_INDEPENDENT_PIPELINE_H_INCLUDED__

#include "nbl/asset/IRenderpassIndependentPipeline.h"
#include "nbl/video/IGPUSpecializedShader.h"
#include "nbl/video/IGPUPipelineLayout.h"
#include "nbl/video/IBackendObject.h"

namespace nbl
{
namespace video
{

//! GPU Version of Renderpass Independent Pipeline
/*
	@see IRenderpassIndependentPipeline
*/

class IGPURenderpassIndependentPipeline : public asset::IRenderpassIndependentPipeline<IGPUSpecializedShader, IGPUPipelineLayout>, public IBackendObject
{
		using base_t = asset::IRenderpassIndependentPipeline<IGPUSpecializedShader, IGPUPipelineLayout>;

	public:
		IGPURenderpassIndependentPipeline(
			ILogicalDevice* dev,
			core::smart_refctd_ptr<IGPUPipelineLayout>&& _layout,
			IGPUSpecializedShader** _shadersBegin, IGPUSpecializedShader** _shadersEnd,
			const asset::SVertexInputParams& _vertexInputParams,
			const asset::SBlendParams& _blendParams,
			const asset::SPrimitiveAssemblyParams& _primAsmParams,
			const asset::SRasterizationParams& _rasterParams
		) : base_t(std::move(_layout), _shadersBegin, _shadersEnd, _vertexInputParams, _blendParams, _primAsmParams, _rasterParams), IBackendObject(dev)
		{

		}

		struct SCreationParams
		{
			core::smart_refctd_ptr<IGPUPipelineLayout> layout;
			core::smart_refctd_ptr<IGPUSpecializedShader> shaders[SHADER_STAGE_COUNT];
			asset::SVertexInputParams vertexInput;
			asset::SBlendParams blend;
			asset::SPrimitiveAssemblyParams primitiveAssembly;
			asset::SRasterizationParams rasterization;
		};

	protected:
		virtual ~IGPURenderpassIndependentPipeline() = default;
};

}
}

#endif