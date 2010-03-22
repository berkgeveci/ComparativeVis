/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCMFEFilter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkCMFEFilter_h
#define __vtkCMFEFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkMultiProcessController.h"

class VTK_EXPORT vtkCMFEFilter : public vtkDataSetAlgorithm
{
public:
  static vtkCMFEFilter* New();
  vtkTypeRevisionMacro(vtkCMFEFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the source connection object
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

protected:
  vtkCMFEFilter();
  ~vtkCMFEFilter();

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkMultiProcessController* Controller;  

private:
  vtkCMFEFilter(const vtkCMFEFilter&);  // Not implemented.
  void operator=(const vtkCMFEFilter&);  // Not implemented.
};

#endif
