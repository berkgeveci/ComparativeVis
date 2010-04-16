/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCVActionsGroup.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

========================================================================*/
#ifndef __pqCVActionsGroup_h 
#define __pqCVActionsGroup_h

#include <QActionGroup>

#include "ui_pqCVColumnChooser.h"

// Adds actions for co-processing.
class pqCVActionsGroup : public QActionGroup
{
  Q_OBJECT
  typedef QActionGroup Superclass;
public:
  pqCVActionsGroup(QObject* parent=0);
  virtual ~pqCVActionsGroup();

protected slots:
  void tableToDataCollection();
  void openPathDialog();

protected:
  Ui::pqCVColumnChooser UserInterface;
  
private:
  Q_DISABLE_COPY(pqCVActionsGroup)
};

#endif


