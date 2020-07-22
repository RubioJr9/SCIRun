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

#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/Dyadic3DTensor.h>
#include <Core/Datatypes/TensorFwd.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "Core/Utils/Exception.h"

using namespace SCIRun;
using namespace Core::Datatypes;
using namespace Core::Geometry;
using namespace ::testing;

const std::vector<Vector> nativeEigvecs = {
    Vector(1.6, 0.9, 4.3), Vector(4.0, 6.4, 7), Vector(6, 34, 1)};
const std::string eigvecsString = "[1.6 0.9 4.3 4 6.4 7 6 34 1]";

std::vector<DenseColumnMatrixGeneric<double, 3>> getEigenEigvecs()
{
  std::vector<DenseColumnMatrixGeneric<double, 3>> eigvecs(3);
  for (int i = 0; i < 3; ++i)
  {
    eigvecs[i] = DenseColumnMatrixGeneric<double, 3>();
    for (int j = 0; j < 3; ++j)
      eigvecs[i][j] = nativeEigvecs[i][j];
  }
  return eigvecs;
}

TEST(Dyadic3DTensorTest, ConstructTensorWithNativeVectors)
{
  Dyadic3DTensor t(nativeEigvecs[0], nativeEigvecs[1], nativeEigvecs[2]);
  std::stringstream ss;
  ss << t;
  ASSERT_EQ(eigvecsString, ss.str());
}

TEST(Dyadic3DTensorTest, ConstructTensorWithEigenColumnMatrices1)
{
  Dyadic3DTensor t(getEigenEigvecs());
  std::stringstream ss;
  ss << t;
  ASSERT_EQ(eigvecsString, ss.str());
}

TEST(Dyadic3DTensorTest, ConstructTensorWithEigenColumnMatrices2)
{
  auto eigvecs = getEigenEigvecs();
  Dyadic3DTensor t(eigvecs[0], eigvecs[1], eigvecs[2]);
  std::stringstream ss;
  ss << t;
  ASSERT_EQ(eigvecsString, ss.str());
}

TEST(Dyadic3DTensorTest, ConstructTensorWithDoubles)
{
  Dyadic3DTensor t(1, 2, 3, 4, 5, 6);
  std::stringstream ss;
  ss << t;
  ASSERT_EQ("[1 2 3 2 4 5 3 5 6]", ss.str());
}

TEST(Dyadic3DTensorTest, CanConstructWithColumnMatrixOfSixValues)
{
  Dyadic3DTensor t =
      symmetricTensorFromSixElementArray(DenseColumnMatrixGeneric<double, 6>({1, 2, 3, 4, 5, 6}));
  std::stringstream ss;
  ss << t;
  ASSERT_EQ("[1 2 3 2 4 5 3 5 6]", ss.str());
}

TEST(Dyadic3DTensorTest, CannotConstructWithColumnMatrixOfFiveValues)
{
  ASSERT_ANY_THROW(
      symmetricTensorFromSixElementArray(DenseColumnMatrixGeneric<double, 5>({1, 2, 3, 4, 5})));
}

TEST(Dyadic3DTensorTest, CanConstructWithVectorOfSixValues)
{
  std::vector<double> list = {1, 2, 3, 4, 5, 6};
  Dyadic3DTensor t = symmetricTensorFromSixElementArray(list);
  std::stringstream ss;
  ss << t;
  ASSERT_EQ("[1 2 3 2 4 5 3 5 6]", ss.str());
}

TEST(Dyadic3DTensorTest, CanConstructWithInitializerListOfSixValues)
{
  Dyadic3DTensor t = symmetricTensorFromSixElementArray({1, 2, 3, 4, 5, 6});
  std::stringstream ss;
  ss << t;
  ASSERT_EQ("[1 2 3 2 4 5 3 5 6]", ss.str());
}

TEST(Dyadic3DTensorTest, CannotConstructTensorWithLessThanThreeNativeEigenvectors)
{
  ASSERT_ANY_THROW(Dyadic3DTensor t({nativeEigvecs[0], nativeEigvecs[1]}));
}

TEST(Dyadic3DTensorTest, CannotConstructTensorWithLessThanThreeEigenEigenvectors)
{
  auto eigvecs = getEigenEigvecs();
  ASSERT_ANY_THROW(Dyadic3DTensor t({eigvecs[0], eigvecs[1]}));
}

TEST(Dyadic3DTensorTest, CannotConstructTensorWithMoreThanThreeEigenvectors)
{
  ASSERT_ANY_THROW(
      Dyadic3DTensor t({nativeEigvecs[0], nativeEigvecs[1], nativeEigvecs[2], nativeEigvecs[0]}));
}

TEST(Dyadic3DTensorTest, CannotConstructTensorWithMoreThanThreeEigenEigenvectors)
{
  auto eigvecs = getEigenEigvecs();
  ASSERT_ANY_THROW(Dyadic3DTensor t({eigvecs[0], eigvecs[1], eigvecs[2], eigvecs[0]}));
}

TEST(Dyadic3DTensorTest, LinearCertainty)
{
  Dyadic3DTensor t(DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 5, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 2}));
  ASSERT_EQ(3.0 / 8.0, t.linearCertainty());
}

TEST(Dyadic3DTensorTest, PlanarCertainty)
{
  Dyadic3DTensor t(DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 5, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 2}));
  ASSERT_EQ(1.0 / 4.0, t.planarCertainty());
}

TEST(Dyadic3DTensorTest, SphericalCertainty)
{
  Dyadic3DTensor t(DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 5, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 2}));
  ASSERT_EQ(3.0 / 8.0, t.sphericalCertainty());
}

TEST(Dyadic3DTensorTest, CertaintySum)
{
  Dyadic3DTensor t(DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 5, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 2}));
  ASSERT_EQ(1.0, t.linearCertainty() + t.planarCertainty() + t.sphericalCertainty());
}

TEST(DyadicTensorTest, CanConstructWithMatrix)
{
  auto mat = DenseMatrix(3, 3, 0.0);
  mat(0, 0) = 1;
  mat(1, 1) = 4;
  mat(2, 2) = 6;
  mat(0, 1) = mat(1, 0) = 2;
  mat(0, 2) = mat(2, 0) = 3;
  mat(1, 2) = mat(2, 1) = 5;

  DyadicTensorGeneric<double, 3> t(mat);
  std::stringstream ss;
  ss << t;
  ASSERT_EQ("[1 2 3 2 4 5 3 5 6]", ss.str());
}

TEST(DyadicTensorTest, ConstructTensorWithEigenVectors)
{
  std::vector<DenseColumnMatrixGeneric<double, 4>> eigvecs = {DenseColumnMatrixGeneric<double, 4>(),
      DenseColumnMatrixGeneric<double, 4>(), DenseColumnMatrixGeneric<double, 4>(),
      DenseColumnMatrixGeneric<double, 4>()};
  int n = 0;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      eigvecs[i][j] = ++n;
  Dyadic4DTensor t(eigvecs);
  std::stringstream ss;
  ss << t;

  ASSERT_EQ("[1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16]", ss.str());
}

TEST(DyadicTensorTest, StringConversion)
{
  std::vector<DenseColumnMatrixGeneric<double, 3>> vecs = {
      DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 2, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 3})};
  Dyadic3DTensor t(nativeEigvecs[0], nativeEigvecs[1], nativeEigvecs[2]);
  DyadicTensorGeneric<double, 3> t2(vecs);

  std::stringstream ss;
  ss << t;
  ss >> t2;
  ASSERT_TRUE(t == t2);
}

TEST(DyadicTensorTest, GetEigenvalues)
{
  std::vector<Vector> vecs = {Vector(1, 0, 0), Vector(0, 2, 0), Vector(0, 0, 3)};
  Dyadic3DTensor t(vecs[0], vecs[1], vecs[2]);
  auto eigvals = t.getEigenvalues();
  ASSERT_EQ(3.0, eigvals[0]);
  ASSERT_EQ(2.0, eigvals[1]);
  ASSERT_EQ(1.0, eigvals[2]);
}

TEST(DyadicTensorTest, GetEigenvectors)
{
  std::vector<DenseColumnMatrixGeneric<double, 3>> vecs = {
      DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 2, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 3})};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      vecs[i][j] = (i == j) ? i + 1 : 0;

  Dyadic3DTensor t(vecs);
  auto eigvecs = t.getEigenvectors();

  std::vector<DenseColumnMatrixGeneric<double, 3>> expected = {
      DenseColumnMatrixGeneric<double, 3>({0, 0, 1}),
      DenseColumnMatrixGeneric<double, 3>({0, 1, 0}),
      DenseColumnMatrixGeneric<double, 3>({1, 0, 0})};

  for (int i = 0; i < 3; ++i)
    ASSERT_EQ(expected[i], eigvecs[i]);
}

TEST(DyadicTensorTest, GetEigenvector)
{
  std::vector<DenseColumnMatrixGeneric<double, 3>> vecs = {
      DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 2, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 3})};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      vecs[i][j] = (i == j) ? i + 1 : 0;

  Dyadic3DTensor t(vecs);

  auto expected = DenseColumnMatrixGeneric<double, 3>({0, 1, 0});
  ASSERT_EQ(expected, t.getEigenvector(1));
}

TEST(DyadicTensorTest, Equivalent)
{
  std::vector<DenseColumnMatrixGeneric<double, 3>> vecs = {
      DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 2, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 3})};
  Dyadic3DTensor t(nativeEigvecs[0], nativeEigvecs[1], nativeEigvecs[2]);
  DyadicTensorGeneric<double, 3> t2(vecs);
  auto t3 = DyadicTensorGeneric<double, 3>();

  ASSERT_TRUE(t != t2);
  ASSERT_TRUE(t2 != t);
  ASSERT_TRUE(t != t2);
  ASSERT_TRUE(t2 != t);
  ASSERT_TRUE(t == t);
  ASSERT_TRUE(t2 == t2);
  ASSERT_TRUE(t2 != t3);
}

TEST(DyadicTensorTest, DifferentDimensionsNotEquivalent)
{
  Dyadic3DTensor t({DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 1, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 1})});
  Dyadic4DTensor t2({DenseColumnMatrixGeneric<double, 4>({1, 0, 0, 0}),
      DenseColumnMatrixGeneric<double, 4>({0, 1, 0, 0}),
      DenseColumnMatrixGeneric<double, 4>({0, 0, 1, 0}),
      DenseColumnMatrixGeneric<double, 4>({0, 0, 0, 1})});

  ASSERT_DEATH(t != t2, "");
}

TEST(DyadicTensorTest, EqualsOperatorTensor)
{
  Dyadic3DTensor t({DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 1, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 1})});
  Dyadic3DTensor t2 = t;

  ASSERT_TRUE(t == t2);

  t2(1, 1) = 3;

  ASSERT_TRUE(t != t2);
}

TEST(DyadicTensorTest, EqualsOperatorDouble)
{
  Dyadic3DTensor t({DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 1, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0, 1})});
  Dyadic3DTensor t2({DenseColumnMatrixGeneric<double, 3>({5, 5, 5}),
      DenseColumnMatrixGeneric<double, 3>({5, 5, 5}),
      DenseColumnMatrixGeneric<double, 3>({5, 5, 5})});

  t = 5;

  ASSERT_TRUE(t == t2);
}

TEST(DyadicTensorTest, PlusEqualsTensorOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor t2(
      {DenseColumnMatrixGeneric<double, 2>({6, 3}), DenseColumnMatrixGeneric<double, 2>({4, 6})});
  Dyadic2DTensor expected(
      {DenseColumnMatrixGeneric<double, 2>({8, 11}), DenseColumnMatrixGeneric<double, 2>({9, 9})});

  Dyadic2DTensor result = t;
  result += t2;

  ASSERT_TRUE(expected == result);
}

// Coefficient wise multiplication
TEST(DyadicTensorTest, MultiplyTensorOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor t2(
      {DenseColumnMatrixGeneric<double, 2>({6, 3}), DenseColumnMatrixGeneric<double, 2>({4, 6})});
  Dyadic2DTensor expected({DenseColumnMatrixGeneric<double, 2>({12, 24}),
      DenseColumnMatrixGeneric<double, 2>({20, 18})});

  Dyadic2DTensor result = t * t2;

  ASSERT_TRUE(expected == result);
}

// Coefficient wise multiplication
TEST(DyadicTensorTest, MultiplyDoubleOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor expected(
      {DenseColumnMatrixGeneric<double, 2>({6, 24}), DenseColumnMatrixGeneric<double, 2>({15, 9})});

  Dyadic2DTensor result = t * 3.0;

  ASSERT_TRUE(expected == result);
}

// Coefficient wise multiplication
TEST(DyadicTensorTest, MultiplyDoubleReverseOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor expected(
      {DenseColumnMatrixGeneric<double, 2>({6, 24}), DenseColumnMatrixGeneric<double, 2>({15, 9})});

  Dyadic2DTensor result = 3.0 * t;

  ASSERT_TRUE(expected == result);
}

// Coefficient wise multiplication
TEST(DyadicTensorTest, MultiplyEqualsTensorOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor t2(
      {DenseColumnMatrixGeneric<double, 2>({6, 3}), DenseColumnMatrixGeneric<double, 2>({4, 6})});
  Dyadic2DTensor expected({DenseColumnMatrixGeneric<double, 2>({12, 24}),
      DenseColumnMatrixGeneric<double, 2>({20, 18})});

  Dyadic2DTensor result = t;
  result *= t2;

  ASSERT_TRUE(expected == result);
}

// Analog Equivalent of matrix multiplication
TEST(DyadicTensorTest, Contraction)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor t2(
      {DenseColumnMatrixGeneric<double, 2>({6, 3}), DenseColumnMatrixGeneric<double, 2>({4, 6})});
  Dyadic2DTensor expected({DenseColumnMatrixGeneric<double, 2>({27, 57}),
      DenseColumnMatrixGeneric<double, 2>({38, 50})});

  Dyadic2DTensor result = t.contract(t2);

  ASSERT_TRUE(expected == result);
}

TEST(DyadicTensorTest, MinusOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor t2(
      {DenseColumnMatrixGeneric<double, 2>({6, 3}), DenseColumnMatrixGeneric<double, 2>({4, 6})});
  Dyadic2DTensor expected(
      {DenseColumnMatrixGeneric<double, 2>({-4, 5}), DenseColumnMatrixGeneric<double, 2>({1, -3})});

  Dyadic2DTensor result = t - t2;

  ASSERT_TRUE(expected == result);
}

TEST(DyadicTensorTest, MinusEqualsOperator)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({2, 8}), DenseColumnMatrixGeneric<double, 2>({5, 3})});
  Dyadic2DTensor t2(
      {DenseColumnMatrixGeneric<double, 2>({6, 3}), DenseColumnMatrixGeneric<double, 2>({4, 6})});
  Dyadic2DTensor expected(
      {DenseColumnMatrixGeneric<double, 2>({-4, 5}), DenseColumnMatrixGeneric<double, 2>({1, -3})});

  Dyadic2DTensor result = t;
  result -= t2;

  ASSERT_TRUE(expected == result);
}

TEST(DyadicTensorTest, FrobeniusNorm)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({3, 0}), DenseColumnMatrixGeneric<double, 2>({0, 6})});
  ASSERT_EQ(std::sqrt(45), t.frobeniusNorm());
}

TEST(DyadicTensorTest, MaxNorm)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({3, 0}), DenseColumnMatrixGeneric<double, 2>({0, 6})});
  ASSERT_EQ(6, t.maxNorm());
}

TEST(DyadicTensorTest, SetEigens)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({3, 0}), DenseColumnMatrixGeneric<double, 2>({0, 6})});
  std::vector<double> eigvals = {3, 4};
  std::vector<DenseColumnMatrixGeneric<double, 2>> eigvecs = {
      DenseColumnMatrixGeneric<double, 2>({0, 1}), DenseColumnMatrixGeneric<double, 2>({1, 0})};
  t.setEigens(eigvecs, eigvals);
  ASSERT_EQ(eigvals, t.getEigenvalues());
  ASSERT_EQ(eigvecs, t.getEigenvectors());
}

TEST(DyadicTensorTest, SetEigensFail1)
{
  Dyadic2DTensor t(
      {DenseColumnMatrixGeneric<double, 2>({3, 0}), DenseColumnMatrixGeneric<double, 2>({0, 6})});
  ASSERT_ANY_THROW(t.setEigens(
      {DenseColumnMatrixGeneric<double, 2>({0, 1}), DenseColumnMatrixGeneric<double, 2>({1, 0})},
      {3, 4, 5}));
}

// TEST(DyadicTensorTest, SetEigensFail2)
// {
  // Dyadic2DTensor t(
      // {DenseColumnMatrixGeneric<double, 2>({3, 0}), DenseColumnMatrixGeneric<double, 2>({0, 6})});
  // ASSERT_FAIL(t.setEigens({DenseColumnMatrixGeneric<double, 3>({0, 0, 1}),
                              // DenseColumnMatrixGeneric<double, 3>({1, 0, 0}),
                              // DenseColumnMatrixGeneric<double, 3>({0, 1, 0})},
      // {3, 4}));
// }

TEST(DyadicTensorTest, EigenSolver)
{
  Eigen::Matrix3d m;
  m << 3, 0, 0, 0, 2, 0.5, 0, 0.5, 1;
  auto t = Dyadic3DTensor(m);

  std::vector<DenseColumnMatrix> expected = {DenseColumnMatrixGeneric<double, 3>({1.0, 0, 0}),
      DenseColumnMatrixGeneric<double, 3>({0, 0.923879, 0.382684}),
      DenseColumnMatrixGeneric<double, 3>({0, -0.382681, 0.923881})};

  auto eigvecs = t.getEigenvectors();

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      ASSERT_NEAR(expected[i][j], eigvecs[i][j], 0.00001);
}

TEST(DyadicTensorTest, NonSymmetricTestFail)
{
  Eigen::Matrix3d m;
  m << 3, 0, 0, 0, 2, 0, 0, 0.5, 1;
  ASSERT_ANY_THROW(auto t = Dyadic3DTensor(m));
}
