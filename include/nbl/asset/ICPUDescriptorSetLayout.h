// Copyright (C) 2018-2024 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h
#ifndef _NBL_ASSET_I_CPU_DESCRIPTOR_SET_LAYOUT_H_INCLUDED_
#define _NBL_ASSET_I_CPU_DESCRIPTOR_SET_LAYOUT_H_INCLUDED_

#include "nbl/asset/IDescriptorSetLayout.h"
#include "nbl/asset/IAsset.h"
#include "nbl/asset/ICPUSampler.h"

namespace nbl::asset
{

//! CPU Version of Descriptor Set Layout
/*
    @see IDescriptorSetLayout
    @see IAsset
*/

class ICPUDescriptorSetLayout : public IDescriptorSetLayout<ICPUSampler>, public IAsset
{
    using base_t = asset::IDescriptorSetLayout<ICPUSampler>;

	public:
		_NBL_STATIC_INLINE_CONSTEXPR uint32_t IMMUTABLE_SAMPLER_HIERARCHYLEVELS_BELOW = 1u;

        ICPUDescriptorSetLayout(const SBinding* const _begin, const SBinding* const _end) : base_t({_begin,_end}) {}

        core::smart_refctd_ptr<IAsset> clone(uint32_t _depth = ~0u) const override
        {
            auto cp = core::make_smart_refctd_ptr<ICPUDescriptorSetLayout>(nullptr,nullptr);

            for (uint32_t t = 0; t < static_cast<uint32_t>(asset::IDescriptor::E_TYPE::ET_COUNT); ++t)
                cp->m_descriptorRedirects[t] = m_descriptorRedirects[t].clone();
            cp->m_immutableSamplerRedirect = m_immutableSamplerRedirect.clone();
            cp->m_mutableSamplerRedirect = m_mutableSamplerRedirect.clone();

            if (m_samplers)
            {
                cp->m_samplers = core::make_refctd_dynamic_array<decltype(m_samplers)>(m_samplers->size());

                if (_depth > 0u)
                {
                    for (size_t i = 0ull; i < m_samplers->size(); ++i)
                        (*cp->m_samplers)[i] = core::smart_refctd_ptr_static_cast<ICPUSampler>((*m_samplers)[i]->clone(_depth - 1u));
                }
                else
                {
                    std::copy(m_samplers->begin(), m_samplers->end(), cp->m_samplers->begin());
                }
            }

            return cp;
        }

        constexpr static inline bool HasDependents = true;

        constexpr static inline auto AssetType = ET_DESCRIPTOR_SET_LAYOUT;
        inline E_TYPE getAssetType() const override { return AssetType; }

	protected:
		virtual ~ICPUDescriptorSetLayout() = default;
};

}
#endif