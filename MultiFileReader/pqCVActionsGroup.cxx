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
#include "vtkPVDataInformation.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include "pqActiveObjects.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"

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
  vtkGenericWarningMacro(<<di->GetDataClassName());
  
  // - Bring up a dialog to choose the column name

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

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    cdrp->GetProperty("Input"));
  ip->SetInputConnection(0, op->getSource()->getProxy(), op->getPortNumber());
  
  cdrp->Update();
  
  vtkSmartPointer<vtkDelimitedTextWriter> csvWriter = 
    vtkSmartPointer<vtkDelimitedTextWriter>::New();
  csvWriter->SetInput(cdrp->GetOutput());
  csvWriter->WriteToOutputStringOn();
  csvWriter->Write();
  vtkGenericWarningMacro(<<csvWriter->RegisterAndGetOutputString());
  // - Serialize the table to string
  // - Create the FileCollectionReader
  // - Set the table string
  // - Set the column name
  
}

