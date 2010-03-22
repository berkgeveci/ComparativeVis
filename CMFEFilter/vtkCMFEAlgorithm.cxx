/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCMFEAlgorithm.cxx,v $

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
#include <vtkCMFEAlgorithm.h>

#include <vtkCMFEUtility.h>
#include <vtkCMFEDesiredPoints.h>
#include <vtkCMFEFastLookupGrouping.h>
#include <vtkCMFESpatialPartition.h>

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkMultiProcessController.h>
#include <vtkMPICommunicator.h>

#include <float.h>
#include <vtkstd/algorithm>

//----------------------------------------------------------------------------
vtkDataSet* vtkCMFEAlgorithm::PerformCMFE(vtkDataSet *output_mesh, vtkDataSet *mesh_to_be_sampled,
  const std::string &output_var, const std::string &mesh_var,  const std::string &outvar)
{
  int pointProperty;
  int numberOfComponents;

  if ( mesh_to_be_sampled->GetPointData()->HasArray( mesh_var.c_str() ) )
    {
    pointProperty = 1;
    numberOfComponents = mesh_to_be_sampled->GetPointData()->GetArray( mesh_var.c_str() )->GetNumberOfComponents();
    }
  else
    {
    pointProperty = 0;
    numberOfComponents =  mesh_to_be_sampled->GetCellData()->GetArray( mesh_var.c_str() )->GetNumberOfComponents();
    }

  //make sure we have the updated values on all processors
  pointProperty = CMFEUtility::UnifyMaximumValue(pointProperty);
  numberOfComponents = CMFEUtility::UnifyMaximumValue(numberOfComponents);

  bool isNodal = (pointProperty==1);
      
  // Set up the data structure so that we can locate sample points in the
  // mesh to be sampled quickly.    
  vtkCMFEFastLookupGrouping flg(mesh_var, isNodal);
  flg.AddMesh( mesh_to_be_sampled );

  // Set up the data structure that keeps track of the sample points we need.
  vtkCMFEDesiredPoints dp(isNodal, numberOfComponents);
  dp.AddDataset( output_mesh );

  double outBounds[6];
  double meshBounds[6];
  double bounds[6];
  output_mesh->GetBounds( outBounds );
  mesh_to_be_sampled->GetBounds( meshBounds );

  for(int i=0; i < 6; i+=2)
    {
    bounds[i] = vtkstd::min(outBounds[i],meshBounds[i]);
    bounds[i+1] = vtkstd::max(outBounds[i+1],meshBounds[i+1]);
    }
  CMFEUtility::UnifyMinMax(bounds,6);

  // Need to "finalize" in pre-partitioned form so that the spatial
  // partitioner can access their data.
  dp.Finalize();

  //
  // There is no guarantee that the "dp" and "flg" overlap spatially.  It's
  // likely that the parts of the "flg" mesh that the points in "dp" are
  // interested in are located on different processors.  So we do a large
  // communication phase to get all of the points on the right processors.
  //    
  vtkCMFESpatialPartition spat_part;    
  spat_part.CreatePartition(&dp, &flg, bounds);
  dp.RelocatePointsUsingPartition(&spat_part);
  flg.RelocateDataUsingPartition(&spat_part);

  flg.Finalize();
  dp.Finalize();

  //
  // Now, for each sample, locate the sample point in the mesh to be sampled
  // and evaluate that point.
  //    
  int npts = dp.GetNumberOfPoints();
  float *comps = new float[numberOfComponents];
  for (int i = 0 ; i < npts ; i++)
    {
    float pt[3];
    dp.GetPoint(i, pt);
    bool gotValue = flg.GetValue(pt, comps);
    if (!gotValue)
      {
      comps[0] = FLT_MAX;
      }
    dp.SetValue(i, comps);
    }
  delete [] comps;    
  
  // We had to distribute the "dp" and "flg" structures across all 
  // processors (see comments in sections above).  So now we need to
  // get the correct values back to this processor so that we can set
  // up the output variable array.  
  dp.UnRelocatePoints(&spat_part);    


  // Now create the variable that contains all of the values for the sample
  // points we evaluated.

  //find the property on the output, can't trust isNodal
  //since it was on the sampled mesh, not the output mesh    
  vtkDataArray *outProp = output_mesh->GetPointData()->GetArray( output_var.c_str() );
  if ( !outProp)
    {
    outProp = output_mesh->GetCellData()->GetArray( output_var.c_str() );
    }

  vtkFloatArray *resultArray = vtkFloatArray::New();    
  resultArray->SetName( outvar.c_str() );
  resultArray->SetNumberOfComponents( numberOfComponents );
  int numValues = (isNodal) ? output_mesh->GetNumberOfPoints() : output_mesh->GetNumberOfCells();
  resultArray->SetNumberOfTuples( numValues );

  //copy over all the updated values from the desired points
  //FLT_MAX signifies that dp doesn't have an updated value for the output
  //GetValue supports multiple files, by 
  int meshIndex = 0;
  for (int i = 0 ; i < numValues ; ++i)
    {
    const float *val = dp.GetValue(meshIndex, i);
    if (*val != FLT_MAX)
      {
      resultArray->SetTuple(i, dp.GetValue(meshIndex, i));
      }
    else
      {
      resultArray->SetTuple(i, outProp->GetTuple(i));
      }
    }

  vtkDataSet *output = output_mesh->NewInstance();
  output->ShallowCopy( output_mesh );  

  (isNodal)? output->GetPointData()->AddArray( resultArray ) : output->GetCellData()->AddArray( resultArray );
  resultArray->Delete();
  return output;
}



