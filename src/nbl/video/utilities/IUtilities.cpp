#include "nbl/video/utilities/IUtilities.h"
#include "nbl/asset/filters/CConvertFormatImageFilter.h"
#include <numeric>

namespace nbl::video
{

// TODO: Consider Moving this to a better place (Iutilities.h)
class ImageRegionIterator
{
public:
    ImageRegionIterator(
        const core::SRange<const asset::IImage::SBufferCopy>& copyRegions,
        IPhysicalDevice::SQueueFamilyProperties queueFamilyProps,
        const void* srcBufferData,
        asset::E_FORMAT srcImageFormat,
        video::IGPUImage* const dstImage
    )
        : regions(copyRegions)
        , srcBufferData(srcBufferData)
        , dstImage(dstImage)
        , srcImageFormat(srcImageFormat)
    {
        dstImageFormat = dstImage->getCreationParameters().format;
        if(srcImageFormat == asset::EF_UNKNOWN)
            srcImageFormat = dstImageFormat;
        asset::TexelBlockInfo dstImageTexelBlockInfo(dstImageFormat);

        memcpy(&minImageTransferGranularity, &queueFamilyProps.minImageTransferGranularity, sizeof(VkExtent3D));

        // bufferOffsetAlignment:
            // [x] If Depth/Stencil -> must be multiple of 4
            // [ ] If multi-planar -> bufferOffset must be a multiple of the element size of the compatible format for the aspectMask of imagesubresource
            // [x] If Queue doesn't support GRAPHICS_BIT or COMPUTE_BIT ->  must be multiple of 4
            // [x] bufferOffset must be a multiple of texel block size in bytes
        bufferOffsetAlignment = dstImageTexelBlockInfo.getBlockByteSize();
        if(asset::isDepthOrStencilFormat(dstImageFormat))
            bufferOffsetAlignment = std::lcm(bufferOffsetAlignment, 4u);

        bool queueSupportsCompute = queueFamilyProps.queueFlags.hasFlags(IPhysicalDevice::EQF_COMPUTE_BIT);
        bool queueSupportsGraphics = queueFamilyProps.queueFlags.hasFlags(IPhysicalDevice::EQF_GRAPHICS_BIT);
        if((queueSupportsGraphics || queueSupportsCompute) == false)
            bufferOffsetAlignment = std::lcm(bufferOffsetAlignment, 4u);
        // TODO: Need to have a function to get equivalent format of the specific plane of this format (in aspectMask)
        // if(asset::isPlanarFormat(dstImageFormat->getCreationParameters().format))
        assert(core::is_alignment(bufferOffsetAlignment));
        
        // Queues supporting graphics and/or compute operations must report (1,1,1) in minImageTransferGranularity, meaning that there are no additional restrictions on the granularity of image transfer operations for these queues.
        // Other queues supporting image transfer operations are only required to support whole mip level transfers, thus minImageTransferGranularity for queues belonging to such queue families may be (0,0,0)
        canTransferMipLevelsPartially = !(minImageTransferGranularity.width == 0 && minImageTransferGranularity.height == 0 && minImageTransferGranularity.depth == 0);
    }
    
    // ! Memory you need to allocate to transfer the remaining regions in one submit.
    // ! WARN: It's okay to use less memory than the return value of this function for your staging memory, in that usual case more than 1 copy regions will be needed to transfer the remaining regions.
    size_t getMemoryNeededForRemainingRegions() const
    {
        asset::TexelBlockInfo dstImageTexelBlockInfo(dstImageFormat);
        auto texelBlockDim = dstImageTexelBlockInfo.getDimension();
        size_t memoryNeededForRemainingRegions = 0ull;
        for (uint32_t i = currentRegion; i < regions.size(); ++i)
        {
            memoryNeededForRemainingRegions = core::alignUp(memoryNeededForRemainingRegions, bufferOffsetAlignment);

            const asset::IImage::SBufferCopy & region = regions[i];

            auto subresourceSize = dstImage->getMipSize(region.imageSubresource.mipLevel);

            assert(static_cast<uint32_t>(region.imageSubresource.aspectMask) != 0u);
            assert(core::isPoT(static_cast<uint32_t>(region.imageSubresource.aspectMask)) && "region.aspectMask should only have a single bit set.");
            // Validate Region
            // canTransferMipLevelsPartially
            if(!canTransferMipLevelsPartially)
            {
                assert(region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0);
                assert(region.imageExtent.width == subresourceSize.x && region.imageExtent.height == subresourceSize.y && region.imageExtent.depth == subresourceSize.z);
            }

            // region.imageOffset.{xyz} should be multiple of minImageTransferGranularity.{xyz} scaled up by block size
            bool isImageOffsetAlignmentValid =
                (region.imageOffset.x % (minImageTransferGranularity.width  * texelBlockDim.x) == 0) &&
                (region.imageOffset.y % (minImageTransferGranularity.height * texelBlockDim.y) == 0) &&
                (region.imageOffset.z % (minImageTransferGranularity.depth  * texelBlockDim.z) == 0);
            assert(isImageOffsetAlignmentValid);

            // region.imageExtent.{xyz} should be multiple of minImageTransferGranularity.{xyz} scaled up by block size,
            // OR ELSE (region.imageOffset.{x/y/z} + region.imageExtent.{width/height/depth}) MUST be equal to subresource{Width,Height,Depth}
            bool isImageExtentAlignmentValid = 
                (region.imageExtent.width  % (minImageTransferGranularity.width  * texelBlockDim.x) == 0 || (region.imageOffset.x + region.imageExtent.width   == subresourceSize.x)) && 
                (region.imageExtent.height % (minImageTransferGranularity.height * texelBlockDim.y) == 0 || (region.imageOffset.y + region.imageExtent.height  == subresourceSize.y)) &&
                (region.imageExtent.depth  % (minImageTransferGranularity.depth  * texelBlockDim.z) == 0 || (region.imageOffset.z + region.imageExtent.depth   == subresourceSize.z));
            assert(isImageExtentAlignmentValid);

            bool isImageExtentAndOffsetValid = 
                (region.imageExtent.width + region.imageOffset.x <= subresourceSize.x) &&
                (region.imageExtent.height + region.imageOffset.y <= subresourceSize.y) &&
                (region.imageExtent.depth + region.imageOffset.z <= subresourceSize.z);
            assert(isImageExtentAndOffsetValid);

            auto imageExtent = core::vector3du32_SIMD(region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth);
            auto imageExtentInBlocks = dstImageTexelBlockInfo.convertTexelsToBlocks(imageExtent);
            auto imageExtentBlockStridesInBytes = dstImageTexelBlockInfo.convert3DBlockStridesTo1DByteStrides(imageExtentInBlocks);

            if(i == currentRegion)
            {
                auto remainingBlocksInRow = imageExtentInBlocks.x - currentBlockInRow;
                auto remainingRowsInSlice = imageExtentInBlocks.y - currentRowInSlice;
                auto remainingSlicesInLayer = imageExtentInBlocks.z - currentSliceInLayer;
                auto remainingLayersInRegion = region.imageSubresource.layerCount - currentLayerInRegion;

                if(currentBlockInRow == 0 && currentRowInSlice == 0 && currentSliceInLayer == 0 && remainingLayersInRegion > 0)
                    memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[3] * remainingLayersInRegion;
                else if (currentBlockInRow == 0 && currentRowInSlice == 0 && currentSliceInLayer > 0)
                {
                    memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[2] * remainingSlicesInLayer;
                    if(remainingLayersInRegion > 1u)
                        memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[3] * (remainingLayersInRegion - 1u);
                }
                else if (currentBlockInRow == 0 && currentRowInSlice > 0)
                {
                    memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[1] * remainingRowsInSlice;
                    if(remainingSlicesInLayer > 1u)
                        memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[2] * (remainingSlicesInLayer - 1u);
                    if(remainingLayersInRegion > 1u)
                        memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[3] * (remainingLayersInRegion - 1u);
                }
                else if (currentBlockInRow > 0)
                {
                    memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[0] * remainingBlocksInRow;
                    if(remainingRowsInSlice > 1u)
                        memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[1] * (remainingRowsInSlice - 1u);
                    if(remainingSlicesInLayer > 1u)
                        memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[2] * (remainingSlicesInLayer - 1u);
                    if(remainingLayersInRegion > 1u)
                        memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[3] * (remainingLayersInRegion - 1u);
                }
            }
            else
            {
                memoryNeededForRemainingRegions += imageExtentBlockStridesInBytes[3] * region.imageSubresource.layerCount; // = blockByteSize * imageExtentInBlocks.x * imageExtentInBlocks.y * imageExtentInBlocks.z * region.imageSubresource.layerCount
            }
        }
        return memoryNeededForRemainingRegions;
    }

    // ! Gives `regionToCopyNext` based on `availableMemory`
    // ! memcopies the data from `srcBuffer` to `stagingBuffer`, preparing it for launch and submit to copy to GPU buffer
    // ! updates `availableMemory` (availableMemory -= consumedMemory)
    // ! updates `stagingBufferOffset` based on consumed memory and alignment requirements
    // ! this function may do format conversions when copying from `srcBuffer` to `stagingBuffer` if srcBufferFormat != dstImage->Format passed as constructor parameters
    bool advanceAndCopyToStagingBuffer(asset::IImage::SBufferCopy & regionToCopyNext, size_t& availableMemory, size_t& stagingBufferOffset, void * stagingBufferPointer)
    {
        if(isFinished())
            return false;
        
        auto addToCurrentUploadBufferOffset = [&](uint32_t size) -> bool 
        {
            const auto initialOffset = stagingBufferOffset;
            stagingBufferOffset += size;
            stagingBufferOffset = core::alignUp(stagingBufferOffset, bufferOffsetAlignment);
            const auto consumedMemory = stagingBufferOffset - initialOffset;
            if(consumedMemory <= availableMemory)
            {
                availableMemory -= consumedMemory;
                return true;
            }
            else
            {
                assert(false);
                return false;
            }
        };

        if(!addToCurrentUploadBufferOffset(0u)) // in order to fix initial alignment to bufferOffsetAlignment
        {
            return false;
        }

        const asset::IImage::SBufferCopy & region = regions[currentRegion];
        
        asset::TexelBlockInfo srcImageTexelBlockInfo(srcImageFormat);
        asset::TexelBlockInfo dstImageTexelBlockInfo(dstImageFormat);
        
        const auto srcBufferByteStrides = srcImageTexelBlockInfo.convert3DBlockStridesTo1DByteStrides(region.getBlockStrides(srcImageTexelBlockInfo));

        const auto subresourceSize = dstImage->getMipSize(region.imageSubresource.mipLevel);
        const auto subresourceSizeInBlocks = dstImageTexelBlockInfo.convertTexelsToBlocks(subresourceSize);
        
        // regionBlockStrides = <BufferRowLengthInBlocks, BufferImageHeightInBlocks, ImageDepthInBlocks>
        const auto regionBlockStrides = region.getBlockStrides(dstImageTexelBlockInfo);
        // regionBlockStridesInBytes = <BlockByteSize,
        //                              BlockBytesSize * BufferRowLengthInBlocks,
        //                              BlockBytesSize * BufferRowLengthInBlocks * BufferImageHeightInBlocks,
        //                              BlockBytesSize * BufferRowLengthInBlocks * BufferImageHeightInBlocks * ImageDepthInBlocks>
        const auto regionBlockStridesInBytes = dstImageTexelBlockInfo.convert3DBlockStridesTo1DByteStrides(regionBlockStrides);
        
        auto texelBlockDim = dstImageTexelBlockInfo.getDimension();

        const auto imageExtent = core::vector3du32_SIMD(region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth);
        const auto imageOffset = core::vector3du32_SIMD(region.imageOffset.x, region.imageOffset.y, region.imageOffset.z);
        const auto imageOffsetInBlocks = dstImageTexelBlockInfo.convertTexelsToBlocks(imageOffset);
        const auto imageExtentInBlocks = dstImageTexelBlockInfo.convertTexelsToBlocks(imageExtent);
        const auto imageExtentBlockStridesInBytes = dstImageTexelBlockInfo.convert3DBlockStridesTo1DByteStrides(imageExtentInBlocks);
             
        // region <-> region.imageSubresource.layerCount <-> imageExtentInBlocks.z <-> imageExtentInBlocks.y <-> imageExtentInBlocks.x
        auto updateCurrentOffsets = [&]() -> void
        {
            if(currentBlockInRow >= imageExtentInBlocks.x) 
            {
                currentBlockInRow = 0u;
                currentRowInSlice++;
            }
            if(currentRowInSlice >= imageExtentInBlocks.y)
            {
                assert(currentBlockInRow == 0);
                currentRowInSlice = 0u;
                currentSliceInLayer++;
            }
            if(currentSliceInLayer >= imageExtentInBlocks.z)
            {
                assert(currentBlockInRow == 0 && currentRowInSlice == 0);
                currentSliceInLayer = 0u;
                currentLayerInRegion++;
            }
            if(currentLayerInRegion >= region.imageSubresource.layerCount) 
            {
                assert(currentBlockInRow == 0 && currentRowInSlice == 0 && currentSliceInLayer == 0);
                currentLayerInRegion = 0u; 
                currentRegion++;
            }
        };

        uint32_t eachBlockNeededMemory  = imageExtentBlockStridesInBytes[0];  // = blockByteSize
        uint32_t eachRowNeededMemory    = imageExtentBlockStridesInBytes[1];  // = blockByteSize * imageExtentInBlocks.x
        uint32_t eachSliceNeededMemory  = imageExtentBlockStridesInBytes[2];  // = blockByteSize * imageExtentInBlocks.x * imageExtentInBlocks.y
        uint32_t eachLayerNeededMemory  = imageExtentBlockStridesInBytes[3];  // = blockByteSize * imageExtentInBlocks.x * imageExtentInBlocks.y * imageExtentInBlocks.z

        // There is remaining layers in region that needs copying
        uint32_t uploadableArrayLayers = availableMemory / eachLayerNeededMemory;
        uint32_t remainingLayers = region.imageSubresource.layerCount - currentLayerInRegion;
        uploadableArrayLayers = core::min(uploadableArrayLayers, remainingLayers);
        // A: There is remaining layers left in region -> Copy Slices (Depths)
        uint32_t uploadableSlices = availableMemory / eachSliceNeededMemory;
        uint32_t remainingSlices = imageExtentInBlocks.z - currentSliceInLayer;
        uploadableSlices = core::min(uploadableSlices, remainingSlices);
        if(uploadableSlices > 0 && minImageTransferGranularity.depth > 1u && (imageOffsetInBlocks.z + currentSliceInLayer + uploadableSlices) < subresourceSizeInBlocks.z)
            uploadableSlices = core::alignDown(uploadableSlices, minImageTransferGranularity.depth);
        // B: There is remaining slices left in layer -> Copy Rows
        uint32_t uploadableRows = availableMemory / eachRowNeededMemory;
        uint32_t remainingRows = imageExtentInBlocks.y - currentRowInSlice;
        uploadableRows = core::min(uploadableRows, remainingRows);
        if(uploadableRows > 0 && minImageTransferGranularity.height > 1u && (imageOffsetInBlocks.y + currentRowInSlice + uploadableRows) < subresourceSizeInBlocks.y)
            uploadableRows = core::alignDown(uploadableRows, minImageTransferGranularity.height);
        // C: There is remaining slices left in layer -> Copy Blocks
        uint32_t uploadableBlocks = availableMemory / eachBlockNeededMemory;
        uint32_t remainingBlocks = imageExtentInBlocks.x - currentBlockInRow;
        uploadableBlocks = core::min(uploadableBlocks, remainingBlocks);
        if(uploadableBlocks > 0 && minImageTransferGranularity.width > 1u && (imageOffsetInBlocks.x + currentBlockInRow + uploadableBlocks) < subresourceSizeInBlocks.x)
            uploadableBlocks = core::alignDown(uploadableBlocks, minImageTransferGranularity.width);


        if(currentBlockInRow == 0 && currentRowInSlice == 0 && currentSliceInLayer == 0 && uploadableArrayLayers > 0)
        {
            uint32_t layersToUploadMemorySize = eachLayerNeededMemory * uploadableArrayLayers;

            bool copySuccess = false;
            
            regionToCopyNext.bufferOffset = stagingBufferOffset;
            regionToCopyNext.bufferRowLength = imageExtent.x;
            regionToCopyNext.bufferImageHeight = imageExtent.y;
            regionToCopyNext.imageSubresource.aspectMask = region.imageSubresource.aspectMask;
            regionToCopyNext.imageSubresource.mipLevel = region.imageSubresource.mipLevel;
            regionToCopyNext.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer + currentLayerInRegion;
            regionToCopyNext.imageOffset.x = region.imageOffset.x + 0u;
            regionToCopyNext.imageOffset.y = region.imageOffset.y + 0u;
            regionToCopyNext.imageOffset.z = region.imageOffset.z + 0u;
            regionToCopyNext.imageExtent.width    = imageExtent.x;
            regionToCopyNext.imageExtent.height   = imageExtent.y;
            regionToCopyNext.imageExtent.depth    = imageExtent.z;
            regionToCopyNext.imageSubresource.layerCount = uploadableArrayLayers;

            auto dstImageParams = dstImage->getCreationParameters();
            
            auto cpuImageRegionsDynArray = core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<asset::ICPUImage::SBufferCopy>>(1);
            auto& cpuImageRegion = cpuImageRegionsDynArray->front();
            cpuImageRegion = {};
            cpuImageRegion.bufferOffset = 0u;
            cpuImageRegion.bufferRowLength = imageExtent.x;
            cpuImageRegion.bufferImageHeight = imageExtent.y;
            cpuImageRegion.imageSubresource.aspectMask = region.imageSubresource.aspectMask;
            cpuImageRegion.imageSubresource.mipLevel = 0u;
            cpuImageRegion.imageSubresource.baseArrayLayer = 0u;
            cpuImageRegion.imageOffset.x = 0u;
            cpuImageRegion.imageOffset.y = 0u;
            cpuImageRegion.imageOffset.z = 0u;
            cpuImageRegion.imageExtent.width    = imageExtent.x;
            cpuImageRegion.imageExtent.height   = imageExtent.y;
            cpuImageRegion.imageExtent.depth    = imageExtent.z;
            cpuImageRegion.imageSubresource.layerCount = uploadableArrayLayers;

            auto imageOffset = core::vector3du32_SIMD(regionToCopyNext.imageOffset.x, regionToCopyNext.imageOffset.y, regionToCopyNext.imageOffset.z);


            /*
                We have to first construct two `ICPUImage`s from each of those buffers let's call them `inCPUImage` and `outCPUImage`
                Then we will create fake ICPUBuffers that point to srcBufferData and stagingBufferPointer with correct offsets
                Then we have to set the buffer and regions for each one of those ICPUImages using setBufferAndRegions
                Finally we fill the filter state and `execute` which require in/out CPUImages
            */

            // inCPUImage is an image matching the params of dstImage but with the extents and layer count of the current region being copied and mipLevel 1u and the format being srcImageFormat
            // the buffer of this image is set to (srcBufferData+Offset) and the related region is set to cover the whole copy region (offset from 0)
            uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(imageOffset, srcBufferByteStrides)[0];
            uint8_t* inCpuBufferPointer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(srcBufferData) + offsetInCPUBuffer);
            asset::ICPUImage::SCreationParams inCPUImageParams = dstImageParams;
            inCPUImageParams.format = srcImageFormat;
            inCPUImageParams.extent = regionToCopyNext.imageExtent;
            inCPUImageParams.arrayLayers = regionToCopyNext.imageSubresource.layerCount;
            inCPUImageParams.mipLevels = 1u;
            core::smart_refctd_ptr<asset::ICPUImage> inCPUImage = asset::ICPUImage::create(std::move(inCPUImageParams));
            core::smart_refctd_ptr<asset::ICPUBuffer> inCPUBuffer = core::make_smart_refctd_ptr< asset::CCustomAllocatorCPUBuffer<core::null_allocator<uint8_t>, true> >(layersToUploadMemorySize/*TODO*/, inCpuBufferPointer, core::adopt_memory);
            inCPUImage->setBufferAndRegions(std::move(inCPUBuffer), cpuImageRegionsDynArray);

            // outCPUImage is an image matching the params of dstImage but with the extents and layer count of the current region being copied and mipLevel 1u
            // the buffer of this image is set to (stagingBufferPointer + stagingBufferOffset) and the rwlated region is set to cover the whole copy region (offset from 0)
            asset::ICPUImage::SCreationParams outCPUImageParams = dstImageParams;
            uint8_t* outCpuBufferPointer = reinterpret_cast<uint8_t*>(stagingBufferPointer) + stagingBufferOffset;
            outCPUImageParams.extent = regionToCopyNext.imageExtent;
            outCPUImageParams.arrayLayers = regionToCopyNext.imageSubresource.layerCount;
            outCPUImageParams.mipLevels = 1u;
            core::smart_refctd_ptr<asset::ICPUImage> outCPUImage = asset::ICPUImage::create(std::move(outCPUImageParams));
            core::smart_refctd_ptr<asset::ICPUBuffer> outCPUBuffer = core::make_smart_refctd_ptr<asset::CCustomAllocatorCPUBuffer<core::null_allocator<uint8_t>>>(layersToUploadMemorySize, outCpuBufferPointer, core::adopt_memory);
            outCPUImage->setBufferAndRegions(std::move(outCPUBuffer), cpuImageRegionsDynArray);

            // In = srcBuffer, Out = stagingBuffer
            if (srcImageFormat == dstImageFormat)
            {
                using CopyFilter = asset::CCopyImageFilter;
                CopyFilter copyFilter;
                CopyFilter::state_type state = {};

                state.extent = regionToCopyNext.imageExtent;
                state.layerCount = regionToCopyNext.imageSubresource.layerCount;

                state.inOffset.x = 0;
                state.inOffset.y = 0;
                state.inOffset.z = 0;
                state.inBaseLayer = 0u;
                state.inMipLevel = 0u;
                state.inImage = inCPUImage.get();

                state.outOffset.x = 0;
                state.outOffset.y = 0;
                state.outOffset.z = 0;
                state.outBaseLayer = 0u;
                state.outMipLevel = 0u;
                state.outImage = outCPUImage.get();

                if (copyFilter.execute(core::execution::par_unseq,&state))
                    copySuccess = true;
            }
            else
            {
                using ConverFilter = asset::CConvertFormatImageFilter<>;
                ConverFilter convertFilter;
                ConverFilter::state_type state = {};
                
                state.extent = regionToCopyNext.imageExtent;
                state.layerCount = regionToCopyNext.imageSubresource.layerCount;
                
                state.inOffset.x = 0;
                state.inOffset.y = 0;
                state.inOffset.z = 0;
                state.inBaseLayer = 0u;
                state.inMipLevel = 0u;
                state.inImage = inCPUImage.get();

                state.outOffset.x = 0;
                state.outOffset.y = 0;
                state.outOffset.z = 0;
                state.outBaseLayer = 0u;
                state.outMipLevel = 0u;
                state.outImage = outCPUImage.get();
            
                if (convertFilter.execute(core::execution::par_unseq,&state))
                    copySuccess = true;
            }

            if(copySuccess)
            {
                addToCurrentUploadBufferOffset(layersToUploadMemorySize);

                currentLayerInRegion += uploadableArrayLayers;
                updateCurrentOffsets();
                return true;
            }
            else
            {
                assert(false);
                return false;
            }
        }
        else if (currentBlockInRow == 0 && currentRowInSlice == 0 && canTransferMipLevelsPartially && uploadableSlices > 0)
        {
            // tryFillLayer();
            uint32_t slicesToUploadMemorySize = eachSliceNeededMemory * uploadableSlices;

            if(regionBlockStrides.x != imageExtentInBlocks.x)
            {
                // Can't copy all rows at once, there is more padding at the end of rows, copy row by row:
                for(uint32_t z = 0; z < uploadableSlices; ++z)
                {
                    for(uint32_t y = 0; y < imageExtentInBlocks.y; ++y)
                    {
                        auto localImageOffset = core::vector3du32_SIMD(0u, 0u + y, 0u + currentSliceInLayer + z, currentLayerInRegion);
                        uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(localImageOffset, regionBlockStridesInBytes)[0];
                        uint64_t offsetInUploadBuffer = stagingBufferOffset + z * eachSliceNeededMemory + y * eachRowNeededMemory;
                        memcpy( reinterpret_cast<uint8_t*>(stagingBufferPointer)+offsetInUploadBuffer,
                                reinterpret_cast<uint8_t const*>(srcBufferData)+offsetInCPUBuffer,
                                eachRowNeededMemory);
                    }
                }
            }
            else if (regionBlockStrides.y != imageExtentInBlocks.y)
            {
                assert(imageOffsetInBlocks.x == 0u);
                // Can't copy all slices at once, there is more padding at the end of slices, copy slice by slice
                for(uint32_t z = 0; z < uploadableSlices; ++z)
                {
                    auto localImageOffset = core::vector3du32_SIMD(0u, 0u, 0u + currentSliceInLayer + z, currentLayerInRegion);
                    uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(localImageOffset, regionBlockStridesInBytes)[0];
                    uint64_t offsetInUploadBuffer = stagingBufferOffset + z * eachSliceNeededMemory;
                    memcpy( reinterpret_cast<uint8_t*>(stagingBufferPointer)+offsetInUploadBuffer,
                            reinterpret_cast<uint8_t const*>(srcBufferData)+offsetInCPUBuffer,
                            eachSliceNeededMemory);
                }
            }
            else
            {
                // We can copy all arrays and slices at once, because imageExtent is fit to bufferRowLength and bufferImageHeight
                assert(imageOffsetInBlocks.x == 0u);
                assert(imageOffsetInBlocks.y == 0u);
                auto localImageOffset = core::vector3du32_SIMD(0u, 0u, 0u + currentSliceInLayer, currentLayerInRegion);
                uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(localImageOffset, regionBlockStridesInBytes)[0];
                uint64_t offsetInUploadBuffer = stagingBufferOffset;
                memcpy( reinterpret_cast<uint8_t*>(stagingBufferPointer)+offsetInUploadBuffer,
                        reinterpret_cast<uint8_t const*>(srcBufferData)+offsetInCPUBuffer,
                        slicesToUploadMemorySize);
            }
                                
            regionToCopyNext.bufferOffset = stagingBufferOffset;
            regionToCopyNext.bufferRowLength = imageExtentInBlocks.x * texelBlockDim.x;
            regionToCopyNext.bufferImageHeight = imageExtentInBlocks.y * texelBlockDim.y;
            regionToCopyNext.imageSubresource.aspectMask = region.imageSubresource.aspectMask;
            regionToCopyNext.imageSubresource.mipLevel = region.imageSubresource.mipLevel;
            regionToCopyNext.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer + currentLayerInRegion;
            regionToCopyNext.imageOffset.x = region.imageOffset.x + 0u; assert(currentBlockInRow == 0);
            regionToCopyNext.imageOffset.y = region.imageOffset.y + 0u; assert(currentRowInSlice == 0);
            regionToCopyNext.imageOffset.z = region.imageOffset.z + currentSliceInLayer * texelBlockDim.z;
            regionToCopyNext.imageExtent.width    = imageExtent.x;
            regionToCopyNext.imageExtent.height   = imageExtent.y;
            regionToCopyNext.imageExtent.depth    = core::min(uploadableSlices * texelBlockDim.z, imageExtent.z);
            regionToCopyNext.imageSubresource.layerCount = 1u;

            addToCurrentUploadBufferOffset(slicesToUploadMemorySize);

            currentSliceInLayer += uploadableSlices;
            updateCurrentOffsets();
            return true;
        }
        else if (currentBlockInRow == 0 && canTransferMipLevelsPartially && uploadableRows > 0)
        {
            // tryFillSlice();
            uint32_t rowsToUploadMemorySize = eachRowNeededMemory * uploadableRows;
                                
            if(regionBlockStrides.x != imageExtentInBlocks.x)
            {
                // Can't copy all rows at once, there is padding, copy row by row
                for(uint32_t y = 0; y < uploadableRows; ++y)
                {
                    auto localImageOffset = core::vector3du32_SIMD(0u, 0u + currentRowInSlice + y, 0u + currentSliceInLayer, currentLayerInRegion);
                    uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(localImageOffset, regionBlockStridesInBytes)[0];
                    uint64_t offsetInUploadBuffer = stagingBufferOffset + y*eachRowNeededMemory;
                    memcpy( reinterpret_cast<uint8_t*>(stagingBufferPointer)+offsetInUploadBuffer,
                            reinterpret_cast<uint8_t const*>(srcBufferData)+offsetInCPUBuffer,
                            eachRowNeededMemory);
                }
            }
            else
            {
                // We can copy all rows at once, because imageExtent is fit to rowLength
                assert(imageOffsetInBlocks.x == 0);
                auto localImageOffset = core::vector3du32_SIMD(0u, 0u + currentRowInSlice, 0u + currentSliceInLayer, currentLayerInRegion);
                uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(localImageOffset, regionBlockStridesInBytes)[0];
                uint64_t offsetInUploadBuffer = stagingBufferOffset;
                memcpy( reinterpret_cast<uint8_t*>(stagingBufferPointer)+offsetInUploadBuffer,
                        reinterpret_cast<uint8_t const*>(srcBufferData)+offsetInCPUBuffer,
                        rowsToUploadMemorySize);
            }

            regionToCopyNext.bufferOffset = stagingBufferOffset;
            regionToCopyNext.bufferRowLength = imageExtentInBlocks.x * texelBlockDim.x;
            regionToCopyNext.bufferImageHeight = imageExtentInBlocks.y * texelBlockDim.y;
            regionToCopyNext.imageSubresource.aspectMask = region.imageSubresource.aspectMask;
            regionToCopyNext.imageSubresource.mipLevel = region.imageSubresource.mipLevel;
            regionToCopyNext.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer + currentLayerInRegion;
            regionToCopyNext.imageOffset.x = region.imageOffset.x + 0u; assert(currentBlockInRow == 0);
            regionToCopyNext.imageOffset.y = region.imageOffset.y + currentRowInSlice * texelBlockDim.y;
            regionToCopyNext.imageOffset.z = region.imageOffset.z + currentSliceInLayer * texelBlockDim.z;
            regionToCopyNext.imageExtent.width    = imageExtent.x;
            regionToCopyNext.imageExtent.height   = core::min(uploadableRows * texelBlockDim.y, imageExtent.y);
            regionToCopyNext.imageExtent.depth    = core::min(1u * texelBlockDim.z, imageExtent.z);
            regionToCopyNext.imageSubresource.layerCount = 1u;

            addToCurrentUploadBufferOffset(rowsToUploadMemorySize);

            currentRowInSlice += uploadableRows;
            updateCurrentOffsets();
            return true;
        }
        else if (canTransferMipLevelsPartially && uploadableBlocks > 0)
        {
            // tryFillRow();
            uint32_t blocksToUploadMemorySize = eachBlockNeededMemory * uploadableBlocks;
            auto localImageOffset = core::vector3du32_SIMD(0u + currentBlockInRow, 0u + currentRowInSlice, 0u + currentSliceInLayer, currentLayerInRegion);
            uint64_t offsetInCPUBuffer = region.bufferOffset + core::dot(localImageOffset, regionBlockStridesInBytes)[0];
            memcpy( reinterpret_cast<uint8_t*>(stagingBufferPointer)+stagingBufferOffset,
                    reinterpret_cast<uint8_t const*>(srcBufferData)+offsetInCPUBuffer,
                    blocksToUploadMemorySize);
                
            regionToCopyNext.bufferOffset = stagingBufferOffset;
            regionToCopyNext.bufferRowLength = imageExtentInBlocks.x * texelBlockDim.x;
            regionToCopyNext.bufferImageHeight = imageExtentInBlocks.y * texelBlockDim.y;
            regionToCopyNext.imageSubresource.aspectMask = region.imageSubresource.aspectMask;
            regionToCopyNext.imageSubresource.mipLevel = region.imageSubresource.mipLevel;
            regionToCopyNext.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer + currentLayerInRegion;
            regionToCopyNext.imageOffset.x = region.imageOffset.x + currentBlockInRow * texelBlockDim.x;
            regionToCopyNext.imageOffset.y = region.imageOffset.y + currentRowInSlice * texelBlockDim.y;
            regionToCopyNext.imageOffset.z = region.imageOffset.z + currentSliceInLayer * texelBlockDim.z;
            regionToCopyNext.imageExtent.width    = core::min(uploadableBlocks * texelBlockDim.x, imageExtent.x);
            regionToCopyNext.imageExtent.height   = core::min(1u * texelBlockDim.y, imageExtent.y);
            regionToCopyNext.imageExtent.depth    = core::min(1u * texelBlockDim.z, imageExtent.z);
            regionToCopyNext.imageSubresource.layerCount = 1u;

            addToCurrentUploadBufferOffset(blocksToUploadMemorySize);

            currentBlockInRow += uploadableBlocks;
            updateCurrentOffsets();
            return true;
        }
        else
        {
            assert(false);
            return false;
        }
    }

    // ! returns true when there is no more regions left over to copy
    bool isFinished() const 
    {
        return currentRegion == regions.size();
    }

    uint32_t getCurrentBlockInRow() const { return currentBlockInRow; }
    uint32_t getCurrentRowInSlice() const { return currentRowInSlice; }
    uint32_t getCurrentSliceInLayer() const { return currentSliceInLayer; }
    uint32_t getCurrentLayerInRegion() const { return currentLayerInRegion; }
    uint32_t getCurrentRegion() const { return currentRegion; }

private:
    core::SRange<const asset::IImage::SBufferCopy> regions;

    bool canTransferMipLevelsPartially = false;
    VkExtent3D minImageTransferGranularity;
    uint32_t bufferOffsetAlignment = 1u;

    asset::E_FORMAT srcImageFormat;
    asset::E_FORMAT dstImageFormat;
    const void* srcBufferData;
    video::IGPUImage* const dstImage;
    
    // Block Offsets 
    // (1 blockInRow = texelBlockDimensions.x texels)
    // (1 rowInSlice = texelBlockDimensions.y texel rows)
    // (1 sliceInLayer = texelBlockDimensions.z texel depths)
    uint32_t currentBlockInRow = 0u;
    uint32_t currentRowInSlice = 0u;
    uint32_t currentSliceInLayer = 0u;
    uint32_t currentLayerInRegion = 0u;
    uint32_t currentRegion = 0u;
};

void IUtilities::updateImageViaStagingBuffer(
    IGPUCommandBuffer* cmdbuf, IGPUFence* fence, IGPUQueue* queue,
    asset::ICPUBuffer const* srcBuffer, const core::SRange<const asset::IImage::SBufferCopy>& regions, video::IGPUImage* dstImage, asset::E_IMAGE_LAYOUT dstImageLayout,
    uint32_t& waitSemaphoreCount, IGPUSemaphore*const * &semaphoresToWaitBeforeOverwrite, const asset::E_PIPELINE_STAGE_FLAGS* &stagesToWaitForPerSemaphore)
{
    const auto& limits = m_device->getPhysicalDevice()->getLimits();
    const uint32_t allocationAlignment = static_cast<uint32_t>(limits.nonCoherentAtomSize);

    auto* cmdpool = cmdbuf->getPool();
    
    if(cmdbuf == nullptr || fence == nullptr || queue == nullptr || regions.size() == 0 || dstImage == nullptr || (srcBuffer == nullptr || srcBuffer->getPointer() == nullptr))
    {
        assert(false);
        return;
    }

    assert(cmdbuf->isResettable());
    assert(cmdpool->getQueueFamilyIndex()==queue->getFamilyIndex());
    assert(cmdbuf->getRecordingFlags().hasFlags(IGPUCommandBuffer::EU_ONE_TIME_SUBMIT_BIT));
    if(dstImage->getCreationParameters().samples != asset::IImage::ESCF_1_BIT)
    {
        _NBL_TODO("Erfan hasn't figured out yet how to copy to multisampled images");
    }

    auto texelBlockInfo = asset::TexelBlockInfo(dstImage->getCreationParameters().format);
    auto queueFamProps = m_device->getPhysicalDevice()->getQueueFamilyProperties()[queue->getFamilyIndex()];
    auto minImageTransferGranularity = queueFamProps.minImageTransferGranularity;

    ImageRegionIterator regionIterator = ImageRegionIterator(regions, queueFamProps, srcBuffer->getPointer(), /*TODO*/dstImage->getCreationParameters().format, dstImage);

    // Assuming each thread can handle minImageTranferGranularitySize of texelBlocks:
    const uint32_t maxResidentImageTransferSize = limits.maxResidentInvocations * texelBlockInfo.getBlockByteSize() * (minImageTransferGranularity.width * minImageTransferGranularity.height * minImageTransferGranularity.depth); 

    while (!regionIterator.isFinished())
    {
        size_t memoryNeededForRemainingRegions = regionIterator.getMemoryNeededForRemainingRegions();

        // memoryLowerBound = max(maxResidentImageTransferSize, the largest rowPitch of regions); 
        uint32_t memoryLowerBound = maxResidentImageTransferSize;
        for (uint32_t i = regionIterator.getCurrentRegion(); i < regions.size(); ++i)
        {
            const asset::IImage::SBufferCopy & region = regions[i];
            auto imageExtent = core::vector3du32_SIMD(region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth);
            auto imageExtentInBlocks = texelBlockInfo.convertTexelsToBlocks(imageExtent);
            auto imageExtentBlockStridesInBytes = texelBlockInfo.convert3DBlockStridesTo1DByteStrides(imageExtentInBlocks);
            memoryLowerBound = core::max(memoryLowerBound, imageExtentBlockStridesInBytes[1]); // rowPitch = imageExtentBlockStridesInBytes[1]
        }

        uint32_t localOffset = video::StreamingTransientDataBufferMT<>::invalid_value;
        uint32_t maxFreeBlock = m_defaultUploadBuffer.get()->max_size();
        const uint32_t allocationSize = getAllocationSizeForStreamingBuffer(memoryNeededForRemainingRegions, allocationAlignment, maxFreeBlock, memoryLowerBound);
        // cannot use `multi_place` because of the extra padding size we could have added
        m_defaultUploadBuffer.get()->multi_allocate(std::chrono::steady_clock::now()+std::chrono::microseconds(500u), 1u, &localOffset, &allocationSize, &allocationAlignment);
        bool failedAllocation = (localOffset == video::StreamingTransientDataBufferMT<>::invalid_value);

        // keep trying again
        if (failedAllocation)
        {
            // but first sumbit the already buffered up copies
            cmdbuf->end();
            IGPUQueue::SSubmitInfo submit;
            submit.commandBufferCount = 1u;
            submit.commandBuffers = &cmdbuf;
            submit.signalSemaphoreCount = 0u;
            submit.pSignalSemaphores = nullptr;
            assert(!waitSemaphoreCount || semaphoresToWaitBeforeOverwrite && stagesToWaitForPerSemaphore);
            submit.waitSemaphoreCount = waitSemaphoreCount;
            submit.pWaitSemaphores = semaphoresToWaitBeforeOverwrite;
            submit.pWaitDstStageMask = stagesToWaitForPerSemaphore;
            queue->submit(1u, &submit, fence);
            m_device->blockForFences(1u, &fence);
            waitSemaphoreCount = 0u;
            semaphoresToWaitBeforeOverwrite = nullptr;
            stagesToWaitForPerSemaphore = nullptr;
            // before resetting we need poll all events in the allocator's deferred free list
            m_defaultUploadBuffer->cull_frees();
            // we can reset the fence and commandbuffer because we fully wait for the GPU to finish here
            m_device->resetFences(1u, &fence);
            cmdbuf->reset(IGPUCommandBuffer::ERF_RELEASE_RESOURCES_BIT);
            cmdbuf->begin(IGPUCommandBuffer::EU_ONE_TIME_SUBMIT_BIT);
            continue;
        }
        else
        {
            size_t currentUploadBufferOffset = localOffset;
            size_t availableUploadBufferMemory = allocationSize;

            bool anyTransferRecorded = false;
            core::vector<asset::IImage::SBufferCopy> regionsToCopy;

            // Worst case iterations: remaining blocks --> remaining rows --> remaining slices --> full layers
            const uint32_t maxIterations = regions.size() * 4u; 
            for (uint32_t d = 0u; d < maxIterations && !regionIterator.isFinished(); ++d)
            {
                asset::IImage::SBufferCopy nextRegionToCopy = {};
                if(availableUploadBufferMemory > 0u && regionIterator.advanceAndCopyToStagingBuffer(nextRegionToCopy, availableUploadBufferMemory, currentUploadBufferOffset, m_defaultUploadBuffer->getBufferPointer()))
                {
                    anyTransferRecorded = true;
                    regionsToCopy.push_back(nextRegionToCopy);
                }
                else
                    break;
            }

            if(!regionsToCopy.empty())
            {
                cmdbuf->copyBufferToImage(m_defaultUploadBuffer.get()->getBuffer(), dstImage, dstImageLayout, regionsToCopy.size(), regionsToCopy.data());
            }

            assert(anyTransferRecorded && "allocationSize is not enough to support the smallest possible transferable units to image, may be caused if your queueFam's minImageTransferGranularity is large or equal to <0,0,0>.");
            
            // some platforms expose non-coherent host-visible GPU memory, so writes need to be flushed explicitly
            if (m_defaultUploadBuffer.get()->needsManualFlushOrInvalidate()) {
                IDeviceMemoryAllocation::MappedMemoryRange flushRange(m_defaultUploadBuffer.get()->getBuffer()->getBoundMemory(), localOffset, allocationSize);
                m_device->flushMappedMemoryRanges(1u, &flushRange);
            }
        }

        // this doesn't actually free the memory, the memory is queued up to be freed only after the GPU fence/event is signalled
        m_defaultUploadBuffer.get()->multi_deallocate(1u, &localOffset, &allocationSize, core::smart_refctd_ptr<IGPUFence>(fence), &cmdbuf); // can queue with a reset but not yet pending fence, just fine
    }
}

void IUtilities::updateImageViaStagingBuffer(
    IGPUFence* fence, IGPUQueue* queue,
    asset::ICPUBuffer const* srcBuffer, const core::SRange<const asset::IImage::SBufferCopy>& regions, video::IGPUImage* dstImage, asset::E_IMAGE_LAYOUT dstImageLayout,
    uint32_t waitSemaphoreCount, IGPUSemaphore* const* semaphoresToWaitBeforeOverwrite, const asset::E_PIPELINE_STAGE_FLAGS* stagesToWaitForPerSemaphore,
    const uint32_t signalSemaphoreCount, IGPUSemaphore* const* semaphoresToSignal
)
{
    core::smart_refctd_ptr<IGPUCommandPool> pool = m_device->createCommandPool(queue->getFamilyIndex(),IGPUCommandPool::ECF_RESET_COMMAND_BUFFER_BIT);
    core::smart_refctd_ptr<IGPUCommandBuffer> cmdbuf;
    m_device->createCommandBuffers(pool.get(),IGPUCommandBuffer::EL_PRIMARY,1u,&cmdbuf);
    assert(cmdbuf);
    cmdbuf->begin(IGPUCommandBuffer::EU_ONE_TIME_SUBMIT_BIT);
    updateImageViaStagingBuffer(cmdbuf.get(),fence,queue,srcBuffer,regions,dstImage,dstImageLayout,waitSemaphoreCount,semaphoresToWaitBeforeOverwrite,stagesToWaitForPerSemaphore);
    cmdbuf->end();
    IGPUQueue::SSubmitInfo submit;
    submit.commandBufferCount = 1u;
    submit.commandBuffers = &cmdbuf.get();
    submit.signalSemaphoreCount = signalSemaphoreCount;
    submit.pSignalSemaphores = semaphoresToSignal;
    assert(!waitSemaphoreCount || semaphoresToWaitBeforeOverwrite && stagesToWaitForPerSemaphore);
    submit.waitSemaphoreCount = waitSemaphoreCount;
    submit.pWaitSemaphores = semaphoresToWaitBeforeOverwrite;
    submit.pWaitDstStageMask = stagesToWaitForPerSemaphore;
    queue->submit(1u,&submit,fence);
}

void IUtilities::updateImageViaStagingBuffer(
    IGPUQueue* queue,
    asset::ICPUBuffer const* srcBuffer, const core::SRange<const asset::IImage::SBufferCopy>& regions, video::IGPUImage* dstImage, asset::E_IMAGE_LAYOUT dstImageLayout,
    uint32_t waitSemaphoreCount, IGPUSemaphore* const* semaphoresToWaitBeforeOverwrite, const asset::E_PIPELINE_STAGE_FLAGS* stagesToWaitForPerSemaphore,
    const uint32_t signalSemaphoreCount, IGPUSemaphore* const* semaphoresToSignal
)
{
    auto fence = m_device->createFence(static_cast<IGPUFence::E_CREATE_FLAGS>(0));
    updateImageViaStagingBuffer(fence.get(),queue,srcBuffer,regions,dstImage,dstImageLayout,waitSemaphoreCount,semaphoresToWaitBeforeOverwrite,stagesToWaitForPerSemaphore,signalSemaphoreCount,semaphoresToSignal);
    auto* fenceptr = fence.get();
    m_device->blockForFences(1u,&fenceptr);
}

} // namespace nbl::video