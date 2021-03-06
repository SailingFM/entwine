/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <vector>

#include <entwine/types/point-pool.hpp>

namespace entwine
{

class Bounds;
class Metadata;
class Schema;

class PointInfo
{
public:
    PointInfo(const Point& point, const char* data, uint64_t tick = 0)
        : m_point(point)
        , m_data(data)
        , m_tick(tick)
    { }

    PointInfo(uint64_t tick) : m_tick(tick) { }

    const Point& point() const { return m_point; }
    const char* data() const { return m_data; }
    uint64_t tick() const { return m_tick; }

    bool operator<(const PointInfo& other) const
    {
        return m_tick < other.m_tick;
    }

private:
    Point m_point;
    const char* m_data = nullptr;
    uint64_t m_tick;
};

// Ordered by Z-tick to perform the tubular-quadtree-as-octree query.
class ChunkReader
{
public:
    ChunkReader(
            const Metadata& metadata,
            const Id& id,
            std::size_t depth,
            std::unique_ptr<std::vector<char>> data);

    using PointOrder = std::vector<PointInfo>;
    using It = PointOrder::const_iterator;

    struct QueryRange
    {
        QueryRange(It begin, It end) : begin(begin), end(end) { }

        It begin;
        It end;
    };

    QueryRange candidates(const Bounds& queryBounds) const;
    std::size_t size() const
    {
        if (m_data) return m_data->size();
        else throw std::runtime_error("No ChunkReader data:" + m_id.str());
    }

    const Id& id() const { return m_id; }

private:
    const Schema& schema() const { return m_schema; }

    std::size_t normalize(const Id& rawIndex) const
    {
        return (rawIndex - m_id).getSimple();
    }

    const Schema& m_schema;
    const Bounds& m_bounds;
    const Id m_id;
    const std::size_t m_depth;

    std::unique_ptr<std::vector<char>> m_data;
    std::vector<PointInfo> m_points;
};

// Ordered by normal BaseChunk ordering for traversal.
class BaseChunkReader
{
public:
    BaseChunkReader(
            const Metadata& metadata,
            const Schema& celledSchema,
            const Id& id,
            std::unique_ptr<std::vector<char>> data);

    using TubeData = std::vector<PointInfo>;

    const TubeData& getTubeData(const Id& id) const
    {
        return m_tubes.at(normalize(id));
    }

private:
    std::size_t normalize(const Id& rawIndex) const
    {
        return (rawIndex - m_id).getSimple();
    }

    const Id m_id;
    std::unique_ptr<std::vector<char>> m_data;
    std::vector<TubeData> m_tubes;
};

} // namespace entwine

