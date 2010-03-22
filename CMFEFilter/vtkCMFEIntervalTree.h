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
// Keeps meta-data for fields.  The meta-data is stored as a tree and each
// node covers the range of its children.  Each leaf node contains a range
// that corresponds to the extents of a specific domain.  The range is of
// spacial or data extents.


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

#ifndef __vtkCMFEIntervalTree_h
#define __vtkCMFEIntervalTree_h

#include <vtkstd/vector>

class vtkCMFEIntervalTree
{
public:
  vtkCMFEIntervalTree(int, int, bool = true);
  vtkCMFEIntervalTree(const vtkCMFEIntervalTree *);
  
  virtual ~vtkCMFEIntervalTree();

  // Description: 
  // calls the destructor of the interval tree passed in
  static void Destruct(void *tree);

  // Description:
  //Sees what the range of values the variable contained by the interval
  //tree has. This is all contained in the first node.
  void GetExtents(double *) const;

  // Description:
  //Adds the extents for one element.
  void AddElement(int element, double *d);

    // Description:
  //Calculates the interval tree.  This means collecting the information
  //from other processors (if we are in parallel) and constructing the tree.  
  void Calculate(bool alreadyCollectedAllInformation = false);

  // Description:
  //Takes in a linear equation and determines which elements have values that satisfy the equation.
  //The equation is of the form:  params[0]*x + params[1]*y ... = solution
  void GetElementsList(const double *params, double solution, std::vector<int> &list) const;

  // Description:
  //Takes in a ray origin and direction, and determines which elements are intersected by the ray.   
  void GetElementsList(double [3], double[3], vtkstd::vector<int> &) const;

  // Description:
  //Takes in a linear equation and determines which elements have values that satisfy the equation.
  //The equation is of the form:  params[0]*x + params[1]*y ... = solution
  void GetElementsListFromRange(const double *min_vec, const double *max_vec, std::vector<int> &list) const;

  // Description:
  //This test will only work with 2D items and  intersections with a 3D line.  The idea is that the terms are
  //revolved into 3D and we need to figure out which items intersect with a line when revolved into 3D.
  void GetElementsFromAxiallySymmetricLineIntersection(const double *P1, const double *D1, std::vector<int> &list) const;
  
  //Description:
  //Gets the extents for a leaf node.
  //Pass in the leaf number, not the element number
  int GetLeafExtents(int leafIndex, double *extents) const;
  
  //Description:
  //Get the exetents of an Element
  void GetElementExtents(int elementIndex, double *extents) const;

  //Description:
  //Get the Dimensions
  int GetDimensions(void) const { return this->NumberOfDims; };
  
  //Description:
  //Get the Number of leaves
  int GetNumberLeaves(void) const { return this->NumberOfElements; };

protected:
  int NumberOfElements;
  int NumberOfNodes;
  int NumberOfDims;
  int VectorSize;

  double *NodeExtents;
  int *NodeIDs;

  bool HasBeenCalculated;
  bool RequiresCommunication;

  void CollectInformation(void);
  void ConstructTree(void);
  void SetIntervals(void);
  int  SplitSize(int);

private:
  vtkCMFEIntervalTree(const vtkCMFEIntervalTree &) {;};
  vtkCMFEIntervalTree &operator=(const vtkCMFEIntervalTree &) { return *this; };
};


#endif

  
