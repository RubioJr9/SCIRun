/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2020 Scientific Computing and Imaging Institute,
   University of Utah.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


#include <Graphics/Glyphs/GlyphGeom.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/ColorMap.h>
#include <Core/Math/MiscMath.h>
#include <Core/GeometryPrimitives/Transform.h>

using namespace SCIRun;
using namespace Graphics;
using namespace Datatypes;
using namespace Core::Geometry;
using namespace Core::Datatypes;

GlyphGeom::GlyphGeom() : numVBOElements_(0), lineIndex_(0)
{

}

void GlyphGeom::buildObject(GeometryObjectSpire& geom, const std::string& uniqueNodeID,
  const bool isTransparent, const double transparencyValue, const ColorScheme& colorScheme,
  RenderState state, const SpireIBO::PRIMITIVE& primIn, const BBox& bbox, const bool isClippable,
  const Core::Datatypes::ColorMapHandle colorMap)
{
  bool useColor = colorScheme == ColorScheme::COLOR_IN_SITU || colorScheme == ColorScheme::COLOR_MAP;
  bool useNormals = normals_.size() == points_.size();
  int numAttributes = 3;

  RenderType renderType = RenderType::RENDER_VBO_IBO;
  ColorRGB dft = state.defaultColor;

  std::string shader = (useNormals ? "Shaders/Phong" : "Shaders/Flat");
  std::vector<SpireVBO::AttributeData> attribs;
  std::vector<SpireSubPass::Uniform> uniforms;

  attribs.push_back(SpireVBO::AttributeData("aPos", 3 * sizeof(float)));
  uniforms.push_back(SpireSubPass::Uniform("uUseClippingPlanes", isClippable));
  uniforms.push_back(SpireSubPass::Uniform("uUseFog", true));

  if (useNormals)
  {
    numAttributes += 3;
    attribs.push_back(SpireVBO::AttributeData("aNormal", 3 * sizeof(float)));
    uniforms.push_back(SpireSubPass::Uniform("uAmbientColor", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)));
    uniforms.push_back(SpireSubPass::Uniform("uSpecularColor", glm::vec4(0.1f, 0.1f, 0.1f, 0.1f)));
    uniforms.push_back(SpireSubPass::Uniform("uSpecularPower", 32.0f));
  }

  SpireText text;
  SpireTexture2D texture;
  if (useColor)
  {
    if(colorMap)
    {
      numAttributes += 2;
      shader += "_ColorMap";
      attribs.push_back(SpireVBO::AttributeData("aTexCoords", 2 * sizeof(float)));

      const static int colorMapResolution = 256;
      for(int i = 0; i < colorMapResolution; ++i)
      {
        ColorRGB color = colorMap->valueToColor(static_cast<float>(i)/colorMapResolution * 2.0f - 1.0f);
        texture.bitmap.push_back(color.r()*255.99f);
        texture.bitmap.push_back(color.g()*255.99f);
        texture.bitmap.push_back(color.b()*255.99f);
        texture.bitmap.push_back(color.a()*255.99f);
      }

      texture.name = "ColorMap";
      texture.height = 1;
      texture.width = colorMapResolution;
    }
    else
    {
      numAttributes += 4;
      shader += "_Color";
      attribs.push_back(SpireVBO::AttributeData("aColor", 4 * sizeof(float)));
    }
  }
  else
  {
    uniforms.push_back(SpireSubPass::Uniform("uDiffuseColor",
      glm::vec4(dft.r(), dft.g(), dft.b(), static_cast<float>(transparencyValue))));
  }

  if (isTransparent) uniforms.push_back(SpireSubPass::Uniform("uTransparency", static_cast<float>(transparencyValue)));

  size_t pointsLeft = points_.size();
  size_t startOfPass = 0;
  int passNumber = 0;
  while(pointsLeft > 0)
  {
    std::string passID = uniqueNodeID + "_" + std::to_string(passNumber++);
    std::string vboName = passID + "VBO";
    std::string iboName = passID + "IBO";
    std::string passName = passID + "Pass";

    const static size_t maxPointsPerPass = 3 << 24; //must be a number divisible by 2, 3 and, 4
    uint32_t pointsInThisPass = std::min(pointsLeft, maxPointsPerPass);
    size_t endOfPass = startOfPass + pointsInThisPass;
    pointsLeft -= pointsInThisPass;

    size_t vboSize = static_cast<size_t>(pointsInThisPass) * numAttributes * sizeof(float);
    size_t iboSize = static_cast<size_t>(pointsInThisPass) * sizeof(uint32_t);
    std::shared_ptr<spire::VarBuffer> iboBufferSPtr(new spire::VarBuffer(iboSize));
    std::shared_ptr<spire::VarBuffer> vboBufferSPtr(new spire::VarBuffer(vboSize));
    auto iboBuffer = iboBufferSPtr.get();
    auto vboBuffer = vboBufferSPtr.get();

    for (auto a : indices_) if(a >= startOfPass && a < endOfPass)
      iboBuffer->write(static_cast<uint32_t>(a - startOfPass));

    BBox newBBox;
    for (size_t i = startOfPass; i < endOfPass; ++i)
    {
      Vector point = points_.at(i);
      newBBox.extend(Point(point.x(), point.y(), point.z()));
      vboBuffer->write(static_cast<float>(point.x()));
      vboBuffer->write(static_cast<float>(point.y()));
      vboBuffer->write(static_cast<float>(point.z()));

      if (useNormals)
      {
        Vector normal = normals_.at(i);
        vboBuffer->write(static_cast<float>(normal.x()));
        vboBuffer->write(static_cast<float>(normal.y()));
        vboBuffer->write(static_cast<float>(normal.z()));
      }

      if (useColor)
      {
        ColorRGB color = colors_.at(i);
        if(!colorMap)
        {
          vboBuffer->write(static_cast<float>(color.r()));
          vboBuffer->write(static_cast<float>(color.g()));
          vboBuffer->write(static_cast<float>(color.b()));
          vboBuffer->write(static_cast<float>(color.a()));
        }
        else
        {
          vboBuffer->write(static_cast<float>(color.r()));
          vboBuffer->write(static_cast<float>(color.r()));
        }
      }
    }
    if(!bbox.valid()) newBBox.reset();

    startOfPass = endOfPass;

    SpireVBO geomVBO(vboName, attribs, vboBufferSPtr, numVBOElements_, newBBox, true);
    SpireIBO geomIBO(iboName, primIn, sizeof(uint32_t), iboBufferSPtr);

    state.set(RenderState::IS_ON, true);
    state.set(RenderState::HAS_DATA, true);
    SpireSubPass pass(passName, vboName, iboName, shader, colorScheme, state, renderType, geomVBO, geomIBO, text, texture);

    for (const auto& uniform : uniforms) pass.addUniform(uniform);

    geom.vbos().push_back(geomVBO);
    geom.ibos().push_back(geomIBO);
    geom.passes().push_back(pass);
  }
}

void GlyphGeom::addArrow(const Point& p1, const Point& p2, double radius, double ratio, int resolution,
                         const ColorRGB& color1, const ColorRGB& color2, bool render_cylinder_base, bool render_cone_base)
{
  Point mid((p1.x() * ratio + p2.x() * (1 - ratio)), (p1.y() * ratio + p2.y() * (1 - ratio)), (p1.z() * ratio + p2.z() * (1 - ratio)));

  generateCylinder(p1, mid, radius / 6.0, radius / 6.0, resolution, color1, color2, render_cylinder_base, false);
  generateCone(mid, p2, radius, resolution, render_cone_base, color1, color2);
}

void GlyphGeom::addSphere(const Point& p, double radius, int resolution, const ColorRGB& color)
{
  generateSphere(p, radius, resolution, color);
}

void GlyphGeom::addComet(const Point& p1, const Point& p2, double radius, int resolution,
                         const ColorRGB& color1, const ColorRGB& color2, double sphere_extrusion)
{
  generateComet(p1, p2, radius, resolution, color1, color2, sphere_extrusion);
}

void GlyphGeom::addBox(const Point& center, Tensor& t, double scale, ColorRGB& node_color, bool normalize)
{
  generateBox(center, t, scale, node_color, normalize);
}

void GlyphGeom::addEllipsoid(const Point& p, Tensor& t, double scale, int resolution, const ColorRGB& color, bool normalize)
{
  generateEllipsoid(p, t, scale, resolution, color, false, normalize);
}

void GlyphGeom::addSuperquadricTensor(const Point& p, Tensor& t, double scale, int resolution,
                                      const ColorRGB& color, bool normalize, double emphasis)
{
  generateSuperquadricTensor(p, t, scale, resolution, color, normalize, emphasis);
}

void GlyphGeom::addCylinder(const Point& p1, const Point& p2, double radius, int resolution,
                            const ColorRGB& color1, const ColorRGB& color2,
                            bool renderBase1, bool renderBase2)
{
  generateCylinder(p1, p2, radius, radius, resolution, color1, color2, renderBase1, renderBase2);
}

void GlyphGeom::addCylinder(const Point& p1, const Point& p2, double radius1, double radius2,
                            int resolution, const ColorRGB& color1, const ColorRGB& color2,
                            bool renderBase1, bool renderBase2)
{
  generateCylinder(p1, p2, radius1, radius2, resolution, color1, color2, renderBase1, renderBase2);
}

void GlyphGeom::addDisk(const Point& p1, const Point& p2, double radius, int resolution,
                            const ColorRGB& color1, const ColorRGB& color2)
{
  generateCylinder(p1, p2, radius, radius, resolution, color1, color2, true, true);
}

void GlyphGeom::addTorus(const Point& p1, const Point& p2, double major_radius, double minor_radius, int resolution,
                        const ColorRGB& color1, const ColorRGB& color2)
{
  generateTorus(p1, p2, major_radius, minor_radius, resolution, color1);
}

void GlyphGeom::addCone(const Point& p1, const Point& p2, double radius, int resolution,
                        bool render_base, const ColorRGB& color1, const ColorRGB& color2)
{
  generateCone(p1, p2, radius, resolution, render_base, color1, color2);
}

void GlyphGeom::addClippingPlane(const Point& p1, const Point& p2,
  const Point& p3, const Point& p4, double radius, int resolution,
  const ColorRGB& color1, const ColorRGB& color2)
{
  addSphere(p1, radius, resolution, color1);
  addSphere(p2, radius, resolution, color1);
  addSphere(p3, radius, resolution, color1);
  addSphere(p4, radius, resolution, color1);
  addCylinder(p1, p2, radius, resolution, color1, color2);
  addCylinder(p2, p3, radius, resolution, color1, color2);
  addCylinder(p3, p4, radius, resolution, color1, color2);
  addCylinder(p4, p1, radius, resolution, color1, color2);
}

void GlyphGeom::addPlane(const Point& p1, const Point& p2,
  const Point& p3, const Point& p4,
  const ColorRGB& color1)
{
  generatePlane(p1, p2, p3, p4, color1);
}

void GlyphGeom::addLine(const Point& p1, const Point& p2, const ColorRGB& color1, const ColorRGB& color2)
{
  generateLine(p1, p2, color1, color2);
}

void GlyphGeom::addNeedle(const Point& p1, const Point& p2, const ColorRGB& color1, const ColorRGB& color2)
{
  Point mid(0.5 * (p1.x() + p2.x()), 0.5 * (p1.y() + p2.y()), 0.5 * (p1.z() + p2.z()));
  ColorRGB endColor(color2.r(), color2.g(), color2.b(), 0.5);
  generateLine(p1, mid, color1, endColor);
  generateLine(mid, p2, color1, endColor);
}

void GlyphGeom::addPoint(const Point& p, const ColorRGB& color)
{
  generatePoint(p, color);
}

void GlyphGeom::generateCylinder(const Point& p1, const Point& p2, double radius1,
                                 double radius2, int resolution, const ColorRGB& color1,
                                 const ColorRGB& color2)
{
  double num_strips = resolution;
  if (num_strips < 0) num_strips = 20.0;
  double r1 = radius1 < 0 ? 1.0 : radius1;
  double r2 = radius2 < 0 ? 1.0 : radius2;

  //generate triangles for the cylinders.
  Vector n((p1 - p2).normal());
  Vector crx = n.getArbitraryTangent();
  Vector u = Cross(crx, n).normal();
  Vector p;
  for (int strips = 0; strips <= num_strips; strips++)
  {
    size_t offset = static_cast<size_t>(numVBOElements_);
    p = std::cos(2. * M_PI * strips / num_strips) * u +
      std::sin(2. * M_PI * strips / num_strips) * crx;
    p.normalize();
    Vector normals(((p2-p1).length() * p + (r2-r1)*n).normal());

    points_.push_back(r1 * p + Vector(p1));
    colors_.push_back(color1);
    normals_.push_back(normals);
    numVBOElements_++;
    points_.push_back(r2 * p + Vector(p2));
    colors_.push_back(color2);
    normals_.push_back(normals);
    numVBOElements_++;

    indices_.push_back(0 + offset);
    indices_.push_back(1 + offset);
    indices_.push_back(2 + offset);
    indices_.push_back(2 + offset);
    indices_.push_back(1 + offset);
    indices_.push_back(3 + offset);
  }
  for (int jj = 0; jj < 6; jj++) indices_.pop_back();
}

void GlyphGeom::generateCone(const Point& p1, const Point& p2, double radius,
                             int resolution, bool renderBase,
                             const ColorRGB& color1, const ColorRGB& color2)
{
  resolution = resolution < 0 ? 20 : resolution;
  radius = radius < 0 ? 1 : radius;

  //generate triangles for the cylinders.
  Vector n((p1 - p2).normal());
  Vector crx = n.getArbitraryTangent();
  Vector u = Cross(crx, n).normal();

  // Center of base
  size_t base_index = numVBOElements_;
  int points_per_loop = 2;
  if(renderBase)
  {
    points_.push_back(Vector(p1));
    colors_.push_back(color1);
    normals_.push_back(n);
    numVBOElements_++;
    points_per_loop = 3;
  }

  // Precalculate
  double length = (p2-p1).length();
  double strip_angle = 2. * M_PI / resolution;
  size_t offset = static_cast<size_t>(numVBOElements_);

  Vector p;

  // Add points, normals, and colors
  for (int strips = 0; strips <= resolution; strips++)
  {
    p = std::cos(strip_angle * strips) * u +
      std::sin(strip_angle * strips) * crx;
    p.normalize();
    Vector normals((length * p - radius * n).normal());

    points_.push_back(radius * p + Vector(p1));
    colors_.push_back(color1);
    normals_.push_back(normals);
    points_.push_back(Vector(p2));
    colors_.push_back(color2);
    normals_.push_back(normals);
    numVBOElements_ += 2;

    if(renderBase)
    {
      points_.push_back(radius * p + Vector(p1));
      colors_.push_back(color1);
      normals_.push_back(n);
      numVBOElements_++;
    }
  }

  // Add indices
  for (int strips = offset; strips < resolution * points_per_loop + offset; strips += points_per_loop)
  {
    indices_.push_back(strips);
    indices_.push_back(strips + 1);
    indices_.push_back(strips + points_per_loop);
    if(renderBase)
    {
      indices_.push_back(base_index);
      indices_.push_back(strips + 2);
      indices_.push_back(strips + points_per_loop + 2);
    }
  }
}

void GlyphGeom::generateCylinder(const Point& p1, const Point& p2, double radius1,
                                 double radius2, int resolution, const ColorRGB& color1,
                                 const ColorRGB& color2, bool renderBase1, bool renderBase2)
{
  resolution = resolution < 0 ? 20 : resolution;
  radius1 = radius1 < 0 ? 1.0 : radius1;
  radius2 = radius2 < 0 ? 1.0 : radius2;

  //generate triangles for the cylinders.
  Vector n((p1 - p2).normal());
  Vector crx = n.getArbitraryTangent();
  Vector u = Cross(crx, n).normal();

  int points_per_loop = 2 + renderBase1 + renderBase2;

  // Add center points so flat sides can be drawn
  int p1_index, p2_index;
  if(renderBase1)
  {
    points_.push_back(Vector(p1));
    p1_index = numVBOElements_;
    colors_.push_back(color1);
    normals_.push_back(n);
    numVBOElements_++;
  }
  if(renderBase2)
  {
    points_.push_back(Vector(p2));
    p2_index = numVBOElements_;
    colors_.push_back(color2);
    normals_.push_back(-n);
    numVBOElements_++;
  }

  // Precalculate
  double length = (p2-p1).length();
  double strip_angle = 2. * M_PI / resolution;
  size_t offset = static_cast<size_t>(numVBOElements_);

  Vector p;
  // Add points, normals, and colors
  for (int strips = 0; strips <= resolution; strips++)
  {
    p = std::cos(strip_angle * strips) * u +
      std::sin(strip_angle * strips) * crx;
    p.normalize();
    Vector normals((length * p + (radius2-radius1)*n).normal());
    points_.push_back(radius1 * p + Vector(p1));
    colors_.push_back(color1);
    normals_.push_back(normals);
    points_.push_back(radius2 * p + Vector(p2));
    colors_.push_back(color2);
    normals_.push_back(normals);

    // Points for base
    if(renderBase1)
    {
      points_.push_back(radius1 * p + Vector(p1));
      colors_.push_back(color1);
      normals_.push_back(n);
    }
    if(renderBase2)
    {
      points_.push_back(radius2 * p + Vector(p2));
      colors_.push_back(color2);
      normals_.push_back(-n);
    }
    numVBOElements_ += points_per_loop;
  }

  // Add indices
  for (int strips = offset; strips < resolution * points_per_loop + offset; strips += points_per_loop)
  {
    indices_.push_back(strips);
    indices_.push_back(strips + 1);
    indices_.push_back(strips + points_per_loop);
    indices_.push_back(strips + points_per_loop);
    indices_.push_back(strips + 1);
    indices_.push_back(strips + points_per_loop + 1);

    // Render base
    if(renderBase1)
    {
      indices_.push_back(p1_index);
      indices_.push_back(strips + 2);
      indices_.push_back(strips + points_per_loop + 2);
    }
    if(renderBase2)
    {
      // Increment 1 if base 1 is present
      indices_.push_back(strips + 2 + renderBase1);
      indices_.push_back(p2_index);
      indices_.push_back(strips + points_per_loop + 2 + renderBase1);
    }
  }
}

void GlyphGeom::generateSphere(const Point& center, double radius, int resolution, const ColorRGB& color)
{
  double num_strips = resolution;
  if (num_strips < 0) num_strips = 20.0;
  double r = radius < 0 ? 1.0 : radius;
  Vector pp1, pp2;
  double theta_inc = /*2. */ M_PI / num_strips, phi_inc = 0.5 * M_PI / num_strips;

  //generate triangles for the spheres
  for (double phi = 0.; phi <= M_PI - phi_inc; phi += phi_inc)
  {
    for (double theta = 0.; theta <= 2. * M_PI; theta += theta_inc)
    {
      uint32_t offset = static_cast<uint32_t>(numVBOElements_);
      pp1 = Vector(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      pp2 = Vector(sin(theta) * cos(phi + phi_inc), sin(theta) * sin(phi + phi_inc), cos(theta));

      normals_.push_back(pp1);
      normals_.push_back(pp2);
      pp1 *= r;
      pp2 *= r;
      points_.push_back(pp1 + Vector(center));
      colors_.push_back(color);
      numVBOElements_++;
      points_.push_back(pp2 + Vector(center));
      colors_.push_back(color);
      numVBOElements_++;

      //preserve vertex ordering for double sided rendering
      int v1 = 1, v2 = 2;
      if(theta < M_PI)
      {
        v1 = 2;
        v2 = 1;
      }

      indices_.push_back(0 + offset);
      indices_.push_back(v1 + offset);
      indices_.push_back(v2 + offset);
      indices_.push_back(v2 + offset);
      indices_.push_back(v1 + offset);
      indices_.push_back(3 + offset);
    }
    for (int jj = 0; jj < 6; jj++) indices_.pop_back();
  }
}

void GlyphGeom::generateComet(const Point& p1, const Point& p2,
                              double radius, int resolution,
                              const ColorRGB& color1, const ColorRGB& color2,
                              double sphere_extrusion)
{
  Vector dir = (p2-p1).normal();

  // First, generate cone
  resolution = resolution < 0 ? 20 : resolution;
  radius = radius < 0 ? 1 : radius;

  //generate triangles for the cylinders.
  Vector n((p1 - p2).normal());
  Vector crx = n.getArbitraryTangent();
  Vector u = Cross(crx, n).normal();
  Point cone_p2 = p2 - dir * radius * sphere_extrusion * M_PI;
  double cone_radius = radius * cos(sphere_extrusion * M_PI);

  // Center of base
  int points_per_loop = 2;

  // Precalculate
  double length = (p2-p1).length();
  double strip_angle = 2. * M_PI / resolution;

  uint32_t offset = static_cast<uint32_t>(numVBOElements_);

  std::vector<Vector> cone_rim_points;

  Vector p;
  // Add points, normals, and colors
  for (int strips = 0; strips <= resolution; strips++)
  {
    p = std::cos(strip_angle * strips) * u +
      std::sin(strip_angle * strips) * crx;
    p.normalize();
    Vector normals((length * p + radius * n).normal());

    Vector new_point = cone_radius * p + Vector(cone_p2);
    points_.push_back(new_point);
    colors_.push_back(color1);
    normals_.push_back(normals);
    points_.push_back(Vector(p1));
    colors_.push_back(color2);
    normals_.push_back(normals);
    numVBOElements_ += 2;

    cone_rim_points.push_back(new_point);
  }

  // Add indices
  for (int strips = offset; strips < resolution * points_per_loop + offset; strips += points_per_loop)
  {
    indices_.push_back(strips);
    indices_.push_back(strips + points_per_loop);
    indices_.push_back(strips + 1);
  }


  // Generate ellipsoid
  Vector tangent = dir.getArbitraryTangent();
  Vector bitangent = Cross(dir, tangent);

  Transform trans, rotate;
  generateTransforms(p2, tangent, bitangent, dir, trans, rotate);

  trans.post_scale ( Vector(1.0,1.0,1.0) * radius );
  rotate.post_scale( Vector(1.0,1.0,1.0) / radius );

  int nu = resolution + 1;

  // Half ellipsoid criteria.
  int nv = resolution * (0.5 + sphere_extrusion);

  // Should only happen when doing half ellipsoids.
  if (nv < 2) nv = 2;

  double end = M_PI * (0.5 + sphere_extrusion);

  SinCosTable tab1(nu, 0, 2 * M_PI);
  SinCosTable tab2(nv, 0, end);

  int cone_rim_index = 0;

  // Draw the ellipsoid
  for (int v = 0; v<nv - 1; v++)
  {
    double nr1 = tab2.sin(v + 1);
    double nr2 = tab2.sin(v);

    double nz1 = tab2.cos(v + 1);
    double nz2 = tab2.cos(v);

    for (int u = 0; u<nu; u++)
    {
      uint32_t offset = static_cast<uint32_t>(numVBOElements_);
      double nx = tab1.sin(u);
      double ny = tab1.cos(u);

      double x1 = nr1 * nx;
      double y1 = nr1 * ny;
      double z1 = nz1;

      double x2 = nr2 * nx;
      double y2 = nr2 * ny;
      double z2 = nz2;

      // Rotate and translate points
      Vector p1 = Vector(trans * Point(x1, y1, z1));
      Vector p2 = Vector(trans * Point(x2, y2, z2));

      // Rotate norms
      Vector v1 = rotate * Vector(x1, y1, z1);
      Vector v2 = rotate * Vector(x2, y2, z2);

      v1.safe_normalize();
      v2.safe_normalize();

      // Use cone points around rim of ellipsoid
      if(v == nv - 2)
      {
        points_.push_back(cone_rim_points[cone_rim_index]);
        cone_rim_index++;
      }
      else
      {
        points_.push_back(p1);
      }
      points_.push_back(p2);

      // Add normals
      normals_.push_back(v1);
      normals_.push_back(v2);

      // Add color vectors from parameters
      colors_.push_back(color1);
      colors_.push_back(color1);

      numVBOElements_ += 2;

      indices_.push_back(0 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(3 + offset);
    }
    for(int jj = 0; jj < 6; jj++) indices_.pop_back();
  }
}

void reorderTensor(std::vector<Vector>& eigvectors, Vector& eigvals)
{
  if(eigvals[0] < eigvals[1])
  {
    double temp = eigvals[0];
    eigvals[0] = eigvals[1];
    eigvals[1] = temp;
    Vector tempVec = eigvectors[0];
    eigvectors[0] = eigvectors[1];
    eigvectors[1] = tempVec;
  }
  if(eigvals[1] < eigvals[2])
  {
    double temp = eigvals[1];
    eigvals[1] = eigvals[2];
    eigvals[2] = temp;
    Vector tempVec = eigvectors[1];
    eigvectors[1] = eigvectors[2];
    eigvectors[2] = tempVec;
  }
  if(eigvals[0] < eigvals[1])
  {
    double temp = eigvals[0];
    eigvals[0] = eigvals[1];
    eigvals[1] = temp;
    Vector tempVec = eigvectors[0];
    eigvectors[0] = eigvectors[1];
    eigvectors[1] = tempVec;
  }
}

std::vector<Vector> GlyphGeom::generateBoxPoints(const Transform& trans, const Vector& eigvals)
{
  std::vector<Vector> box_points;
  for(int x : {-1, 1})
    {
      for(int y : {-1, 1})
        {
          for(int z : {-1, 1})
            {
              box_points.emplace_back(trans * Point(x * eigvals.x(), y * eigvals.y(), z * eigvals.z()));
            }
        }
    }
  return box_points;
}

void GlyphGeom::generateBox(const Point& center, Tensor& t, double scale, ColorRGB& node_color, bool normalize)
{
  static const double zeroThreshold = 0.000001;
  double eigval1, eigval2, eigval3;
  t.get_eigenvalues(eigval1, eigval2, eigval3);

  Vector eigvals(abs(eigval1), abs(eigval2), abs(eigval3));
  if(normalize)
    eigvals.normalize();
  eigvals *= scale;

  std::vector<Vector> eigvectors(3);
  t.get_eigenvectors(eigvectors[0], eigvectors[1], eigvectors[2]);

  // Checks to see if eigenvalues are close to 0
  bool eig_x_0 = eigvals.x() <= zeroThreshold;
  bool eig_y_0 = eigvals.y() <= zeroThreshold;
  bool eig_z_0 = eigvals.z() <= zeroThreshold;

  // Set to 0 if below threshold
  eigvals[0] = (!eig_x_0) * eigvals[0];
  eigvals[1] = (!eig_y_0) * eigvals[1];
  eigvals[2] = (!eig_z_0) * eigvals[2];

  bool flatTensor = (eig_x_0 + eig_y_0 + eig_z_0) >= 1;
  if(flatTensor)
  {
    reorderTensor(eigvectors, eigvals);

    eig_x_0 = eigvals.x() <= zeroThreshold;
    eig_y_0 = eigvals.y() <= zeroThreshold;
    eig_z_0 = eigvals.z() <= zeroThreshold;
    // Check for zero eigenvectors
    if(eig_x_0)
    {
      eigvectors[0] = Cross(eigvectors[1], eigvectors[2]);
    }
    else if(eig_y_0)
    {
      eigvectors[1] = Cross(eigvectors[0], eigvectors[2]);
    }
    else if(eig_z_0)
    {
      eigvectors[2] = Cross(eigvectors[0], eigvectors[1]);
    }
  }

  Transform trans, rotate;
  generateTransforms(center, eigvectors[0], eigvectors[1], eigvectors[2], trans, rotate);

  std::vector<Vector> box_points = generateBoxPoints(trans, eigvals);
  std::vector<Vector> column_vectors = rotate.get_column_vectors();

  generateBoxSide(box_points[5], box_points[4], box_points[7], box_points[6], column_vectors[0], node_color);
  generateBoxSide(box_points[7], box_points[6], box_points[3], box_points[2], column_vectors[1], node_color);
  generateBoxSide(box_points[1], box_points[5], box_points[3], box_points[7], column_vectors[2], node_color);
  generateBoxSide(box_points[3], box_points[2], box_points[1], box_points[0], -column_vectors[0], node_color);
  generateBoxSide(box_points[1], box_points[0], box_points[5], box_points[4], -column_vectors[1], node_color);
  generateBoxSide(box_points[2], box_points[6], box_points[0], box_points[4], -column_vectors[2], node_color);
}

void GlyphGeom::generateBoxSide(const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4,
                                const Vector& normal, const ColorRGB& node_color)
{
  size_t offset = static_cast<size_t>(numVBOElements_);
  points_.push_back(p1);
  points_.push_back(p2);
  points_.push_back(p3);
  points_.push_back(p4);

  for(int i = 0; i < 4; i++)
  {
    normals_.push_back(normal);
    colors_.push_back(node_color);
  }
  numVBOElements_ += 4;

  indices_.push_back(offset + 2);
  indices_.push_back(offset);
  indices_.push_back(offset + 3);
  indices_.push_back(offset + 1);
  indices_.push_back(offset + 3);
  indices_.push_back(offset);
}

void GlyphGeom::generateEllipsoid(const Point& center, Tensor& t, double scale, int resolution, const ColorRGB& color, bool half, bool normalize)
{
  static const double zeroThreshold = 0.000001;
  std::vector<Vector> eigvectors(3);
  t.get_eigenvectors(eigvectors[0], eigvectors[1], eigvectors[2]);

  double eigval1, eigval2, eigval3;
  t.get_eigenvalues(eigval1, eigval2, eigval3);
  Vector eigvals = Vector(fabs(eigval1), fabs(eigval2), fabs(eigval3));
  if(normalize)
    eigvals.normalize();
  eigvals *= scale;

  // Checks to see if eigenvalues are close to 0
  bool eig_x_0 = eigvals.x() <= zeroThreshold;
  bool eig_y_0 = eigvals.y() <= zeroThreshold;
  bool eig_z_0 = eigvals.z() <= zeroThreshold;

  // Set to 0 if below threshold
  eigvals[0] = (!eig_x_0) * eigvals[0];
  eigvals[1] = (!eig_y_0) * eigvals[1];
  eigvals[2] = (!eig_z_0) * eigvals[2];

  bool flatTensor = (eig_x_0 + eig_y_0 + eig_z_0) >= 1;
  Vector zero_norm;

  if(flatTensor)
  {
    reorderTensor(eigvectors, eigvals);

    eig_x_0 = eigvals.x() <= zeroThreshold;
    eig_y_0 = eigvals.y() <= zeroThreshold;
    eig_z_0 = eigvals.z() <= zeroThreshold;
    // Check for zero eigenvectors
    if(eig_x_0)
    {
      zero_norm = Cross(eigvectors[1], eigvectors[2]);
      eigvectors[0] = zero_norm;
    }
    else if(eig_y_0)
    {
      zero_norm = Cross(eigvectors[0], eigvectors[2]);
      eigvectors[1] = zero_norm;
    }
    else if(eig_z_0)
    {
      zero_norm = Cross(eigvectors[0], eigvectors[1]);
      eigvectors[2] = zero_norm;
    }
  }

  Transform trans, rotate;
  generateTransforms(center, eigvectors[0], eigvectors[1], eigvectors[2], trans, rotate);

  trans.post_scale (Vector(1.0,1.0,1.0) * eigvals);
  rotate.post_scale(Vector(1.0,1.0,1.0) / eigvals);

  int nu = resolution + 1;

  // Half ellipsoid criteria.
  int nv = resolution;
  if (half) nv /= 2;

  // Should only happen when doing half ellipsoids.
  if (nv < 2) nv = 2;

  double end = half ? M_PI / 2 : M_PI;

  SinCosTable tab1(nu, 0, 2 * M_PI);
  SinCosTable tab2(nv, 0, end);

  // Draw the ellipsoid
  for (int v = 0; v<nv - 1; v++)
  {
    double nr1 = tab2.sin(v + 1);
    double nr2 = tab2.sin(v);

    double nz1 = tab2.cos(v + 1);
    double nz2 = tab2.cos(v);

    for (int u = 0; u<nu; u++)
    {
      uint32_t offset = static_cast<uint32_t>(numVBOElements_);
      double nx = tab1.sin(u);
      double ny = tab1.cos(u);

      double x1 = nr1 * nx;
      double y1 = nr1 * ny;
      double z1 = nz1;

      double x2 = nr2 * nx;
      double y2 = nr2 * ny;
      double z2 = nz2;

      // Rotate and translate points
      Vector p1 = Vector(trans * Point(x1, y1, z1));
      Vector p2 = Vector(trans * Point(x2, y2, z2));

      Vector v1, v2;

      if(flatTensor)
      {
        // Avoids recalculating norm vector and prevents vectors with infinite length
        bool first_half = v < nv/2;
        v1 = first_half ? zero_norm : -zero_norm;
        v2 = first_half ? zero_norm : -zero_norm;
      }
      else
      {
        // Rotate norms
        v1 = rotate * Vector(x1, y1, z1);
        v2 = rotate * Vector(x2, y2, z2);
      }

      v1.safe_normalize();
      v2.safe_normalize();

      // Transorm points and add to points list
      points_.push_back(p1);
      points_.push_back(p2);

      // Add normals
      normals_.push_back(v1);
      normals_.push_back(v2);

      // Add color vectors from parameters
      colors_.push_back(color);
      colors_.push_back(color);

      numVBOElements_ += 2;

      indices_.push_back(0 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(3 + offset);
    }
    for(int jj = 0; jj < 6; jj++) indices_.pop_back();
  }
}

inline double spow(double e, double x)
{
  // This for round off of very small numbers.
  if( abs( e ) < 1.0e-6)
    e = 0.0;

  if (e < 0.0)
  {
    return (double)(pow(abs(e), x) * -1.0);
  }
  else
  {
    return (double)(pow(e, x));
  }
}

void GlyphGeom::generateSuperquadricTensor(const Point& center, Tensor& t, double scale,
                                           int resolution, const ColorRGB& color, bool normalize,
                                           double emphasis)
{
  static const double zeroThreshold = 0.000001;
  std::vector<Vector> eigvectors(3);
  t.get_eigenvectors(eigvectors[0], eigvectors[1], eigvectors[2]);

  double eigval1, eigval2, eigval3;
  t.get_eigenvalues(eigval1, eigval2, eigval3);
  Vector eigvals = Vector(fabs(eigval1), fabs(eigval2), fabs(eigval3));
  if(normalize)
    eigvals.normalize();
  eigvals *= scale;

  // Checks to see if eigenvalues are close to 0
  bool eig_x_0 = eigvals.x() <= zeroThreshold;
  bool eig_y_0 = eigvals.y() <= zeroThreshold;
  bool eig_z_0 = eigvals.z() <= zeroThreshold;

  // Set to 0 if below threshold
  eigvals[0] = (!eig_x_0) * eigvals[0];
  eigvals[1] = (!eig_y_0) * eigvals[1];
  eigvals[2] = (!eig_z_0) * eigvals[2];

  bool flatTensor = (eig_x_0 + eig_y_0 + eig_z_0) >= 1;
  Vector zero_norm;

  if(flatTensor)
  {
    reorderTensor(eigvectors, eigvals);

    eig_x_0 = eigvals.x() <= zeroThreshold;
    eig_y_0 = eigvals.y() <= zeroThreshold;
    eig_z_0 = eigvals.z() <= zeroThreshold;
    // Check for zero eigenvectors
    if(eig_x_0)
    {
      zero_norm = Cross(eigvectors[1], eigvectors[2]);
      eigvectors[0] = zero_norm;
    }
    else if(eig_y_0)
    {
      zero_norm = Cross(eigvectors[0], eigvectors[2]);
      eigvectors[1] = zero_norm;
    }
    else if(eig_z_0)
    {
      zero_norm = Cross(eigvectors[0], eigvectors[1]);
      eigvectors[2] = zero_norm;
    }
  }

  Transform trans, rotate;
  generateTransforms(center, eigvectors[0], eigvectors[1], eigvectors[2], trans, rotate);

  trans.post_scale (Vector(1.0,1.0,1.0) * eigvals);
  rotate.post_scale(Vector(1.0,1.0,1.0) / eigvals);

  int nu = resolution + 1;
  int nv = resolution;

  SinCosTable tab1(nu, 0, 2 * M_PI);
  SinCosTable tab2(nv, 0, M_PI);

  double cl = (eigvals[0] - eigvals[1]) / (eigvals[0] + eigvals[1] + eigvals[2]);
  double cp = 2.0 * (eigvals[1] - eigvals[2]) / (eigvals[0] + eigvals[1] + eigvals[2]);
  bool linear = cl >= cp;
  double A = linear ? spow((1.0 - cp), emphasis) : spow((1.0 - cl), emphasis);
  double B = linear ? spow((1.0 - cl), emphasis) : spow((1.0 - cp), emphasis);

  double nr[2];
  double nz[2];

  for (int v=0; v < nv-1; v++)
  {
    nr[0] = tab2.sin(v+1);
    nr[1] = tab2.sin(v);

    nz[0] = tab2.cos(v+1);
    nz[1] = tab2.cos(v);

    for (int u=0; u<nu; u++)
    {
      double nx = tab1.sin(u);
      double ny = tab1.cos(u);

      uint32_t offset = static_cast<uint32_t>(numVBOElements_);
      for(unsigned int i=0; i<2; i++)
      {
        // Transorm points and add to points list
        double x, y, z;
        if(linear) // Generate around x-axis
        {
          x =  spow(nz[i], B);
          y = -spow(nr[i], B) * spow(ny, A);
          z =  spow(nr[i], B) * spow(nx, A);
        }
        else       // Generate around z-axis
        {
          x = spow(nr[i], B) * spow(nx, A);
          y = spow(nr[i], B) * spow(ny, A);
          z = spow(nz[i], B);
        }
        Vector point = Vector(trans * Point(x, y, z));
        points_.push_back(point);

        // Add normals
        double nnx, nny, nnz;
        if(linear)
        {
          nnx =  spow(nz[i], 2.0-B);
          nny = -spow(nr[i], 2.0-B) * spow(ny, 2.0-A);
          nnz =  spow(nr[i], 2.0-B) * spow(nx, 2.0-A);
        }
        else
        {
          nnx = spow(nr[i], 2.0-B) * spow(nx, 2.0-A);
          nny = spow(nr[i], 2.0-B) * spow(ny, 2.0-A);
          nnz = spow(nz[i], 2.0-B);
        }
        Vector normal = rotate * Vector(nnx, nny, nnz);
        normal.safe_normalize();
        normals_.push_back(normal);

        // Add color vectors from parameters
        colors_.push_back(color);

        numVBOElements_++;
      }

      indices_.push_back(0 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(3 + offset);
    }
  }
  for(int jj = 0; jj < 6; jj++) indices_.pop_back();
}

void GlyphGeom::generateTorus(const Point& p1, const Point& p2, double major_radius, double minor_radius,
                              int resolution, const ColorRGB& color)
{
  int nv = resolution;
  int nu = nv + 1;

  SinCosTable tab1(nu, 0, 2*M_PI);
  SinCosTable tab2(nv, 0, 2*M_PI, minor_radius);

  Transform trans;
  Transform rotate;
  /* Point center = p1 + (p2-p1) * 0.5; */
  generateTransforms( p1, (p2-p1), trans, rotate );

  // Draw the torus
  for (int v=0; v<nv-1; v++)
  {
    double z1 = tab2.cos(v+1);
    double z2 = tab2.cos(v);

    double nr1 = tab2.sin(v+1);
    double nr2 = tab2.sin(v);

    double r1 = major_radius + nr1;
    double r2 = major_radius + nr2;

    for (int u=0; u<nu; u++)
    {
      uint32_t offset = static_cast<uint32_t>(numVBOElements_);

      double nx = tab1.sin(u);
      double ny = tab1.cos(u);

      double x1 = r1 * nx;
      double y1 = r1 * ny;

      double x2 = r2 * nx;
      double y2 = r2 * ny;

      Vector p1 = Vector(trans * Point(x1, y1, z1));
      Vector p2 = Vector(trans * Point(x2, y2, z2));
      points_.push_back(p1);
      points_.push_back(p2);

      Vector v1 = rotate * Vector(nr1*nx, nr1*ny, z1);
      Vector v2 = rotate * Vector(nr2*nx, nr2*ny, z2);
      v1.safe_normalize();
      v2.safe_normalize();
      normals_.push_back(v1);
      normals_.push_back(v2);

      colors_.push_back(color);
      colors_.push_back(color);

      numVBOElements_ += 2;

      indices_.push_back(0 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(2 + offset);
      indices_.push_back(1 + offset);
      indices_.push_back(3 + offset);
    }
  }
  for(int jj = 0; jj < 6; jj++) indices_.pop_back();
}

void GlyphGeom::generateLine(const Point& p1, const Point& p2, const ColorRGB& color1, const ColorRGB& color2)
{
  points_.push_back(Vector(p1));
  colors_.push_back(color1);
  indices_.push_back(lineIndex_);
  ++lineIndex_;
  points_.push_back(Vector(p2));
  colors_.push_back(color2);
  indices_.push_back(lineIndex_);
  ++lineIndex_;
  ++numVBOElements_;
}

void GlyphGeom::generatePoint(const Point& p, const ColorRGB& color)
{
  points_.push_back(Vector(p));
  colors_.push_back(color);
  indices_.push_back(lineIndex_);
  ++lineIndex_;
  ++numVBOElements_;
}

void GlyphGeom::generatePlane(const Point& p1, const Point& p2,
  const Point& p3, const Point& p4, const ColorRGB& color)
{
  points_.push_back(Vector(p1));
  points_.push_back(Vector(p2));
  points_.push_back(Vector(p3));
  points_.push_back(Vector(p4));
  colors_.push_back(color);
  colors_.push_back(color);
  colors_.push_back(color);
  colors_.push_back(color);
  Vector n;
  n = Cross(p2 - p1, p4 - p1).normal();
  normals_.push_back(n);
  n = Cross(p3 - p2, p1 - p2).normal();
  normals_.push_back(n);
  n = Cross(p4 - p3, p2 - p3).normal();
  normals_.push_back(n);
  n = Cross(p1 - p4, p3 - p4).normal();
  normals_.push_back(n);
  indices_.push_back(0 + numVBOElements_);
  indices_.push_back(1 + numVBOElements_);
  indices_.push_back(2 + numVBOElements_);
  indices_.push_back(2 + numVBOElements_);
  indices_.push_back(3 + numVBOElements_);
  indices_.push_back(0 + numVBOElements_);
  numVBOElements_ += 4;
}

// Addarrow from SCIRun 4
void GlyphGeom::addArrow(const Point& center, const Vector& t,
                         double radius, double length, int nu, int nv)
{
  std::vector<QuadStrip> quadstrips;
  double ratio = 2.0;
  Transform trans;
  Transform rotate;
  generateTransforms(center, t, trans, rotate);

  Vector offset = rotate * Vector(0,0,1);
  offset.safe_normalize();
  offset *= length * ratio;

  generateCylinder(center, t, radius/10.0, radius/10.0, length*ratio, nu, nv, quadstrips);
  generateCylinder(center+offset, t, radius, 0.0, length, nu, nv, quadstrips);

  // add strips to the object
}

// from SCIRun 4
void GlyphGeom::addBox(const Point& center, const Vector& t,
                       double x_side, double y_side, double z_side)
{
  std::vector<QuadStrip> quadstrips;
  generateBox(center, t, x_side, y_side, z_side, quadstrips);

  // add strips to object
}

// from SCIRun 4
void GlyphGeom::addCylinder(const Point& center, const Vector& t,
                            double radius1, double length, int nu, int nv)
{
  std::vector<QuadStrip> quadstrips;
  generateCylinder(center, t, radius1, radius1, length, nu, nv, quadstrips);

  // add the strips to the object
}

// from SCIRun 4
void GlyphGeom::addSphere(const Point& center, double radius,
                          int nu, int nv, int half)
{
  std::vector<QuadStrip> quadstrips;
  generateEllipsoid(center, Vector(0, 0, 1), radius, nu, nv, half, quadstrips);

  // add strips to the object

}

// Generate cylinder from SCIRun 4
void GlyphGeom::generateCylinder(const Point& center, const Vector& t, double radius1,
                                 double radius2, double length, int nu, int nv,
                                 std::vector<QuadStrip>& quadstrips)
{
  nu++; //Bring nu to expected value for shape

  if (nu > 20) nu = 20;
  if (nv == 0) nv = 20;
  SinCosTable& tab1 = tables_[nu];

  Transform trans;
  Transform rotate;
  generateTransforms(center, t, trans, rotate);

  // Draw the cylinder
  double dz = length / static_cast<float>(nv);
  double dr = (radius2 - radius1) / static_cast<float>(nv);

  for (int v = 0; v<nv; v++)
  {
    double z1 = dz * static_cast<float>(v);
    double z2 = z1 + dz;

    double r1 = radius1 + dr * static_cast<float>(v);
    double r2 = r1 + dr;

    QuadStrip quadstrip;

    for (int u = 0; u<nu; u++)
    {
      double nx = tab1.sin(u);
      double ny = tab1.cos(u);

      double x1 = r1 * nx;
      double y1 = r1 * ny;

      double x2 = r2 * nx;
      double y2 = r2 * ny;

      double nx1 = length * nx;
      double ny1 = length * ny;

      Point p1 = trans * Point(x1, y1, z1);
      Point p2 = trans * Point(x2, y2, z2);

      Vector v1 = rotate * Vector(nx1, ny1, -dr);
      v1.safe_normalize();

      quadstrip.push_back(std::make_pair(p1, v1));
      quadstrip.push_back(std::make_pair(p2, v1));
    }

    quadstrips.push_back(quadstrip);
  }

}

// generate ellipsoid from SCIRun 4
void GlyphGeom::generateEllipsoid(const Point& center, const Vector& t, double scales,
                                  int nu, int nv, int half, std::vector<QuadStrip>& quadstrips)
{
  nu++; //Bring nu to expected value for shape.

  double start = 0, stop =  M_PI;

  // Half ellipsoid criteria.
  if (half == -1) start = M_PI / 2.0;
  if (half == 1) stop = M_PI / 2.0;
  if (half != 0) nv /= 2;

  // Should only happen when doing half ellipsoids.
  if (nv < 2) nv = 2;

  SinCosTable tab1(nu, 0, 2 * M_PI);
  SinCosTable tab2(nv, start, stop);

  Transform trans;
  Transform rotate;
  generateTransforms(center, t, trans, rotate);

  trans.post_scale(Vector(1.0, 1.0, 1.0) * scales);
  rotate.post_scale(Vector(1.0, 1.0, 1.0) / scales);

  // Draw the ellipsoid
  for (int v = 0; v<nv - 1; v++)
  {
    double nr1 = tab2.sin(v + 1);
    double nr2 = tab2.sin(v);

    double nz1 = tab2.cos(v + 1);
    double nz2 = tab2.cos(v);

    QuadStrip quadstrip;

    for (int u = 0; u<nu; u++)
    {
      double nx = tab1.sin(u);
      double ny = tab1.cos(u);

      double x1 = nr1 * nx;
      double y1 = nr1 * ny;
      double z1 = nz1;

      double x2 = nr2 * nx;
      double y2 = nr2 * ny;
      double z2 = nz2;

      Point p1 = trans * Point(x1, y1, z1);
      Point p2 = trans * Point(x2, y2, z2);

      Vector v1 = rotate * Vector(x1, y1, z1);
      Vector v2 = rotate * Vector(x2, y2, z2);

      v1.safe_normalize();
      v2.safe_normalize();

      quadstrip.push_back(std::make_pair(p1, v1));
      quadstrip.push_back(std::make_pair(p2, v2));
    }

    quadstrips.push_back(quadstrip);
  }
}

//generate box from SCIRun 4
void GlyphGeom::generateBox(const Point& center, const Vector& t, double x_side, double y_side,
                            double z_side, std::vector<QuadStrip>& quadstrips)
{
  double half_x_side = x_side * 0.5;
  double half_y_side = y_side * 0.5;
  double half_z_side = z_side * 0.5;

  Transform trans;
  Transform rotate;
  generateTransforms(center, t, trans, rotate);

  //Draw the Box
  Point p1 = trans * Point(-half_x_side, half_y_side, half_z_side);
  Point p2 = trans * Point(-half_x_side, half_y_side, -half_z_side);
  Point p3 = trans * Point(half_x_side, half_y_side, half_z_side);
  Point p4 = trans * Point(half_x_side, half_y_side, -half_z_side);

  Point p5 = trans * Point(-half_x_side, -half_y_side, half_z_side);
  Point p6 = trans * Point(-half_x_side, -half_y_side, -half_z_side);
  Point p7 = trans * Point(half_x_side, -half_y_side, half_z_side);
  Point p8 = trans * Point(half_x_side, -half_y_side, -half_z_side);

  Vector v1 = rotate * Vector(half_x_side, 0, 0);
  Vector v2 = rotate * Vector(0, half_y_side, 0);
  Vector v3 = rotate * Vector(0, 0, half_z_side);

  Vector v4 = rotate * Vector(-half_x_side, 0, 0);
  Vector v5 = rotate * Vector(0, -half_y_side, 0);
  Vector v6 = rotate * Vector(0, 0, -half_z_side);

  QuadStrip quadstrip1;
  QuadStrip quadstrip2;
  QuadStrip quadstrip3;
  QuadStrip quadstrip4;
  QuadStrip quadstrip5;
  QuadStrip quadstrip6;

  // +X
  quadstrip1.push_back(std::make_pair(p7, v1));
  quadstrip1.push_back(std::make_pair(p8, v1));
  quadstrip1.push_back(std::make_pair(p3, v1));
  quadstrip1.push_back(std::make_pair(p4, v1));

  // +Y
  quadstrip2.push_back(std::make_pair(p3, v2));
  quadstrip2.push_back(std::make_pair(p4, v2));
  quadstrip2.push_back(std::make_pair(p1, v2));
  quadstrip2.push_back(std::make_pair(p2, v2));

  // +Z
  quadstrip3.push_back(std::make_pair(p5, v3));
  quadstrip3.push_back(std::make_pair(p7, v3));
  quadstrip3.push_back(std::make_pair(p1, v3));
  quadstrip3.push_back(std::make_pair(p3, v3));

  // -X
  quadstrip4.push_back(std::make_pair(p1, v4));
  quadstrip4.push_back(std::make_pair(p2, v4));
  quadstrip4.push_back(std::make_pair(p5, v4));
  quadstrip4.push_back(std::make_pair(p6, v4));

  // -Y
  quadstrip5.push_back(std::make_pair(p5, v5));
  quadstrip5.push_back(std::make_pair(p6, v5));
  quadstrip5.push_back(std::make_pair(p7, v5));
  quadstrip5.push_back(std::make_pair(p8, v5));

  // -Z
  quadstrip6.push_back(std::make_pair(p2, v6));
  quadstrip6.push_back(std::make_pair(p4, v6));
  quadstrip6.push_back(std::make_pair(p6, v6));
  quadstrip6.push_back(std::make_pair(p8, v6));

  quadstrips.push_back(quadstrip1);
  quadstrips.push_back(quadstrip2);
  quadstrips.push_back(quadstrip3);
  quadstrips.push_back(quadstrip4);
  quadstrips.push_back(quadstrip5);
  quadstrips.push_back(quadstrip6);
}

// from SCIRun 4
void GlyphGeom::generateTransforms(const Point& center, const Vector& normal,
                                   Transform& trans, Transform& rotate)
{
  Vector axis = normal;

  axis.normalize();

  Vector z(0, 0, 1), zrotaxis;

  if((Abs(axis.x()) + Abs(axis.y())) < 1.e-5)
  {
    // Only x-z plane...
    zrotaxis = Vector(0, 1, 0);
  }
  else
  {
    zrotaxis = Cross(axis, z);
    zrotaxis.normalize();
  }

  double cangle = Dot(z, axis);
  double zrotangle = -acos(cangle);

  trans.pre_translate((Vector) center);
  trans.post_rotate(zrotangle, zrotaxis);

  rotate.post_rotate(zrotangle, zrotaxis);
}

void GlyphGeom::generateTransforms(const Point& center, const Vector& eigvec1, const Vector& eigvec2,
                                   const Vector& eigvec3, Transform& translate, Transform& rotate)
{
  static const Point origin(0.0, 0.0, 0.0);
  rotate = Transform(origin, eigvec1, eigvec2, eigvec3);
  translate = rotate;
  translate.pre_translate((Vector) center);
}
