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
#include "vtkXMLGenericDataObjectReader.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMyDelimitedTextReader.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkFileCollectionReader, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkFileCollectionReader);

vtkFileCollectionReader::vtkFileCollectionReader()
{
#ifdef USE_XML_READERS
  this->Reader = vtkXMLGenericDataObjectReader::New();
#else
  this->Reader = vtkGenericDataObjectReader::New();
#endif
  this->Table = vtkTable::New();
  
  this->DirectoryPath=0;
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
  this->SetDirectoryPath(0);
  this->SetFileNameColumn(0);
}

void vtkFileCollectionReader::SetTableFromString(const char* string)
{
  this->Table->Initialize();

  vtkSmartPointer<vtkMyDelimitedTextReader> reader = 
    vtkSmartPointer<vtkMyDelimitedTextReader>::New();
  reader->SetInputString(string);
  reader->SetHaveHeaders(true);
  reader->Update();

  this->Table->ShallowCopy(reader->GetOutput());
  
  this->Modified();
}

int vtkFileCollectionReader::GetNumberOfRows()
{
  return this->Table->GetNumberOfRows();
}

int vtkFileCollectionReader::SetReaderFileName()
{
  if(this->DirectoryPath==0)
    {
    vtkDebugMacro("The DirectoryPath was not specified.");
    return 0;
    }
  
  if (!this->FileNameColumn)
    {
    vtkDebugMacro("The FileNameColumn was not specified.");
    return 0;
    }

  vtkDataSetAttributes* rowData = this->Table->GetRowData();

  vtkStringArray* fileNames = vtkStringArray::SafeDownCast(
    rowData->GetAbstractArray(this->FileNameColumn));
  if (!fileNames)
    {
    vtkDebugMacro("There is no file name column");
    return 0;
    }
  
  if (this->RowIndex >= fileNames->GetNumberOfValues())
    {
    vtkDebugMacro("RowIndex is too large. There are only " 
      << fileNames->GetNumberOfValues() << " columns.");
    return 0;
    }
  
  vtkStdString& fname = fileNames->GetValue(this->RowIndex);
  
  vtksys_ios::ostringstream ost;
  ost << this->DirectoryPath << "/" << fname;
  vtkStdString fullname=ost.str();
  
  this->Reader->SetFileName(fullname.c_str());
  
  return 1;
}

int vtkFileCollectionReader::RequestDataObject(
  vtkInformation* request,
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  if (!this->SetReaderFileName())
    {
    // Create a dummy output instead of reporting errors.
    // This is because we know that ParaView will call this
    // function before the filename is set.
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkSmartPointer<vtkDataObject> dobj = 
      vtkSmartPointer<vtkDataObject>::New();
    dobj->SetPipelineInformation(info);
    return 1;
    }

  return this->Reader->ProcessRequest(request, inputVector, outputVector);
}

int vtkFileCollectionReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->Reader->GetFileName())
    {
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
    }
  return 1;
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
