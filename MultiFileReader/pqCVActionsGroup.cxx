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

#include "vtkObject.h"
#include "vtkPVDataInformation.h"

#include "pqActiveObjects.h"
#include "pqOutputPort.h"

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
}

