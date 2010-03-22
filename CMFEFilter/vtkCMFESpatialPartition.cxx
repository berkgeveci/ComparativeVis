/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCMFEFilter.cxx,v $

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

#include "vtkCMFESpatialPartition.h"

#include "vtkCMFEUtility.h"
#include "vtkCMFEIntervalTree.h"

#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkMultiProcessController.h"


vtkCMFESpatialPartition::vtkCMFESpatialPartition()
{
    this->IntervalTree = NULL;
}


// ****************************************************************************
//  Method: SpatialPartition destructor
//
//  Programmer: Hank Childs
//  Creation:   October 12, 2005
//
// ****************************************************************************

vtkCMFESpatialPartition::~vtkCMFESpatialPartition()
{
    delete this->IntervalTree;
}


// ****************************************************************************
//  Class: Boundary
//
//  Purpose:
//      This class is for setting up a spatial partition.  It contains methods
//      that allow the spatial partitioning routine to not be so cumbersome.
//
//  Programmer: Hank Childs
//  Creation:   January 9, 2006
//
//  Modifications:
//
//    Hank Childs, Sat Mar 18 10:15:23 PST 2006
//    Add support for rectilinear grids.
//
// ****************************************************************************

typedef enum
{
    X_AXIS,
    Y_AXIS, 
    Z_AXIS
} Axis;

const int npivots = 5;
class Boundary
{
   public:
                      Boundary(const float *, int, Axis);
     virtual         ~Boundary() {;};

     float           *GetBoundary() { return bounds; };
     bool             AttemptSplit(Boundary *&, Boundary *&);
     bool             IsDone(void) { return isDone; };
     bool             IsLeaf(void) { return (numProcs == 1); };
     void             AddCell(const float *);
     void             AddPoint(const float *);
     void             AddRGrid(const float *, const float *, const float *,
                               int, int, int);
     static void      SetIs2D(bool b) { is2D = b; };
     static void      PrepareSplitQuery(Boundary **, int);
     
   protected:
     float            bounds[6];
     float            pivots[npivots];
     int              numCells[npivots+1];
     int              numProcs;
     int              nAttempts;
     Axis             axis;
     bool             isDone;
     static bool      is2D;
};

bool Boundary::is2D = false;


// ****************************************************************************
//  Method: Boundary constructor
//
//  Programmer: Hank Childs
//  Creation:   January 9, 2006
// 
// ****************************************************************************

Boundary::Boundary(const float *b, int n, Axis a)
{
    int  i;

    for (i = 0 ; i < 6 ; i++)
        bounds[i] = b[i];
    numProcs = n;
    axis = a;
    isDone = (numProcs == 1);
    nAttempts = 0;

    //
    // Set up the pivots.
    //
    int index = 0;
    if (axis == Y_AXIS)
        index = 2;
    else if (axis == Z_AXIS)
        index = 4;
    float min = bounds[index];
    float max = bounds[index+1];
    float step = (max-min) / (npivots+1);
    for (i = 0 ; i < npivots ; i++)
    {
        pivots[i] = min + (i+1)*step;
    }
    for (i = 0 ; i < npivots+1 ; i++)
        numCells[i] = 0;
}


// ****************************************************************************
//  Method: Boundary::PrepareSplitQuery
//
//  Purpose:
//      Each Boundary is operating only with the information on this processor.
//      When it comes time to determine if we can split a boundary, the info
//      from each processor needs to be unified.  That is the purpose of this
//      method.  It unifies the information so that Boundaries can later make
//      good decisions regarding whether or not they can split themselves.
//
//  Programmer: Hank Childs
//  Creation:   January 9, 2006
// 
// ****************************************************************************

void
Boundary::PrepareSplitQuery(Boundary **b_list, int listSize)
{
    int   i, j;
    int   idx;

    int  num_vals = listSize*(npivots+1);
    int *in_vals = new int[num_vals];
    idx = 0;
    for (i = 0 ; i < listSize ; i++)
        for (j = 0 ; j < npivots+1 ; j++)
            in_vals[idx++] = b_list[i]->numCells[j];

    int *out_vals = new int[num_vals];   
    CMFEUtility::SumIntArrayAcrossAllProcessors(in_vals, out_vals, num_vals);

    idx = 0;
    for (i = 0 ; i < listSize ; i++)
        for (j = 0 ; j < npivots+1 ; j++)
            b_list[i]->numCells[j] = out_vals[idx++];

    delete [] in_vals;
    delete [] out_vals;
}


// ****************************************************************************
//  Method: Boundary::AddPoint
//
//  Purpose:
//      Adds a point to the boundary.
//
//  Programmer: Hank Childs
//  Creation:   January 9, 2006
// 
// ****************************************************************************

void
Boundary::AddPoint(const float *pt)
{
    float p = (axis == X_AXIS ? pt[0] : axis == Y_AXIS ? pt[1] : pt[2]);
    for (int i = 0 ; i < npivots ; i++)
        if (p < pivots[i])
        {
            numCells[i]++;
            return;
        }
    numCells[npivots]++;
}


// ****************************************************************************
//  Method: Boundary::AddRGrid
//
//  Purpose:
//      Adds an rgrid to the boundary.
//
//  Programmer: Hank Childs
//  Creation:   March 18, 2006
// 
// ****************************************************************************

void
Boundary::AddRGrid(const float *x, const float *y, const float *z, int nX,
                   int nY, int nZ)
{
    //
    // Start by narrowing the total rgrid down to just the portion that is
    // relevant to this boundary.
    //
    int xStart = 0;
    while (x[xStart] < bounds[0] && xStart < nX)
        xStart++;
    int xEnd = nX-1;
    while (x[xEnd] > bounds[1] && xEnd > 0)
        xEnd--;
    int yStart = 0;
    while (y[yStart] < bounds[2] && yStart < nY)
        yStart++;
    int yEnd = nY-1;
    while (y[yEnd] > bounds[3] && yEnd > 0)
        yEnd--;
    int zStart = 0;
    while (z[zStart] < bounds[4] && zStart < nZ)
        zStart++;
    int zEnd = nZ-1;
    while (z[zEnd] > bounds[5] && zEnd > 0)
        zEnd--;

    const float *arr  = NULL;
    int          arrStart = 0;
    int          arrEnd   = 0;
    int          slab     = 0;

    switch (axis)
    {
      case X_AXIS:
        arr  = x;
        arrStart = xStart;
        arrEnd   = xEnd;
        slab = (yEnd-yStart+1)*(zEnd-zStart+1);
        break;
      case Y_AXIS:
        arr  = y;
        arrStart = yStart;
        arrEnd   = yEnd;
        slab = (xEnd-xStart+1)*(zEnd-zStart+1);
        break;
      case Z_AXIS:
        arr  = z;
        arrStart = zStart;
        arrEnd   = zEnd;
        slab = (xEnd-xStart+1)*(yEnd-yStart+1);
        break;
    }

    int curIdx = arrStart;
    for (int i = 0 ; i < npivots ; i++)
        while (curIdx <= arrEnd && arr[curIdx] < pivots[i])
        {
            curIdx++;
            numCells[i] += slab;
        }
    while (curIdx <= arrEnd)
    {
        curIdx++;
        numCells[npivots] += slab;
    }
}


// ****************************************************************************
//  Method: Boundary::AttemptSplit
//
//  Purpose:
//      Sees if the boundary has found an acceptable pivot to split around.
//
//  Programmer: Hank Childs
//  Creation:   January 9, 2006
// 
// ****************************************************************************

bool
Boundary::AttemptSplit(Boundary *&b1, Boundary *&b2)
{
    int  i;

    int numProcs1 = numProcs/2;
    int numProcs2 = numProcs-numProcs1;
    
    int totalCells = 0;
    for (i = 0 ; i < npivots+1 ; i++)
        totalCells += numCells[i]; 

    if (totalCells == 0)
    {
        // Should never happen...
        isDone = true;
        return false;
    }

    int cellsSoFar = 0;
    float amtSeen[npivots];
    for (i = 0 ; i < npivots ; i++)
    {
        cellsSoFar += numCells[i];
        amtSeen[i] = cellsSoFar / ((float) totalCells);
    }

    float proportion = ((float) numProcs1) / ((float) numProcs);
    float closest  = fabs(proportion - amtSeen[0]); // == proportion
    int   closestI = 0;
    for (i = 1 ; i < npivots ; i++)
    {
        float diff = fabs(proportion - amtSeen[i]);
        if (diff < closest)
        {
            closest  = diff;
            closestI = i;
        }
    }

    nAttempts++;
    if (closest < 0.02 || nAttempts > 3)
    {
        float b_tmp[6];
        for (i = 0 ; i < 6 ; i++)
            b_tmp[i] = bounds[i];
        if (axis == X_AXIS)
        {
            b_tmp[1] = pivots[closestI];
            b1 = new Boundary(b_tmp, numProcs1, Y_AXIS);
            b_tmp[0] = pivots[closestI];
            b_tmp[1] = bounds[1];
            b2 = new Boundary(b_tmp, numProcs2, Y_AXIS);
        }
        else if (axis == Y_AXIS)
        {
            Axis next_axis = (is2D ? X_AXIS : Z_AXIS);
            b_tmp[3] = pivots[closestI];
            b1 = new Boundary(b_tmp, numProcs1, next_axis);
            b_tmp[2] = pivots[closestI];
            b_tmp[3] = bounds[3];
            b2 = new Boundary(b_tmp, numProcs2, next_axis);
        }
        else
        {
            b_tmp[5] = pivots[closestI];
            b1 = new Boundary(b_tmp, numProcs1, X_AXIS);
            b_tmp[4] = pivots[closestI];
            b_tmp[5] = bounds[5];
            b2 = new Boundary(b_tmp, numProcs2, X_AXIS);
        }
        isDone = true;
    }
    else
    {
        //
        // Set up the pivots.  We are going to reset the pivot positions to be
        // in between the two closest pivots.
        //
        int firstBigger = -1;
        int amtSeen = 0;
        for (i = 0 ; i < npivots+1 ; i++)
        {
            amtSeen += numCells[i];
            float soFar = ((float) amtSeen) / ((float) totalCells);
            if (soFar > proportion)
            {
                firstBigger = i;
                break;
            }
        }

        float min, max;

        int index = 0;
        if (axis == Y_AXIS)
            index = 2;
        else if (axis == Z_AXIS)
            index = 4;

        if (firstBigger <= 0)
        {
            min = pivots[0] - (pivots[1] - pivots[0]);
            max = pivots[0];
        } 
        else if (firstBigger >= npivots)
        {
            min = pivots[npivots-1];
            max = pivots[npivots-1] + (pivots[1] - pivots[0]);
        }
        else
        {
            min = pivots[firstBigger-1];
            max = pivots[firstBigger];
        }
        float step = (max-min) / (npivots+1);
        for (i = 0 ; i < npivots ; i++)
            pivots[i] = min + (i+1)*step;
        for (i = 0 ; i < npivots+1 ; i++)
            numCells[i] = 0;

        return false;
    }

    return true;
}


// ****************************************************************************
//  Method: SpatialPartition::CreatePartition
//
//  Purpose:
//      Creates a partition that is balanced for both the desired points and
//      the fast lookup grouping.
//
//  Programmer: Hank Childs
//  Creation:   October 12, 2005
//
//  Modification:
//
//    Hank Childs, Fri Mar 10 14:35:32 PST 2006
//    Add fix for parallel engines of 1 processor.
//
//    Hank Childs, Sat Mar 18 10:15:23 PST 2006
//    Add support for rectilinear meshes.
//
//    Kathleen Bonnell, Mon Aug 14 16:40:30 PDT 2006
//    API change for avtIntervalTree.
//
//    Hank Childs, Tue Dec 18 13:50:12 PST 2007
//    Do not use copy constructor for interval tree, since it was recently 
//    declared to be private.
//
// ****************************************************************************

void vtkCMFESpatialPartition::CreatePartition(vtkCMFEDesiredPoints *dp,
                                       vtkCMFEFastLookupGrouping *flg, double *bounds)
{
    int   i, j, k;   
    if (this->IntervalTree != NULL)
        delete this->IntervalTree;

    //
    // Here's the gameplan:
    // We are going to start off with a single "boundary".  Ultimately, we
    // are going to want to have N boundaries, where N is the number of
    // processors.  So we tell this initial boundary that it represents N
    // processors.  Then we tell it to choose some pivots that it thinks
    // might allow itself to split into two boundaries, each with half the 
    // amount of work and each representing half the number of processor. 
    // Now we have two boundaries, and we keep splitting them (across 
    // different axes) until we get N boundaries, where each one represents
    // a single processor.
    //
    // Once we do that, we can construct an interval tree of the boundaries,
    // which represents our spatial partitioning.
    //
    bool is2D = (bounds[4] == bounds[5]);
    Boundary::SetIs2D(is2D);
    int nProcs = CMFEUtility::PAR_Size();
    Boundary **b_list = new Boundary*[2*nProcs];
    float fbounds[6];
    fbounds[0] = bounds[0];
    fbounds[1] = bounds[1];
    fbounds[2] = bounds[2];
    fbounds[3] = bounds[3];
    fbounds[4] = bounds[4];
    fbounds[5] = bounds[5];
    if (is2D)
    {
        fbounds[4] -= 1.;
        fbounds[5] += 1.;
    }
    b_list[0] = new Boundary(fbounds, nProcs, X_AXIS);
    int listSize = 1;
    int *bin_lookup = new int[2*nProcs];
    bool keepGoing = (nProcs > 1);
    while (keepGoing)
    {
        // Figure out how many boundaries need to keep going.
        int nBins = 0;
        for (i = 0 ; i < listSize ; i++)
            if (!(b_list[i]->IsDone()))
            {
                bin_lookup[nBins] = i;
                nBins++;
            }

        // Construct an interval tree out of the boundaries.  We need this
        // because we want to be able to quickly determine which boundaries
        // a point falls in.
        vtkCMFEIntervalTree it(nBins, 3);
        nBins = 0;
        for (i = 0 ; i < listSize ; i++)
        {
            if (b_list[i]->IsDone())
                continue;
            float *b = b_list[i]->GetBoundary();
            double db[6] = {b[0], b[1], b[2], b[3], b[4], b[5]};
            it.AddElement(nBins, db);
            nBins++;
        }
        it.Calculate(true);

        // Now add each point to the boundary it falls in.  Start by doing
        // the points that come from unstructured or structured meshes.
        const int nPoints = dp->GetRGridStart();
        vtkstd::vector<int> list;
        float pt[3];
        for (i = 0 ; i < nPoints ; i++)
        {
            dp->GetPoint(i, pt);
            double dpt[3] = {pt[0], pt[1], pt[2]};
            it.GetElementsListFromRange(dpt, dpt, list);
            for (j = 0 ; j < list.size() ; j++)
            {
                Boundary *b = b_list[bin_lookup[list[j]]];
                b->AddPoint(pt);
            }
        }

        // Now do the points that come from rectlinear meshes.
        int num_rgrid = dp->GetNumberOfRGrids();
        for (i = 0 ; i < num_rgrid ; i++)
        {
            const float *x, *y, *z;
            int          nX, nY, nZ;
            dp->GetRGrid(i, x, y, z, nX, nY, nZ);
            double min[3];
            min[0] = x[0];
            min[1] = y[0];
            min[2] = z[0];
            double max[3];
            max[0] = x[nX-1];
            max[1] = y[nY-1];
            max[2] = z[nY-1];
            it.GetElementsListFromRange(min, max, list);
            for (j = 0 ; j < list.size() ; j++)
            {
                Boundary *b = b_list[bin_lookup[list[j]]];
                b->AddRGrid(x, y, z, nX, nY, nZ);
            }
        }

        // Now do the cells.  We are using the cell centers, which is a decent
        // approximation.
        vtkstd::vector<vtkDataSet *> meshes = flg->GetMeshes();
        for (i = 0 ; i < meshes.size() ; i++)
        {
            const int ncells = meshes[i]->GetNumberOfCells();
            double bbox[6];
            double pt[3];
            for (j = 0 ; j < ncells ; j++)
            {
                vtkCell *cell = meshes[i]->GetCell(j);
                cell->GetBounds(bbox);
                pt[0] = (bbox[0] + bbox[1]) / 2.;
                pt[1] = (bbox[2] + bbox[3]) / 2.;
                pt[2] = (bbox[4] + bbox[5]) / 2.;
                it.GetElementsListFromRange(pt, pt, list);
                float fpt[3] = {pt[0], pt[1], pt[2]};
                for (k = 0 ; k < list.size() ; k++)
                {
                    Boundary *b = b_list[bin_lookup[list[k]]];
                    b->AddPoint(fpt);
                }
            }
        }

        // See which boundaries found a suitable pivot and can now split.
        Boundary::PrepareSplitQuery(b_list, listSize);
        int numAtStartOfLoop = listSize;
        for (i = 0 ; i < numAtStartOfLoop ; i++)
        {
            if (b_list[i]->IsDone())
                continue;
            Boundary *b1, *b2;
            if (b_list[i]->AttemptSplit(b1, b2))
            {
                b_list[listSize++] = b1;
                b_list[listSize++] = b2;
            }
        }

        // See if there are any boundaries that need more processing.  
        // Obviously, all the boundaries that were just split need more 
        // processing, because they haven't done any yet.
        keepGoing = false;
        for (i = 0 ; i < listSize ; i++)
            if (!(b_list[i]->IsDone()))
                keepGoing = true;
    }

    // Construct an interval tree out of the boundaries.  This interval tree
    // contains the actual spatial partitioning.
    this->IntervalTree = new vtkCMFEIntervalTree(nProcs, 3);
    int count = 0;
    for (i = 0 ; i < listSize ; i++)
    {
        if (b_list[i]->IsLeaf())
        {
            float *b = b_list[i]->GetBoundary();
            double db[6] = {b[0], b[1], b[2], b[3], b[4], b[5]};
            this->IntervalTree->AddElement(count++, db);
        }
    }
    this->IntervalTree->Calculate(true);

    bool determineBalance = false;
    if (determineBalance)
    {
        count = 0;
        for (i = 0 ; i < listSize ; i++)
        {
            if (b_list[i]->IsLeaf())
            {
                float *b = b_list[i]->GetBoundary();
               /* debug1 << "Boundary " << count++ << " = " << b[0] << "-" <<b[1]
                       << ", " << b[2] << "-" << b[3] << ", " << b[4] << "-"
                       << b[5] << endl;*/
            }
        }

        int *cnts = new int[nProcs];
        for (i = 0 ; i < nProcs ; i++)
            cnts[i] = 0;
        const int nPoints = dp->GetNumberOfPoints();
        vtkstd::vector<int> list;
        float pt[3];
        for (i = 0 ; i < nPoints ; i++)
        {
            dp->GetPoint(i, pt);
            double dpt[3] = {(double)pt[0], (double)pt[1], (double)pt[2]};
            this->IntervalTree->GetElementsListFromRange(dpt, dpt, list);
            for (j = 0 ; j < list.size() ; j++)
            {
                cnts[list[j]]++;
            }
        }
        int *cnts_out = new int[nProcs];
        CMFEUtility::SumIntArrayAcrossAllProcessors(cnts, cnts_out, nProcs);
        /*for (i = 0 ; i < nProcs ; i++)
            debug5 << "Amount for processor " << i << " = " << cnts_out[i] 
                   << endl;*/
        
        delete [] cnts;
        delete [] cnts_out;
    }

    // Clean up.
    for (i = 0 ; i < listSize ; i++)
        delete b_list[i];
    delete [] b_list;
    delete [] bin_lookup;

}


// ****************************************************************************
//  Method: SpatialPartition::GetProcessor
//
//  Purpose:
//      Gets the processor that contains this point
//
//  Programmer: Hank Childs
//  Creation:   October 12, 2005
//
//  Modifications:
//    Kathleen Bonnell, Mon Aug 14 16:40:30 PDT 2006
//    API change for avtIntervalTree.
//
// ****************************************************************************

int vtkCMFESpatialPartition::GetProcessor(float *pt)
{
    vtkstd::vector<int> list;

    double dpt[3] = {(double)pt[0], (double)pt[1],(double) pt[2]};
    this->IntervalTree->GetElementsListFromRange(dpt, dpt, list);
    if (list.size() <= 0)
    {
        //EXCEPTION0(ImproperUseException);
        return -1;
    }

    return list[0];
}


// ****************************************************************************
//  Method: SpatialPartition::GetProcessor
//
//  Purpose:
//      Gets the processor that contains this cell
//
//  Programmer: Hank Childs
//  Creation:   October 12, 2005
//
//  Modifications:
//    Kathleen Bonnell, Mon Aug 14 16:40:30 PDT 2006
//    API change for avtIntervalTree.
//
// ****************************************************************************

int
vtkCMFESpatialPartition::GetProcessor(vtkCell *cell)
{
    double bounds[6];
    cell->GetBounds(bounds);
    double mins[3];
    mins[0] = bounds[0];
    mins[1] = bounds[2];
    mins[2] = bounds[4];
    double maxs[3];
    maxs[0] = bounds[1];
    maxs[1] = bounds[3];
    maxs[2] = bounds[5];

    vtkstd::vector<int> list;
    this->IntervalTree->GetElementsListFromRange(mins, maxs, list);
    if (list.size() <= 0)
    {
        return -2;
    }
    if (list.size() > 1)
    {
        return -1;
    }

    return list[0];
}


// ****************************************************************************
//  Method: SpatialPartition::GetProcessorList
//
//  Purpose:
//      Gets the processor that contains this cell.  This should be called
//      when a list of processors contain a cell.
//
//  Programmer: Hank Childs
//  Creation:   October 12, 2005
//
//  Modifications:
//    Kathleen Bonnell, Mon Aug 14 16:40:30 PDT 2006
//    API change for avtIntervalTree.
//
// ****************************************************************************

void
vtkCMFESpatialPartition::GetProcessorList(vtkCell *cell,
                                                        std::vector<int> &list)
{
    list.clear();

    double bounds[6];
    cell->GetBounds(bounds);
    double mins[3];
    mins[0] = bounds[0];
    mins[1] = bounds[2];
    mins[2] = bounds[4];
    double maxs[3];
    maxs[0] = bounds[1];
    maxs[1] = bounds[3];
    maxs[2] = bounds[5];

    this->IntervalTree->GetElementsListFromRange(mins, maxs, list);
}


// ****************************************************************************
//  Method: SpatialPartition::GetProcessorBoundaries
//
//  Purpose:
//      Gets the processor that contains this cell.  This should be called
//      when a list of processors contain a cell.
//
//  Programmer: Hank Childs
//  Creation:   March 19, 2006
//
//  Modifications:
//    Kathleen Bonnell, Mon Aug 14 16:40:30 PDT 2006
//    API change for avtIntervalTree.
//
// ****************************************************************************

void
vtkCMFESpatialPartition::GetProcessorBoundaries(float *bounds,
                                std::vector<int> &list, std::vector<float> &db)
{
    list.clear();

    double mins[3];
    mins[0] = bounds[0];
    mins[1] = bounds[2];
    mins[2] = bounds[4];
    double maxs[3];
    maxs[0] = bounds[1];
    maxs[1] = bounds[3];
    maxs[2] = bounds[5];

    this->IntervalTree->GetElementsListFromRange(mins, maxs, list);

    int numMatches = list.size();
    db.resize(numMatches*6);
    for (int i = 0 ; i < numMatches ; i++)
    {
        double domBounds[6];
        this->IntervalTree->GetElementExtents(list[i], domBounds);
        for (int j = 0 ; j < 6 ; j++)
            db[6*i+j] = domBounds[j];
    }
}

