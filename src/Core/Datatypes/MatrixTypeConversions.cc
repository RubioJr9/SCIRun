/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
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

#include <Core/Datatypes/MatrixTypeConversions.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/SparseRowMatrix.h>
#include <Core/Datatypes/DenseColumnMatrix.h>
#include <Core/Datatypes/SparseRowMatrixFromMap.h>

using namespace SCIRun::Core::Datatypes;
using namespace SCIRun;

DenseMatrixHandle matrix_cast::as_dense(const MatrixHandle& mh)
{
  return to<DenseMatrix>(mh);
}

SparseRowMatrixHandle matrix_cast::as_sparse(const MatrixHandle& mh)
{
  return to<SparseRowMatrix>(mh);
}

DenseColumnMatrixHandle matrix_cast::as_column(const MatrixHandle& mh)
{
  return to<DenseColumnMatrix>(mh);
}

bool matrix_is::dense(const MatrixHandle& mh) 
{ 
  return matrix_cast::as_dense(mh) != 0; 
}

bool matrix_is::sparse(const MatrixHandle& mh) 
{ 
  return matrix_cast::as_sparse(mh) != 0; 
}

bool matrix_is::column(const MatrixHandle& mh) 
{ 
  return matrix_cast::as_column(mh) != 0; 
}

std::string matrix_is::whatType(const MatrixHandle& mh)
{
  if (!mh)
    return "<null>";
  if (matrix_is::column(mh))
    return "DenseColumnMatrix";
  else if (matrix_is::dense(mh))
    return "DenseMatrix";
  else if (matrix_is::sparse(mh))
    return "SparseRowMatrix";
  return typeid(*mh).name(); 
}

DenseColumnMatrixHandle matrix_convert::to_column(const MatrixHandle& mh)
{
  auto col = matrix_cast::as_column(mh);
  if (col)
    return col;

  auto dense = matrix_cast::as_dense(mh);
  if (dense)
    return boost::make_shared<DenseColumnMatrix>(dense->col(0));

  return DenseColumnMatrixHandle();
}

DenseColumnMatrixHandle matrix_convert::to_column_md(const MatrixHandle& mh)
{
  auto col = matrix_cast::as_column(mh);
  if (col)
    return col;

  auto dense = matrix_cast::as_dense(mh);
  if (dense)
    return boost::make_shared<DenseColumnMatrix>(dense->col(0));

  auto sparse = matrix_cast::as_sparse(mh);
  if (sparse)
     {
       DenseColumnMatrix dense_col(sparse->nrows());
       for (Eigen::SparseVector<double>::InnerIterator it(sparse->row(0)); it; ++it)
          dense_col(it.index())=it.value(); 
   
       return boost::make_shared<DenseColumnMatrix>(dense_col);
     }
          
  return DenseColumnMatrixHandle();
}

DenseMatrixHandle matrix_convert::to_dense_md(const MatrixHandle& mh)
{
  auto dense = matrix_cast::as_dense(mh);
  if (dense)
    return dense;
  
  auto col = matrix_cast::as_column(mh);
  if (col)
    return boost::make_shared<DenseMatrix>(col->row(0));
   
  auto sparse = matrix_cast::as_sparse(mh);
  if (sparse)
    {
     DenseMatrix dense_matrix(sparse->nrows(),sparse->ncols()); 
     for (index_type k = 0; k < sparse->outerSize(); k++)
     {
      for (Eigen::SparseVector<double>::InnerIterator it(sparse->row(k)); it; ++it)
        dense_matrix(k,it.index())=sparse->coeff(k,it.index());   
     }
     return boost::make_shared<DenseMatrix>(dense_matrix);
    }
   
  return DenseMatrixHandle();
}

SparseRowMatrixHandle matrix_convert::to_sparse_md(const MatrixHandle& mh)
{
 auto sparse = matrix_cast::as_sparse(mh);
 if (sparse)
   return sparse;

 auto col = matrix_cast::as_column(mh);
 if (col)
   {
     SparseRowMatrixFromMap::Values data;
     for (index_type i=0; i<col->nrows(); i++)
       if (col->coeff(i,0)!=0)
         data[i][0]=col->coeff(i,0);
          
     return SparseRowMatrixFromMap::make(col->nrows(), 1, data);
   }

 auto dense = matrix_cast::as_dense(mh);
 if (dense)
   {
    SparseRowMatrixFromMap::Values data;  
    for (index_type i=0; i<dense->nrows(); i++)
      for (index_type j=0; j<dense->ncols(); j++) 
        if (col->coeff(i,j)!=0)
	  data[i][j]=col->coeff(i,j);
	  
     return SparseRowMatrixFromMap::make(dense->nrows(), dense->ncols(), data); 
   }

 return SparseRowMatrixHandle();
}
