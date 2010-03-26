/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCMFEDesiredPoints.cxx,v $

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



#include "vtkCMFEAlgorithm.h"

#include "vtkCell.h"
#include "vtkCMFEDesiredPoints.h"
#include "vtkCMFESpatialPartition.h"
#include "vtkCMFEUtility.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkRectilinearGrid.h"

#include <math.h>

//----------------------------------------------------------------------------
vtkCMFEDesiredPoints::vtkCMFEDesiredPoints(bool isN, int nc)
{
  this->IsNodal   = isN;
  this->NumberOfComps    = nc;
  this->MapToDataSets = NULL;
  this->DataSetStartIndices  = NULL;
  this->Values      = NULL;
  this->TotalNumberOfValues  = 0;
  this->NumberOfDatasets = 0;
  this->NumberOfGrids   = 0;    
}

//----------------------------------------------------------------------------
vtkCMFEDesiredPoints::~vtkCMFEDesiredPoints()
{
  delete [] this->MapToDataSets;
  delete [] this->DataSetStartIndices;
  delete [] this->Values;

  for (int i = 0 ; i < this->pt_list.size() ; ++i)
    {
    delete [] this->pt_list[i];
    }
  for (int i = 0 ; i < this->rgrid_pts.size() ; ++i)
    {
    delete [] this->rgrid_pts[i];
    }
}

//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::AddDataset(vtkDataSet *ds)
{    
  int  i;

  int nValues= (this->IsNodal ? ds->GetNumberOfPoints() : ds->GetNumberOfCells());
  if (ds->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {    
    // Get the rectilinear grid and determine its dimensions.  Be leery
    // of situations where the grid is flat in a dimension.    
    vtkRectilinearGrid *rgrid = (vtkRectilinearGrid *) ds;
    int dims[3];
    rgrid->GetDimensions(dims);
    if (!this->IsNodal)
      {
      dims[0] = (dims[0] > 1 ? dims[0]-1 : 1);
      dims[1] = (dims[1] > 1 ? dims[1]-1 : 1);
      dims[2] = (dims[2] > 1 ? dims[2]-1 : 1);
      }

    // Set up the X-coordinates.
    vtkDataArray *x = rgrid->GetXCoordinates();
    float *newX = new float[dims[0]];
    for (i = 0 ; i < dims[0] ; i++)
      {
      if (this->IsNodal || dims[0] == 1)
        {
        newX[i] = x->GetTuple1(i);
        }      
      else
        {
        newX[i] = (x->GetTuple1(i) + x->GetTuple1(i+1)) / 2.;
        }
      }
    this->rgrid_pts_size.push_back(dims[0]);
    this->rgrid_pts.push_back(newX);
    
    // Set up the Y-coordinates.    
    vtkDataArray *y = rgrid->GetYCoordinates();
    float *newY = new float[dims[1]];
    for (i = 0 ; i < dims[1] ; i++)
      {
      if (this->IsNodal || dims[1] == 1)
        {
        newY[i] = y->GetTuple1(i);
        }
      else
        {
        newY[i] = (y->GetTuple1(i) + y->GetTuple1(i+1)) / 2.;
        }
      }
    this->rgrid_pts_size.push_back(dims[1]);
    this->rgrid_pts.push_back(newY);

    // Set up the Z-coordinates.
    vtkDataArray *z = rgrid->GetZCoordinates();
    float *newZ = new float[dims[2]];
    for (i = 0 ; i < dims[2] ; i++)
      {
      if (this->IsNodal || dims[2] == 1)
        {
        newZ[i] = z->GetTuple1(i);
        }      
      else
        {
        newZ[i] = (z->GetTuple1(i) + z->GetTuple1(i+1)) / 2.;
        }
      }
    this->rgrid_pts_size.push_back(dims[2]);
    this->rgrid_pts.push_back(newZ);
    }
  else
    {
    float *plist = new float[3*nValues];
    this->pt_list.push_back(plist);
    this->pt_list_size.push_back(nValues);

    double dcp[3]; 
    for (i = 0 ; i <nValues; i++)
      {
      float *cur_pt = plist + 3*i;
      if (this->IsNodal)
        {
        ds->GetPoint(i, dcp);
        }
      else
        {
        vtkCell *cell = ds->GetCell(i);
        CMFEUtility::GetCellCenter(cell, dcp);
        }
      cur_pt[0] = (float)dcp[0];
      cur_pt[1] = (float)dcp[1];
      cur_pt[2] = (float)dcp[2];
      }
    }
}

//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::Finalize(void)
{
  int   i, j;
  int   index = 0;

  if (this->Values != NULL)
    {
    delete [] this->Values;
    }

  if (this->DataSetStartIndices != NULL)
    {
    delete [] this->DataSetStartIndices;
    }
  
  if (this->MapToDataSets != NULL)
    {
    delete [] this->MapToDataSets;
    }

  this->TotalNumberOfValues  = 0;
  this->NumberOfGrids = this->rgrid_pts.size() / 3;

  int numNonRGrid = this->pt_list_size.size();
  this->NumberOfDatasets = numNonRGrid + this->NumberOfGrids;
  
  for (i = 0 ; i < numNonRGrid ; i++)
    {
    this->TotalNumberOfValues += this->pt_list_size[i];
    }

  this->GridStart = this->TotalNumberOfValues;
  for (i = 0 ; i < this->NumberOfGrids ; i++)
    {
    int nValues = this->rgrid_pts_size[3*i] * this->rgrid_pts_size[3*i+1] *this->rgrid_pts_size[3*i+2];
    this->TotalNumberOfValues += nValues;
    }

  int *ds_size = new int[this->NumberOfDatasets];
  for (i = 0 ; i < numNonRGrid ; i++)
    {
    ds_size[i] = this->pt_list_size[i];
    }
  
  for (i = 0 ; i < this->NumberOfGrids ; i++)
    {
    ds_size[i+numNonRGrid] = this->rgrid_pts_size[3*i] * this->rgrid_pts_size[3*i+1] * this->rgrid_pts_size[3*i+2];
    }

  this->DataSetStartIndices = new int[this->NumberOfDatasets];
  this->DataSetStartIndices[0] = 0;
  for (i = 1 ; i < this->NumberOfDatasets ; i++)
    {
    this->DataSetStartIndices[i] = this->DataSetStartIndices[i-1] + ds_size[i-1];
    }
  delete [] ds_size;

  this->MapToDataSets = new int[this->TotalNumberOfValues];
  index = 0;
  for (i = 0 ; i < numNonRGrid ; i++)
    {
    for (j = 0 ; j < this->pt_list_size[i] ; j++)
      {
      this->MapToDataSets[index] = i;
      index++;
      }
    }

  for (i = 0 ; i < this->NumberOfGrids ; i++)
    {
    int nValues = this->rgrid_pts_size[3*i] * this->rgrid_pts_size[3*i+1] * this->rgrid_pts_size[3*i+2];
    for (j = 0 ; j < nValues ; j++)
      {
      this->MapToDataSets[index] = i+numNonRGrid;
      index++;
      }
    }

  this->Values = new float[this->TotalNumberOfValues*this->NumberOfComps];
}

//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::GetRGrid(int idx, const float *&x, const float *&y, const float *&z, int &nx, int &ny, int &nz)
{
  if (idx < 0 || idx >= this->NumberOfGrids)
    {
    return;
    }
  x = this->rgrid_pts[3*idx];
  y = this->rgrid_pts[3*idx+1];
  z = this->rgrid_pts[3*idx+2];
  nx = this->rgrid_pts_size[3*idx];
  ny = this->rgrid_pts_size[3*idx+1];
  nz = this->rgrid_pts_size[3*idx+2];
}


//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::GetPoint(int p, float *pt) const
{
  if (p < 0 || p >= this->TotalNumberOfValues)
    {    
    return;
    }

  int ds = this->MapToDataSets[p];
  int start = this->DataSetStartIndices[ds];
  int rel_index = p-start;
  if (p < this->GridStart)
    {
    float *ptr = this->pt_list[ds] + 3*rel_index;
    pt[0] = ptr[0];
    pt[1] = ptr[1];
    pt[2] = ptr[2];
    }
  else
    {
    int ds_rel_index = ds - this->GridStart;
    int nX = this->rgrid_pts_size[3*ds_rel_index];
    int xIdx = rel_index % nX;
    pt[0] = this->rgrid_pts[3*ds_rel_index][xIdx];
    int nY = this->rgrid_pts_size[3*ds_rel_index+1];
    int yIdx = (rel_index/nX) % nY;
    pt[1] = this->rgrid_pts[3*ds_rel_index+1][yIdx];
    int zIdx = rel_index/(nX*nY);
    pt[2] = this->rgrid_pts[3*ds_rel_index+2][zIdx];
    }
}


//----------------------------------------------------------------------------
void  vtkCMFEDesiredPoints::SetValue(int p, float *v)
{
  for (int i = 0 ; i < this->NumberOfComps ; i++)
    {
    this->Values[p*this->NumberOfComps + i] = v[i];
    }
}

//----------------------------------------------------------------------------
const float * vtkCMFEDesiredPoints::GetValue(int ds_idx, int pt_idx) const
{ 
  int true_idx = this->DataSetStartIndices[ds_idx] + pt_idx;
  return this->Values + this->NumberOfComps*true_idx;
}


//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::GetProcessorsForGrid(int grid, vtkstd::vector<int> &procId, vtkstd::vector<float> &procBoundary, vtkCMFESpatialPartition *spat_part)
{
  float *xp = this->rgrid_pts[3*grid];
  float *yp = this->rgrid_pts[3*grid+1];
  float *zp = this->rgrid_pts[3*grid+2];
  int nX = this->rgrid_pts_size[3*grid];
  int nY = this->rgrid_pts_size[3*grid+1];
  int nZ = this->rgrid_pts_size[3*grid+2];
  float bounds[6];
  bounds[0] = xp[0];
  bounds[1] = xp[nX-1];
  bounds[2] = yp[0];
  bounds[3] = yp[nY-1];
  bounds[4] = zp[0];
  bounds[5] = zp[nZ-1];
  spat_part->GetProcessorBoundaries(bounds, procId, procBoundary);
}
         
//----------------------------------------------------------------------------
bool vtkCMFEDesiredPoints::GetSubgridForBoundary(int grid, float *bounds, int *extents)
{
// ****************************************************************************
//  Method: DesiredPoints::GetSubgridForBoundary
//
//  Purpose:
//      When a rectilinear grid overlaps with a region from the spatial
//      partition, it is likely that parts of the grid are within the region
//      and parts are outside the region.  If we naively send the whole
//      grid to the processor that owns the region, it will have more points
//      than it needs to sample.  The solution is to find the subgrid that
//      lies totally within the boundary.  That is the purpose of this method.
//
//  Notes:      Even though a boundary may overlap a region, it is possible
//              for this method to return "false" for no overlap.  This is 
//              because the grid may overlap the region, but none of its
//              points are actually within the region.
//              As in:
//                  G-------G
//                  |       |
//             R----+-------+----R
//             |    |       |    |
//             |    |       |    |
//             |    |       |    |
//             R----+-------+----R
//                  |       |
//                  G-------G
//
//             If R represents the boundaries of the region and G represents
//             grid points on the grid, then no point in G lies within R,
//             even though they overlap.  Obviously, this case comes up very
//             rarely, as the grid must be as coarse as actual regions, but
//             it does come up, so the caller must prepare for it.
//
//  Programmer: Hank Childs
//  Creation:   March 19, 2006
//
// ****************************************************************************

  float *xp = this->rgrid_pts[3*grid];
  float *yp = this->rgrid_pts[3*grid+1];
  float *zp = this->rgrid_pts[3*grid+2];
  int nX = this->rgrid_pts_size[3*grid];
  int nY = this->rgrid_pts_size[3*grid+1];
  int nZ = this->rgrid_pts_size[3*grid+2];

  int xStart = 0;
  while (xp[xStart] < bounds[0] && xStart < nX)
    {
    xStart++;
    }
  int xEnd = nX-1;
  while (xp[xEnd] > bounds[1] && xEnd > 0)
    {
    xEnd--;
    }
  if (xEnd < xStart)
    {
    return false;
    }

  int yStart = 0;
  while (yp[yStart] < bounds[2] && yStart < nY)
    {
    yStart++;
    }
  int yEnd = nY-1;
  while (yp[yEnd] > bounds[3] && yEnd > 0)
    {
    yEnd--;
    }
  if (yEnd < yStart)
    {
    return false;
    }

  int zStart = 0;
  while (zp[zStart] < bounds[4] && zStart < nZ)
    {
    zStart++;
    }
  int zEnd = nZ-1;
  while (zp[zEnd] > bounds[5] && zEnd > 0)
    {
    zEnd--;
    }
  if (zEnd < zStart)
    {
    return false;
    }

  extents[0] = xStart;
  extents[1] = xEnd;
  extents[2] = yStart;
  extents[3] = yEnd;
  extents[4] = zStart;
  extents[5] = zEnd;

  return true;
}


//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::RelocatePointsUsingPartition( vtkCMFESpatialPartition *spat_part)
{
  int   i, j, k;      
  int   nProcs = CMFEUtility::PAR_Size();
  
  // Start off by assessing how much data needs to be sent, and to where. 
  vtkstd::vector<int> pt_cts(nProcs, 0);
  for (i = 0 ; i < this->pt_list_size.size() ; i++)
    {
    const int npts = this->pt_list_size[i];
    float    *pts  = this->pt_list[i];
    for (j = 0 ; j < npts ; j++)
      {
      float pt[3];
      pt[0] = *pts++;
      pt[1] = *pts++;
      pt[2] = *pts++;
      int proc = spat_part->GetProcessor(pt);
      pt_cts[proc]++;
      }
    }
  vtkstd::vector<int> grids(nProcs, 0);
  vtkstd::vector<int> total_size(nProcs, 0);
  for (i = 0 ; i < this->NumberOfGrids ; i++) 
    {
    vtkstd::vector<int> procId;
    vtkstd::vector<float> procBoundary;
    this->GetProcessorsForGrid(i, procId, procBoundary, spat_part);
    for (j = 0 ; j < procId.size() ; j++)
      {
      int extents[6];
      float bounds[6];
      for (k = 0 ; k < 6 ; k++)
        {
        bounds[k] = procBoundary[6*j+k];
        }
      bool overlaps = this->GetSubgridForBoundary(i, bounds, extents);
      if (!overlaps)
        {
        continue;
        }

      grids[procId[j]]++;
      int npts = (extents[1]-extents[0]+1) + (extents[3]-extents[2]+1) + (extents[5]-extents[4]+1);
      total_size[procId[j]] += npts;
      }
    }


  // Now construct the messages to send to the other processors.
  // Construct the actual sizes for each message. 
  int *sendcount = new int[nProcs];
  int  total_msg_size = 0;
  for (j = 0 ; j < nProcs ; j++)
    {
    sendcount[j] = sizeof(int); // npts for non-rgrids;
    sendcount[j] += 3*sizeof(float)*pt_cts[j];
    sendcount[j] += sizeof(int); // num rgrids;
    sendcount[j] += 3*grids[j]*sizeof(int); // dims for each rgrid
    sendcount[j] += total_size[j]*sizeof(float); // coords for all rgrids
    total_msg_size += sendcount[j];
    }

  // Now allocate the memory and set up the "sub-messages", which allow
  // us to directly access the memory based on which processor a piece
  // of data is going to.
  char *big_send_msg = new char[total_msg_size];
  char **sub_ptr = new char*[nProcs];
  sub_ptr[0] = big_send_msg;
  for (i = 1 ; i < nProcs ; i++)
    {
    sub_ptr[i] = sub_ptr[i-1] + sendcount[i-1];
    }

  // Now add the initial header info ... "how many points I have for your
  // processor".
  for (j = 0 ; j < nProcs ; j++)
    {
    int numFromMeToProcJ = pt_cts[j];
    memcpy(sub_ptr[j], (void *) &numFromMeToProcJ, sizeof(int));
    sub_ptr[j] += sizeof(int);
    }

  // Now add the actual points to the message.
  for (i = 0 ; i < this->pt_list_size.size() ; i++)
    {
    const int npts = this->pt_list_size[i];
    float    *pts  = this->pt_list[i];
    for (j = 0 ; j < npts ; j++)
      {
      float pt[3];
      pt[0] = *pts++;
      pt[1] = *pts++;
      pt[2] = *pts++;
      int proc = spat_part->GetProcessor(pt);
      memcpy(sub_ptr[proc], (void *) pt, 3*sizeof(float));
      sub_ptr[proc] += 3*sizeof(float);
      }
    }
  
  // Now add the initial header info for "how many rgrids I have for your
  // processor".
  for (j = 0 ; j < nProcs ; j++)
    {
    int numGridsFromMeToProcJ = grids[j];
    memcpy(sub_ptr[j], (void *) &numGridsFromMeToProcJ, sizeof(int));
    sub_ptr[j] += sizeof(int);
    }

 
  // And add the actual information about the rgrid. 
  for (i = 0 ; i < this->NumberOfGrids ; i++) 
    {
    vtkstd::vector<int> procId;
    vtkstd::vector<float> procBoundary;
    this->GetProcessorsForGrid(i, procId, procBoundary, spat_part);
    for (j = 0 ; j < procId.size() ; j++)
      {
      int extents[6];
      float bounds[6];
      for (k = 0 ; k < 6 ; k++)
        {
        bounds[k] = procBoundary[6*j+k];
        }
      bool overlaps = this->GetSubgridForBoundary(i, bounds, extents);
      if (!overlaps)
        {
        continue;
        }
      int proc = procId[j];
      int nX = extents[1] - extents[0] + 1;
      int nY = extents[3] - extents[2] + 1;
      int nZ = extents[5] - extents[4] + 1;
      memcpy(sub_ptr[proc], (void *) &nX, sizeof(int));
      sub_ptr[proc] += sizeof(int);
      memcpy(sub_ptr[proc], (void *) &nY, sizeof(int));
      sub_ptr[proc] += sizeof(int);
      memcpy(sub_ptr[proc], (void *) &nZ, sizeof(int));
      sub_ptr[proc] += sizeof(int);

      memcpy(sub_ptr[proc],this->rgrid_pts[3*i] + extents[0],sizeof(float)*nX);
      sub_ptr[proc] += sizeof(float)*nX;
      memcpy(sub_ptr[proc],this->rgrid_pts[3*i+1]+extents[2],sizeof(float)*nY);
      sub_ptr[proc] += sizeof(float)*nY;
      memcpy(sub_ptr[proc],this->rgrid_pts[3*i+2]+extents[4],sizeof(float)*nZ);
      sub_ptr[proc] += sizeof(float)*nZ;
      }
    }

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

  // Set up the buffers so we can read the information out.
  sub_ptr[0] = big_recv_msg;
  for (i = 1 ; i < nProcs ; i++)
  sub_ptr[i] = sub_ptr[i-1] + recvcount[i-1];

  // Translate the buffers we just received into the points we should look
  // at.
  int numPts = 0;
  this->pt_list_came_from.clear();
  vtkstd::vector<float *> new_pt_list;
  vtkstd::vector<int> new_pt_list_size;
  for (j = 0 ; j < nProcs ; j++)
    {
    int numFromProcJ = 0;
    memcpy((void *) &numFromProcJ, sub_ptr[j], sizeof(int));
    sub_ptr[j] += sizeof(int);
    if (numFromProcJ == 0)
      {
      continue;
      }
    float *newPts = new float[3*numFromProcJ];
    memcpy(newPts, sub_ptr[j], numFromProcJ * 3 * sizeof(float));
    new_pt_list.push_back(newPts);
    new_pt_list_size.push_back(numFromProcJ);
    this->pt_list_came_from.push_back(j);
    sub_ptr[j] += numFromProcJ * 3 * sizeof(float);
    }

  // Translate the buffers that correspond to rgrids.
  vtkstd::vector<float *> new_rgrid_pts;
  vtkstd::vector<int>     new_rgrid_pts_size;
  this->rgrid_came_from.clear();
  for (j = 0 ; j < nProcs ; j++)
    {
    int numGridsFromProcJToMe;
    memcpy((void *) &numGridsFromProcJToMe, sub_ptr[j], sizeof(int));
    sub_ptr[j] += sizeof(int);
    for (k = 0 ; k < numGridsFromProcJToMe ; k++)
      {
      int nX, nY, nZ;
      memcpy((void *) &nX, sub_ptr[j], sizeof(int));
      sub_ptr[j] += sizeof(int);
      memcpy((void *) &nY, sub_ptr[j], sizeof(int));
      sub_ptr[j] += sizeof(int);
      memcpy((void *) &nZ, sub_ptr[j], sizeof(int));
      sub_ptr[j] += sizeof(int);

      float *x = new float[nX];
      memcpy(x, sub_ptr[j], sizeof(float)*nX);
      sub_ptr[j] += sizeof(float)*nX;
      float *y = new float[nY];
      memcpy(y, sub_ptr[j], sizeof(float)*nY);
      sub_ptr[j] += sizeof(float)*nY;
      float *z = new float[nZ];
      memcpy(z, sub_ptr[j], sizeof(float)*nZ);
      sub_ptr[j] += sizeof(float)*nZ;

      new_rgrid_pts.push_back(x);
      new_rgrid_pts.push_back(y);
      new_rgrid_pts.push_back(z);
      new_rgrid_pts_size.push_back(nX);
      new_rgrid_pts_size.push_back(nY);
      new_rgrid_pts_size.push_back(nZ);

      this->rgrid_came_from.push_back(j);
      }
    }

  // Now take our relocated points and use them as the new "desired points."
  this->orig_pt_list = this->pt_list;
  this->orig_pt_list_size = this->pt_list_size;
  this->pt_list = new_pt_list;
  this->pt_list_size = new_pt_list_size;

  this->orig_rgrid_pts = this->rgrid_pts;
  this->orig_rgrid_pts_size = this->rgrid_pts_size;
  this->rgrid_pts = new_rgrid_pts;
  this->rgrid_pts_size = new_rgrid_pts_size;

  delete [] sub_ptr;
  delete [] recvmessages;
  delete [] big_recv_msg;
  delete [] recvcount;
  delete [] recvdisp;    
}


//----------------------------------------------------------------------------
void vtkCMFEDesiredPoints::UnRelocatePoints( vtkCMFESpatialPartition *spat_part)
{
  int   i, j, k;

  int   nProcs = CMFEUtility::PAR_Size();

  // Clean up the current points and restore the "orig" points.
  // Do this first, because it will buy us a little memory in case we're
  // close to going over.
  for (i = 0 ; i < this->pt_list.size() ; i++)
    {
    delete [] this->pt_list[i];
    }
  for (i = 0 ; i < this->rgrid_pts.size() ; i++)
    {
    delete [] this->rgrid_pts[i];
    }
  
  // We need to take the this->Values for our point list and send them back to the
  // processor they came from.
  int *sendcount = new int[nProcs];
  for (i = 0 ; i < nProcs ; i++)
    {
    sendcount[i] = 0;
    }
  
  for (i = 0 ; i < this->pt_list_came_from.size() ; i++)
    {
    sendcount[this->pt_list_came_from[i]]+=this->pt_list_size[i]*sizeof(float)*this->NumberOfComps;
    }
  
  for (i = 0 ; i < this->rgrid_came_from.size() ; i++)
    {
    int npts = this->rgrid_pts_size[3*i] * this->rgrid_pts_size[3*i+1] * this->rgrid_pts_size[3*i+2];
    sendcount[this->rgrid_came_from[i]] += npts*sizeof(float)*this->NumberOfComps;
    }

  int  totalSend = 0;
  for (i = 0 ; i < nProcs ; i++)
    {
    totalSend += sendcount[i];
    }
  
  // Set up the message that contains the actual point values.
  char *big_send_msg = new char[totalSend];
  char **sub_ptr = new char*[nProcs];
  sub_ptr[0] = big_send_msg;
  for (i = 1 ; i < nProcs ; i++)
    {
    sub_ptr[i] = sub_ptr[i-1] + sendcount[i-1];
    }

  float *values_tmp = this->Values;
  for (i = 0 ; i < this->pt_list_came_from.size() ; i++)
    {
    int msgGoingTo = this->pt_list_came_from[i];
    memcpy(sub_ptr[msgGoingTo], values_tmp, this->pt_list_size[i]*sizeof(float)*this->NumberOfComps);
    sub_ptr[msgGoingTo] += this->pt_list_size[i]*sizeof(float)*this->NumberOfComps;
    values_tmp += this->pt_list_size[i]*this->NumberOfComps;
    }
  for (i = 0 ; i < this->rgrid_came_from.size() ; i++)
    {
    int msgGoingTo = this->rgrid_came_from[i];
    int npts = this->rgrid_pts_size[3*i] * this->rgrid_pts_size[3*i+1] * this->rgrid_pts_size[3*i+2];
    memcpy(sub_ptr[msgGoingTo], values_tmp, npts*sizeof(float)*this->NumberOfComps);
    sub_ptr[msgGoingTo] += npts*sizeof(float)*this->NumberOfComps;
    values_tmp += npts*this->NumberOfComps;
    }

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


  // Now put our point list back in order like it was never modified for
  // parallel reasons.
  this->pt_list = this->orig_pt_list;
  this->pt_list_size = this->orig_pt_list_size;
  this->orig_pt_list.clear();
  this->orig_pt_list_size.clear();
  this->rgrid_pts = this->orig_rgrid_pts;
  this->rgrid_pts_size = this->orig_rgrid_pts_size;
  this->orig_rgrid_pts.clear();
  this->orig_rgrid_pts_size.clear();
  this->Finalize(); // Have it set up internal data structures based on the "new"
  // (i.e. original) points list.

  //
  // Now go through the recently sent messages and get the "this->Values" sent by
  // the other processors.  Encode them into a new "this->Values" array.
  //
  int idx = 0;
  for (i = 0 ; i < this->pt_list_size.size() ; i++)
    {
    const int npts = this->pt_list_size[i];
    float *pts  = this->pt_list[i];
    for (j = 0 ; j < npts ; j++)
      {
      float pt[3];
      pt[0] = *pts++;
      pt[1] = *pts++;
      pt[2] = *pts++;
      int proc = spat_part->GetProcessor(pt);
      float *p = (float *) recvmessages[proc];
      for (k = 0 ; k < this->NumberOfComps ; k++)
        {
        this->Values[idx++] = *p++;
        }
      recvmessages[proc] += sizeof(float)*this->NumberOfComps;
      }
    }
  for (i = 0 ; i < this->NumberOfGrids ; i++) 
    {
    int realNX = this->rgrid_pts_size[3*i];
    int realNY = this->rgrid_pts_size[3*i+1];
    int realNZ = this->rgrid_pts_size[3*i+2];
    int npts = realNX * realNY * realNZ;
    vtkstd::vector<int> procId;
    vtkstd::vector<float> procBoundary;
    this->GetProcessorsForGrid(i, procId, procBoundary, spat_part);
    for (j = 0 ; j < procId.size() ; j++)
      {
      int extents[6];
      float bounds[6];
      for (k = 0 ; k < 6 ; k++)
        {
        bounds[k] = procBoundary[6*j+k];
        }
      // Find out how much of the rectilinear grid overlapped with
      // the boundary for processor procId[j].  How much overlap there
      // is is how much data that processor is sending us back.
      bool overlaps = this->GetSubgridForBoundary(i, bounds, extents);
      
      if (!overlaps)
        {
        continue;
        }
      for (int z = extents[4] ; z <= extents[5] ; z++)
        {
        for (int y = extents[2] ; y <= extents[3] ; y++)
          {
          for (int x = extents[0] ; x <= extents[1] ; x++)
            {
            int valIDX = z*realNX*realNY + y*realNX + x;
            float *p = (float *) recvmessages[procId[j]];
            for (k = 0 ; k < this->NumberOfComps ; k++)
              {
              this->Values[idx + this->NumberOfComps*valIDX + k] = *p;
              }
            recvmessages[procId[j]] += sizeof(float)*this->NumberOfComps;
            }
          }
        }
      }
    idx += npts*this->NumberOfComps;
    }

  delete [] recvmessages;
  delete [] big_recv_msg;
  delete [] recvcount;
  delete [] recvdisp;
}