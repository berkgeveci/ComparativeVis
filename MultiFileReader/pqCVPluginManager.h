/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCVPluginManager.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

========================================================================*/
#ifndef __pqCVPluginManager_h 
#define __pqCVPluginManager_h

#include <QObject>

/// pqCVPluginManager is the central class that orchestrates the behaviour of
/// this co-processing plugin.
class pqCVPluginManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqCVPluginManager(QObject* parent=0);
  ~pqCVPluginManager();

  /// Methods used to shartup and shutdown the plugin.
  void startup();
  
  /// Methods used to shartup and shutdown the plugin.
  void shutdown();

private:
  pqCVPluginManager(const pqCVPluginManager&); // Not implemented.
  void operator=(const pqCVPluginManager&); // Not implemented.
};

#endif


