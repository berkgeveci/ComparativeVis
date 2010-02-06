/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkFileCollectionReader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFileCollectionReader.h"

#include "vtkDataObject.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMyDelimitedTextReader.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkFileCollectionReader, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkFileCollectionReader);

vtkFileCollectionReader::vtkFileCollectionReader()
{
  this->Reader = vtkGenericDataObjectReader::New();
  this->Table = vtkTable::New();
  
  this->FileNameColumn = 0;
  
  this->SetNumberOfInputPorts(0);
  
  this->RowIndex = 0;
}

vtkFileCollectionReader::~vtkFileCollectionReader()
{
  if (this->Reader)
    {
    this->Reader->Delete();
    }
  if (this->Table)
    {
    this->Table->Delete();
    }
  this->SetFileNameColumn(0);
}

void vtkFileCollectionReader::SetTableFromString(const char* string)
{
  this->Table->Initialize();

  vtkMyDelimitedTextReader* reader = vtkMyDelimitedTextReader::New();
  reader->SetInputString(string);
  reader->SetHaveHeaders(true);
  reader->Update();

  this->Table->ShallowCopy(reader->GetOutput());
}

int vtkFileCollectionReader::GetNumberOfRows()
{
  return this->Table->GetNumberOfRows();
}

int vtkFileCollectionReader::SetReaderFileName()
{
  if (!this->FileNameColumn)
    {
    vtkErrorMacro("The FileNameColumn was not specified.");
    return 0;
    }

  vtkDataSetAttributes* rowData = this->Table->GetRowData();

  vtkStringArray* fileNames = vtkStringArray::SafeDownCast(
    rowData->GetAbstractArray(this->FileNameColumn));
  if (!fileNames)
    {
    vtkErrorMacro("There is no file name column");
    return 0;
    }
  
  if (this->RowIndex >= fileNames->GetNumberOfValues())
    {
    vtkErrorMacro("RowIndex is too large. There are only " 
      << fileNames->GetNumberOfValues() << " columns.");
    return 0;
    }
  
  vtkStdString& fname = fileNames->GetValue(this->RowIndex);
  this->Reader->SetFileName(fname.c_str());
  
  return 1;
}

int vtkFileCollectionReader::RequestDataObject(
  vtkInformation* request,
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  if (!this->SetReaderFileName())
    {
    return 0;
    }

  return this->Reader->ProcessRequest(request, inputVector, outputVector);
}

int vtkFileCollectionReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  return this->Reader->ProcessRequest(request, inputVector, outputVector);
}

int vtkFileCollectionReader::RequestUpdateExtent(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  return this->Reader->ProcessRequest(request, inputVector, outputVector);
}

int vtkFileCollectionReader::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  return this->Reader->ProcessRequest(request, inputVector, outputVector);
}

int vtkFileCollectionReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

void vtkFileCollectionReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
