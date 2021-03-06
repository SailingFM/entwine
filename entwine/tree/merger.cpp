/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#include <entwine/tree/builder.hpp>
#include <entwine/tree/merger.hpp>
#include <entwine/tree/thread-pools.hpp>
#include <entwine/types/manifest.hpp>
#include <entwine/types/metadata.hpp>
#include <entwine/types/subset.hpp>
#include <entwine/util/unique.hpp>

namespace entwine
{

Merger::Merger(
        const std::string path,
        const std::size_t threads,
        const std::size_t* subsetId,
        const bool verbose,
        std::shared_ptr<arbiter::Arbiter> arbiter)
    : m_builder()
    , m_path(path)
    , m_others()
    , m_threads(threads)
    , m_outerScope(makeUnique<OuterScope>())
    , m_pos(0)
    , m_verbose(verbose)
{
    m_outerScope->setArbiter(arbiter);

    m_builder = Builder::create(
            path,
            threads,
            subsetId,
            *m_outerScope);

    if (!m_builder)
    {
        if (m_verbose)
        {
            std::cout << "No builder for " << path;
            if (subsetId) std::cout << " at " << *subsetId;
            std::cout << std::endl;
        }

        throw std::runtime_error("Path not mergeable");
    }
    else if (!m_builder->metadata().subset())
    {
        throw std::runtime_error(
                "This path is already whole - no merge needed");
    }

    m_builder->verbose(m_verbose);
    m_outerScope->setPointPool(m_builder->sharedPointPool());
    m_outerScope->setHierarchyPool(m_builder->sharedHierarchyPool());

    if (!subsetId)
    {
        if (const Subset* subset = m_builder->metadata().subset())
        {
            // No subset ID was passed, so we will want to merge everything -
            // and we have already awakened subset 0.
            for (std::size_t i(1); i < subset->of(); ++i)
            {
                m_others.push_back(i);
            }
        }
    }

    if (m_verbose)
    {
        std::cout << "Awakened 1 / " << m_others.size() + 1 << std::endl;
    }
}

Merger::~Merger() { }

void Merger::go()
{
    const std::size_t total(m_others.size() + 1);

    for (const auto id : m_others)
    {
        m_pos = id + 1;

        if (m_verbose)
        {
            std::cout << "Merging " << m_pos << " / " << total << std::endl;
        }

        auto current(
                Builder::create(
                    m_path,
                    m_threads,
                    &id,
                    *m_outerScope));

        if (!current) throw std::runtime_error("Couldn't create subset");

        current->verbose(m_verbose);
        m_builder->merge(*current);
    }

    m_builder->makeWhole();

    if (m_verbose) std::cout << "Merge complete.  Saving..." << std::endl;
    m_builder->save();
    m_builder.reset();
    if (m_verbose) std::cout << "\tFinal save complete." << std::endl;
}

} // namespace entwine

