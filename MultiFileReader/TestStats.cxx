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

int main(int argc,
         char *argv[])
{
  int result=0;
  
  vtkFileCollectionReader *reader=vtkFileCollectionReader::New();
  reader->SetTableFromString("/home/fbertel/projects/development/data/compvis/2010-04-28-temperature-subset/new-temperature-subset/temperature-subset.db");
  reader->SetRowIndex(0); // read the first file.
  reader->SetDirectoryPath("/home/fbertel/projects/development/data/compvis/2010-04-28-temperature-subset/new-temperature-subset/");
  reader->SetFileNameColumn("Handle"); // specific to the db.
  
  // how to select the "Temperature" table for the the db?
  
  cout << "update reader." << endl;
  reader->Update(); // read the first dataset.
  cout << "reader updated." << endl;
  vtkRectilinearGrid *g=static_cast<vtkRectilinearGrid *>(reader->GetOutput());
  
  int dims[3];
  g->GetDimensions(dims);
  int ext[6];
  g->GetExtent(ext);
  
  // extract the slices of z extent 0.
  
  
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
  
  vtkExtractRectilinearGrid *extract=vtkExtractRectilinearGrid::New();
  
  extract->SetInputConnection(reader->GetOutputPort());
  
  // low z extent, ext[4],ext[4] is not a bug.
  extract->SetVOI(ext[0],ext[1],ext[2],ext[3],ext[4],ext[4]); 
  
  int dataset=0;
  while(dataset<numberOfDatasets)
    {
    reader->SetRowIndex(dataset);
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
          s->GetValue(j*dims[0]+i));
        ++i;
        }
      ++j;
      }
    ++dataset;
    }
  
  vtkDescriptiveStatistics *ds=vtkDescriptiveStatistics::New();
  ds->AddColumn("point");
  ds->SetNominalParameter("Mean");
  ds->SetDeviationParameter("Standard Deviation");
  // Test Learn, Derive, Test, and Assess options
  ds->SetLearnOption(true);
  ds->SetDeriveOption(true);
  ds->SetAssessOption(true);
  ds->SetTestOption(true);
  ds->SignedDeviationsOff();
  
  vtkRectilinearGrid *statResult=vtkRectilinearGrid::New();
  statResult->SetExtent(ext[0],ext[1],ext[2],ext[3],ext[4],ext[4]);
  
  vtkDoubleArray *meanArray=vtkDoubleArray::New();
  meanArray->SetName("mean");
  meanArray->SetNumberOfComponents(1);
  meanArray->SetNumberOfTuples(static_cast<vtkIdType>(size));
  
  vtkDoubleArray *stddevArray=vtkDoubleArray::New();
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
      vtkTable *t=vtkTable::New();
      t->AddColumn(a);
      ds->SetInput(vtkStatisticsAlgorithm::INPUT_DATA,t);
      t->Delete();
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
  
  // visualization pipeline:
  vtkRenderWindowInteractor *interactor=vtkRenderWindowInteractor::New();
  vtkRenderWindow *w=vtkRenderWindow::New();
  interactor->SetRenderWindow(w);
  
  vtkRenderer *r=vtkRenderer::New();
  w->AddRenderer(r);
  
  vtkActor *a=vtkActor::New();
  r->AddViewProp(a);
  
  vtkRectilinearGridGeometryFilter *geo=vtkRectilinearGridGeometryFilter::New();
  
  geo->SetInput(statResult);
  
  vtkPolyDataMapper *mapper=vtkPolyDataMapper::New();
  
  mapper->SetInputConnection(geo->GetOutputPort());
  a->SetMapper(mapper);
  
  interactor->Start();
  
  delete arrays;
  
  return result;
}
