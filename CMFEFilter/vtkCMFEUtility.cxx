/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCMFEUtility.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
*
* Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file is  part of VisIt. For  details, see https://visit.llnl.gov/.  The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of  the LLNS/LLNL nor the names of  its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED. IN  NO EVENT  SHALL LAWRENCE  LIVERMORE NATIONAL  SECURITY,
* LLC, THE  U.S.  DEPARTMENT OF  ENERGY  OR  CONTRIBUTORS BE  LIABLE  FOR  ANY
* DIRECT,  INDIRECT,   INCIDENTAL,   SPECIAL,   EXEMPLARY,  OR   CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

#include <vtkCMFEUtility.h>

#include <float.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDataSetWriter.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkGenericCell.h>
#include <vtkHexahedron.h>
#include <vtkIntArray.h>
#include <vtkMPICommunicator.h>
#include <vtkMultiProcessController.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkRectilinearGrid.h>
#include <vtkShortArray.h>
#include <vtkStructuredGrid.h>
#include <vtkToolkits.h>

#include <cstring>
  

namespace
  {  
#ifdef VTK_USE_MPI  
  static MPI_Op MPI_MINMAX_FUNC = MPI_OP_NULL;  
  static int numberOfProcesses = 1;
  static bool mpiOn = true;
  MPI_Comm *comm;

  //----------------------------------------------------------------------------
  static void MinMaxOp(void *ibuf, void *iobuf, int *len, MPI_Datatype *dtype)
  {
    int i;
    double *iovals = (double *) iobuf;
    double  *ivals = (double *) ibuf;


    // there is a chance, albeit slim for small values of *len, that if MPI
    // decides to chop up the buffers, it could decide to chop them on an
    // odd boundary. That would be catastrophic!
    if (*len % 2 != 0)
    {
        return;
    }

    // handle the minimums by copying any values in ibuff that are less than
    // respective value in iobuff into iobuff
    for (i = 0; i < *len; i += 2)
    {
        if (ivals[i] < iovals[i])
            iovals[i] = ivals[i];
    }

    // handle the maximums by copying any values in ibuff that are greater than
    // respective value in iobuff into iobuff
    for (i = 1; i < *len; i += 2)
    {
        if (ivals[i] > iovals[i])
            iovals[i] = ivals[i];
    }
    return;
  }
#endif
  //----------------------------------------------------------------------------
  double EquationsValueAtPoint(const double *params, int block, int point, int nDims, const double *nodeExtents)
  {
    static int  encoding[32];
    static bool firstTimeThrough = true;
    if (firstTimeThrough)
      {
      encoding[0] = 1;
      for (int i = 1 ; i < 32 ; i++)
        {
        encoding[i] = encoding[i-1] << 1;
        }
      }

    double rv = 0;
    for (int i = 0 ; i < nDims ; i++)
      {
      if (point & encoding[i])
        {
        // The encoded point wants us to take the maximum extent for this
        // dimension.
        rv += params[i] * nodeExtents[block*nDims*2 + 2*i + 1];
        }
      else
        {
        // The encoded point wants us to take the minimum extent for this
        // dimension.
        rv += params[i] * nodeExtents[block*nDims*2 + 2*i];
        }
      }

      return rv;
    }
}



//----------------------------------------------------------------------------
void CMFEUtility::Setup( )
{
#ifdef VTK_USE_MPI
  vtkMultiProcessController *controller = vtkMultiProcessController::GetGlobalController();
  vtkMPICommunicator *communicator = vtkMPICommunicator::SafeDownCast( 
    controller->GetCommunicator() );

  if ( communicator )
    {
    numberOfProcesses = communicator->GetNumberOfProcesses();    
    comm = communicator->GetMPIComm()->GetHandle();
    mpiOn = true;
    }
  else
    {    
    numberOfProcesses = 1;
    comm = NULL;
    mpiOn = false;
    }
#endif
}

#ifdef VTK_USE_MPI
//----------------------------------------------------------------------------
MPI_Comm* CMFEUtility::GetMPIComm()
  {
  return  comm;
  }
#endif

//----------------------------------------------------------------------------
int CMFEUtility::PAR_Size(void)
{
#ifdef VTK_USE_MPI
  return numberOfProcesses;    
#else
  return 1;
#endif
}

//----------------------------------------------------------------------------
bool CMFEUtility::UnifyMinMax(double *buff, int size)
{
//have to make sure that the user is running with mpi compiled, and than
//make sure they are running not in standalone mode
#ifdef VTK_USE_MPI
  if ( mpiOn ) 
    {
    // if it hasn't been created yet, create the min/max MPI reduction operator
    if (MPI_MINMAX_FUNC == MPI_OP_NULL)
      {
      MPI_Op_create(MinMaxOp, true, &MPI_MINMAX_FUNC);
      }
     
    if (size % 2 != 0)
      {
      return false;
      }

    double *rbuff;
    rbuff = new double[size];
    MPI_Allreduce(buff, rbuff, size, MPI_DOUBLE, MPI_MINMAX_FUNC, *CMFEUtility::GetMPIComm());
    
    // put the reduced results back into buff
    for (int i = 0; i < size ; i++)
      {
      buff[i] = rbuff[i];
      }

    delete [] rbuff;
    return true;    
    }
#endif
  return true;
}

//----------------------------------------------------------------------------
float CMFEUtility::UnifyMinimumValue(float value)
{
#ifdef VTK_USE_MPI
  if ( mpiOn ) 
    {
    float allmin;
    MPI_Allreduce(&value, &allmin, 1, MPI_FLOAT, MPI_MIN, *CMFEUtility::GetMPIComm());
    return allmin;
    }
#endif  
  return value;
}

//----------------------------------------------------------------------------
float CMFEUtility::UnifyMaximumValue(float value)
{
#ifdef VTK_USE_MPI
  if ( mpiOn ) 
    {
    float allmax;
    MPI_Allreduce(&value, &allmax, 1, MPI_FLOAT, MPI_MAX, *CMFEUtility::GetMPIComm());
    return allmax;    
    }
#endif
  return value;
}

//----------------------------------------------------------------------------
int CMFEUtility::SumIntAcrossAllProcessors(int value)
{
#ifdef VTK_USE_MPI
  if ( mpiOn ) 
    {
    int sum;
    MPI_Allreduce(&value, &sum, 1, MPI_INT, MPI_SUM, *CMFEUtility::GetMPIComm());
    return sum;
    }
#endif
  return value;
}

//----------------------------------------------------------------------------
void CMFEUtility::SumIntArrayAcrossAllProcessors(int *inArray, int *outArray, int size)
{
#ifdef VTK_USE_MPI
  if ( mpiOn ) 
    {
    MPI_Allreduce(inArray, outArray, size, MPI_INT, MPI_SUM, *CMFEUtility::GetMPIComm());
    return;
    }
  //fall through for both compiled && enabled
#endif
  for (int i = 0 ; i < size ; i++)
    {
    outArray[i] = inArray[i];
    }
  return;
}

//----------------------------------------------------------------------------
char * CMFEUtility::CreateMessageStrings(char **lists, int *count, int nl)
{  
  // Determine how big the big array should be.  
  int total = 0;
  for (int i = 0 ; i < nl ; i++)
    {
    total += count[i];
    }
  
  // Make the array of pointers just point into our one big array.
  char *totallist = new char[total];
  char *totaltmp  = totallist;
  for (int i = 0 ; i < nl ; i++)
    {
    lists[i] = totaltmp;
    totaltmp += count[i];
    }
  
  // Return the big array so the top level routine can clean it up later.  
  return totallist;
}
 
//----------------------------------------------------------------------------
vtkPoints * CMFEUtility::GetPoints(vtkDataSet *dataset)
{
  vtkPoints *pts = vtkPoints::New( );
  if ( !dataset )
    {
    return pts;
    }
 
  vtkIdType size = dataset->GetNumberOfPoints();
  pts->SetNumberOfPoints(size );
    
  for ( vtkIdType i = 0; i < size; ++i)
    {
    pts->InsertPoint(i,dataset->GetPoint(i));
    }      
  return pts;
}

//----------------------------------------------------------------------------
void CMFEUtility::GetCellCenter(vtkCell* cell, double center[3])
{
  double parametricCenter[3] = {0., 0., 0.};
  double coord[3] = {0., 0., 0.};
  int subId = -1;
  if (cell->GetNumberOfPoints() <= 27)
    {
    double weights[27];
    subId = cell->GetParametricCenter(parametricCenter);
    cell->EvaluateLocation(subId, parametricCenter, coord, weights);
    }
  else
    {
    double *weights = new double[cell->GetNumberOfPoints()];
    subId = cell->GetParametricCenter(parametricCenter);
    cell->EvaluateLocation(subId, parametricCenter, coord, weights);
    delete [] weights;
    }
  center[0] = coord[0];
  center[1] = coord[1];
  center[2] = coord[2];
}

//----------------------------------------------------------------------------
bool CMFEUtility::CellContainsPoint(vtkCell *cell, const double *point)
{
  int   i;
  int cellType = cell->GetCellType();
  if (cellType == VTK_HEXAHEDRON)
    {
    vtkHexahedron *hex = (vtkHexahedron *) cell;
    vtkPoints *pts = hex->GetPoints();
    // vtkCell sets its points object data type to double. 
    double *pts_ptr = (double *) pts->GetVoidPointer(0);
    static int faces[6][4] = { {0,4,7,3}, {1,2,6,5},
                       {0,1,5,4}, {3,7,6,2},
                       {0,3,2,1}, {4,5,6,7} };

    double center[3] = { 0., 0., 0. };
    for (i = 0 ; i < 8 ; i++)
      {
      center[0] += pts_ptr[3*i];
      center[1] += pts_ptr[3*i+1];
      center[2] += pts_ptr[3*i+2];
      }
    center[0] /= 8.;
    center[1] /= 8.;
    center[2] /= 8.;

    for (i = 0 ; i < 6 ; i++)
      {
      double dir1[3], dir2[3];
      int idx0 = faces[i][0];
      int idx1 = faces[i][1];
      int idx2 = faces[i][3];
      dir1[0] = pts_ptr[3*idx1] - pts_ptr[3*idx0];
      dir1[1] = pts_ptr[3*idx1+1] - pts_ptr[3*idx0+1];
      dir1[2] = pts_ptr[3*idx1+2] - pts_ptr[3*idx0+2];
      dir2[0] = pts_ptr[3*idx0] - pts_ptr[3*idx2];
      dir2[1] = pts_ptr[3*idx0+1] - pts_ptr[3*idx2+1];
      dir2[2] = pts_ptr[3*idx0+2] - pts_ptr[3*idx2+2];
      double cross[3];
      cross[0] = dir1[1]*dir2[2] - dir1[2]*dir2[1];
      cross[1] = dir1[2]*dir2[0] - dir1[0]*dir2[2];
      cross[2] = dir1[0]*dir2[1] - dir1[1]*dir2[0];
      double origin[3];
      origin[0] = pts_ptr[3*idx0];
      origin[1] = pts_ptr[3*idx0+1];
      origin[2] = pts_ptr[3*idx0+2];

      //
      // The plane is of the form Ax + By + Cz - D = 0.
      //
      // Using the origin, we can calculate D:
      // D = A*origin[0] + B*origin[1] + C*origin[2]
      //
      // We want to know if 'point' gives:
      // A*point[0] + B*point[1] + C*point[2] - D >=? 0.
      //
      // We can substitute in D to get
      // A*(point[0]-origin[0]) + B*(point[1]-origin[1]) + C*(point[2-origin[2])
      //    ?>= 0
      //
      double val1 = cross[0]*(point[0] - origin[0])
                 + cross[1]*(point[1] - origin[1])
                 + cross[2]*(point[2] - origin[2]);

      //
      // If the hexahedron is inside out, then val1 could be
      // negative, because the face orientation is wrong.
      // Find the sign for the cell center.
      //
      double val2 = cross[0]*(center[0] - origin[0])
                 + cross[1]*(center[1] - origin[1])
                 + cross[2]*(center[2] - origin[2]);

      // 
      // If the point in question (point) and the center are on opposite
      // sides of the cell, then declare the point outside the cell.
      //      
      if (val1*val2 < 0.)
        {
        return false;
        }
      }
    return true;
    }


  double closestPt[3];
  int subId;
  double pcoords[3];
  double dist2;
  double weights[100]; // MUST BE BIGGER THAN NPTS IN A CELL (ie 8).
  double non_const_pt[3];
  non_const_pt[0] = point[0];
  non_const_pt[1] = point[1];
  non_const_pt[2] = point[2];
  return (cell->EvaluatePosition(non_const_pt, closestPt, subId,
                                pcoords, dist2, weights) > 0);
}


//----------------------------------------------------------------------------
int CMFEUtility::IntersectBox(const double bounds[6],  const double origin[3], const double dir[3], double coord[3]) 
{
  double Tnear = -DBL_MAX;
  double Tfar = DBL_MAX;
  if (!SlabTest(dir[0], origin[0], bounds[0], bounds[1], Tnear, Tfar))
    {
    return false;
    }
  if (!SlabTest(dir[1], origin[1], bounds[2], bounds[3], Tnear, Tfar))
    {
    return false;
    }
  if (!SlabTest(dir[2], origin[2], bounds[4], bounds[5], Tnear, Tfar))
    {
    return false;
    }

  coord[0] = origin[0] + Tnear *dir[0];
  coord[1] = origin[1] + Tnear *dir[1];
  coord[2] = origin[2] + Tnear *dir[2];

  return true;
}
//----------------------------------------------------------------------------
int CMFEUtility::LineIntersectBox(const double bounds[6], const double pt1[3], const double pt2[3], double coord[3])
{
  double si, ei, bmin, bmax, t;
  double st, et, fst = 0, fet = 1;

  for (int i = 0; i < 3; i++)
    {
    si = pt1[i];
    ei = pt2[i];
    bmin = bounds[2*i];
    bmax = bounds[2*i+1];
    if (si < ei)
      {
      if (si > bmax || ei < bmin)
        {
        return false;
        }
      double di = ei - si;
      st = (si < bmin) ? (bmin -si) / di : 0;
      et = (ei > bmax) ? (bmax -si) / di : 1;
      }
    else
      {
      if (ei > bmax || si < bmin)
        {
        return false;
        }
      double di = ei - si;
      
      st = (si > bmax) ? (bmax -si) / di : 0;
      et = (ei < bmin) ? (bmin -si) / di : 1;
      }
    if (st > fst) 
      {
      fst = st;
      }
    if (et < fet) 
      {
      fet = et;
      }
    if (fet < fst)
      {
      return false;
      }
    }
  t = fst;
  coord[0] = pt1[0] + t * (pt2[0]-pt1[0]);
  coord[1] = pt1[1] + t * (pt2[1]-pt1[1]);
  coord[2] = pt1[2] + t * (pt2[2]-pt1[2]);
  return true;
}

//----------------------------------------------------------------------------
bool  CMFEUtility::SlabTest(const double d, const double o, const double lo, const double hi, double &tnear, double &tfar)
{
  if (d == 0) 
    { 
    if (o < lo || o > hi) 
      {
      return false; 
      }
    } 
  else 
    { 
    double T1 = (lo - o) / d; 
    double T2 = (hi - o) / d; 
    if (T1 > T2) 
      { 
      double temp = T1; 
      T1 = T2; 
      T2 = temp; 
      } 
    if (T1 > tnear)  
      { 
      tnear = T1; 
      } 
    if (T2 < tfar)  
      { 
      tfar = T2; 
      } 
    if (tnear > tfar || tfar < 0 || tnear == tfar) 
      {
      return false; 
      }    
    } 
  return true; 
} 

//----------------------------------------------------------------------------
bool CMFEUtility::Intersects(const double *params, double solution, int block, int nDims,
           const double *nodeExtents)
{
  int  i;
  int  numEncodings = 1;
  for (i = 0 ; i < nDims ; i++)
    {
    numEncodings *= 2;
    }

  double  valAtMin  = EquationsValueAtPoint(params, block, 0, nDims, nodeExtents);
  if (fabs(valAtMin-solution) < 1e-12)
    {
    // It happens to be that at the minimum extents the value of the
    // linear equation equals the solution, so we have an intersection.
    return true;
    }

  bool tooSmall = (valAtMin < solution);
  for (i = 1 ; i < numEncodings ; i++)
    {
    double solutionAtI = EquationsValueAtPoint(params, block, i, nDims, nodeExtents);
    if (tooSmall && solutionAtI >= solution)
      {
      // F(Point 0) is too small and F(Point i) is too large.  This
      // means that there is an intersection.  And I thought the eight
      // weeks I spent proving the Intermediate Value Theorem in college
      // were useless!!!
      return true;
      }
    if (!tooSmall && solutionAtI <= solution)
      {      
      // F(Point 0) is too large and F(Point i) is too small.  This
      // means that there is an intersection.
      return true;
      }
     }  
  // All of the extents were too large or too small, so there was no
  // intersection.  
  return false;
}

//----------------------------------------------------------------------------
bool CMFEUtility::AxiallySymmetricLineIntersection(const double *P1, const double *D1, int block, const double *nodeExtents)
{
  double Zmin = nodeExtents[4*block];
  double Zmax = nodeExtents[4*block+1];
  double Rmin = nodeExtents[4*block+2];
  double Rmax = nodeExtents[4*block+3];

    
  // We are solving for "t" in two inequalities:
  //  Zmin <= P1[2] + t*D1[2] <= Zmax
  //  Rmin <= (P1[0] + t*D1[0])^2 + (P1[1] + t*D1[1])^2 <= Rmax
  
  // In both cases, we solve for "t" (sorting out special cases as we go)
  // and see if the line intersects the cylinder.
  // Solve for 't' with the inequality focusing on 'Z'.
  
  double tRangeFromZ[2];
  if (D1[2] == 0)
    {
    if (Zmin <= P1[2] && P1[2] <= Zmax)
      {
      tRangeFromZ[0] = -10e30;
      tRangeFromZ[1] = +10e30;
      }
    else
      {
      return false;
      }
    }
  else
    {
    double t1 = (Zmin-P1[2]) / D1[2];
    double t2 = (Zmax-P1[2]) / D1[2];
    tRangeFromZ[0] = (t1 < t2 ? t1 : t2);
    tRangeFromZ[1] = (t1 > t2 ? t1 : t2);
    }
  
  // Solve for 't' with the inequality focusing on 'R'.
  //
  // Expanding out gives:
  //    Rmin^2 <= At^2 + Bt + C <= Rmax^2
  //    with:
  //       A = D1[0]^2 + D1[1]^2
  //       B = 2*P1[0]*D1[0] + 2*P1[1]*D1[1]
  //       C = P1[0]^2 + P1[1]^2
  //
  // Note that there is some subtlety to solving quadratic equations with
  // inequalities:
  //  C0 <= t^2 <= C1
  //  has solutions
  //    C0^0.5 <= t <=  C1^0.5
  //   -C1^0.5 <= t <= -C0^0.5
  //  
  // Here's the game plan for solving the inequalities:
  //  Find the roots for Rmin = At^2 + Bt + C
  //  This will divide the number line into 3 intervals:
  //  (-inf, R0), (R0, R1), (R1, inf)
  //  Then check to see if the intervals meet the inequality or not.
  //  Keep the intervals that meet the inequality, discard those that don't.
  //  If there are no roots, then check the interval (-inf, inf) and see
  //  if that satisfies the inequality.
  //
  //  Then repeat for the other inequality.
  //
  double r1_int1[2] = { 10e30, -10e30 };
  double r1_int2[2] = { 10e30, -10e30 };
  double A = D1[0]*D1[0] + D1[1]*D1[1];
  double B = 2*P1[0]*D1[0] + 2*P1[1]*D1[1];
  double C0 = P1[0]*P1[0] + P1[1]*P1[1];
  double C = C0-Rmin*Rmin;
  double discriminant = B*B - 4*A*C;
  if (A == 0 || discriminant < 0)
    {
    // There are no roots, so the inequality is either always true
    // or always false (for all t).  So evaluate it and see which one.
    // Use T = 0.  And if this inequality can't be true, then there are
    // no solutions, so just return.
    if (C0 >= Rmin*Rmin)
      {
      r1_int1[0] = -10e30;
      r1_int1[1] = +10e30;
      }
    else
      {
      return false;
      }
    }
  else
    {
    double root = sqrt(discriminant);
    double soln1 = (-B + root) / (2*A);
    double soln2 = (-B - root) / (2*A);
    if (soln1 == soln2)
      {
      // This is a parabola with its vertex on the X-axis.  So the
      // vertex is ambiguous, everything else is either one way or 
      // another.  Test a non-solution point and see whether it is
      // valid.  If so, declare the whole range valid.
      double I0 = (soln1 == 0. ? 1. : 0.);
      if (A*I0*I0 + B*I0 + C0 >= Rmin*Rmin)
        {
        r1_int1[0] = -10e30;
        r1_int1[1] = +10e30;
        }
      else if (A*soln1*soln1 + B*soln1 + C0 >= Rmin*Rmin)
        {
        r1_int1[0] = soln1;
        r1_int1[1] = soln1;
        }
      }
    else
      {
      if (soln1 > soln2)
        {
        double tmp = soln1;
        soln1 = soln2;
        soln2 = tmp;
        }

      double half = (soln2-soln1) * 0.5;
      double val = soln1 - half;
      if (A*val*val + B*val + C0 >= Rmin*Rmin)
        {
        r1_int1[0] = -10e30;
        r1_int1[1] = soln1;
        r1_int2[0] = soln2;
        r1_int2[1] = +10e30;
        }
      else
        {
        r1_int1[0] = soln1;
        r1_int1[1] = soln2;
        }
      }
    }

  double r2_int1[2] = { 10e30, -10e30 };
  double r2_int2[2] = { 10e30, -10e30 };
  C = C0-Rmax*Rmax;
  discriminant = B*B - 4*A*C;
  if (A == 0 || discriminant < 0)
    {
    // There are no roots, so the inequality is either always true
    // or always false (for all t).  So evaluate it and see which one.
    // Use T = 0.  And if this inequality can't be true, then there are
    // no solutions, so just return.
    if (C0 <= Rmax*Rmax)
      {
      r2_int1[0] = -10e30;
      r2_int1[1] = +10e30;
      }
    else
      {
      return false;
      }
    }
  else
    {
    double root = sqrt(discriminant);
    double soln1 = (-B + root) / (2*A);
    double soln2 = (-B - root) / (2*A);
    if (soln1 == soln2)
      {
      // This is a parabola with its vertex on the X-axis.  So the
      // vertex is ambiguous, everything else is either one way or 
      // another.  Test a non-solution point and see whether it is
      // valid.  If so, declare the whole range valid.
      double I0 = (soln1 == 0. ? 1. : 0.);
      if (A*I0*I0 + B*I0 + C0 <= Rmax*Rmax)
        {
        r2_int1[0] = -10e30;
        r2_int1[1] = +10e30;
        }
      else if (A*soln1*soln1 + B*soln1 + C0 <= Rmax*Rmax)
        {
        r2_int1[0] = soln1;
        r2_int1[1] = soln1;
        }
      }
    else
      {
      if (soln1 > soln2)
        {
        double tmp = soln1;
        soln1 = soln2;
        soln2 = tmp;
        }

      double half = (soln2-soln1) * 0.5;
      double val = soln1 - half;
      if (A*val*val + B*val + C0 <= Rmax*Rmax)
        {
        r2_int1[0] = -10e30;
        r2_int1[1] = soln1;
        r2_int2[0] = soln2;
        r2_int2[1] = +10e30;
        }
      else
        {
        r2_int1[0] = soln1;
        r2_int1[1] = soln2;
        }
      }
    }


  //
  // Now that we have t-ranges for all the inequalities, see if we
  // can find a 't' that makes them all true.  If so, we are inside
  // the cylinder.  If not, we are outside.
  //
  double t[2];

  // Try Z, R1_int1, R2_int1.
  t[0] = tRangeFromZ[0];
  t[1] = tRangeFromZ[1];
  t[0] = (t[0] < r1_int1[0] ? r1_int1[0] : t[0]);
  t[1] = (t[1] > r1_int1[1] ? r1_int1[1] : t[1]);
  t[0] = (t[0] < r2_int1[0] ? r2_int1[0] : t[0]);
  t[1] = (t[1] > r2_int1[1] ? r2_int1[1] : t[1]);
  if (t[0] <= t[1])
    {
    return true;
    }

  // Try Z, R1_int1, R2_int2.
  t[0] = tRangeFromZ[0];
  t[1] = tRangeFromZ[1];
  t[0] = (t[0] < r1_int1[0] ? r1_int1[0] : t[0]);
  t[1] = (t[1] > r1_int1[1] ? r1_int1[1] : t[1]);
  t[0] = (t[0] < r2_int2[0] ? r2_int2[0] : t[0]);
  t[1] = (t[1] > r2_int2[1] ? r2_int2[1] : t[1]);
  if (t[0] <= t[1])
    {
    return true;
    }

  // Try Z, R1_int2, R2_int1.
  t[0] = tRangeFromZ[0];
  t[1] = tRangeFromZ[1];
  t[0] = (t[0] < r1_int2[0] ? r1_int2[0] : t[0]);
  t[1] = (t[1] > r1_int2[1] ? r1_int2[1] : t[1]);
  t[0] = (t[0] < r2_int1[0] ? r2_int1[0] : t[0]);
  t[1] = (t[1] > r2_int1[1] ? r2_int1[1] : t[1]);
  if (t[0] <= t[1])
    {
    return true;
    }

  // Try Z, R1_int2, R2_int2.
  t[0] = tRangeFromZ[0];
  t[1] = tRangeFromZ[1];
  t[0] = (t[0] < r1_int2[0] ? r1_int2[0] : t[0]);
  t[1] = (t[1] > r1_int2[1] ? r1_int2[1] : t[1]);
  t[0] = (t[0] < r2_int2[0] ? r2_int2[0] : t[0]);
  t[1] = (t[1] > r2_int2[1] ? r2_int2[1] : t[1]);
  if (t[0] <= t[1])
    {
    return true;
    }
  
  // None of the intervals overlap.  No 't' will make this true, so there
  // is no intersection.
  return false;
}

//----------------------------------------------------------------------------
bool CMFEUtility::IntersectsRay(double origin[3], double rayDir[3], int block, int nDims, const double *nodeExtents)
{
  double bnds[6] = { 0., 0., 0., 0., 0., 0.};
  double coord[3] = { 0., 0., 0.};

  for (int i = 0; i < nDims; i++)
    {
    bnds[2*i] = nodeExtents[block*nDims*2 + 2*i];
    bnds[2*i+1] = nodeExtents[block*nDims*2 + 2*i + 1];
    }

  return (CMFEUtility::IntersectBox(bnds, origin, rayDir, coord));
}

