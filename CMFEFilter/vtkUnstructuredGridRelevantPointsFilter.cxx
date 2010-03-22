/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkUnstructuredGridRelevantPointsFilter.C,v $
  Language:  C++
  Date:      $Date: 2000/05/17 16:05:12 $
  Version:   $Revision: 1.54 $


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkUnstructuredGridRelevantPointsFilter.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkUnstructuredGridRelevantPointsFilter, "$Revision: 0.1$");
vtkStandardNewMacro(vtkUnstructuredGridRelevantPointsFilter);

//----------------------------------------------------------------------------
vtkUnstructuredGridRelevantPointsFilter::vtkUnstructuredGridRelevantPointsFilter( )
{

}

//----------------------------------------------------------------------------
vtkUnstructuredGridRelevantPointsFilter::~vtkUnstructuredGridRelevantPointsFilter()
{


}

int vtkUnstructuredGridRelevantPointsFilter::RequestData(vtkInformation *vtkNotUsed(request), vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
   // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0); 
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast( inInfo->Get(vtkDataObject::DATA_OBJECT())); 
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkDebugMacro(<<"Beginning UnstructuredGrid Relevant Points Filter ");

  if (input == NULL) 
    {
    vtkErrorMacro(<<"Input is NULL");
    return 0;
    }

  vtkPoints    *inPts  = input->GetPoints();
  int numInPts = input->GetNumberOfPoints();
  int numCells = input->GetNumberOfCells();
  output->Allocate(numCells);
  
  if ( (numInPts<1) || (inPts == NULL ) ) 
    {
    vtkErrorMacro(<<"No data to Operate On!");
    return 0;
    }
  
  input->BuildLinks();
  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(numInPts);
  
  vtkCellData  *inputCD = input->GetCellData();
  vtkCellData  *outputCD = output->GetCellData();
  outputCD->PassData(inputCD);
  
  vtkPointData *inputPD  = input->GetPointData();
  vtkPointData *outputPD = output->GetPointData();
  outputPD->CopyAllocate(inputPD);
  
  int numNewPts = 0;
  vtkIdList *cellIds = vtkIdList::New();
  int *pointMap = new int[numInPts];
  int i;
  // search through all given input points, marking those not
  // associated with cells, adding good points to the new list
  for (i = 0; i < numInPts; i++)
    {
    input->GetPointCells(i, cellIds);
    if (0 == cellIds->GetNumberOfIds()) 
      {
      pointMap[i] = -1;
      }
    else
      {
      newPts->InsertNextPoint(inPts->GetPoint(i));
      pointMap[i] = numNewPts;
      outputPD->CopyData(inputPD, i, numNewPts);
      numNewPts++;
      }
    }

  newPts->Squeeze();
  output->SetPoints(newPts);

  // now work through cells, changing associated point id to coincide
  // with the new ones as specified in the pointmap;

  vtkIdList *oldIds = vtkIdList::New(); 
  vtkIdList *newIds = vtkIdList::New();
  int j, id, cellType;
  for (i = 0; i < numCells; i++) 
    {
    input->GetCellPoints(i, oldIds);
    cellType = input->GetCellType(i);
    newIds->SetNumberOfIds(oldIds->GetNumberOfIds());
    for (j = 0; j < oldIds->GetNumberOfIds(); j++)
      {
      id = oldIds->GetId(j); 
      newIds->SetId(j, pointMap[id]);
      }
      output->InsertNextCell(cellType, newIds);
    }

  newPts->Delete();
  oldIds->Delete();
  newIds->Delete();
  cellIds->Delete();
  delete [] pointMap;

  return 1;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridRelevantPointsFilter::
PrintSelf(ostream& os, vtkIndent indent) 
{
  this->Superclass::PrintSelf(os,indent);
}
