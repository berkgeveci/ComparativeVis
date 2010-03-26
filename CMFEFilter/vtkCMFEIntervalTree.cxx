/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCMFEIntervalTree.cxx,v $

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


#include <vtkCMFEIntervalTree.h>
#include <vtkCMFEUtility.h>

#include <vtkstd/algorithm>
#include <float.h>
#include <math.h>
#include <stdlib.h>



//
// Types and globals (needed for qsort). 
//
namespace
{
  typedef union
  {
      double f;
      int   b;
  } DoubleInt;

  int globalCurrentDepth;
  int globalNDims;

  
// ****************************************************************************
//  Function: QsortBoundsSorter
//
//  Purpose:
//      Sorts a vector of bounds with respect to the dimension that
//      corresponds to the depth.
//
//  Notes:
//     Adapted from deprecated methods 'Sort' and 'Less' written in 
//     August of 2000.
//
//  Programmer: Hank Childs
//  Creation:   June 27, 2005
//
//  Modifications:
//
//    Mark C. Miller, Wed Aug 23 08:53:58 PDT 2006
//    Changed return values from true/false to 1, -1, 0
// ****************************************************************************

int QsortBoundsSorter(const void *arg1, const void *arg2)
{
  DoubleInt *A = (DoubleInt *) arg1;
  DoubleInt *B = (DoubleInt *) arg2;

  // Walk through the mid-points of the extents, starting with the current
  // depth and moving forward.
  int size = 2*globalNDims;
  for (int i = 0 ; i < globalNDims ; i++)
    {
    double A_mid = (A[(2*globalCurrentDepth + 2*i) % size].f
      + A[(2*globalCurrentDepth+1 + 2*i) % size].f) / 2;
    double B_mid = (B[(2*globalCurrentDepth + 2*i) % size].f
      + B[(2*globalCurrentDepth+1 + 2*i) % size].f) / 2;
    if (A_mid < B_mid)
      {
      return 1;
      }
    else if (A_mid > B_mid)
      {
      return -1;
      }
    }

  // Bounds are exactly identical.  This can cause problems if you are
  // not careful.
  return 0;
}
}


//----------------------------------------------------------------------------
vtkCMFEIntervalTree::vtkCMFEIntervalTree(int els, int dims, bool rc)
{
  this->NumberOfElements    = els;
  this->NumberOfDims       = dims;
  this->HasBeenCalculated = false;
  this->RequiresCommunication = rc;

  //
  // A vector for one element should have the min and max for each dimension.
  // 
  this->VectorSize  = 2*this->NumberOfDims;

  //
  // Calculate number of nodes needed to make a complete binary tree when
  // there should be this->NumberOfElements leaf nodes.
  //
  int numCompleteTrees = 1, exp = 1;
  while (2*exp < this->NumberOfElements)
    {
    numCompleteTrees = 2*numCompleteTrees + 1;
    exp *= 2;
    }
  this->NumberOfNodes = numCompleteTrees + 2 * (this->NumberOfElements - exp);

  this->NodeExtents = new double[this->NumberOfNodes*this->VectorSize];
  this->NodeIDs     = new int[this->NumberOfNodes];
  for (int i = 0 ; i < this->NumberOfNodes ; i++)
    {
    for (int j = 0 ; j < this->VectorSize ; j++)
      {
      this->NodeExtents[i*this->VectorSize + j] = 0.;
      }
    this->NodeIDs[i]  = -1;
    }
}


//----------------------------------------------------------------------------
vtkCMFEIntervalTree::vtkCMFEIntervalTree(const vtkCMFEIntervalTree *it)
{
  this->NumberOfElements = it->NumberOfElements;
  this->NumberOfNodes = it->NumberOfNodes;
  this->NumberOfDims = it->NumberOfDims;
  this->VectorSize = it->VectorSize;
  this->NodeExtents = new double[this->NumberOfNodes*this->VectorSize];
  this->NodeIDs     = new int[this->NumberOfNodes];
  for (int i = 0 ; i < this->NumberOfNodes ; i++)
    {
    for (int j = 0 ; j < this->VectorSize ; j++)
      {
      this->NodeExtents[i*this->VectorSize + j] = it->NodeExtents[i*this->VectorSize + j];
      }
    this->NodeIDs[i]  = it->NodeIDs[i];
    }
  this->HasBeenCalculated = it->HasBeenCalculated;
  this->RequiresCommunication = it->RequiresCommunication;
}



//----------------------------------------------------------------------------
vtkCMFEIntervalTree::~vtkCMFEIntervalTree()
{
  if (this->NodeExtents != NULL)
    {
    delete [] this->NodeExtents;
    }
  if (this->NodeIDs != NULL)
    {
    delete [] this->NodeIDs;
    }
}



//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::Destruct(void *i)
{
  vtkCMFEIntervalTree *itree = (vtkCMFEIntervalTree *) i;
  delete itree;
}


//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::GetExtents(double *extents) const
{
  if ((this->NodeExtents == NULL) || (this->HasBeenCalculated == false))
    {
    return;
    }

  //
  // Copy the root element into the extents.
  //
  for (int i = 0 ; i < this->NumberOfDims*2 ; i++)
    {
    extents[i] = this->NodeExtents[i];
    }
}


//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::AddElement(int element, double *d)
{
  //
  // Sanity Check
  //
  if (element < 0 || element >= this->NumberOfElements)
    {
    return;
    }

  for (int i = 0 ; i < this->VectorSize ; i++)
    {
    this->NodeExtents[element*this->VectorSize + i] = d[i];
    }
}

//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::Calculate(bool alreadyCollectedAllInformation)
{
  if (this->RequiresCommunication && !alreadyCollectedAllInformation)
    {
    this->CollectInformation();
    }
  this->ConstructTree();
  this->HasBeenCalculated = true;
}


//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::CollectInformation(void)
{
  //
  // Extents are spread across all of the processors.  Since this->NodeExtents was
  // initialized to have value 0., we can do an MPI_SUM and get the correct
  // list on processor 0.
  //
#ifdef VTK_USE_MPI
  int totalElements = this->NumberOfElements*this->VectorSize;
  double *outBuff = new double[totalElements];
  MPI_Allreduce(this->NodeExtents, outBuff, totalElements, MPI_DOUBLE, MPI_SUM,
                *CMFEUtility::GetMPIComm());

  for (int i = 0 ; i < totalElements ; i++)
    {
    this->NodeExtents[i] = outBuff[i];
    }
  delete [] outBuff;
#endif
}


//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::ConstructTree(void)
{
  int    i, j;

  //
  // Make a local copy of the bounds so that we can move them around.
  //
  int totalSize = this->VectorSize+1;
  globalNDims = this->NumberOfDims;
  DoubleInt  *bounds = new DoubleInt[this->NumberOfElements*totalSize];
  for (i = 0 ; i < this->NumberOfElements ; i++)
    {
    for (j = 0 ; j < this->VectorSize ; j++)
      {
      bounds[totalSize*i+j].f = this->NodeExtents[this->VectorSize*i+j];
      }
    bounds[totalSize*i+(totalSize-1)].b = i;
    }

  int offsetStack[100];   // Only need log this->NumberOfElements
  int nodeStack  [100];   // Only need log this->NumberOfElements
  int sizeStack  [100];   // Only need log this->NumberOfElements
  int depthStack [100];   // Only need log this->NumberOfElements
  int stackCount = 0;

  //
  // Initialize the arguments for the stack
  //
  offsetStack[0]  = 0;
  sizeStack[0]  = this->NumberOfElements;
  depthStack[0]  = 0;
  nodeStack[0]  = 0;
  ++stackCount;

  int currentOffset;
  int currentSize; 
  int currentDepth;
  int leftSize;
  int currentNode;
  int count = 0;
  int thresh = (this->NumberOfElements > 10 ? this->NumberOfElements/10 : 1);
  while (stackCount > 0)
    {
    count++;
    --stackCount;
    currentOffset = offsetStack[stackCount];
    currentSize   = sizeStack  [stackCount];
    currentDepth  = depthStack [stackCount];
    currentNode   = nodeStack  [stackCount];

    if (currentSize <= 1)
      {
      this->NodeIDs[currentNode] =  bounds[currentOffset*totalSize + (totalSize-1)].b;
      for (int j = 0 ; j < this->VectorSize ; j++)
        {
        this->NodeExtents[currentNode*this->VectorSize + j] = bounds[currentOffset*totalSize + j].f;
        }
      continue;
      }

    globalCurrentDepth = currentDepth;
    if (count % thresh == 0)
      {
      qsort(bounds + currentOffset*totalSize, currentSize, totalSize*sizeof(DoubleInt), QsortBoundsSorter);
      }

    leftSize = this->SplitSize(currentSize);

    offsetStack[stackCount]  = currentOffset;
    sizeStack[stackCount]  = leftSize;
    depthStack[stackCount]  = currentDepth + 1;
    nodeStack[stackCount]  = 2*currentNode + 1;
    ++stackCount;

    offsetStack[stackCount]  = currentOffset + leftSize;
    sizeStack[stackCount]  = currentSize - leftSize;
    depthStack[stackCount]  = currentDepth + 1;
    nodeStack[stackCount]  = 2*currentNode + 2;
    ++stackCount;
    }

  this->SetIntervals();

  delete [] bounds;
}

//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::SetIntervals()
{
  //Now that the intervals for all of the leaf nodes have been set and put
  //in their proper place in the interval tree, calculate the bounds for
  //all of the parenting nodes.
  int parent;

  for (int i = this->NumberOfNodes-1 ; i > 0 ; i -= 2)
    {
    parent = (i - 2) / 2;
    for (int k = 0 ; k < this->NumberOfDims ; k++)
      {
      this->NodeExtents[parent*this->VectorSize + 2*k] =
        vtkstd::min(this->NodeExtents[(i-1)*this->VectorSize + 2*k],
        this->NodeExtents[i*this->VectorSize + 2*k]);
      this->NodeExtents[parent*this->VectorSize + 2*k + 1] =
        vtkstd::max(this->NodeExtents[(i-1)*this->VectorSize + 2*k + 1],
        this->NodeExtents[i*this->VectorSize + 2*k + 1]);
      }
    }
}

//----------------------------------------------------------------------------
int vtkCMFEIntervalTree::SplitSize(int size)
{
  //
  // Decompose size into 2^y + n where 0 <= n < 2^y
  //
  int power = 1;
  while (power*2 <= size)
    {
    power *= 2;
    }
  int n = size - power;

  if (n == 0)
    {
    return power/2;
    }
  if (n < power/2)
    {
    return (power/2 + n);
    }

  return power;
}


//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::GetElementsList(const double *params, double solution, std::vector<int> &list) const
{
  if (this->HasBeenCalculated == false)
    {
    return;
    }

  list.clear();

  int nodeStack[100]; // Only need log amount
  int nodeStackSize = 0;

  //
  // Populate the stack by putting on the root element.  This element contains
  // all the other elements in its extents.
  //
  nodeStack[0] = 0;
  nodeStackSize++;

  while (nodeStackSize > 0)
    {
    nodeStackSize--;
    int stackIndex = nodeStack[nodeStackSize];
    if ( CMFEUtility::Intersects(params, solution, stackIndex, this->NumberOfDims, this->NodeExtents) )
      {
      //
      // The equation has a solution contained by the current extents.
      //
      if (this->NodeIDs[stackIndex] < 0)
        {
        //
        // This is not a leaf, so put children on stack
        //
        nodeStack[nodeStackSize] = 2 * stackIndex + 1;
        nodeStackSize++;
        nodeStack[nodeStackSize] = 2 * stackIndex + 2;
        nodeStackSize++;
        }
      else
        {
        //
        // Leaf node, put in list
        //
        list.push_back(this->NodeIDs[stackIndex]);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::GetElementsList(double origin[3], double rayDir[3],
                                 std::vector<int> &list) const
{
  if (this->HasBeenCalculated == false)
    {   
    return;
    }

  list.clear();

  int nodeStack[100]; // Only need log amount
  int nodeStackSize = 0;

  // Populate the stack by putting on the root element.  This element contains
  // all the other elements in its extents.
  nodeStack[0] = 0;
  nodeStackSize++;
  while (nodeStackSize > 0)
    {
    nodeStackSize--;
    int stackIndex = nodeStack[nodeStackSize];
    if ( CMFEUtility::IntersectsRay(origin, rayDir, stackIndex, this->NumberOfDims, this->NodeExtents) )
      {
      // The equation has a solution contained by the current extents.
      if (this->NodeIDs[stackIndex] < 0)
        {
        // This is not a leaf, so put children on stack
        nodeStack[nodeStackSize] = 2 * stackIndex + 1;
        nodeStackSize++;
        nodeStack[nodeStackSize] = 2 * stackIndex + 2;
        nodeStackSize++;
        }
      else
        {
        // Leaf node, put in list
        list.push_back(this->NodeIDs[stackIndex]);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::GetElementsListFromRange(const double *min_vec, const double *max_vec, std::vector<int> &list) const
{
  if (this->HasBeenCalculated == false)
    {
    return;
    }

  list.clear();

  int nodeStack[100]; // Only need log amount
  int nodeStackSize = 0;

  //
  // Populate the stack by putting on the root element.  This element contains
  // all the other element in its extents.
  //
  nodeStack[0] = 0;
  nodeStackSize++;

  while (nodeStackSize > 0)
    {
    nodeStackSize--;
    int stackIndex = nodeStack[nodeStackSize];
    bool inBlock = true;
    for (int i = 0 ; i < this->NumberOfDims ; i++)
      {
      double min_extent = min_vec[i];
      double max_extent = max_vec[i];
      double min_node   = this->NodeExtents[stackIndex*this->NumberOfDims*2 + 2*i];
      double max_node   = this->NodeExtents[stackIndex*this->NumberOfDims*2 + 2*i+1];
      if (min_node > max_extent)
        {
        inBlock = false;
        }
      if (max_node < min_extent)
        {
        inBlock = false;
        }
      if (!inBlock)
        {
        break;
        }
      }
    if (inBlock)
      {
      //
      // The equation has a solution contained by the current extents.
      //
      if (this->NodeIDs[stackIndex] < 0)
        {
        //
        // This is not a leaf, so put children on stack
        //
        nodeStack[nodeStackSize] = 2 * stackIndex + 1;
        nodeStackSize++;
        nodeStack[nodeStackSize] = 2 * stackIndex + 2;
        nodeStackSize++;
        }
      else
        {
        //
        // Leaf node, put in list
        //
        list.push_back(this->NodeIDs[stackIndex]);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::GetElementsFromAxiallySymmetricLineIntersection( const double *P1, const double *D1, std::vector<int> &list) const
{
  if (this->HasBeenCalculated == false)
    {
    return;
    }

  if (this->NumberOfDims != 2)
    {
    return;
    }

  list.clear();

  int nodeStack[100]; // Only need log amount
  int nodeStackSize = 0;

  //
  // Populate the stack by putting on the root domain.  This domain contains
  // all the other domains in its extents.
  //
  nodeStack[0] = 0;
  nodeStackSize++;

  while (nodeStackSize > 0)
    {
    nodeStackSize--;
    int stackIndex = nodeStack[nodeStackSize];
    if (CMFEUtility::AxiallySymmetricLineIntersection(P1, D1, stackIndex, this->NodeExtents))
      {
      //
      // The equation has a solution contained by the current extents.
      //
      if (this->NodeIDs[stackIndex] < 0)
        {
        //
        // This is not a leaf, so put children on stack
        //
        nodeStack[nodeStackSize] = 2 * stackIndex + 1;
        nodeStackSize++;
        nodeStack[nodeStackSize] = 2 * stackIndex + 2;
        nodeStackSize++;
        }
      else
        {
        //
        // Leaf node, put in list
        //
        list.push_back(this->NodeIDs[stackIndex]);
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkCMFEIntervalTree::GetLeafExtents(int leafIndex, double *extents) const
{  
  //The input integer is _not_ the element number.  It is the leaf number. 
  //This is done to prevent having to search the NodeIDs list for each element.  
  //This routine is only useful when iterating over all elements.
  int nodeIndex = this->NumberOfNodes-this->NumberOfElements + leafIndex;
  for (int i = 0 ; i < this->VectorSize ; i++)
    {
    extents[i] = this->NodeExtents[this->VectorSize*nodeIndex + i];
    }

  return  this->NodeIDs[nodeIndex];
}

//----------------------------------------------------------------------------
void vtkCMFEIntervalTree::GetElementExtents(int elementIndex, double *extents) const
{
  int startIndex = this->NumberOfNodes-this->NumberOfElements;
  for (int i = startIndex ; i < this->NumberOfNodes ; i++)
    {
    if (this->NodeIDs[i] == elementIndex)
      {
      for (int j = 0 ; j < this->VectorSize ; j++)
        {
        extents[j] = this->NodeExtents[this->VectorSize*i + j];
        }
      return;
      }
    }
}

