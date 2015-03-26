/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#include <entwine/tree/sleepy-tree.hpp>

#include <pdal/Dimension.hpp>
#include <pdal/PointView.hpp>
#include <pdal/Utils.hpp>

#include <entwine/tree/roller.hpp>
#include <entwine/tree/registry.hpp>
#include <entwine/types/bbox.hpp>
#include <entwine/types/linking-point-view.hpp>
#include <entwine/types/schema.hpp>
#include <entwine/types/single-point-table.hpp>

namespace entwine
{

SleepyTree::SleepyTree(
        const std::string& path,
        const BBox& bbox,
        const Schema& schema,
        const std::size_t dimensions,
        const std::size_t baseDepth,
        const std::size_t flatDepth,
        const std::size_t diskDepth)
    : m_path(path)
    , m_bbox(new BBox(bbox))
    , m_schema(new Schema(schema))
    , m_originId(m_schema->pdalLayout()->findProprietaryDim("Origin"))
    , m_dimensions(dimensions)
    , m_numPoints(0)
    , m_numTossed(0)
    , m_registry(
            new Registry(
                m_path,
                *m_schema.get(),
                dimensions,
                baseDepth,
                flatDepth,
                diskDepth))
{
    if (m_dimensions != 2)
    {
        // TODO
        throw std::runtime_error("TODO - Only 2 dimensions so far");
    }
}


SleepyTree::SleepyTree(const std::string& path)
    : m_path(path)
    , m_bbox()
    , m_schema()
    , m_dimensions(0)
    , m_numPoints(0)
    , m_numTossed(0)
    , m_registry()
{
    load();
}

SleepyTree::~SleepyTree()
{ }

void SleepyTree::insert(pdal::PointView& pointView, Origin origin)
{
    Point point;

    for (std::size_t i = 0; i < pointView.size(); ++i)
    {
        point.x = pointView.getFieldAs<double>(pdal::Dimension::Id::X, i);
        point.y = pointView.getFieldAs<double>(pdal::Dimension::Id::Y, i);

        if (m_bbox->contains(point))
        {
            Roller roller(*m_bbox.get());

            pointView.setField(m_originId, i, origin);

            PointInfo* pointInfo(
                    new PointInfo(
                        new Point(point),
                        pointView.getPoint(i),
                        m_schema->pointSize()));

            if (m_registry->addPoint(&pointInfo, roller))
            {
                ++m_numPoints;
            }
            else
            {
                ++m_numTossed;
            }
        }
    }
}

void SleepyTree::save()
{
    Json::Value jsonMeta;
    jsonMeta["bbox"] = m_bbox->toJson();
    jsonMeta["schema"] = m_schema->toJson();
    jsonMeta["dimensions"] = static_cast<Json::UInt64>(m_dimensions);
    jsonMeta["numPoints"] = static_cast<Json::UInt64>(m_numPoints);
    jsonMeta["numTossed"] = static_cast<Json::UInt64>(m_numTossed);

    m_registry->save(m_path, jsonMeta["registry"]);

    std::ofstream metaStream(
            metaPath(),
            std::ofstream::out | std::ofstream::trunc);
    if (!metaStream.good())
    {
        throw std::runtime_error("Could not open " + metaPath());
    }

    const std::string metaString(jsonMeta.toStyledString());
    metaStream.write(metaString.data(), metaString.size());
}

void SleepyTree::load()
{
    Json::Value meta;

    {
        Json::Reader reader;
        std::ifstream metaStream(metaPath());
        if (!metaStream.good())
        {
            throw std::runtime_error("Could not open " + metaPath());
        }

        reader.parse(metaStream, meta, false);
    }

    m_bbox.reset(new BBox(BBox::fromJson(meta["bbox"])));
    m_schema.reset(new Schema(Schema::fromJson(meta["schema"])));
    m_originId = m_schema->pdalLayout()->findProprietaryDim("Origin");
    m_dimensions = meta["dimensions"].asUInt64();
    m_numPoints = meta["numPoints"].asUInt64();
    m_numTossed = meta["numTossed"].asUInt64();

    m_registry.reset(
            new Registry(
                m_path,
                *m_schema.get(),
                m_dimensions,
                meta["registry"]));
}

const BBox& SleepyTree::getBounds() const
{
    return *m_bbox.get();
}

std::vector<std::size_t> SleepyTree::query(
        const std::size_t depthBegin,
        const std::size_t depthEnd)
{
    Roller roller(*m_bbox.get());
    std::vector<std::size_t> results;
    m_registry->query(roller, results, depthBegin, depthEnd);
    return results;
}

std::vector<std::size_t> SleepyTree::query(
        const BBox& bbox,
        const std::size_t depthBegin,
        const std::size_t depthEnd)
{
    Roller roller(*m_bbox.get());
    std::vector<std::size_t> results;
    m_registry->query(roller, results, bbox, depthBegin, depthEnd);
    return results;
}

std::vector<char> SleepyTree::getPointData(
        const std::size_t index,
        const Schema& reqSchema)
{
    std::vector<char> schemaPoint;
    std::vector<char> nativePoint(m_registry->getPointData(index));

    if (nativePoint.size())
    {
        schemaPoint.resize(reqSchema.pointSize());

        SinglePointTable table(schema(), nativePoint.data());
        LinkingPointView view(table);

        char* pos(schemaPoint.data());

        for (const auto& reqDim : reqSchema.dims())
        {
            view.getField(pos, reqDim.id(), reqDim.type(), 0);
            pos += reqDim.size();
        }
    }

    return schemaPoint;
}

const Schema& SleepyTree::schema() const
{
    return *m_schema.get();
}

std::size_t SleepyTree::numPoints() const
{
    return m_numPoints;
}

std::string SleepyTree::path() const
{
    return m_path;
}

std::string SleepyTree::metaPath() const
{
    return m_path + "/meta";
}

} // namespace entwine

