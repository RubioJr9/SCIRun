/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2015 Scientific Computing and Imaging Institute,
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

#ifndef Graphics_Widgets_BoundingBoxWidget_H
#define Graphics_Widgets_BoundingBoxWidget_H

#include <Core/Datatypes/Legacy/Field/FieldFwd.h>
#include <Core/GeometryPrimitives/GeomFwd.h>
#include <Graphics/Widgets/Widget.h>
#include <Graphics/Widgets/share.h>

namespace SCIRun {
  namespace Graphics {
    namespace Datatypes {
      enum BoundingBoxWidgetSection {
        BOX,
        CORNER_SCALE,
        FACE_ROTATE,
        FACE_SCALE,
        X_PLUS,
        Y_PLUS,
        Z_PLUS,
        X_MINUS,
        Y_MINUS,
        Z_MINUS };

      class SCISHARE BoundingBoxWidget : public CompositeWidget
      {
      public:
        BoundingBoxWidget(const Core::GeometryIDGenerator& idGenerator, const std::string& name,
                          double scale, const Core::Geometry::Point& center,
                          const std::vector<Core::Geometry::Vector>& eigvecs_,
                          const std::vector<double>& eigvals_,
                          int widget_num, int widget_iter);
        BoundingBoxWidget(const Core::GeometryIDGenerator& idGenerator, const std::string& name,
                          double scale, const BoxPosition& pos, const Core::Geometry::Point& center,
                          int widget_num, int widget_iter, const Core::Geometry::BBox& bbox);

        BoundingBoxWidget(const Core::GeometryIDGenerator& idGenerator, const std::string& name,
                          double scale, const Core::Geometry::Transform& trans,
                          const Core::Geometry::Point& center, int widget_num, int widget_iter);

      private:
        std::string widgetName(size_t i, size_t id, size_t iter);

        int resolution_ = 10;
        const std::string deflPointCol_ {Core::Datatypes::ColorRGB(0.54, 0.6, 1.0).toString()};
        const std::string deflCol_ {Core::Datatypes::ColorRGB(0.5, 0.5, 0.5).toString()};
        const std::string resizeCol_ {Core::Datatypes::ColorRGB(0.54, 1.0, 0.60).toString()};
        std::string diskCol_ = resizeCol_;

        int widgetsIndex_;
        double scale_;
        double diagonalLength_;
        Core::Geometry::BBox bbox_;
        Core::Geometry::Point center_;
        std::vector<double> eigvals_;
        std::vector<Core::Geometry::Vector> eigvecs_;
        std::vector<Core::Geometry::Vector> scaledEigvecs_;
        std::vector<Core::Geometry::Point> corners_;
        std::vector<Core::Geometry::Point> facesStart_;
        std::vector<Core::Geometry::Point> facesEnd_;
        const float boxScale_ {0.75};
        const float rotSphereScale_ {1.5};
        const float resizeSphereScale_ {1.0};
        const float faceScale_ {1.0};
        const float diskWidth_ {0.3};
        const float diskRadius_ {0.75};

        void createWidgets(const Core::GeometryIDGenerator& idGenerator, int widgetNum,
                           int widgetIter);
        void initWidgetCreation(const Core::GeometryIDGenerator& idGenerator, int widgetNum,
                                int widgetIter);
        void getEigenValuesAndEigenVectors();
        void getCorners();
        void getFacesStart();
        void getFacesEnd();
        void addCornerSphere(int i, const Core::GeometryIDGenerator& idGenerator,
                             const Core::Geometry::Vector& flipVec, int widgetNum, int widgetIter);
        void addFaceSphere(int i, const Core::GeometryIDGenerator& idGenerator, int widgetNum,
                           int widgetIter);
        void addFaceCylinder(int i, const Core::GeometryIDGenerator& idGenerator,
                             glm::mat4& scaleTrans, int axisNum, int widgetNum, int widgetIter);
        void addIds();
      };

      using BoundingBoxWidgetHandle = SharedPointer<BoundingBoxWidget>;
    }
  }
}
#endif
