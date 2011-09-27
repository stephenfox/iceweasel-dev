/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Corporation code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Bas Schouten <bschouten@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _MOZILLA_GFX_2D_H
#define _MOZILLA_GFX_2D_H

#include "Point.h"
#include "Rect.h"
#include "Matrix.h"

// This RefPtr class isn't ideal for usage in Azure, as it doesn't allow T**
// outparams using the &-operator. But it will have to do as there's no easy
// solution.
#include "mozilla/RefPtr.h"

struct _cairo_surface;
typedef _cairo_surface cairo_surface_t;

struct ID3D10Device1;
struct ID3D10Texture2D;

namespace mozilla {
namespace gfx {

class SourceSurface;
class DataSourceSurface;
class DrawTarget;

struct NativeSurface {
  NativeSurfaceType mType;
  SurfaceFormat mFormat;
  void *mSurface;
};

struct NativeFont {
  NativeFontType mType;
  void *mFont;
};

/*
 * This structure is used to send draw options that are universal to all drawing
 * operations. It consists of the following:
 *
 * mAlpha         - Alpha value by which the mask generated by this operation is
 *                  multiplied.
 * mCompositionOp - The operator that indicates how the source and destination
 *                  patterns are blended.
 * mAntiAliasMode - The AntiAlias mode used for this drawing operation.
 * mSnapping      - Whether this operation is snapped to pixel boundaries.
 */
struct DrawOptions {
  DrawOptions(Float aAlpha = 1.0f,
              CompositionOp aCompositionOp = OP_OVER,
              AntialiasMode aAntialiasMode = AA_GRAY,
              Snapping aSnapping = SNAP_NONE)
    : mAlpha(aAlpha)
    , mCompositionOp(aCompositionOp)
    , mAntialiasMode(aAntialiasMode)
    , mSnapping(aSnapping)
  {}

  Float mAlpha;
  CompositionOp mCompositionOp : 8;
  AntialiasMode mAntialiasMode : 2;
  Snapping mSnapping : 1;
};

/*
 * This structure is used to send stroke options that are used in stroking
 * operations. It consists of the following:
 *
 * mLineWidth    - Width of the stroke in userspace.
 * mLineJoin     - Join style used for joining lines.
 * mLineCap      - Cap style used for capping lines.
 * mMiterLimit   - Miter limit in units of linewidth
 * mDashPattern  - Series of on/off userspace lengths defining dash.
 *                 Owned by the caller; must live at least as long as
 *                 this StrokeOptions.
 *                 mDashPattern != null <=> mDashLength > 0.
 * mDashLength   - Number of on/off lengths in mDashPattern.
 * mDashOffset   - Userspace offset within mDashPattern at which stroking
 *                 begins.
 */
struct StrokeOptions {
  StrokeOptions(Float aLineWidth = 1.0f,
                JoinStyle aLineJoin = JOIN_MITER_OR_BEVEL,
                CapStyle aLineCap = CAP_BUTT,
                Float aMiterLimit = 10.0f,
                size_t aDashLength = 0,
                const Float* aDashPattern = 0,
                Float aDashOffset = 0.f)
    : mLineWidth(aLineWidth)
    , mMiterLimit(aMiterLimit)
    , mDashPattern(aDashLength > 0 ? aDashPattern : 0)
    , mDashLength(aDashLength)
    , mDashOffset(aDashOffset)
    , mLineJoin(aLineJoin)
    , mLineCap(aLineCap)
  {
    MOZ_ASSERT(aDashLength == 0 || aDashPattern);
  }

  Float mLineWidth;
  Float mMiterLimit;
  const Float* mDashPattern;
  size_t mDashLength;
  Float mDashOffset;
  JoinStyle mLineJoin : 4;
  CapStyle mLineCap : 3;
};

/*
 * This structure supplies additional options for calls to DrawSurface.
 *
 * mFilter - Filter used when resampling source surface region to the
 *           destination region.
 */
struct DrawSurfaceOptions {
  DrawSurfaceOptions(Filter aFilter = FILTER_LINEAR)
    : mFilter(aFilter)
  { }

  Filter mFilter : 3;
};

/*
 * This class is used to store gradient stops, it can only be used with a
 * matching DrawTarget. Not adhering to this condition will make a draw call
 * fail.
 */
class GradientStops : public RefCounted<GradientStops>
{
public:
  virtual ~GradientStops() {}

  virtual BackendType GetBackendType() const = 0;

protected:
  GradientStops() {}
};

/*
 * This is the base class for 'patterns'. Patterns describe the pixels used as
 * the source for a masked composition operation that is done by the different
 * drawing commands. These objects are not backend specific, however for
 * example the gradient stops on a gradient pattern can be backend specific.
 */
class Pattern
{
public:
  virtual ~Pattern() {}

  virtual PatternType GetType() const = 0;

protected:
  Pattern() {}
};

class ColorPattern : public Pattern
{
public:
  ColorPattern(const Color &aColor)
    : mColor(aColor)
  {}

  virtual PatternType GetType() const { return PATTERN_COLOR; }

  Color mColor;
};

/*
 * This class is used for Linear Gradient Patterns, the gradient stops are
 * stored in a separate object and are backend dependent. This class itself
 * may be used on the stack.
 */
class LinearGradientPattern : public Pattern
{
public:
  /*
   * aBegin Start of the linear gradient
   * aEnd End of the linear gradient
   * aStops GradientStops object for this gradient, this should match the
   *        backend type of the draw target this pattern will be used with.
   */
  LinearGradientPattern(const Point &aBegin,
                        const Point &aEnd,
                        GradientStops *aStops)
    : mBegin(aBegin)
    , mEnd(aEnd)
    , mStops(aStops)
  {
  }

  virtual PatternType GetType() const { return PATTERN_LINEAR_GRADIENT; }

  Point mBegin;
  Point mEnd;
  RefPtr<GradientStops> mStops;
};

/*
 * This class is used for Radial Gradient Patterns, the gradient stops are
 * stored in a separate object and are backend dependent. This class itself
 * may be used on the stack.
 */
class RadialGradientPattern : public Pattern
{
public:
  /*
   * aBegin Start of the linear gradient
   * aEnd End of the linear gradient
   * aStops GradientStops object for this gradient, this should match the
   *        backend type of the draw target this pattern will be used with.
   */
  RadialGradientPattern(const Point &aCenter1,
                        const Point &aCenter2,
                        Float aRadius1,
                        Float aRadius2,
                        GradientStops *aStops)
    : mCenter1(aCenter1)
    , mCenter2(aCenter2)
    , mRadius1(aRadius1)
    , mRadius2(aRadius2)
    , mStops(aStops)
  {
  }

  virtual PatternType GetType() const { return PATTERN_RADIAL_GRADIENT; }

  Point mCenter1;
  Point mCenter2;
  Float mRadius1;
  Float mRadius2;
  RefPtr<GradientStops> mStops;
};

/*
 * This class is used for Surface Patterns, they wrap a surface and a
 * repetition mode for the surface. This may be used on the stack.
 */
class SurfacePattern : public Pattern
{
public:
  SurfacePattern(SourceSurface *aSourceSurface, ExtendMode aExtendMode)
    : mSurface(aSourceSurface)
    , mExtendMode(aExtendMode)
  {}

  virtual PatternType GetType() const { return PATTERN_SURFACE; }

  RefPtr<SourceSurface> mSurface;
  ExtendMode mExtendMode;
  Filter mFilter;
};

/*
 * This is the base class for source surfaces. These objects are surfaces
 * which may be used as a source in a SurfacePattern of a DrawSurface call.
 * They cannot be drawn to directly.
 */
class SourceSurface : public RefCounted<SourceSurface>
{
public:
  virtual ~SourceSurface() {}

  virtual SurfaceType GetType() const = 0;
  virtual IntSize GetSize() const = 0;
  virtual SurfaceFormat GetFormat() const = 0;

  /*
   * This function will get a DataSourceSurface for this surface, a
   * DataSourceSurface's data can be accessed directly.
   */
  virtual TemporaryRef<DataSourceSurface> GetDataSurface() = 0;
};

class DataSourceSurface : public SourceSurface
{
public:
  /* Get the raw bitmap data of the surface */
  virtual unsigned char *GetData() = 0;
  /*
   * Stride of the surface, distance in bytes between the start of the image
   * data belonging to row y and row y+1. This may be negative.
   */
  virtual int32_t Stride() = 0;

  virtual TemporaryRef<DataSourceSurface> GetDataSurface() { RefPtr<DataSourceSurface> temp = this; return temp.forget(); }
};

/* This is an abstract object that accepts path segments. */
class PathSink : public RefCounted<PathSink>
{
public:
  virtual ~PathSink() {}

  /* Move the current point in the path, any figure currently being drawn will
   * be considered closed during fill operations, however when stroking the
   * closing line segment will not be drawn.
   */
  virtual void MoveTo(const Point &aPoint) = 0;
  /* Add a linesegment to the current figure */
  virtual void LineTo(const Point &aPoint) = 0;
  /* Add a cubic bezier curve to the current figure */
  virtual void BezierTo(const Point &aCP1,
                        const Point &aCP2,
                        const Point &aCP3) = 0;
  /* Add a quadratic bezier curve to the current figure */
  virtual void QuadraticBezierTo(const Point &aCP1,
                                 const Point &aCP2) = 0;
  /* Close the current figure, this will essentially generate a line segment
   * from the current point to the starting point for the current figure
   */
  virtual void Close() = 0;
  /* Add an arc to the current figure */
  virtual void Arc(const Point &aOrigin, float aRadius, float aStartAngle,
                   float aEndAngle, bool aAntiClockwise = false) = 0;
  /* Point the current subpath is at - or where the next subpath will start
   * if there is no active subpath.
   */
  virtual Point CurrentPoint() const = 0;
};

class PathBuilder;

/* The path class is used to create (sets of) figures of any shape that can be
 * filled or stroked to a DrawTarget
 */
class Path : public RefCounted<Path>
{
public:
  virtual ~Path() {}
  
  virtual BackendType GetBackendType() const = 0;

  /* This returns a PathBuilder object that contains a copy of the contents of
   * this path and is still writable.
   */
  virtual TemporaryRef<PathBuilder> CopyToBuilder(FillRule aFillRule = FILL_WINDING) const = 0;
  virtual TemporaryRef<PathBuilder> TransformedCopyToBuilder(const Matrix &aTransform,
                                                             FillRule aFillRule = FILL_WINDING) const = 0;

  /* This function checks if a point lies within a path. It allows passing a
   * transform that will transform the path to the coordinate space in which
   * aPoint is given.
   */
  virtual bool ContainsPoint(const Point &aPoint, const Matrix &aTransform) const = 0;

  /* This functions gets the bounds of this path. These bounds are not
   * guaranteed to be tight. A transform may be specified that gives the bounds
   * after application of the transform.
   */
  virtual Rect GetBounds(const Matrix &aTransform = Matrix()) const = 0;

  /* This function gets the bounds of the stroke of this path using the
   * specified strokeoptions. These bounds are not guaranteed to be tight.
   * A transform may be specified that gives the bounds after application of
   * the transform.
   */
  virtual Rect GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                                const Matrix &aTransform = Matrix()) const = 0;

  /* This gets the fillrule this path's builder was created with. This is not
   * mutable.
   */
  virtual FillRule GetFillRule() const = 0;
};

/* The PathBuilder class allows path creation. Once finish is called on the
 * pathbuilder it may no longer be written to.
 */
class PathBuilder : public PathSink
{
public:
  /* Finish writing to the path and return a Path object that can be used for
   * drawing. Future use of the builder results in a crash!
   */
  virtual TemporaryRef<Path> Finish() = 0;
};

struct Glyph
{
  uint32_t mIndex;
  Point mPosition;
};

/* This class functions as a glyph buffer that can be drawn to a DrawTarget.
 * XXX - This should probably contain the guts of gfxTextRun in the future as
 * roc suggested. But for now it's a simple container for a glyph vector.
 */
struct GlyphBuffer
{
  // A pointer to a buffer of glyphs. Managed by the caller.
  const Glyph *mGlyphs;
  // Number of glyphs mGlyphs points to.
  uint32_t mNumGlyphs;
};

/* This class is an abstraction of a backend/platform specific font object
 * at a particular size. It is passed into text drawing calls to describe
 * the font used for the drawing call.
 */
class ScaledFont : public RefCounted<ScaledFont>
{
public:
  virtual ~ScaledFont() {}

  virtual FontType GetType() const = 0;

  /* This allows getting a path that describes the outline of a set of glyphs.
   * A target is passed in so that the guarantee is made the returned path
   * can be used with any DrawTarget that has the same backend as the one
   * passed in.
   */
  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget) = 0;

protected:
  ScaledFont() {}
};

/* This is the main class used for all the drawing. It is created through the
 * factory and accepts drawing commands. The results of drawing to a target
 * may be used either through a Snapshot or by flushing the target and directly
 * accessing the backing store a DrawTarget was created with.
 */
class DrawTarget : public RefCounted<DrawTarget>
{
public:
  DrawTarget() : mTransformDirty(false) {}
  virtual ~DrawTarget() {}

  virtual BackendType GetType() const = 0;
  virtual TemporaryRef<SourceSurface> Snapshot() = 0;
  virtual IntSize GetSize() = 0;

  /* Ensure that the DrawTarget backend has flushed all drawing operations to
   * this draw target. This must be called before using the backing surface of
   * this draw target outside of GFX 2D code.
   */
  virtual void Flush() = 0;

  /*
   * Draw a surface to the draw target. Possibly doing partial drawing or
   * applying scaling. No sampling happens outside the source.
   *
   * aSurface Source surface to draw
   * aDest Destination rectangle that this drawing operation should draw to
   * aSource Source rectangle in aSurface coordinates, this area of aSurface
   *         will be stretched to the size of aDest.
   * aOptions General draw options that are applied to the operation
   * aSurfOptions DrawSurface options that are applied
   */
  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions = DrawSurfaceOptions(),
                           const DrawOptions &aOptions = DrawOptions()) = 0;

  /*
   * Blend a surface to the draw target with a shadow. The shadow is drawn as a
   * gaussian blur using a specified sigma. The shadow is clipped to the size
   * of the input surface, so the input surface should contain a transparent
   * border the size of the approximate coverage of the blur (3 * aSigma).
   * NOTE: This function works in device space!
   *
   * aSurface Source surface to draw.
   * aDest Destination point that this drawing operation should draw to.
   * aColor Color of the drawn shadow
   * aOffset Offset of the shadow
   * aSigma Sigma used for the guassian filter kernel
   * aOperator Composition operator used
   */
  virtual void DrawSurfaceWithShadow(SourceSurface *aSurface,
                                     const Point &aDest,
                                     const Color &aColor,
                                     const Point &aOffset,
                                     Float aSigma,
                                     CompositionOp aOperator) = 0;

  /* 
   * Clear a rectangle on the draw target to transparent black. This will
   * respect the clipping region and transform.
   *
   * aRect Rectangle to clear
   */
  virtual void ClearRect(const Rect &aRect) = 0;

  /*
   * This is essentially a 'memcpy' between two surfaces. It moves a pixel
   * aligned area from the source surface unscaled directly onto the
   * drawtarget. This ignores both transform and clip.
   *
   * aSurface Surface to copy from
   * aSourceRect Source rectangle to be copied
   * aDest Destination point to copy the surface to
   */
  virtual void CopySurface(SourceSurface *aSurface,
                           const IntRect &aSourceRect,
                           const IntPoint &aDestination) = 0;

  /*
   * Fill a rectangle on the DrawTarget with a certain source pattern.
   *
   * aRect Rectangle that forms the mask of this filling operation
   * aPattern Pattern that forms the source of this filling operation
   * aOptions Options that are applied to this operation
   */
  virtual void FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aOptions = DrawOptions()) = 0;

  /*
   * Stroke a rectangle on the DrawTarget with a certain source pattern.
   *
   * aRect Rectangle that forms the mask of this stroking operation
   * aPattern Pattern that forms the source of this stroking operation
   * aOptions Options that are applied to this operation
   */
  virtual void StrokeRect(const Rect &aRect,
                          const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions = StrokeOptions(),
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  /*
   * Stroke a line on the DrawTarget with a certain source pattern.
   *
   * aStart Starting point of the line
   * aEnd End point of the line
   * aPattern Pattern that forms the source of this stroking operation
   * aOptions Options that are applied to this operation
   */
  virtual void StrokeLine(const Point &aStart,
                          const Point &aEnd,
                          const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions = StrokeOptions(),
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  /*
   * Stroke a path on the draw target with a certain source pattern.
   *
   * aPath Path that is to be stroked
   * aPattern Pattern that should be used for the stroke
   * aStrokeOptions Stroke options used for this operation
   * aOptions Draw options used for this operation
   */
  virtual void Stroke(const Path *aPath,
                      const Pattern &aPattern,
                      const StrokeOptions &aStrokeOptions = StrokeOptions(),
                      const DrawOptions &aOptions = DrawOptions()) = 0;
  
  /*
   * Fill a path on the draw target with a certain source pattern.
   *
   * aPath Path that is to be filled
   * aPattern Pattern that should be used for the fill
   * aOptions Draw options used for this operation
   */
  virtual void Fill(const Path *aPath,
                    const Pattern &aPattern,
                    const DrawOptions &aOptions = DrawOptions()) = 0;

  /*
   * Fill a series of clyphs on the draw target with a certain source pattern.
   */
  virtual void FillGlyphs(ScaledFont *aFont,
                          const GlyphBuffer &aBuffer,
                          const Pattern &aPattern,
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  /*
   * Push a clip to the DrawTarget.
   *
   * aPath The path to clip to
   */
  virtual void PushClip(const Path *aPath) = 0;

  /* Pop a clip from the DrawTarget. A pop without a corresponding push will
   * be ignored.
   */
  virtual void PopClip() = 0;

  /*
   * Create a SourceSurface optimized for use with this DrawTarget for
   * existing bitmap data in memory.
   */
  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                            const IntSize &aSize,
                                                            int32_t aStride,
                                                            SurfaceFormat aFormat) const = 0;

  /*
   * Create a SourceSurface optimized for use with this DrawTarget from
   * an arbitrary other SourceSurface. This may return aSourceSurface or some
   * other existing surface.
   */
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const = 0;

  /*
   * Create a SourceSurface for a type of NativeSurface. This may fail if the
   * draw target does not know how to deal with the type of NativeSurface passed
   * in.
   */
  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const = 0;

  /*
   * Create a DrawTarget whose snapshot is optimized for use with this DrawTarget.
   */
  virtual TemporaryRef<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const = 0;

  /*
   * Create a path builder with the specified fillmode.
   */
  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FILL_WINDING) const = 0;

  /*
   * Create a GradientStops object that holds information about a set of
   * gradient stops, this object is required for linear or radial gradient
   * patterns to represent the color stops in the gradient.
   *
   * aStops An array of gradient stops
   * aNumStops Number of stops in the array aStops
   */
  virtual TemporaryRef<GradientStops> CreateGradientStops(GradientStop *aStops, uint32_t aNumStops) const = 0;

  const Matrix &GetTransform() const { return mTransform; }

  /*
   * Set a transform on the surface, this transform is applied at drawing time
   * to both the mask and source of the operation.
   */
  virtual void SetTransform(const Matrix &aTransform)
    { mTransform = aTransform; mTransformDirty = true; }

  SurfaceFormat GetFormat() { return mFormat; }

  /* Tries to get a native surface for a DrawTarget, this may fail if the
   * draw target cannot convert to this surface type.
   */
  virtual void *GetNativeSurface(NativeSurfaceType aType) = 0;

protected:
  Matrix mTransform;
  bool mTransformDirty : 1;

  SurfaceFormat mFormat;
};

class Factory
{
public:
#ifdef USE_CAIRO
  static TemporaryRef<DrawTarget> CreateDrawTargetForCairoSurface(cairo_surface_t* aSurface);
#endif

  static TemporaryRef<DrawTarget> CreateDrawTarget(BackendType aBackend, const IntSize &aSize, SurfaceFormat aFormat);
  static TemporaryRef<ScaledFont> CreateScaledFontForNativeFont(const NativeFont &aNativeFont, Float aSize);

#ifdef WIN32
  static TemporaryRef<DrawTarget> CreateDrawTargetForD3D10Texture(ID3D10Texture2D *aTexture, SurfaceFormat aFormat);
  static void SetDirect3D10Device(ID3D10Device1 *aDevice);
  static ID3D10Device1 *GetDirect3D10Device();

private:
  static ID3D10Device1 *mD3D10Device;
#endif
};

}
}

#endif // _MOZILLA_GFX_2D_H
