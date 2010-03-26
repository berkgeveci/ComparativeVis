/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCMFEFastLookupGrouping.cxx,v $

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

#include "vtkCMFEFastLookupGrouping.h"



#include "vtkAppendFilter.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCMFEIntervalTree.h"
#include "vtkCMFESpatialPartition.h"
#include "vtkCMFEUtility.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkHexahedron.h"
#include "vtkMultiProcessController.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGridRelevantPointsFilter.h"
#include "vtkUnstructuredGridWriter.h"


//----------------------------------------------------------------------------
vtkCMFEFastLookupGrouping::vtkCMFEFastLookupGrouping(vtkStdString v,
                                                             bool isN)
{
  this->VarName   = v;
  this->IsNodal   = isN;
  this->IntervalTree     = NULL;
  this->MapToDataSet = NULL;
  this->DataSetStart  = NULL;
}

//----------------------------------------------------------------------------
vtkCMFEFastLookupGrouping::~vtkCMFEFastLookupGrouping()
{
  this->ClearAllInputMeshes();
  delete this->IntervalTree;
  delete [] this->MapToDataSet;
  delete [] this->DataSetStart;
}

//----------------------------------------------------------------------------
void vtkCMFEFastLookupGrouping::AddMesh(vtkDataSet *mesh)
{
   mesh->Register(NULL);
   this->Meshes.push_back(mesh);
}

//----------------------------------------------------------------------------
void vtkCMFEFastLookupGrouping::ClearAllInputMeshes(void)
{
  for (int i = 0 ; i < this->Meshes.size() ; i++)
    {
    this->Meshes[i]->Delete();
    }
  this->Meshes.clear();
}

//----------------------------------------------------------------------------
void vtkCMFEFastLookupGrouping::Finalize(void)
{
  int   i, j;
  int   index = 0;

  this->NumberOfZones = 0;
  for (i = 0 ; i < this->Meshes.size() ; i++)
  this->NumberOfZones += this->Meshes[i]->GetNumberOfCells();

  this->DataSetStart = new int[this->Meshes.size()];
  this->DataSetStart[0] = 0;
  for (i = 1 ; i < this->Meshes.size() ; i++)
  this->DataSetStart[i] = this->DataSetStart[i-1] + this->Meshes[i-1]->GetNumberOfCells();

  bool degenerate = false;
  if (this->NumberOfZones == 0)
    {
    degenerate = true;
    this->NumberOfZones = 1;
    }
  this->IntervalTree = new vtkCMFEIntervalTree(this->NumberOfZones, 3);
  this->MapToDataSet = new int[this->NumberOfZones];
  index = 0;
  for (i = 0 ; i < this->Meshes.size() ; i++)
    {
    int nCells = this->Meshes[i]->GetNumberOfCells();
    for (j = 0 ; j < nCells ; j++)
      {
      vtkCell *cell = this->Meshes[i]->GetCell(j);
      double bounds[6];
      cell->GetBounds(bounds);
      this->IntervalTree->AddElement(index, bounds);

      this->MapToDataSet[index] = i;
      index++;
      }
    }
  if (degenerate)
    {
    double bounds[6] = { 0, 1, 0, 1, 0, 1 };
    this->IntervalTree->AddElement(0, bounds);
    }    
  this->IntervalTree->Calculate(true);    
}


//----------------------------------------------------------------------------
bool vtkCMFEFastLookupGrouping::GetValue(const float *pt, float *val)
{  
  // Start off by using the list from the previous search.  Searching the
  // interval tree is so costly that this is a worthwhile "guess".  
  if (this->ListFromLastSuccessfulSearch.size() > 0)
    {
    bool v = this->GetValueUsingList(this->ListFromLastSuccessfulSearch, pt, val);
    if (v)
      {
      return true;
      }
    }
  
  // OK, we struck out with the list from the last winning search.  So
  // get the correct list from the interval tree.  
  vtkstd::vector<int> list;
  double dpt[3] = {pt[0], pt[1] , pt[2]};
  this->IntervalTree->GetElementsListFromRange(dpt, dpt, list);
  bool v = this->GetValueUsingList(list, pt, val);
  if (v == true)
    {
    this->ListFromLastSuccessfulSearch = list;
    }
  else
    {
    this->ListFromLastSuccessfulSearch.clear();
    }
  return v;
}

//----------------------------------------------------------------------------
bool vtkCMFEFastLookupGrouping::GetValueUsingList(vtkstd::vector<int> &list, const float *pt, float *val)
{
  double closestPt[3];
  int subId;
  double pcoords[3];
  double dist2;
  double weights[100]; // MUST BE BIGGER THAN NPTS IN A CELL (ie 8).
  double non_const_pt[3];
  non_const_pt[0] = pt[0];
  non_const_pt[1] = pt[1];
  non_const_pt[2] = pt[2];

  for (int j = 0 ; j < list.size() ; j++)
    {
    int mesh = this->MapToDataSet[list[j]];
    int index = list[j] - this->DataSetStart[mesh];
    if (this->Meshes[mesh]->GetCellData()->GetArray("avtGhostZones") != NULL)
      {
      vtkUnsignedCharArray *arr = (vtkUnsignedCharArray *) this->Meshes[mesh]->GetCellData()->GetArray("avtGhostZones");
      if (arr->GetValue(index) != 0)
        {
        continue;
        }
      }

    vtkCell *cell = this->Meshes[mesh]->GetCell(index);
    bool inCell = CMFEUtility::CellContainsPoint(cell, non_const_pt);
    if (!inCell)
      {
      continue;
      }

    if (this->IsNodal)
      {
      // Need the weights.
      cell->EvaluatePosition(non_const_pt, closestPt, subId, pcoords, dist2, weights);
      vtkDataArray *arr = this->Meshes[mesh]->GetPointData()->GetArray(this->VarName.c_str());
      if (arr == NULL)
        {
        return false;        
        }

      int nComponents = arr->GetNumberOfComponents();
      int nPts = cell->GetNumberOfPoints();
      for (int c = 0 ; c < nComponents ; c++)
        {
        val[c] = 0.;
        for (int pt = 0 ; pt < nPts ; pt++)
          {
          vtkIdType id = cell->GetPointId(pt);
          val[c] += weights[pt]*arr->GetComponent(id, c);
          }
        }
      }
    else
      {
      vtkDataArray *arr = this->Meshes[mesh]->GetCellData()->GetArray(this->VarName.c_str());
      if (arr == NULL)
        {
        return false;        
        }

      int nComponents = arr->GetNumberOfComponents();
      for (int c = 0 ; c < nComponents ; c++)
        {
        val[c] = arr->GetComponent(index, c);
        }
      }
    return true;
    }

  return false;
}


//----------------------------------------------------------------------------
void vtkCMFEFastLookupGrouping::RelocateDataUsingPartition( vtkCMFESpatialPartition *spat_part)
{
  int  i, j, k;
  int   nProcs = CMFEUtility::PAR_Size();

  vtkUnstructuredGrid **meshForProcP = new vtkUnstructuredGrid*[nProcs];
  int                  *nCellsForP   = new int[nProcs];
  vtkAppendFilter     **appenders    = new vtkAppendFilter*[nProcs];
  for (i = 0 ; i < nProcs ; i++)
    {
    appenders[i] = vtkAppendFilter::New();
    }

  vtkstd::vector<int> list;
  for (i = 0 ; i < this->Meshes.size() ; i++)
    {

    // Get the mesh from the input for this iteration.  During each 
    // iteration, our goal is, for each cell in the mesh, to identify
    // which processors need this cell to do their sampling and to add
    // that cell to a data structure so we can do a large communication
    // afterwards.

    vtkDataSet *mesh = this->Meshes[i];
    const int nCells = mesh->GetNumberOfCells();

    // 
    // Reset the data structures that contain the cells from mesh i that
    // need to be sent to processor P.

    for (j = 0 ; j < nProcs ; j++)
      {
      meshForProcP[j] = NULL;
      nCellsForP[j]   = 0;
      }


    // For each cell in the input mesh, determine which processors that
    // cell needs to be sent to (typically just one other processor).  Then
    // add the cell to a data structure that will holds the cells.  In the
    // next phase we will use this data structure to construct a large
    // message to each of the other processors containing the cells it
    // needs.

    for (j = 0 ; j < nCells ; j++)
      {
      vtkCell *cell = mesh->GetCell(j);
      spat_part->GetProcessorList(cell, list);
      for (k = 0 ; k < list.size() ; k++)
        {
        if (meshForProcP[list[k]] == NULL)
          {
          //delay the construction of the unstructured grid to as late as possible
          //this way if we don't need the grid for a processor we won't construc it.
          meshForProcP[list[k]] = vtkUnstructuredGrid::New();
          vtkPoints *pts = CMFEUtility::GetPoints(mesh);
          meshForProcP[list[k]]->SetPoints(pts);
          meshForProcP[list[k]]->GetPointData()->ShallowCopy(
            mesh->GetPointData());
          meshForProcP[list[k]]->GetCellData()->CopyAllocate(
            mesh->GetCellData());
          meshForProcP[list[k]]->Allocate(nCells*9);
          pts->Delete();
          }
        int cellType = mesh->GetCellType(j);
        vtkIdList *ids = cell->GetPointIds();
        meshForProcP[list[k]]->InsertNextCell(cellType, ids);
        meshForProcP[list[k]]->GetCellData()->CopyData(
          mesh->GetCellData(), j, nCellsForP[list[k]]);
        nCellsForP[list[k]]++;
        }
      }


    // The data structures we used for examining mesh 'i' are temporary.
    // Put them in a more permanent location so we can move on to the
    // next iteration.

    for (j = 0 ; j < nProcs ; j++)
      {
      if (meshForProcP[j] != NULL)
        {
        vtkUnstructuredGridRelevantPointsFilter *ugrpf = vtkUnstructuredGridRelevantPointsFilter::New();
        ugrpf->SetInput(meshForProcP[j]);
        ugrpf->Update();
        appenders[j]->AddInput(ugrpf->GetOutput());
        ugrpf->Delete();
        meshForProcP[j]->Delete();
        }
      }
    }
  delete [] meshForProcP;
  delete [] nCellsForP;


  // We now have, for each processor P, the list of cells from this processor
  // that P will need to do its job.  So construct a big message containing
  // these cell lists and then send it to P.  Of course, while we are busy
  // composing messages to each of the processors, they are busy composing
  // message to us.  So use an 'alltoallV' call that allows us to get the
  // cells that are necessary for *this* processor to do its job.

  int *sendcount = new int[nProcs];
  char **msg_tmp = new char *[nProcs];
  for (j = 0 ; j < nProcs ; j++)
    {
    if (appenders[j]->GetTotalNumberOfInputConnections() == 0)
      {
      sendcount[j] = 0;
      msg_tmp[j]   = NULL;
      appenders[j]->Delete();
      continue;
      }
    appenders[j]->Update();
    vtkUnstructuredGridWriter *wrtr = vtkUnstructuredGridWriter::New();
    wrtr->SetInput(appenders[j]->GetOutput());
    wrtr->SetWriteToOutputString(1);
    wrtr->SetFileTypeToBinary();
    wrtr->Write();
    sendcount[j] = wrtr->GetOutputStringLength();
    msg_tmp[j] = (char *) wrtr->RegisterAndGetOutputString();
    wrtr->Delete();
    appenders[j]->Delete();    
    }
  delete [] appenders;

  int total_msg_size = 0;
  for (j = 0 ; j < nProcs ; j++)
    {
    total_msg_size += sendcount[j];
    }

  char *big_send_msg = new char[total_msg_size];
  char *ptr = big_send_msg;
  for (j = 0 ; j < nProcs ; j++)
    {
    if (msg_tmp[j] != NULL)
      {
      memcpy(ptr, msg_tmp[j], sendcount[j]*sizeof(char));
      ptr += sendcount[j]*sizeof(char);
      delete [] msg_tmp[j];
      }
    }
  delete [] msg_tmp;

  int *recvcount = new int[nProcs];
#ifdef VTK_USE_MPI
  MPI_Alltoall(sendcount, 1, MPI_INT, recvcount, 1, MPI_INT, *CMFEUtility::GetMPIComm());
#endif

  char **recvmessages = new char*[nProcs];
  char *big_recv_msg = CMFEUtility::CreateMessageStrings(recvmessages, recvcount, nProcs);

  int *senddisp  = new int[nProcs];
  int *recvdisp  = new int[nProcs];
  senddisp[0] = 0;
  recvdisp[0] = 0;
  for (j = 1 ; j < nProcs ; j++)
    {
    senddisp[j] = sendcount[j-1] + senddisp[j-1];
    recvdisp[j] = recvcount[j-1] + recvdisp[j-1];
    }
#ifdef VTK_USE_MPI
  MPI_Alltoallv(big_send_msg, sendcount, senddisp, MPI_CHAR,
                big_recv_msg, recvcount, recvdisp, MPI_CHAR,
                *CMFEUtility::GetMPIComm());
#endif
  delete [] sendcount;
  delete [] senddisp;
  delete [] big_send_msg;

  this->ClearAllInputMeshes();
  for (j = 0 ; j < nProcs ; j++)
    {
    if (recvcount[j] == 0)
      {
      continue;
      }

    vtkCharArray *charArray = vtkCharArray::New();
    int iOwnIt = 1;  // 1 means we own it -- you don't delete it.
    charArray->SetArray(recvmessages[j], recvcount[j], iOwnIt);
    vtkUnstructuredGridReader *reader = vtkUnstructuredGridReader::New();
    reader->SetReadFromInputString(1);
    reader->SetInputArray(charArray);
    vtkUnstructuredGrid *ugrid = reader->GetOutput();
    ugrid->Update();
    this->AddMesh(ugrid);

    reader->Delete();
    charArray->Delete();
    }
  delete [] recvmessages;
  delete [] big_recv_msg;
  delete [] recvcount;
  delete [] recvdisp;

    
}
