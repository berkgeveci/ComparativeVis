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

#ifndef __vtkCMFEUtility_h
#define __vtkCMFEUtility_h

#include <vtkToolkits.h>

class vtkCell;
class vtkDataSet;
class vtkPoints;
class vtkRectilinearGrid;


#ifdef VTK_USE_MPI
# include <vtkMPI.h>
#endif
  


namespace CMFEUtility
{
  // Description:
  //Called first to setup all the parallel information including what
  //world we are in, and the number of processors in that world
  void Setup();

#ifdef VTK_USE_MPI
  // Description:
  //returns the mpi commuinicator we should be using
  MPI_Comm* GetMPIComm();
#endif

  // Description:
  // Get the number of processors 
  int PAR_Size(void);

  // Description:
  //Collective call across all processors to unify an array that
  //has alternating minimum and maximum values.
  //returns if the collective call was successful 
  bool UnifyMinMax(double *buffer, int size);

  // Description:
  //Collective call across all processors to return the minimum value.
  float UnifyMinimumValue(float value);

  // Description:
  //Collective call across all processors to return the maximum value.
  float UnifyMaximumValue(float value);

  // Description:
  //Collective call across all processors to find the sum of the integer.
  int SumIntAcrossAllProcessors(int value);
 
  // Description:
  //Collective call across all processors to find the sum of each component of the integer array.
  void SumIntArrayAcrossAllProcessors(int *inArray, int *outArray, int size);  

  // Description:
  // Constructs a single large character array so that all the messages
  //can be stored in the same array. 
  char* CreateMessageStrings(char **lists, int *count, int nl);

  // Description:
  // returns a copy of the vtkPoints that are contained in the dataset.
  // An empty vtkPoints will be returned if the input dataset is NULL or empty
  vtkPoints *GetPoints(vtkDataSet *dataset);

  // Description:
  // calculates the cell center coordinates.
  void GetCellCenter(vtkCell* cell, double center[3]);
  
  //Description:
  //Tests whether or not a cell contains a point.  Does this by testing
  //the side the point lies on each face of the cell.
  bool CellContainsPoint(vtkCell *cell, const double *point);

  //Description:
  //Tests whether or not a point intersects a box bounds
  int IntersectBox(const double bounds[6], const double origin[3], const double dir[3], double coord[3]);
      
  //Description:
  //Tests whether or not a line intersects a box bounds
  int LineIntersectBox(const double bounds[6],  const double pt1[3], const double pt2[3], double coord[3]);

  //Description:
  //Tests whether a point is on the correct side of a plane
  bool SlabTest(const double d, const double o, const double lo,  const double hi, double &tnear, double &tfar);

  //Description:
  //Tests whether or not a line intersects a box bounds
  bool Intersects(const double *params, double solution, int block, int nDims, const double *nodeExtents);

  //Description:
  //Tests whether a cube intersects with a given line
  bool AxiallySymmetricLineIntersection(const double *P1, const double *D1, int block, const double *nodeExtents);
  
  //Description:
  //Tests whether a ray intersects with a cube
  bool IntersectsRay(double origin[3], double rayDir[3], int block, int nDims, const double *nodeExtents);  
}

#endif


