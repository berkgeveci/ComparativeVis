/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMyDelimitedTextReader.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkMyDelimitedTextReader.h"

#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkstd/string>

#include <vtksys/ios/sstream>

#include <ctype.h>

vtkCxxRevisionMacro(vtkMyDelimitedTextReader, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkMyDelimitedTextReader);

struct vtkMyDelimitedTextReaderInternals
{
  ifstream* File;
  vtksys_ios::istringstream* StringStream;
  istream* IS;
};

namespace
{
int 
splitString(const vtkStdString& input, 
            const char *fieldDelimiters,
            char stringDelimiter,
            bool useStringDelimiter,
            bool mergeConsecutiveDelimiters,
            vtkstd::vector<vtkStdString>& results, 
            bool includeEmpties=true)
{
  if (input.size() == 0)
    {
    return 0;
    }

  bool inString = false;
  char thisCharacter = 0;
  char lastCharacter = 0;

  vtkstd::string currentField;

  for (unsigned int i = 0; i < input.size(); ++i)
    {
    thisCharacter = input[i];

    // Zeroth: are we in an escape sequence? If so, interpret this
    // character accordingly.
    if (lastCharacter == '\\')
      {
      char characterToAppend;
      switch (thisCharacter)
        {
        case '0': characterToAppend = '\0'; break;
        case 'a': characterToAppend = '\a'; break;
        case 'b': characterToAppend = '\b'; break;
        case 't': characterToAppend = '\t'; break;
        case 'n': characterToAppend = '\n'; break;
        case 'v': characterToAppend = '\v'; break;
        case 'f': characterToAppend = '\f'; break;
        case 'r': characterToAppend = '\r'; break;
        case '\\': characterToAppend = '\\'; break;
        default:  characterToAppend = thisCharacter; break;
        }

      currentField += characterToAppend;
      lastCharacter = thisCharacter;
      if (lastCharacter == '\\') lastCharacter = 0;
      }
    else 
      {
      // We're not in an escape sequence.

      // First, are we /starting/ an escape sequence?
      if (thisCharacter == '\\')
        {
        lastCharacter = thisCharacter;
        continue;
        }
      else if (useStringDelimiter && thisCharacter == stringDelimiter)
        {
        // this should just toggle inString
        inString = (inString == false);
        }
      else if (!inString && (strchr(fieldDelimiters, thisCharacter) != NULL))
        {
        if (mergeConsecutiveDelimiters && 
            (strchr(fieldDelimiters, lastCharacter) != NULL))
          {
          continue; // We're in the middle of a string of delimiters.
          }

        // A delimiter starts a new field unless we're in a string, in
        // which case it's normal text and we won't even get here.
        if (includeEmpties || currentField.size() > 0)
          {
          results.push_back(currentField);
          }
        currentField = vtkStdString();
        }
      else
        {
        // The character is just plain text.  Accumulate it and move on.
        currentField += thisCharacter;
        }
      
      lastCharacter = thisCharacter;
      }
    }

  results.push_back(currentField);
  return static_cast<int>(results.size());
}

// ----------------------------------------------------------------------
// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
int my_getline(istream& in, vtkStdString &out, int& line_count)
{
  out = vtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;
  ++line_count;
  
  while ((nextValue = in.get()) != EOF &&
         numCharactersRead < out.max_size())
    {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != '\n' && downcast != '\r' && downcast != 0x0d)
      {
      out += downcast;
      }
    else
      {
      if (downcast == '\r' && in.peek() == '\n')
        {
        in.get();
        }
      return numCharactersRead;
      }
    }

  return numCharactersRead;
}

// Standard isspace asserts by testing if the characters is < 256
// This doesnt work so well with unicode.
int my_isspace(int space)
{
  int count = 0;
  switch(space)
    {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      count++;
      break;
    default:
      break;
    }
  return count;
}

// Returns true if the line is entirely whitespace, false otherwise.
bool isSpaceOnlyString(const vtkStdString& s)
{
  vtkStdString::size_type i;
  const char* c = s.c_str();
  for (i = 0; i < s.length(); ++i)
    {
    if(!my_isspace(c[i]))
      {
      return false;
      }
    }
  return true;
}

} // end of anonymous namespace

// ----------------------------------------------------------------------
vtkMyDelimitedTextReader::vtkMyDelimitedTextReader()
{
  this->Internals = new vtkMyDelimitedTextReaderInternals();

  this->Internals->File = 0;
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->ReadBuffer = new char[2048];
  this->HaveHeaders = false;
  this->FieldDelimiterCharacters = NULL;
  this->SetFieldDelimiterCharacters(",");
  this->StringDelimiter = '"';
  this->UseStringDelimiter = true;
  this->MaxRecords = 0;
  this->MergeConsecutiveDelimiters = false;
  this->DetectNumericColumns = false;
  this->PedigreeIdArrayName = NULL;
  this->SetPedigreeIdArrayName("id");
  this->GeneratePedigreeIds = true;
  this->OutputPedigreeIds = false;

  this->InputString = 0;
}

// ----------------------------------------------------------------------
vtkMyDelimitedTextReader::~vtkMyDelimitedTextReader()
{
  this->DeleteStream();
  
  this->SetFileName(0);
  this->SetFieldDelimiterCharacters(NULL);
  this->SetPedigreeIdArrayName(0);

  delete [] this->ReadBuffer;
  delete this->Internals;
  
  this->SetInputString(0);
}

// ----------------------------------------------------------------------
void vtkMyDelimitedTextReader::DeleteStream()
{
  if (this->Internals->File)
    {
    delete this->Internals->File;
    this->Internals->File = 0;
    }
  if (this->Internals->StringStream)
    {
    delete this->Internals->StringStream;
    this->Internals->StringStream = 0;
    }
  this->Internals->IS = 0;

}

// ----------------------------------------------------------------------
void vtkMyDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Field delimiters: '" << this->FieldDelimiterCharacters
     << "'" << endl;
  os << indent << "String delimiter: '" << this->StringDelimiter
     << "'" << endl;
  os << indent << "UseStringDelimiter: " 
     << (this->UseStringDelimiter ? "true" : "false") << endl;
  os << indent << "HaveHeaders: " 
     << (this->HaveHeaders ? "true" : "false") << endl;
  os << indent << "MergeConsecutiveDelimiters: " 
     << (this->MergeConsecutiveDelimiters ? "true" : "false") << endl;
  os << indent << "MaxRecords: " << this->MaxRecords
     << endl;
  os << indent << "DetectNumericColumns: "
    << (this->DetectNumericColumns? "true" : "false") << endl;
  os << indent << "GeneratePedigreeIds: " 
    << this->GeneratePedigreeIds << endl;
  os << indent << "PedigreeIdArrayName: " 
    << this->PedigreeIdArrayName << endl;
  os << indent << "OutputPedigreeIds: "
    << (this->OutputPedigreeIds? "true" : "false") << endl;
}

// ----------------------------------------------------------------------
bool vtkMyDelimitedTextReader::OpenStream()
{
  this->DeleteStream();
  
  if (this->InputString)
    {
    vtkstd::string str(this->InputString);
    this->Internals->StringStream = new vtksys_ios::istringstream(str);
    this->Internals->IS = this->Internals->StringStream;
    }
  else
    {
    // Open the new file.
    vtkDebugMacro(<< "vtkMyDelimitedTextReader is opening file: " << this->FileName);
    this->Internals->File = new ifstream(this->FileName, ios::in | ios::binary);

    // Check to see if open was successful
    if (! this->Internals->File || this->Internals->File->fail())
      {
      vtkErrorMacro(<< "vtkMyDelimitedTextReader could not open file " 
                    << this->FileName);
      this->DeleteStream();
      return false;
      }
    this->Internals->IS = this->Internals->File;
    }
  return true;
}

// ----------------------------------------------------------------------
int vtkMyDelimitedTextReader::RequestData(
                                        vtkInformation*, 
                                        vtkInformationVector**, 
                                        vtkInformationVector* outputVector)
{
  // Check piece request. If requested anything but the 0-th piece, nothing to
  // read.
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  // Open the stream
  if (!this->OpenStream())
    {
    return 1;
    }

  if(this->OutputPedigreeIds && !this->PedigreeIdArrayName)
    {
    vtkErrorMacro(<< "You must specify a pedigree id array name.");
    this->DeleteStream();
    return 0;
    }

  int line_count = 0;

  size_t total_bytes = 0;
  if (this->Internals->File)
    {
    // Get the total size of the file ...
    this->Internals->File->seekg(0, ios::end);
    total_bytes = this->Internals->File->tellg();
    // Go to the top of the file
    this->Internals->File->seekg(0, ios::beg);
    }
  
  // Store the text data into a vtkTable
  vtkTable* table = vtkTable::GetData(outputVector);
  
  // The first line of the file might contain the headers, so we want
  // to be a little bit careful about it.  If we don't have headers
  // we'll have to make something up.
  vtkstd::vector<vtkStdString> headers;


  // Not all platforms support vtkstd::getline(istream&, vtkstd::string) so
  // I have to do this the clunky way.

  vtkstd::vector<vtkStdString> firstLineFields;
  vtkStdString firstLine;

  int status = 0;
  do
    {
    status = my_getline(*(this->Internals->IS), firstLine, line_count);
    } while (isSpaceOnlyString(firstLine) && status);
  // No data in the file, so we are done.
  if (!status)
    {
    this->DeleteStream();
    return 1;
    }

  vtkDebugMacro(<<"First line of file: " << firstLine.c_str());
   
  if (this->HaveHeaders)
    {
    splitString(firstLine,
                this->FieldDelimiterCharacters,
                this->StringDelimiter,
                this->UseStringDelimiter,
                this->MergeConsecutiveDelimiters,
                headers);
    }
  else
    {
    splitString(firstLine,
                this->FieldDelimiterCharacters,
                this->StringDelimiter,
                this->UseStringDelimiter,
                this->MergeConsecutiveDelimiters,
                firstLineFields);

    for (unsigned int i = 0; i < firstLineFields.size(); ++i)
      {
      // I know it's not a great idea to use sprintf.  It's safe right
      // here because an unsigned int will never take up enough
      // characters to fill up this buffer.
      char fieldName[64];
      sprintf(fieldName, "Field %d", i);
      headers.push_back(fieldName);
      }
    }

  // Now we can create the arrays that will hold the data for each
  // field.
  vtkstd::vector<vtkStdString>::const_iterator fieldIter;
  for(fieldIter = headers.begin(); fieldIter != headers.end(); ++fieldIter)
    { 
    vtkStringArray* array = vtkStringArray::New();
    array->SetName((*fieldIter).c_str());
    table->AddColumn(array);
    array->Delete();
    }
  
  // If the first line did not contain headers then we need to add it
  // to the table.
  if (!this->HaveHeaders)
    {
    vtkVariantArray* dataArray = vtkVariantArray::New();
    vtkstd::vector<vtkStdString>::const_iterator I;
    for(I = firstLineFields.begin(); I != firstLineFields.end(); ++I)
      {
      dataArray->InsertNextValue(vtkVariant(*I));
      }
    
    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
    }

  // Okay read the file and add the data to the table
  vtkStdString nextLine;

  while (my_getline(*(this->Internals->IS), nextLine, line_count))
    {
    if(isSpaceOnlyString(nextLine))
      {
      continue;
      }

    if(this->MaxRecords && line_count > this->MaxRecords)
      {
      break;
      }
    
    if (this->Internals->File)
      {
      double progress = total_bytes
        ? static_cast<double>(this->Internals->File->tellg()) / static_cast<double>(total_bytes)
        : 0.5;
      
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }

    vtkDebugMacro(<<"Next line: " << nextLine.c_str());
    vtkstd::vector<vtkStdString> dataVector;

    // Split string on the delimiters
    splitString(nextLine,
                this->FieldDelimiterCharacters,
                this->StringDelimiter,
                this->UseStringDelimiter,
                this->MergeConsecutiveDelimiters,
                dataVector);
    
    vtkDebugMacro(<<"Split into " << dataVector.size() << " fields");
    // Add data to the output arrays

    // Convert from vector to variant array
    vtkVariantArray* dataArray = vtkVariantArray::New();
    vtkstd::vector<vtkStdString>::const_iterator I;
    for(I = dataVector.begin(); I != dataVector.end(); ++I)
      {
      dataArray->InsertNextValue(vtkVariant(*I));
      }
    
    // Pad out any missing columns
    while (dataArray->GetNumberOfTuples() < table->GetNumberOfColumns())
      {
      dataArray->InsertNextValue(vtkVariant());
      }

    // Eliminate any extra columns
    dataArray->SetNumberOfTuples(table->GetNumberOfColumns());

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
    }

  if(this->OutputPedigreeIds)
    {
    if (this->GeneratePedigreeIds)
      {
      vtkSmartPointer<vtkIdTypeArray> pedigreeIds =
        vtkSmartPointer<vtkIdTypeArray>::New();
      vtkIdType numRows = table->GetNumberOfRows();
      pedigreeIds->SetNumberOfTuples(numRows);
      pedigreeIds->SetName(this->PedigreeIdArrayName);
      for (vtkIdType i = 0; i < numRows; ++i)
        {
        pedigreeIds->InsertValue(i, i);
        }
      table->GetRowData()->SetPedigreeIds(pedigreeIds);
      }
    else
      {
      vtkAbstractArray* arr =
        table->GetColumnByName(this->PedigreeIdArrayName);
      if (arr)
        {
        table->GetRowData()->SetPedigreeIds(arr);
        }
      else
        {
        vtkErrorMacro(<< "Could find pedigree id array: "
          << this->PedigreeIdArrayName);
        this->DeleteStream();
        return 0;
        }
      }
    }

  if (this->DetectNumericColumns)
    {
    vtkStringToNumeric* convertor = vtkStringToNumeric::New();
    vtkTable* clone = table->NewInstance();
    clone->ShallowCopy(table);
    convertor->SetInput(clone);
    convertor->Update();
    clone->Delete();
    table->ShallowCopy(convertor->GetOutputDataObject(0));
    convertor->Delete();
    }

  this->DeleteStream();
  return 1;
}

