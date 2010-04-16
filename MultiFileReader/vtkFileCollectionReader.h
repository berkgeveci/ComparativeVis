/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkFileCollectionReader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFileCollectionReader - 

#ifndef __vtkFileCollectionReader_h
#define __vtkFileCollectionReader_h

#include "vtkDataObjectAlgorithm.h"

//#define USE_XML_READERS

#ifndef USE_XML_READERS
class vtkGenericDataObjectReader;
#endif

class vtkTable;

#ifdef USE_XML_READERS
class vtkXMLGenericDataObjectReader;
#endif

class VTK_EXPORT vtkFileCollectionReader : public vtkDataObjectAlgorithm
{
public:
  static vtkFileCollectionReader *New();
  vtkTypeRevisionMacro(vtkFileCollectionReader,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetTableFromString(const char* string);
  
  vtkGetObjectMacro(Table, vtkTable);

  vtkSetStringMacro(DirectoryPath);
  vtkGetStringMacro(DirectoryPath);
  
  vtkSetStringMacro(FileNameColumn);
  vtkGetStringMacro(FileNameColumn);

  vtkSetMacro(RowIndex, unsigned int);
  vtkGetMacro(RowIndex, unsigned int);
  
  int GetNumberOfRows();
  
protected:
  vtkFileCollectionReader();
  ~vtkFileCollectionReader();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  virtual int RequestInformation(vtkInformation* request, 
                                 vtkInformationVector** inputVector, 
                                 vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation* request, 
                                  vtkInformationVector** inputVector, 
                                  vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);


private:
  vtkFileCollectionReader(const vtkFileCollectionReader&);  // Not implemented.
  void operator=(const vtkFileCollectionReader&);  // Not implemented.

  int SetReaderFileName();

  vtkTable* Table;
//BTX  
#ifdef USE_XML_READERS
  vtkXMLGenericDataObjectReader *Reader;
#else
  vtkGenericDataObjectReader* Reader;
#endif
//ETX  
  char *DirectoryPath;
  char* FileNameColumn;
  unsigned int RowIndex;
};

#endif
