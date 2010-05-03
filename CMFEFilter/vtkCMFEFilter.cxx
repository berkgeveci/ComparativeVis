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


#include "vtkCMFEFilter.h"

#include "vtkCMFEAlgorithm.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <sstream>
vtkStandardNewMacro(vtkCMFEFilter);

//----------------------------------------------------------------------------
vtkCMFEFilter::vtkCMFEFilter( )
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkCMFEFilter::~vtkCMFEFilter()
{


}

//----------------------------------------------------------------------------
void vtkCMFEFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
int vtkCMFEFilter::RequestInformation(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);



  return 1;
}

//----------------------------------------------------------------------------
int vtkCMFEFilter::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  if (!source || !input )
    {
    vtkErrorMacro("Source or Input was not found");
    return 0;
    }

  //get the input arrays to mesh
  vtkDataArray *inputProp = this->GetInputArrayToProcess(0,
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataArray *sourceProp = this->GetInputArrayToProcess(1,
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));



  if ( !inputProp || !sourceProp )
    {
    vtkErrorMacro("Unable to find a property the user selected");
    return 0;
    }

  vtkStdString outputName = inputProp->GetName();
  if ( outputName == sourceProp->GetName() )
    {
    outputName +="Result";
    }

  vtkDataSet *temp = vtkCMFEAlgorithm::PerformCMFE( source, input , sourceProp->GetName(),
    inputProp->GetName(), outputName );

  output->ShallowCopy( temp );
  temp->Delete();

  return 1;

}

//----------------------------------------------------------------------------
void vtkCMFEFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
