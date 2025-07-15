//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2023 LucKey Productions.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/Graphics.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../2D/Sprite2D.h"
#include "../2D/TmxFile2D.h"
#include "../Math/AreaAllocator.h"

#include "../DebugNew.h"


namespace Dry
{

extern const float PIXEL_SIZE;

TmxLayer2D::TmxLayer2D(TmxFile2D* tmxFile, TileMapLayerType2D type):
    tmxFile_{ tmxFile },
    type_{ type },
    name_{},
    width_{},
    height_{},
    visible_{},
    propertySet_{ nullptr }
{
}

TmxFile2D* TmxLayer2D::GetTmxFile() const
{
    return tmxFile_;
}

bool TmxLayer2D::HasProperty(const String& name) const
{
    if (!propertySet_)
        return false;

    return propertySet_->HasProperty(name);
}

const String& TmxLayer2D::GetProperty(const String& name) const
{
    if (!propertySet_)
        return String::EMPTY;

    return propertySet_->GetProperty(name);
}

void TmxLayer2D::LoadInfo(const XMLElement& element)
{
    name_ = element.GetAttribute("name");
    width_ = element.GetInt("width");
    height_ = element.GetInt("height");

    if (element.HasAttribute("visible"))
        visible_ = element.GetInt("visible") != 0;
    else
        visible_ = true;
}

void TmxLayer2D::LoadPropertySet(const XMLElement& element)
{
    propertySet_ = new PropertySet2D{};
    propertySet_->Load(element);
}

TmxTileLayer2D::TmxTileLayer2D(TmxFile2D* tmxFile): TmxLayer2D(tmxFile, LT_TILE_LAYER),
    tiles_{}
{
}

bool TmxTileLayer2D::Load(const XMLElement& element, const TileMapInfo2D& /*info*/)
{
    LoadInfo(element);

    XMLElement dataElem{ element.GetChild("data") };
    if (!dataElem)
    {
        DRY_LOGERROR("Could not find data in layer");
        return false;
    }

    LayerEncoding encoding;

    if (dataElem.HasAttribute("compression"))
    {
        DRY_LOGERROR("Compression not supported now");
        return false;
    }

    if (dataElem.HasAttribute("encoding"))
    {
        const String encodingAttribute{ dataElem.GetAttribute("encoding") };

        if (encodingAttribute == "xml")
        {
            encoding = XML;
        }
        else if (encodingAttribute == "csv")
        {
            encoding = CSV;
        }
        else if (encodingAttribute == "base64")
        {
            encoding = Base64;
        }
        else
        {
            DRY_LOGERROR("Invalid encoding: " + encodingAttribute);
            return false;
        }
    }
    else
    {
        encoding = XML;
    }

    tiles_.Resize(static_cast<unsigned>(width_ * height_));

    if (encoding == XML)
    {
        XMLElement tileElem{ dataElem.GetChild("tile") };

        for (int y{ 0 }; y < height_; ++y)
        {
            for (int x{ 0 }; x < width_; ++x)
            {
                if (!tileElem)
                    return false;

                const unsigned gid{ tileElem.GetUInt("gid") };

                if (gid != 0u)
                {
                    SharedPtr<Tile2D> tile{ new Tile2D{} };
                    tile->gid_ = gid;
                    tile->sprite_ = tmxFile_->GetTileSprite(gid & ~FLIP_ALL);
                    tile->propertySet_ = tmxFile_->GetTilePropertySet(gid & ~FLIP_ALL);
                    tiles_[y * width_ + x] = tile;
                }

                tileElem = tileElem.GetNext("tile");
            }
        }
    }
    else if (encoding == CSV)
    {
        String dataValue{ dataElem.GetValue() };
        Vector<String> gidVector{ dataValue.Split(',') };
        unsigned currentIndex{ 0u };

        for (int y{ 0 }; y < height_; ++y)
        {
            for (int x{ 0 }; x < width_; ++x)
            {
                gidVector[currentIndex].Replace("\n", "");
                const unsigned gid{ ToUInt(gidVector[currentIndex]) };

                if (gid != 0u)
                {
                    SharedPtr<Tile2D> tile{ new Tile2D{} };
                    tile->gid_ = gid;
                    tile->sprite_ = tmxFile_->GetTileSprite(gid & ~FLIP_ALL);
                    tile->propertySet_ = tmxFile_->GetTilePropertySet(gid & ~FLIP_ALL);
                    tiles_[y * width_ + x] = tile;
                }

                ++currentIndex;
            }
        }
    }
    else if (encoding == Base64)
    {
        String dataValue{ dataElem.GetValue() };
        unsigned startPosition{ 0u };

        while (!IsAlpha(dataValue[startPosition]) && !IsDigit(dataValue[startPosition])
              && dataValue[startPosition] != '+' && dataValue[startPosition] != '/')
            ++startPosition;

        dataValue = dataValue.Substring(startPosition);
        const PODVector<unsigned char> buffer{ DecodeBase64(dataValue) };
        unsigned currentIndex{ 0u };

        for (int y{ 0 }; y < height_; ++y)
        {
            for (int x{ 0 }; x < width_; ++x)
            {
                // buffer contains 32-bit integers in little-endian format
                const unsigned gid{ static_cast<unsigned>(buffer[currentIndex + 3u]) << 24u |
                                    static_cast<unsigned>(buffer[currentIndex + 2u]) << 16u |
                                    static_cast<unsigned>(buffer[currentIndex + 1u]) <<  8u |
                                    static_cast<unsigned>(buffer[currentIndex]) };
                if (gid > 0u)
                {
                    SharedPtr<Tile2D> tile{ new Tile2D{} };
                    tile->gid_ = gid;
                    tile->sprite_ = tmxFile_->GetTileSprite(gid & ~FLIP_ALL);
                    tile->propertySet_ = tmxFile_->GetTilePropertySet(gid & ~FLIP_ALL);
                    tiles_[y * width_ + x] = tile;
                }

                currentIndex += 4u;
            }
        }
    }

    if (element.HasChild("properties"))
        LoadPropertySet(element.GetChild("properties"));

    return true;
}

Tile2D* TmxTileLayer2D::GetTile(int x, int y) const
{
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        return nullptr;

    return tiles_[y * width_ + x];
}

TmxObjectGroup2D::TmxObjectGroup2D(TmxFile2D* tmxFile): TmxLayer2D(tmxFile, LT_OBJECT_GROUP),
    objects_{}
{
}

bool TmxObjectGroup2D::Load(const XMLElement& element, const TileMapInfo2D& info)
{
    LoadInfo(element);

    for (XMLElement objectElem{ element.GetChild("object") }; objectElem; objectElem = objectElem.GetNext("object"))
    {
        SharedPtr<TileMapObject2D> object{ new TileMapObject2D{} };
        StoreObject(objectElem, object, info);
        objects_.Push(object);
    }

    if (element.HasChild("properties"))
        LoadPropertySet(element.GetChild("properties"));

    return true;
}

void TmxObjectGroup2D::StoreObject(const XMLElement& objectElem, const SharedPtr<TileMapObject2D>& object, const TileMapInfo2D& info, bool isTile)
{
        if (objectElem.HasAttribute("name"))
            object->name_ = objectElem.GetAttribute("name");
        if (objectElem.HasAttribute("type"))
            object->type_ = objectElem.GetAttribute("type");

        if (objectElem.HasAttribute("gid"))
            object->objectType_ = OT_TILE;
        else if (objectElem.HasChild("polygon"))
            object->objectType_ = OT_POLYGON;
        else if (objectElem.HasChild("polyline"))
            object->objectType_ = OT_POLYLINE;
        else if (objectElem.HasChild("ellipse"))
            object->objectType_ = OT_ELLIPSE;
        else
            object->objectType_ = OT_RECTANGLE;

        const Vector2 position{ objectElem.GetFloat("x"), objectElem.GetFloat("y") };
        const Vector2 size{ objectElem.GetFloat("width"), objectElem.GetFloat("height") };

        switch (object->objectType_)
        {
        case OT_RECTANGLE:
        case OT_ELLIPSE:
            object->position_ = info.ConvertPosition(Vector2{ position.x_, position.y_ + size.y_ });
            object->size_ = Vector2{ size.x_ * PIXEL_SIZE, size.y_ * PIXEL_SIZE };
            break;

        case OT_TILE:
            object->position_ = info.ConvertPosition(position);
            object->gid_ = objectElem.GetUInt("gid");
            object->sprite_ = tmxFile_->GetTileSprite(object->gid_ & ~FLIP_ALL);

            if (objectElem.HasAttribute("width") || objectElem.HasAttribute("height"))
            {
                object->size_ = Vector2{ size.x_ * PIXEL_SIZE, size.y_ * PIXEL_SIZE };
            }
            else if (object->sprite_)
            {
                object->size_ = Vector2{ object->sprite_->GetRectangle().Size() };
            }
            break;

        case OT_POLYGON:
        case OT_POLYLINE:
            {
                Vector<String> points;

                const char* name{ object->objectType_ == OT_POLYGON ? "polygon" : "polyline" };
                const XMLElement polygonElem{ objectElem.GetChild(name) };
                points = polygonElem.GetAttribute("points").Split(' ');

                if (points.Size() <= 1)
                    return;

                object->points_.Resize(points.Size());

                for (unsigned i{ 0 }; i < points.Size(); ++i)
                {
                    points[i].Replace(',', ' ');
                    const Vector2 point{ position + ToVector2(points[i]) };
                    object->points_[i] = info.ConvertPosition(point);
                }
            }
            break;

        default: break;
        }

        if (objectElem.HasChild("properties"))
        {
            object->propertySet_ = new PropertySet2D{};
            object->propertySet_->Load(objectElem.GetChild("properties"));
        }
}

TileMapObject2D* TmxObjectGroup2D::GetObject(unsigned index) const
{
    if (index >= objects_.Size())
        return nullptr;

    return objects_[index];
}


TmxImageLayer2D::TmxImageLayer2D(TmxFile2D* tmxFile): TmxLayer2D(tmxFile, LT_IMAGE_LAYER),
    position_{},
    source_{},
    sprite_{}
{
}

bool TmxImageLayer2D::Load(const XMLElement& element, const TileMapInfo2D& info)
{
    LoadInfo(element);

    const XMLElement imageElem{ element.GetChild("image") };

    if (!imageElem)
        return false;

    position_ = Vector2{ 0.0f, info.GetMapHeight() };
    source_ = imageElem.GetAttribute("source");
    const String textureFilePath{ GetParentPath(tmxFile_->GetName()) + source_ };
    auto* cache{ tmxFile_->GetSubsystem<ResourceCache>() };
    const SharedPtr<Texture2D> texture{ cache->GetResource<Texture2D>(textureFilePath) };

    if (!texture)
    {
        DRY_LOGERROR("Could not load texture " + textureFilePath);
        return false;
    }

    sprite_ = new Sprite2D{ tmxFile_->GetContext() };
    sprite_->SetTexture(texture);
    sprite_->SetRectangle(IntRect{ 0, 0, texture->GetWidth(), texture->GetHeight() });
    // Set image hot spot at left top
    sprite_->SetHotSpot(Vector2{ 0.0f, 1.0f });

    if (element.HasChild("properties"))
        LoadPropertySet(element.GetChild("properties"));

    return true;
}

Sprite2D* TmxImageLayer2D::GetSprite() const
{
    return sprite_;
}

TmxFile2D::TmxFile2D(Context* context): Resource(context),
    loadXMLFile_{ nullptr },
    tsxXMLFiles_{},
    info_{},
    gidToSpriteMapping_{},
    gidToPropertySetMapping_{},
    gidToCollisionShapeMapping_{},
    layers_{},
    edgeOffset_{ 0.0f }
{
}

TmxFile2D::~TmxFile2D()
{
    for (unsigned i{ 0 }; i < layers_.Size(); ++i)
        delete layers_[i];
}

void TmxFile2D::RegisterObject(Context* context)
{
    context->RegisterFactory<TmxFile2D>();
}

bool TmxFile2D::BeginLoad(Deserializer& source)
{
    if (GetName().IsEmpty())
        SetName(source.GetName());

    loadXMLFile_ = new XMLFile{ context_ };

    if (!loadXMLFile_->Load(source))
    {
        DRY_LOGERROR("Load XML failed " + source.GetName());
        loadXMLFile_.Reset();
        return false;
    }

    const XMLElement rootElem{ loadXMLFile_->GetRoot("map") };

    if (!rootElem)
    {
        DRY_LOGERROR("Invalid tmx file " + source.GetName());
        loadXMLFile_.Reset();
        return false;
    }

    // If we're async loading, request the texture now. Finish during EndLoad().
    if (GetAsyncLoadState() == ASYNC_LOADING)
    {
        for (XMLElement tileSetElem{ rootElem.GetChild("tileset") }; tileSetElem; tileSetElem = tileSetElem.GetNext("tileset"))
        {
            // Tile set defined in TSX file
            if (tileSetElem.HasAttribute("source"))
            {
                const String source{ tileSetElem.GetAttribute("source") };
                SharedPtr<XMLFile> tsxXMLFile{ LoadTSXFile(source) };

                if (!tsxXMLFile)
                    return false;

                tsxXMLFiles_[source] = tsxXMLFile;
                const String textureFilePath{ GetParentPath(GetName()) +
                                              tsxXMLFile->GetRoot("tileset").GetChild("image").GetAttribute("source") };

                GetSubsystem<ResourceCache>()->BackgroundLoadResource<Texture2D>(textureFilePath, true, this);
            }
            else
            {
                const String textureFilePath{ GetParentPath(GetName()) + tileSetElem.GetChild("image").GetAttribute("source") };
                GetSubsystem<ResourceCache>()->BackgroundLoadResource<Texture2D>(textureFilePath, true, this);
            }
        }

        for (XMLElement imageLayerElem{ rootElem.GetChild("imagelayer") }; imageLayerElem;
             imageLayerElem = imageLayerElem.GetNext("imagelayer"))
        {
            const String textureFilePath{ GetParentPath(GetName()) + imageLayerElem.GetChild("image").GetAttribute("source") };
            GetSubsystem<ResourceCache>()->BackgroundLoadResource<Texture2D>(textureFilePath, true, this);
        }
    }

    return true;
}

bool TmxFile2D::EndLoad()
{
    if (!loadXMLFile_)
        return false;

    const XMLElement rootElem{ loadXMLFile_->GetRoot("map") };

    if (!IsCorrectVersion(rootElem) ||
        !ReadInfo(rootElem) ||
        !ReadLayers(rootElem))
        return false;

    loadXMLFile_.Reset();
    tsxXMLFiles_.Clear();

    return true;
}

bool TmxFile2D::IsCorrectVersion(const XMLElement& elem) const
{
    const String version{ elem.GetAttribute("version") };

    if (version.StartsWith("1."))
        return true;

    DRY_LOGERROR("Invalid TMX version: " + version);
    return false;
}

bool TmxFile2D::ReadInfo(const XMLElement& elem)
{
    const String orientation{ elem.GetAttribute("orientation") };

    if (orientation == "orthogonal")
    {
        info_.orientation_ = O_ORTHOGONAL;
    }
    else if (orientation == "isometric")
    {
        info_.orientation_ = O_ISOMETRIC;
    }
    else if (orientation == "staggered")
    {
        info_.orientation_ = O_STAGGERED;
    }
    else if (orientation == "hexagonal")
    {
        info_.orientation_ = O_HEXAGONAL;
    }
    else
    {
        DRY_LOGERROR("Unsupported orientation type: " + orientation);
        return false;
    }

    info_.width_ = elem.GetInt("width");
    info_.height_ = elem.GetInt("height");
    info_.tileWidth_ = elem.GetFloat("tilewidth") * PIXEL_SIZE;
    info_.tileHeight_ = elem.GetFloat("tileheight") * PIXEL_SIZE;

    return true;
}

bool TmxFile2D::ReadLayers(const XMLElement& elem)
{
    ClearLayers();

    for (XMLElement childElement{ elem.GetChild() }; childElement; childElement = childElement.GetNext())
    {
        bool ret{ true };
        const String name{ childElement.GetName() };

        if (name == "tileset")
        {
            ret = LoadTileSet(childElement);
        }
        else if (name == "layer")
        {
            auto* tileLayer{ new TmxTileLayer2D{ this } };
            ret = tileLayer->Load(childElement, info_);

            AddLayer(tileLayer);
        }
        else if (name == "objectgroup")
        {
            auto* objectGroup{ new TmxObjectGroup2D{ this } };
            ret = objectGroup->Load(childElement, info_);

            AddLayer(objectGroup);
        }
        else if (name == "imagelayer")
        {
            auto* imageLayer{ new TmxImageLayer2D{ this } };
            ret = imageLayer->Load(childElement, info_);

            AddLayer(imageLayer);
        }

        if (!ret)
        {
            loadXMLFile_.Reset();
            tsxXMLFiles_.Clear();

            return false;
        }
    }

    return true;
}

void TmxFile2D::ClearLayers()
{
    for (unsigned i{ 0 }; i < layers_.Size(); ++i)
        delete layers_[i];

    layers_.Clear();
}

bool TmxFile2D::SetInfo(Orientation2D orientation, int width, int height, float tileWidth, float tileHeight)
{
    if (layers_.Size() > 0)
        return false;

    info_.orientation_ = orientation;
    info_.width_ = width;
    info_.height_ = height;
    info_.tileWidth_ = tileWidth * PIXEL_SIZE;
    info_.tileHeight_ = tileHeight * PIXEL_SIZE;

    return true;
}

void TmxFile2D::AddLayer(unsigned index, TmxLayer2D* layer)
{
    if (index > layers_.Size())
        layers_.Push(layer);
    else // index <= layers_.size()
        layers_.Insert(index, layer);
}

void TmxFile2D::AddLayer(TmxLayer2D* layer)
{
    layers_.Push(layer);
}

Sprite2D* TmxFile2D::GetTileSprite(unsigned gid) const
{
    auto i{ gidToSpriteMapping_.Find(gid) };

    if (i == gidToSpriteMapping_.End())
        return nullptr;

    return i->second_;
}

Vector<SharedPtr<TileMapObject2D> > TmxFile2D::GetTileCollisionShapes(unsigned gid) const
{
    Vector<SharedPtr<TileMapObject2D> > tileShapes;
    auto i{ gidToCollisionShapeMapping_.Find(gid) };

    if (i == gidToCollisionShapeMapping_.End())
        return tileShapes;

    return i->second_;
}

PropertySet2D* TmxFile2D::GetTilePropertySet(unsigned gid) const
{
    auto i{ gidToPropertySetMapping_.Find(gid) };

    if (i == gidToPropertySetMapping_.End())
        return nullptr;

    return i->second_;
}

const TmxLayer2D* TmxFile2D::GetLayer(unsigned index) const
{
    if (index >= layers_.Size())
        return nullptr;

    return layers_[index];
}

void TmxFile2D::SetSpriteTextureEdgeOffset(float offset)
{
    edgeOffset_ = offset;

    for (auto& i : gidToSpriteMapping_)
        i.second_->SetTextureEdgeOffset(offset);
}

SharedPtr<XMLFile> TmxFile2D::LoadTSXFile(const String& source)
{
    const String tsxFilePath{ GetParentPath(GetName()) + source };
    const SharedPtr<File> tsxFile{ GetSubsystem<ResourceCache>()->GetFile(tsxFilePath) };
    SharedPtr<XMLFile> tsxXMLFile{ new XMLFile{ context_ } };

    if (!tsxFile || !tsxXMLFile->Load(*tsxFile))
    {
        DRY_LOGERROR("Failed to load TSX file " + tsxFilePath);
        return SharedPtr<XMLFile>();
    }

    return tsxXMLFile;
}

bool TmxFile2D::LoadTileSet(const XMLElement& element)
{
    const unsigned firstgid{ element.GetUInt("firstgid") };
    XMLElement tileSetElem;

    if (element.HasAttribute("source"))
    {
        const String source{ element.GetAttribute("source") };
        auto i{ tsxXMLFiles_.Find(source) };

        if (i == tsxXMLFiles_.End())
        {
            const SharedPtr<XMLFile> tsxXMLFile{ LoadTSXFile(source) };

            if (!tsxXMLFile)
                return false;

            tsxXMLFiles_[source] = tsxXMLFile; // Add to mapping to avoid release
            tileSetElem = tsxXMLFile->GetRoot("tileset");
        }
        else
        {
            tileSetElem = i->second_->GetRoot("tileset");
        }
    }
    else
    {
        tileSetElem = element;
    }

    int tileWidth{ tileSetElem.GetInt("tilewidth") };
    int tileHeight{ tileSetElem.GetInt("tileheight") };
    const int spacing{ tileSetElem.GetInt("spacing") };
    const int margin{ tileSetElem.GetInt("margin") };
    int imageWidth;
    int imageHeight;
    bool isSingleTileSet{ false };

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    {
        const XMLElement imageElem{ tileSetElem.GetChild("image") };

        // Tileset based on single tileset image
        if (!imageElem.IsNull())
        {
            isSingleTileSet = true;
            const String textureFilePath{ GetParentPath(GetName()) + imageElem.GetAttribute("source") };
            const SharedPtr<Texture2D> texture{ cache->GetResource<Texture2D>(textureFilePath) };

            if (!texture)
            {
                DRY_LOGERROR("Could not load texture " + textureFilePath);
                return false;
            }

            // Set hot spot at left bottom
            Vector2 hotSpot{ 0.0f, 0.0f };

            if (tileSetElem.HasChild("tileoffset"))
            {
                const XMLElement offsetElem{ tileSetElem.GetChild("tileoffset") };
                hotSpot.x_ += offsetElem.GetFloat("x") / tileWidth;
                hotSpot.y_ += offsetElem.GetFloat("y") / tileHeight;
            }

            imageWidth = imageElem.GetInt("width");
            imageHeight = imageElem.GetInt("height");

            unsigned gid{ firstgid };

            for (int y{ margin }; y + tileHeight <= imageHeight - margin; y += tileHeight + spacing)
            {
                for (int x{ margin }; x + tileWidth <= imageWidth - margin; x += tileWidth + spacing)
                {
                    SharedPtr<Sprite2D> sprite{ new Sprite2D{ context_ } };
                    sprite->SetTexture(texture);
                    sprite->SetRectangle(IntRect{ x, y, x + tileWidth, y + tileHeight });
                    sprite->SetHotSpot(hotSpot);

                    gidToSpriteMapping_[gid++] = sprite;
                }
            }
        }
    }

    Vector<TileImageInfo> tileImageInfos;

    for (XMLElement tileElem{ tileSetElem.GetChild("tile") }; tileElem; tileElem = tileElem.GetNext("tile"))
    {
        const unsigned gid{ firstgid + tileElem.GetUInt("id") };

        // Tileset based on collection of images
        if (!isSingleTileSet)
        {
            const XMLElement imageElem{ tileElem.GetChild("image") };

            if (!imageElem.IsNull())
            {
                const String textureFilePath{ GetParentPath(GetName()) + imageElem.GetAttribute("source") };
                SharedPtr<Image> image{ cache->GetResource<Image>(textureFilePath) };

                if (!image)
                {
                    DRY_LOGERROR("Could not load image " + textureFilePath);
                    return false;
                }

                tileWidth = imageWidth = imageElem.GetInt("width");
                tileHeight = imageHeight = imageElem.GetInt("height");
                const TileImageInfo info{ image, gid, imageWidth, imageHeight, 0, 0 };
                tileImageInfos.Push(info);
            }
        }

        // Tile collision shape(s)
        TmxObjectGroup2D objectGroup{ this };

        for (XMLElement collisionElem{ tileElem.GetChild("objectgroup") }; collisionElem; collisionElem = collisionElem.GetNext("objectgroup"))
        {
            Vector<SharedPtr<TileMapObject2D> > objects;

            for (XMLElement objectElem{ collisionElem.GetChild("object") }; objectElem; objectElem = objectElem.GetNext("object"))
            {
                SharedPtr<TileMapObject2D> object{ new TileMapObject2D{} };

                // Convert Tiled local position (left top) to Dry local position (left bottom)
                objectElem.SetAttribute("y", String{ info_.GetMapHeight() / PIXEL_SIZE - (tileHeight - objectElem.GetFloat("y")) });

                objectGroup.StoreObject(objectElem, object, info_, true);
                objects.Push(object);
            }
            gidToCollisionShapeMapping_[gid] = objects;
        }

        if (tileElem.HasChild("properties"))
        {
            SharedPtr<PropertySet2D> propertySet{ new PropertySet2D{} };
            propertySet->Load(tileElem.GetChild("properties"));
            gidToPropertySetMapping_[gid] = propertySet;
        }
    }

    if (!isSingleTileSet)
    {
        if (tileImageInfos.IsEmpty())
            return false;

        AreaAllocator allocator{ 128, 128, 2048, 2048 };

        for (int i{ 0 }; i < tileImageInfos.Size(); ++i)
        {
            TileImageInfo& info{ tileImageInfos[i] };

            if (!allocator.Allocate(info.imageWidth + 1, info.imageHeight + 1, info.x, info.y))
            {
                DRY_LOGERROR("Could not allocate area");
                return false;
            }
        }

        SharedPtr<Texture2D> texture{ new Texture2D{ context_ } };
        texture->SetMipsToSkip(QUALITY_LOW, 0);
        texture->SetNumLevels(1);
        texture->SetSize(allocator.GetWidth(), allocator.GetHeight(), Graphics::GetRGBAFormat());

        const auto textureDataSize{ allocator.GetWidth() * allocator.GetHeight() * 4u };
        SharedArrayPtr<unsigned char> textureData{ new unsigned char[textureDataSize] };
        memset(textureData.Get(), 0, textureDataSize);

        for (int i{ 0 }; i < tileImageInfos.Size(); ++i)
        {
            TileImageInfo& info{ tileImageInfos[i] };
            const SharedPtr<Image> image{ info.image->ConvertToRGBA() };

            for (int y{ 0 }; y < image->GetHeight(); ++y)
            {
                memcpy(textureData.Get() + ((info.y + y) * allocator.GetWidth() + info.x) * 4,
                       image->GetData() + y * image->GetWidth() * 4, static_cast<size_t>(image->GetWidth()) * 4);
            }

            SharedPtr<Sprite2D> sprite{ new Sprite2D{ context_ } };
            sprite->SetTexture(texture);
            sprite->SetRectangle(IntRect{ info.x, info.y, info.x + info.imageWidth, info.y + info.imageHeight });
            sprite->SetHotSpot(Vector2::ZERO);
            gidToSpriteMapping_[info.tileGid] = sprite;
        }

        texture->SetData(0, 0, 0, allocator.GetWidth(), allocator.GetHeight(), textureData.Get());
    }

    return true;
}

}
