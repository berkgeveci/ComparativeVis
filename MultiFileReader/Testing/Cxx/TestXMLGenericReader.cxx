#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkXMLGenericDataObjectReader.h"
int TestXMLGenericReader(int argc, char *argv[])
{
  int retVal;
  
  retVal=vtkTesting::PASSED;
  
  vtkXMLGenericDataObjectReader *reader=vtkXMLGenericDataObjectReader::New();
  
  reader->SetFileName("/home/fbertel/projects/development/data/compvis/temperature-subset-clean/sref_rsm_t03z_pgrb212_ctl1_f00_TMP.vtr");
  
  cout << "before reader update" << endl;
  reader->Update();
  cout << "after reader update" << endl;
  
  reader->Delete();
  
  return !retVal;
}
