/*=========================================================================

  Program:Visualization Toolkit
  Module: $RCSfile: vtkCMFEDesiredPoints.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCMFEDesiredPoints -- Storage Container for CMFE.
// .SECTION Description
//
// vtkCMFEDesiredPoints is a tricky class because of two subtleties.
// First: It has two ways to store data.  It stores rectilinear grids
// one way and other grids another way.  Further, its interface tries
// to unify them in places and not unify them in other places.  For
// example, this->pt_list and this->pt_list_size correspond to non-rectilinear grids,
// while this->rgrid_pts and this->rgrid_pts_size correspond to rectilinear grids.
// But this->TotalNumberOfValues corresponds to the total number of values across both.
// The thinking is that the interface for the class should be
// generalized, except where having knowledge of rectilinear layout will
// impact performance, such as is the case for pivot finding.  Of course,
// the bookkeeping under the covers is difficult.
//
// The other subtlety is the two forms this object will take.  Before
// calling "RelocatePointsUsingSpatialPartition", this object contains
// the desired points for this processor.  But, after the call, it
// contains the desired points for this processor's spatial partition.
// When "UnRelocatePoints" is called, it switches back to the desired
// points for this processor.  Again, bookkeeping overhead causes the
// coding of this class to be more complex.


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
* DIRECT,  INDIRECT,INCIDENTAL,SPECIAL,EXEMPLARY,  ORCONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/


#ifndef __vtkCMFEDesiredPoints_h
#define __vtkCMFEDesiredPoints_h

#include <vtkstd/vector>

class vtkCMFESpatialPartition;
class vtkDataSet;
class vtkMultiProcessController;

class vtkCMFEDesiredPoints
{
public:
  vtkCMFEDesiredPoints(bool, int);
  virtual ~vtkCMFEDesiredPoints();

  // Description:
  // TODO: add support vtkImageData
  // Registers a dataset with the internal storage.
  void AddDataset(vtkDataSet *);

  // Description:
  //Gives the desired points object a chance to finish its initialization
  //process. This gives the object the cue that it will not receive
  //any more "AddDataset" calls and that it can initialize itself.
  void Finalize();

  // Description:
  //Gets a rectilinear grid.
  void GetRGrid(int idx, const float *&x, const float *&y, const float *&z, int &nx, int &ny, int &nz);

  // Description:
  //Gets the next point in the sequence that should be sampled.
  void GetPoint(int index, float *point) const;

  // Description:
  //Sets the value of the 'n'th point.
  void SetValue(int index, float *point);
  
  // Description:
  //Gets the value for an index, where the index is a convenient indexing
  //scheme when iterating over the final datasets.
  const float *GetValue(int, int) const;

  // Description:
  //Relocates the points to different processors to create a spatial partition.
  void RelocatePointsUsingPartition(vtkCMFESpatialPartition *);

  // Description:
  //Sends the points from before the spatial partition object back to 
  //the processors they came from.
  void UnRelocatePoints(vtkCMFESpatialPartition *);

  // Description:
  // Get the total number of values being stored.
  int GetNumberOfPoints() { return this->TotalNumberOfValues; };

  // Description:
  //get where the grids start in the values.
  int GetRGridStart()  { return this->GridStart; };

  // Description:
  //get the number of grids that have been added.
  int GetNumberOfRGrids() { return this->NumberOfGrids; };

  
private:
  bool IsNodal;
  int NumberOfComps;
  int TotalNumberOfValues;
  int NumberOfDatasets;
  int NumberOfGrids;
  int GridStart;

  int *MapToDataSets;
  int *DataSetStartIndices;
  float *Values;

  vtkMultiProcessController *Controller;
  
  //BTX
  vtkstd::vector<float *> pt_list;
  vtkstd::vector<int>  pt_list_size;
  vtkstd::vector<float *> rgrid_pts;
  vtkstd::vector<int>  rgrid_pts_size;
  
  vtkstd::vector<float *> orig_pt_list;
  vtkstd::vector<int>  orig_pt_list_size;
  vtkstd::vector<float *> orig_rgrid_pts;
  vtkstd::vector<int>  orig_rgrid_pts_size;
  vtkstd::vector<int>  pt_list_came_from;
  vtkstd::vector<int>  rgrid_came_from;
  //ETX
  
  // Description:
  //Uses the spatial partition to determine which processors a rectilinear
  //grid overlaps with.  Also finds the boundaries of each of those processors.
  void GetProcessorsForGrid(int, vtkstd::vector<int> &, vtkstd::vector<float> &, vtkCMFESpatialPartition *);

  bool GetSubgridForBoundary(int, float *, int *);

  vtkCMFEDesiredPoints(const vtkCMFEDesiredPoints&);  // Not implemented.
  void operator=(const vtkCMFEDesiredPoints&);  // Not implemented.
};  

#endif


