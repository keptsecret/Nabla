    if (maxImageDimension1D > _rhs.maxImageDimension1D) return false;
    if (maxImageDimension2D > _rhs.maxImageDimension2D) return false;
    if (maxImageDimension3D > _rhs.maxImageDimension3D) return false;
    if (maxImageDimensionCube > _rhs.maxImageDimensionCube) return false;
    if (maxImageArrayLayers > _rhs.maxImageArrayLayers) return false;
    if (maxBufferViewTexels > _rhs.maxBufferViewTexels) return false;
    if (maxUBOSize > _rhs.maxUBOSize) return false;
    if (maxSSBOSize > _rhs.maxSSBOSize) return false;
    if (maxPushConstantsSize > _rhs.maxPushConstantsSize) return false;
    if (maxMemoryAllocationCount > _rhs.maxMemoryAllocationCount) return false;
    if (maxSamplerAllocationCount > _rhs.maxSamplerAllocationCount) return false;
    if (bufferImageGranularity < _rhs.bufferImageGranularity) return false;
    if (maxPerStageDescriptorSamplers > _rhs.maxPerStageDescriptorSamplers) return false;
    if (maxPerStageDescriptorUBOs > _rhs.maxPerStageDescriptorUBOs) return false;
    if (maxPerStageDescriptorSSBOs > _rhs.maxPerStageDescriptorSSBOs) return false;
    if (maxPerStageDescriptorImages > _rhs.maxPerStageDescriptorImages) return false;
    if (maxPerStageDescriptorStorageImages > _rhs.maxPerStageDescriptorStorageImages) return false;
    if (maxPerStageDescriptorInputAttachments > _rhs.maxPerStageDescriptorInputAttachments) return false;
    if (maxPerStageResources > _rhs.maxPerStageResources) return false;
    if (maxDescriptorSetSamplers > _rhs.maxDescriptorSetSamplers) return false;
    if (maxDescriptorSetUBOs > _rhs.maxDescriptorSetUBOs) return false;
    if (maxDescriptorSetDynamicOffsetUBOs > _rhs.maxDescriptorSetDynamicOffsetUBOs) return false;
    if (maxDescriptorSetSSBOs > _rhs.maxDescriptorSetSSBOs) return false;
    if (maxDescriptorSetDynamicOffsetSSBOs > _rhs.maxDescriptorSetDynamicOffsetSSBOs) return false;
    if (maxDescriptorSetImages > _rhs.maxDescriptorSetImages) return false;
    if (maxDescriptorSetStorageImages > _rhs.maxDescriptorSetStorageImages) return false;
    if (maxDescriptorSetInputAttachments > _rhs.maxDescriptorSetInputAttachments) return false;
    if (maxVertexOutputComponents > _rhs.maxVertexOutputComponents) return false;
    if (maxWorkgroupSize[0] > _rhs.maxWorkgroupSize[0]) return false;
    if (maxWorkgroupSize[1] > _rhs.maxWorkgroupSize[1]) return false;
    if (maxWorkgroupSize[2] > _rhs.maxWorkgroupSize[2]) return false;
    if (subPixelPrecisionBits > _rhs.subPixelPrecisionBits) return false;
    if (subTexelPrecisionBits > _rhs.subTexelPrecisionBits) return false;
    if (mipmapPrecisionBits > _rhs.mipmapPrecisionBits) return false;
    if (maxDrawIndirectCount > _rhs.maxDrawIndirectCount) return false;
    if (maxSamplerLodBias > _rhs.maxSamplerLodBias) return false;
    if (maxSamplerAnisotropyLog2 > _rhs.maxSamplerAnisotropyLog2) return false;
    if (maxViewports > _rhs.maxViewports) return false;
    if (maxViewportDims[0] > _rhs.maxViewportDims[0]) return false;
    if (maxViewportDims[1] > _rhs.maxViewportDims[1]) return false;
    if (viewportBoundsRange[0] < _rhs.viewportBoundsRange[0] || viewportBoundsRange[1] > _rhs.viewportBoundsRange[1]) return false;
    if (viewportSubPixelBits > _rhs.viewportSubPixelBits) return false;
    if (minMemoryMapAlignment > _rhs.minMemoryMapAlignment) return false;
    if (bufferViewAlignment < _rhs.bufferViewAlignment) return false;
    if (minUBOAlignment < _rhs.minUBOAlignment) return false;
    if (minSSBOAlignment < _rhs.minSSBOAlignment) return false;
    if (minTexelOffset < _rhs.minTexelOffset) return false;
    if (maxTexelOffset > _rhs.maxTexelOffset) return false;
    if (minTexelGatherOffset < _rhs.minTexelGatherOffset) return false;
    if (maxTexelGatherOffset > _rhs.maxTexelGatherOffset) return false;
    if (minInterpolationOffset < _rhs.minInterpolationOffset) return false;
    if (maxInterpolationOffset > _rhs.maxInterpolationOffset) return false;
    if (subPixelInterpolationOffsetBits > _rhs.subPixelInterpolationOffsetBits) return false;
    if (maxFramebufferWidth > _rhs.maxFramebufferWidth) return false;
    if (maxFramebufferHeight > _rhs.maxFramebufferHeight) return false;
    if (maxFramebufferLayers > _rhs.maxFramebufferLayers) return false;
    if (maxSampleMaskWords > _rhs.maxSampleMaskWords) return false;
    if (maxClipDistances > _rhs.maxClipDistances) return false;
    if (maxCullDistances > _rhs.maxCullDistances) return false;
    if (maxCombinedClipAndCullDistances > _rhs.maxCombinedClipAndCullDistances) return false;
    if (discreteQueuePriorities > _rhs.discreteQueuePriorities) return false;
    if (pointSizeRange[0] < _rhs.pointSizeRange[0] || pointSizeRange[1] > _rhs.pointSizeRange[1]) return false;
    if (lineWidthRange[0] < _rhs.lineWidthRange[0] || lineWidthRange[1] > _rhs.lineWidthRange[1]) return false;
    if (pointSizeGranularity < _rhs.pointSizeGranularity) return false;
    if (lineWidthGranularity < _rhs.lineWidthGranularity) return false;
    if (strictLines && !_rhs.strictLines) return false;
    if (standardSampleLocations && !_rhs.standardSampleLocations) return false;
    if (optimalBufferCopyOffsetAlignment < _rhs.optimalBufferCopyOffsetAlignment) return false;
    if (optimalBufferCopyRowPitchAlignment < _rhs.optimalBufferCopyRowPitchAlignment) return false;
    if (nonCoherentAtomSize < _rhs.nonCoherentAtomSize) return false;
    // if (subgroupSize > _rhs.subgroupSize) return false;
    if (!_rhs.subgroupOpsShaderStages.hasFlags(subgroupOpsShaderStages)) return false;
    if (shaderSubgroupClustered && !_rhs.shaderSubgroupClustered) return false;
    if (shaderSubgroupArithmetic && !_rhs.shaderSubgroupArithmetic) return false;
    if (shaderSubgroupQuad && !_rhs.shaderSubgroupQuad) return false;
    if (shaderSubgroupQuadAllStages && !_rhs.shaderSubgroupQuadAllStages) return false;
    if (pointClippingBehavior==EPCB_ALL_CLIP_PLANES && _rhs.pointClippingBehavior==EPCB_USER_CLIP_PLANES_ONLY) return false;
    if (maxMultiviewViewCount > _rhs.maxMultiviewViewCount) return false;
    if (maxMultiviewInstanceIndex > _rhs.maxMultiviewInstanceIndex) return false;
    if (maxPerSetDescriptors > _rhs.maxPerSetDescriptors) return false;
    if (maxMemoryAllocationSize > _rhs.maxMemoryAllocationSize) return false;
    if (shaderSignedZeroInfNanPreserveFloat64 && !_rhs.shaderSignedZeroInfNanPreserveFloat64) return false;
    if (shaderDenormPreserveFloat16 && !_rhs.shaderDenormPreserveFloat16) return false;
    if (shaderDenormPreserveFloat32 && !_rhs.shaderDenormPreserveFloat32) return false;
    if (shaderDenormPreserveFloat64 && !_rhs.shaderDenormPreserveFloat64) return false;
    if (shaderDenormFlushToZeroFloat16 && !_rhs.shaderDenormFlushToZeroFloat16) return false;
    if (shaderDenormFlushToZeroFloat32 && !_rhs.shaderDenormFlushToZeroFloat32) return false;
    if (shaderDenormFlushToZeroFloat64 && !_rhs.shaderDenormFlushToZeroFloat64) return false;
    if (shaderRoundingModeRTEFloat16 && !_rhs.shaderRoundingModeRTEFloat16) return false;
    if (shaderRoundingModeRTEFloat32 && !_rhs.shaderRoundingModeRTEFloat32) return false;
    if (shaderRoundingModeRTEFloat64 && !_rhs.shaderRoundingModeRTEFloat64) return false;
    if (shaderRoundingModeRTZFloat16 && !_rhs.shaderRoundingModeRTZFloat16) return false;
    if (shaderRoundingModeRTZFloat32 && !_rhs.shaderRoundingModeRTZFloat32) return false;
    if (shaderRoundingModeRTZFloat64 && !_rhs.shaderRoundingModeRTZFloat64) return false;
    if (maxUpdateAfterBindDescriptorsInAllPools > _rhs.maxUpdateAfterBindDescriptorsInAllPools) return false;
    if (shaderUniformBufferArrayNonUniformIndexingNative && !_rhs.shaderUniformBufferArrayNonUniformIndexingNative) return false;
    if (shaderSampledImageArrayNonUniformIndexingNative && !_rhs.shaderSampledImageArrayNonUniformIndexingNative) return false;
    if (shaderStorageBufferArrayNonUniformIndexingNative && !_rhs.shaderStorageBufferArrayNonUniformIndexingNative) return false;
    if (shaderStorageImageArrayNonUniformIndexingNative && !_rhs.shaderStorageImageArrayNonUniformIndexingNative) return false;
    if (shaderInputAttachmentArrayNonUniformIndexingNative && !_rhs.shaderInputAttachmentArrayNonUniformIndexingNative) return false;
    if (robustBufferAccessUpdateAfterBind && !_rhs.robustBufferAccessUpdateAfterBind) return false;
    if (quadDivergentImplicitLod && !_rhs.quadDivergentImplicitLod) return false;
    if (maxPerStageDescriptorUpdateAfterBindSamplers > _rhs.maxPerStageDescriptorUpdateAfterBindSamplers) return false;
    if (maxPerStageDescriptorUpdateAfterBindUBOs > _rhs.maxPerStageDescriptorUpdateAfterBindUBOs) return false;
    if (maxPerStageDescriptorUpdateAfterBindSSBOs > _rhs.maxPerStageDescriptorUpdateAfterBindSSBOs) return false;
    if (maxPerStageDescriptorUpdateAfterBindImages > _rhs.maxPerStageDescriptorUpdateAfterBindImages) return false;
    if (maxPerStageDescriptorUpdateAfterBindStorageImages > _rhs.maxPerStageDescriptorUpdateAfterBindStorageImages) return false;
    if (maxPerStageDescriptorUpdateAfterBindInputAttachments > _rhs.maxPerStageDescriptorUpdateAfterBindInputAttachments) return false;
    if (maxPerStageUpdateAfterBindResources > _rhs.maxPerStageUpdateAfterBindResources) return false;
    if (maxDescriptorSetUpdateAfterBindSamplers > _rhs.maxDescriptorSetUpdateAfterBindSamplers) return false;
    if (maxDescriptorSetUpdateAfterBindUBOs > _rhs.maxDescriptorSetUpdateAfterBindUBOs) return false;
    if (maxDescriptorSetUpdateAfterBindDynamicOffsetUBOs > _rhs.maxDescriptorSetUpdateAfterBindDynamicOffsetUBOs) return false;
    if (maxDescriptorSetUpdateAfterBindSSBOs > _rhs.maxDescriptorSetUpdateAfterBindSSBOs) return false;
    if (maxDescriptorSetUpdateAfterBindDynamicOffsetSSBOs > _rhs.maxDescriptorSetUpdateAfterBindDynamicOffsetSSBOs) return false;
    if (maxDescriptorSetUpdateAfterBindImages > _rhs.maxDescriptorSetUpdateAfterBindImages) return false;
    if (maxDescriptorSetUpdateAfterBindStorageImages > _rhs.maxDescriptorSetUpdateAfterBindStorageImages) return false;
    if (maxDescriptorSetUpdateAfterBindInputAttachments > _rhs.maxDescriptorSetUpdateAfterBindInputAttachments) return false;
    if (!_rhs.supportedDepthResolveModes.hasFlags(supportedDepthResolveModes)) return false;
    if (!_rhs.supportedStencilResolveModes.hasFlags(supportedStencilResolveModes)) return false;
    if (independentResolveNone && !_rhs.independentResolveNone) return false;
    if (independentResolve && !_rhs.independentResolve) return false;
    if (minSubgroupSize < _rhs.minSubgroupSize) return false;
    if (maxSubgroupSize > _rhs.maxSubgroupSize) return false;
    if (maxComputeWorkgroupSubgroups > _rhs.maxComputeWorkgroupSubgroups) return false;
    if (!_rhs.requiredSubgroupSizeStages.hasFlags(requiredSubgroupSizeStages)) return false;
    if (integerDotProduct8BitUnsignedAccelerated && !_rhs.integerDotProduct8BitUnsignedAccelerated) return false;
    if (integerDotProduct8BitSignedAccelerated && !_rhs.integerDotProduct8BitSignedAccelerated) return false;
    if (integerDotProduct8BitMixedSignednessAccelerated && !_rhs.integerDotProduct8BitMixedSignednessAccelerated) return false;
    if (integerDotProduct4x8BitPackedUnsignedAccelerated && !_rhs.integerDotProduct4x8BitPackedUnsignedAccelerated) return false;
    if (integerDotProduct4x8BitPackedSignedAccelerated && !_rhs.integerDotProduct4x8BitPackedSignedAccelerated) return false;
    if (integerDotProduct4x8BitPackedMixedSignednessAccelerated && !_rhs.integerDotProduct4x8BitPackedMixedSignednessAccelerated) return false;
    if (integerDotProduct16BitUnsignedAccelerated && !_rhs.integerDotProduct16BitUnsignedAccelerated) return false;
    if (integerDotProduct16BitSignedAccelerated && !_rhs.integerDotProduct16BitSignedAccelerated) return false;
    if (integerDotProduct16BitMixedSignednessAccelerated && !_rhs.integerDotProduct16BitMixedSignednessAccelerated) return false;
    if (integerDotProduct32BitUnsignedAccelerated && !_rhs.integerDotProduct32BitUnsignedAccelerated) return false;
    if (integerDotProduct32BitSignedAccelerated && !_rhs.integerDotProduct32BitSignedAccelerated) return false;
    if (integerDotProduct32BitMixedSignednessAccelerated && !_rhs.integerDotProduct32BitMixedSignednessAccelerated) return false;
    if (integerDotProduct64BitUnsignedAccelerated && !_rhs.integerDotProduct64BitUnsignedAccelerated) return false;
    if (integerDotProduct64BitSignedAccelerated && !_rhs.integerDotProduct64BitSignedAccelerated) return false;
    if (integerDotProduct64BitMixedSignednessAccelerated && !_rhs.integerDotProduct64BitMixedSignednessAccelerated) return false;
    if (integerDotProductAccumulatingSaturating8BitUnsignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating8BitUnsignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating8BitSignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating8BitSignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated && !_rhs.integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated) return false;
    if (integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated && !_rhs.integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated) return false;
    if (integerDotProductAccumulatingSaturating16BitUnsignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating16BitUnsignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating16BitSignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating16BitSignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated && !_rhs.integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated) return false;
    if (integerDotProductAccumulatingSaturating32BitUnsignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating32BitUnsignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating32BitSignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating32BitSignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated && !_rhs.integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated) return false;
    if (integerDotProductAccumulatingSaturating64BitUnsignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating64BitUnsignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating64BitSignedAccelerated && !_rhs.integerDotProductAccumulatingSaturating64BitSignedAccelerated) return false;
    if (integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated && !_rhs.integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated) return false;
    if (maxBufferSize > _rhs.maxBufferSize) return false;
    if (shaderTrinaryMinmax && !_rhs.shaderTrinaryMinmax) return false;
    if (shaderExplicitVertexParameter && !_rhs.shaderExplicitVertexParameter) return false;
    if (gpuShaderHalfFloatAMD && !_rhs.gpuShaderHalfFloatAMD) return false;
    if (shaderImageLoadStoreLod && !_rhs.shaderImageLoadStoreLod) return false;
    if (queueFamilyForeign && !_rhs.queueFamilyForeign) return false;
    if (shaderStencilExport && !_rhs.shaderStencilExport) return false;
    if (postDepthCoverage && !_rhs.postDepthCoverage) return false;
    if (decorateString && !_rhs.decorateString) return false;
    if (shaderNonSemanticInfo && !_rhs.shaderNonSemanticInfo) return false;
    if (shaderEarlyAndLateFragmentTests && !_rhs.shaderEarlyAndLateFragmentTests) return false;
    if (fragmentShaderBarycentric && !_rhs.fragmentShaderBarycentric) return false;
    if (logicOp && !_rhs.logicOp) return false;
    if (vertexPipelineStoresAndAtomics && !_rhs.vertexPipelineStoresAndAtomics) return false;
    if (fragmentStoresAndAtomics && !_rhs.fragmentStoresAndAtomics) return false;
    if (shaderTessellationAndGeometryPointSize && !_rhs.shaderTessellationAndGeometryPointSize) return false;
    if (shaderStorageImageMultisample && !_rhs.shaderStorageImageMultisample) return false;
    if (shaderStorageImageReadWithoutFormat && !_rhs.shaderStorageImageReadWithoutFormat) return false;
    if (shaderStorageImageArrayDynamicIndexing && !_rhs.shaderStorageImageArrayDynamicIndexing) return false;
    if (shaderFloat64 && !_rhs.shaderFloat64) return false;
    if (variableMultisampleRate && !_rhs.variableMultisampleRate) return false;
    if (drawIndirectCount && !_rhs.drawIndirectCount) return false;
    if (storagePushConstant8 && !_rhs.storagePushConstant8) return false;
    if (shaderFloat16 && !_rhs.shaderFloat16) return false;
    if (samplerFilterMinmax && !_rhs.samplerFilterMinmax) return false;
    if (vulkanMemoryModelAvailabilityVisibilityChains && !_rhs.vulkanMemoryModelAvailabilityVisibilityChains) return false;
    if (computeUnits > _rhs.computeUnits) return false;
    if (dispatchBase && !_rhs.dispatchBase) return false;
    if (allowCommandBufferQueryCopies && !_rhs.allowCommandBufferQueryCopies) return false;
    if (maxOptimallyResidentWorkgroupInvocations > _rhs.maxOptimallyResidentWorkgroupInvocations) return false;
    if (maxResidentInvocations > _rhs.maxResidentInvocations) return false;
    if (spirvVersion > _rhs.spirvVersion) return false;
