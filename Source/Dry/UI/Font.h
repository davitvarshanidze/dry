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

/// \file

#pragma once

#include "../Container/ArrayPtr.h"
#include "../Resource/Resource.h"

namespace Dry
{

class FontFace;

static const int FONT_TEXTURE_MIN_SIZE = 128;
static const int FONT_DPI = 96;

/// %Font file type.
enum FontType
{
    FONT_NONE = 0,
    FONT_FREETYPE,
    FONT_BITMAP,
    MAX_FONT_TYPES
};

/// %Font resource.
class DRY_API Font : public Resource
{
    DRY_OBJECT(Font, Resource);

public:
    /// Construct.
    explicit Font(Context* context);
    /// Destruct.
    ~Font() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource as a new bitmap font type in XML format. Return true if successful.
    bool SaveXML(Serializer& dest, int pointSize, bool usedGlyphs = false, const String& indentation = "\t");
    /// Set absolute (in pixels) position adjustment for glyphs.
    void SetAbsoluteGlyphOffset(const IntVector2& offset);
    /// Set point size scaled position adjustment for glyphs.
    void SetScaledGlyphOffset(const Vector2& offset);

    /// Return font face. Pack and render to a texture if not rendered yet. Return null on error.
    FontFace* GetFace(float pointSize);

    /// Return font type.
    FontType GetFontType() const { return fontType_; }

    /// Is signed distance field font.
    bool IsSDFFont() const { return sdfFont_; }

    /// Return absolute position adjustment for glyphs.
    const IntVector2& GetAbsoluteGlyphOffset() const { return absoluteOffset_; }

    /// Return point size scaled position adjustment for glyphs.
    const Vector2& GetScaledGlyphOffset() const { return scaledOffset_; }

    /// Return the total effective offset for a point size.
    IntVector2 GetTotalGlyphOffset(float pointSize) const;

    /// Release font faces and recreate them next time when requested. Called when font textures lost or global font properties change.
    void ReleaseFaces();

private:
    /// Load font glyph offset parameters from an optional XML file. Called internally when loading TrueType fonts.
    void LoadParameters();
    /// Return font face using FreeType. Called internally. Return null on error.
    FontFace* GetFaceFreeType(float pointSize);
    /// Return bitmap font face. Called internally. Return null on error.
    FontFace* GetFaceBitmap(float pointSize);

    /// Created faces.
    HashMap<int, SharedPtr<FontFace> > faces_;
    /// Font data.
    SharedArrayPtr<unsigned char> fontData_;
    /// Size of font data.
    unsigned fontDataSize_;
    /// Absolute position adjustment for glyphs.
    IntVector2 absoluteOffset_;
    /// Point size scaled position adjustment for glyphs.
    Vector2 scaledOffset_;
    /// Font type.
    FontType fontType_;
    /// Signed distance field font flag.
    bool sdfFont_;
};

}
