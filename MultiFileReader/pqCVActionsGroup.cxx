/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCVActionsGroup.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

========================================================================*/
#include "pqCVActionsGroup.h"

#include "vtkDelimitedTextWriter.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMObject.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"

#include "ui_pqCVColumnChooser.h"

#include <QComboBox>

//-----------------------------------------------------------------------------
pqCVActionsGroup::pqCVActionsGroup(QObject* parentObject)
  : Superclass(parentObject)
{
  QAction* export_action = this->addAction("Table to DataSet Collection");
  export_action->setToolTip("Load a collection of dataset listed in a table.");
  export_action->setStatusTip("Load a collection of dataset listed in a table.");

 QObject::connect(export_action, SIGNAL(triggered()),
   this, SLOT(tableToDataCollection()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqCVActionsGroup::~pqCVActionsGroup()
{
}

//-----------------------------------------------------------------------------
void pqCVActionsGroup::tableToDataCollection()
{
  pqActiveObjects& aos = pqActiveObjects::instance();
  pqOutputPort* op = aos.activePort();
  if (!op)
    {
    return;
    }
  vtkPVDataInformation* di = op->getDataInformation();
  if (di->GetCompositeDataClassName() || 
      strcmp(di->GetDataClassName(), "vtkTable") != 0)
    {
    return;
    }
  
  // - Get the table to client
  vtkIdType connectionID = aos.activeServer()->GetConnectionID();
  vtkSMClientDeliveryRepresentationProxy* cdrp = 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
      vtkSMObject::GetProxyManager()->NewProxy("representations", 
                                             "ClientDeliveryRepresentation"));
  if (!cdrp)
    {
    vtkGenericWarningMacro("Bummer!");
    return;
    }
    
  // Use a smart pointer so that we don't have to worry about deleting
  // after every return
  vtkSmartPointer<vtkSMClientDeliveryRepresentationProxy> cdrp_s;
  cdrp_s.TakeReference(cdrp);

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    cdrp_s->GetProperty("Input"));
  ip->SetInputConnection(0, op->getSource()->getProxy(), op->getPortNumber());
  
  cdrp_s->Update();
  
  // - Bring up a dialog to choose the column name
  
  QDialog myDialog;
  Ui::pqCVColumnChooser ui;
  ui.setupUi(&myDialog);
  
  vtkPVDataSetAttributesInformation* pdi = di->GetRowDataInformation();
  int numArrays = pdi->GetNumberOfArrays();
  const char* defaultValue = 0;
  for (int i=0; i<numArrays; i++)
    {
    vtkPVArrayInformation* ai = pdi->GetArrayInformation(i);
    if (ai->GetDataType() == VTK_STRING)
      {
      ui.comboBox->addItem(ai->GetName());
      }
    }
  if (myDialog.exec() != QDialog::Accepted)
    {
    return;
    }
    
  // - Serialize the table to string

  vtkSmartPointer<vtkDelimitedTextWriter> csvWriter = 
    vtkSmartPointer<vtkDelimitedTextWriter>::New();
  csvWriter->SetInput(cdrp_s->GetOutput());
  csvWriter->WriteToOutputStringOn();
  csvWriter->Write();
  
  // - Create the FileCollectionReader
  
  pqApplicationCore* acore = pqApplicationCore::instance();
  pqObjectBuilder* builder = acore->getObjectBuilder();
  pqPipelineSource* reader = builder->createSource(
    "sources", "FileCollectionReader", aos.activeServer());

  // - Set the table string

  char* tableStr = csvWriter->RegisterAndGetOutputString();
  vtkSMPropertyHelper(reader->getProxy(), "Table").Set(tableStr);
  delete[] tableStr;
  
  // - Set the column name
  vtkstd::string col = ui.comboBox->currentText().toStdString();
  vtkSMPropertyHelper(reader->getProxy(), "FileNameColumn").Set(col.c_str());
  reader->getProxy()->UpdateVTKObjects();
  reader->getProxy()->UpdatePropertyInformation();
}

