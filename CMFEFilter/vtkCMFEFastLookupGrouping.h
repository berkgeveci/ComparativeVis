/*=========================================================================

  Program:   Visualization Toolkit
  Module:$RCSfile: vtkCMFEFastLookupGrouping.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCMFEFastLookupGrouping -- Connects the VTK pipeline to the vtkVisItDatabase class.
// .SECTION Description
//
// Collection of this->Meshes that are used by the CMFE algorithm for point comparison to the info stored
// in vtkCMFEDesiredPoints. Unlike vtkCMFEDesiredPoints this class doesn't restore itself
// after redistrubtion with the spatial partition.
//
// .SECTION See Also
// vtkCMFEPosCMFEAlgorithm, vtkCMFEDesiredPoints

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



#ifndef __vtkCMFEFastLookupGrouping_h
#define __vtkCMFEFastLookupGrouping_h

#include "vtkStdString.h"
#include <vtkstd/vector>

class vtkCell;
class vtkDataArray;
class vtkDataSet;
class vtkCMFEIntervalTree;
class vtkCMFESpatialPartition;

class vtkCMFEFastLookupGrouping
{
public:
  vtkCMFEFastLookupGrouping(vtkStdString varName, bool nodal);
  virtual ~vtkCMFEFastLookupGrouping();

  // Description:
  //Gives the fast lookup grouping object another mesh to include in the grouping.
  void AddMesh(vtkDataSet *);

  // Description:
  //Clears all the input this->Meshes.  This is typically called from within RelocateDataUsingPartition.
  void ClearAllInputMeshes(void);
  
  // Description:
  //Gives the fast lookup groupin object a chance to finish its
  //initializtion process.  This gives the object the cue that it will not
  //receive any more "AddMesh" calls and that it can initialize itself.
  void Finalize();
  
  // Description:
  //Evaluates the value at a position.  Does this for the grouping of
  //this->Meshes its been given and does it with fast lookups.
  //This method is actually a thin layer on top of GetValueUsingList.
  //It calls that method using the last successful list and then, if
  //necessary, using a list that comes from the interval tree.
  bool GetValue(const float *point, float *value);
  
  // Description:
  //Evaluates the value at a position.  Does this for the grouping of
  //this->Meshes its been given and does it with fast lookups.
  bool GetValueUsingList(vtkstd::vector<int> &list, const float *pt, float *val);

  // Description:
  //Relocates the data to different processors to honor the spatial 
  //partition.
  void RelocateDataUsingPartition(vtkCMFESpatialPartition *spat_pat);
  
  // Description:
  // returns the collection of this->Meshes being stored
  vtkstd::vector<vtkDataSet *> GetMeshes(void) { return this->Meshes; };  
  
protected:
  vtkStdString VarName;
  bool IsNodal;
  int NumberOfZones;

  vtkstd::vector<vtkDataSet *> Meshes;
  vtkCMFEIntervalTree *IntervalTree;
  int *MapToDataSet;
  int *DataSetStart;
  vtkstd::vector<int> ListFromLastSuccessfulSearch;


private:
  vtkCMFEFastLookupGrouping(const vtkCMFEFastLookupGrouping&);  // Not implemented.
  void operator=(const vtkCMFEFastLookupGrouping&);  // Not implemented.
};
  

#endif


