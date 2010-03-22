/*=========================================================================

  Program:   Visualization Toolkit
  Module:$RCSfile: vtkVisItDatabaseBridge.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCMFESpatialPartition -- Determines the best balance for each processor 
// .SECTION Description
//
// Uses the vtkCMFEDesiredPoints and vtkCMFEFastLookupGrouping to determing the best
// partition of space for each processor. 
//
// .SECTION See Also
// vtkCMFEFastLookupGrouping vtkCMFEDesiredPoints

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
*     this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*     this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*     documentation and/or other materials provided with the distribution.
*  - Neither the name of  the LLNS/LLNL nor the names of  its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
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



#ifndef __vtkCMFESpatialPartition_h
#define __vtkCMFESpatialPartition_h


#include <vtkstd/vector>
#include "vtkCMFEFastLookupGrouping.h"
#include "vtkCMFEDesiredPoints.h"

class vtkCMFEIntervalTree;
class vtkCell;

class vtkCMFESpatialPartition
{
public:
  vtkCMFESpatialPartition();
  virtual ~vtkCMFESpatialPartition();
  void CreatePartition(vtkCMFEDesiredPoints *, vtkCMFEFastLookupGrouping *, double *);

  // Description:
  //Get the processor that contains this point
  int GetProcessor(float *point);

  // Description:
  //Get the processor that contains this cell
  int GetProcessor(vtkCell *cell);

  // Description:
  //Gets the processor that contains this cell.  This should be called
  //when a list of processors contain a cell.
  void GetProcessorList(vtkCell *cell, vtkstd::vector<int> &list);

  // Description:
  //Gets the processor that contains this cell.  This should be called
  //when a list of processors contain a cell.
  void GetProcessorBoundaries(float *bounds, vtkstd::vector<int> &list, vtkstd::vector<float> &db);

protected:
  vtkCMFEIntervalTree  *IntervalTree;

private:
  vtkCMFESpatialPartition(const vtkCMFESpatialPartition&);  // Not implemented.
  void operator=(const vtkCMFESpatialPartition&);  // Not implemented.
};


#endif


