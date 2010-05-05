#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDescriptiveStatistics.h"
#include <cassert>
#include "vtkSmartPointer.h"
#include "vtkRectilinearGrid.h"
#include "vtkPointData.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkFileCollectionReader.h"
#include "vtkDelimitedTextWriter.h"
#include "vtkDatabaseConnection.h"
#include "vtkMetadataBrowser.h"
#include "vtkLookupTable.h"
#include "vtkScalarBarWidget.h"
#include "vtkScalarBarActor.h"
#include "vtkContourFilter.h"
#include "vtkProperty.h"
#include "vtkXMLRectilinearGridWriter.h"

double KelvinToCelsius(double kelvin)
{
  return kelvin-273.15;
}

int main(int argc,
         char *argv[])
{
  int result=0;
  
  // db:
  // /home/fbertel/projects/development/data/compvis/2010-04-28-temperature-subset/new-temperature-subset/temperature-subset.db
  
  // Temperature" table from db ?
  
  vtkSmartPointer<vtkDatabaseConnection> connection=
    vtkSmartPointer<vtkDatabaseConnection>::New();
  connection->UseSQLite();
  connection->SetFileName("/home/fbertel/projects/development/data/compvis/2010-04-28-temperature-subset/new-temperature-subset/temperature-subset.db");
  
  //test connection
  if(connection->ConnectToDatabase())
    {
    cout << "Connection successful!" << endl;
    }
  else
    {
    cout << "Connection failed!" << endl;
    return 1;
    }
  
  vtkSmartPointer<vtkMetadataBrowser> browser=
    vtkSmartPointer<vtkMetadataBrowser>::New();
  browser->SetDatabaseConnection(connection);
  std::string experimentName="Temperature";
  std::string query = "SELECT * FROM "+experimentName;
  vtkSmartPointer<vtkTable> table=
    browser->GetDataFromExperiment(experimentName, query);
  
  
  vtkSmartPointer<vtkDelimitedTextWriter> csvWriter = 
    vtkSmartPointer<vtkDelimitedTextWriter>::New();
  csvWriter->SetInput(table); // a vtkTable
  csvWriter->WriteToOutputStringOn();
  csvWriter->Write();
  char *tableStr=csvWriter->RegisterAndGetOutputString();
  
  vtkSmartPointer<vtkFileCollectionReader> reader=
    vtkSmartPointer<vtkFileCollectionReader>::New();
  reader->SetTableFromString(tableStr);
  reader->SetRowIndex(0); // read the first file.
  reader->SetDirectoryPath("/home/fbertel/projects/development/data/compvis/2010-04-28-temperature-subset/new-temperature-subset/");
  reader->SetFileNameColumn("Handle"); // specific to the db.
  
  
  cout << "update reader." << endl;
  reader->Update(); // read the first dataset.
  cout << "reader updated." << endl;
  vtkRectilinearGrid *g=static_cast<vtkRectilinearGrid *>(reader->GetOutput());
  
  int dims[3];
  g->GetDimensions(dims);
  int ext[6];
  g->GetExtent(ext);
  
  vtkSmartPointer<vtkRectilinearGrid> statResult=
    vtkSmartPointer<vtkRectilinearGrid>::New();
  statResult->SetExtent(ext[0],ext[1],ext[2],ext[3],ext[4],ext[4]);
  statResult->GetXCoordinates()->DeepCopy(g->GetXCoordinates());
  statResult->GetYCoordinates()->DeepCopy(g->GetYCoordinates());
  
  // extract the slices of z extent 0.
  vtkSmartPointer<vtkExtractRectilinearGrid> extract=
    vtkSmartPointer<vtkExtractRectilinearGrid>::New();
  
  extract->SetInputConnection(reader->GetOutputPort());
  // low z extent, ext[4],ext[4] is not a bug.
  extract->SetVOI(ext[0],ext[1],ext[2],ext[3],ext[4],ext[4]); 
  
  extract->Update();
  
  double range[2];
  
  extract->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
  cout << "range=" <<range[0]<< ", " << range[1] << endl;
  // visualization pipeline:
  vtkSmartPointer<vtkRenderWindowInteractor> interactor=
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> w=vtkSmartPointer<vtkRenderWindow>::New();
  interactor->SetRenderWindow(w);
  
  vtkSmartPointer<vtkRenderer> r=vtkSmartPointer<vtkRenderer>::New();
  w->AddRenderer(r);
  
  vtkSmartPointer<vtkActor> actor=vtkSmartPointer<vtkActor>::New();
  r->AddViewProp(actor);
  
  vtkSmartPointer<vtkRectilinearGridGeometryFilter> geo=
    vtkSmartPointer<vtkRectilinearGridGeometryFilter>::New();
  
  geo->SetInput(extract->GetOutput());
  
  vtkSmartPointer<vtkPolyDataMapper> mapper=
    vtkSmartPointer<vtkPolyDataMapper>::New();
  
  mapper->SetInputConnection(geo->GetOutputPort());
  actor->SetMapper(mapper);
  
  mapper->SetScalarVisibility(true);
  mapper->SetInterpolateScalarsBeforeMapping(true);
  mapper->SetUseLookupTableScalarRange(false);
  
  // no effect when UseLookupTableScalarRange is true
  mapper->SetScalarRange(range);
  
  vtkSmartPointer<vtkLookupTable> lut=vtkSmartPointer<vtkLookupTable>::New();
  mapper->SetLookupTable(lut);
  
  lut->SetHueRange(0.66667,0.0); // blue to red, not red to blue
  
  w->Render();
//  interactor->Start();
  
  
  int numberOfDatasets=reader->GetNumberOfRows();
  
  // Allocate dims[0]*dims[1] arrays. Each array as numberOfDatasets value.s
  
  typedef std::vector<vtkSmartPointer<vtkDoubleArray> > arraysType;
  
  size_t size=static_cast<size_t>(dims[0]*dims[1]);
  
//arraysType *arrays=new arraysTypes(size);
  
  arraysType *arrays=new std::vector<vtkSmartPointer<vtkDoubleArray> >(size);
  
  size_t idx=0;
  while(idx<size)
    {
    (*arrays)[idx]=vtkSmartPointer<vtkDoubleArray>::New();
    (*arrays)[idx]->SetNumberOfComponents(1);
    (*arrays)[idx]->SetNumberOfTuples(numberOfDatasets);
    ++idx;
    }
  
  int dataset=0;
  while(dataset<numberOfDatasets)
    {
    reader->SetRowIndex(static_cast<unsigned int>(dataset));
    reader->Update(); // for debugging
    extract->Update();
    g=static_cast<vtkRectilinearGrid *>(extract->GetOutput());
    vtkDoubleArray *s=static_cast<vtkDoubleArray *>(
      g->GetPointData()->GetScalars());
    
    assert("check: one_component" && s->GetNumberOfComponents()==1);
    
    int j=0;
    while(j<dims[1])
      {
      int i=0;
      while(i<dims[0])
        {
        (*arrays)[static_cast<size_t>(j*dims[0]+i)]->SetValue(
          dataset,
          KelvinToCelsius(s->GetValue(j*dims[0]+i)));
        ++i;
        }
      ++j;
      }
    ++dataset;
    }
  
  vtkSmartPointer<vtkDescriptiveStatistics> ds=
    vtkSmartPointer<vtkDescriptiveStatistics>::New();
  ds->AddColumn("point");
  ds->SetNominalParameter("Mean");
  ds->SetDeviationParameter("Standard Deviation");
  // Test Learn, Derive, Test, and Assess options
  ds->SetLearnOption(true);
  ds->SetDeriveOption(true);
  ds->SetAssessOption(true);
  ds->SetTestOption(true);
  ds->SignedDeviationsOff();
  
  vtkSmartPointer<vtkDoubleArray> meanArray=
    vtkSmartPointer<vtkDoubleArray>::New();
  meanArray->SetName("mean");
  meanArray->SetNumberOfComponents(1);
  meanArray->SetNumberOfTuples(static_cast<vtkIdType>(size));
  
  vtkSmartPointer<vtkDoubleArray> stddevArray=
    vtkSmartPointer<vtkDoubleArray>::New();
  stddevArray->SetName("stddev");
  stddevArray->SetNumberOfComponents(1);
  stddevArray->SetNumberOfTuples(static_cast<vtkIdType>(size));
  
  statResult->GetPointData()->AddArray(meanArray);
  statResult->GetPointData()->AddArray(stddevArray);
  
  int j=0;
  while(j<dims[1])
    {
    int i=0;
    while(i<dims[0])
      {
      vtkIdType index=j*dims[0]+i;
      vtkDoubleArray *a=(*arrays)[static_cast<size_t>(index)];
      a->SetName("point");
      vtkSmartPointer<vtkTable> t=vtkTable::New();
      t->AddColumn(a);
      ds->SetInput(vtkStatisticsAlgorithm::INPUT_DATA,t);
      ds->Update();
      vtkMultiBlockDataSet *meta=vtkMultiBlockDataSet::SafeDownCast(
        ds->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
      vtkTable *primary=vtkTable::SafeDownCast(meta->GetBlock(0));
      vtkTable *derived=vtkTable::SafeDownCast(meta->GetBlock(1));
      double mean=primary->GetValueByName(0,"Mean").ToDouble();
      double stddev=derived->GetValueByName(0,"Standard Deviation").ToDouble();
      meanArray->SetValue(index,mean);
      stddevArray->SetValue(index,stddev);
      ++i;
      }
    ++j;
    }  
  
  // write the result in a file
  vtkSmartPointer<vtkXMLRectilinearGridWriter> writer=
    vtkSmartPointer<vtkXMLRectilinearGridWriter>::New();
  
  writer->SetInput(statResult);
  writer->SetFileName("statResult.vtr");
  writer->Write();
  
  // visualization pipeline:
  geo->SetInput(statResult);
  
  cout << "set the active scalars" << endl;
  
  // "mean" or "stddev"
  
  statResult->GetPointData()->
    SetActiveAttribute("mean",vtkDataSetAttributes::SCALARS);
  
  cout << "get the active scalars" << endl;
  vtkDataArray *s=
    statResult->GetPointData()->GetAttribute(vtkDataSetAttributes::SCALARS);
  cout << "get the range of the active scalars" << endl;
  s->GetRange(range);
  cout << "range=" <<range[0]<< ", " << range[1] << endl;
  
  mapper->SetScalarRange(range);
  
  vtkSmartPointer<vtkScalarBarWidget> scalarWidget=
    vtkSmartPointer<vtkScalarBarWidget>::New();
  scalarWidget->SetInteractor(interactor);
  scalarWidget->GetScalarBarActor()->SetTitle("Temperature mean");
  scalarWidget->GetScalarBarActor()->SetLookupTable(mapper->GetLookupTable());
  scalarWidget->EnabledOn();
  
  // Standard deviation as contours.
  vtkSmartPointer<vtkContourFilter> contour=
    vtkSmartPointer<vtkContourFilter>::New();
  
  vtkSmartPointer<vtkRectilinearGrid> clone=
    vtkSmartPointer<vtkRectilinearGrid>::New();
  clone->DeepCopy(statResult);
  
  contour->SetInput(clone);
  
  clone->GetPointData()->
    SetActiveAttribute("stddev",vtkDataSetAttributes::SCALARS);
  
  s=clone->GetPointData()->GetAttribute(vtkDataSetAttributes::SCALARS);
  cout << "get the range of the active scalars" << endl;
  s->GetRange(range);
  cout << "range=" <<range[0]<< ", " << range[1] << endl;
  
  
  contour->GenerateValues(5,range); // 5,10
  
  contour->Update();
  
  vtkSmartPointer<vtkPolyDataMapper> mapper2=
    vtkSmartPointer<vtkPolyDataMapper>::New();
    
  mapper2->SetInputConnection(contour->GetOutputPort());
  mapper2->SetScalarVisibility(false);
  
  vtkSmartPointer<vtkActor> actor2=vtkSmartPointer<vtkActor>::New();
  
  actor2->SetMapper(mapper2);
  vtkProperty *p=actor2->GetProperty();
  p->SetLighting(false);
  p->SetColor(0.0,0.0,0.0);
  
//  r->AddActor(actor2);
  
  w->Render();
  interactor->Start();
  
  delete arrays;
  
  return result;
}
