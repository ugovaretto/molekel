#
# Molekel - Molecular Visualization Program
# Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.
#
# $Author$
# $Date$
# $Revision$
#

############################################
# Molekel 5.x
# File included by main CMakeLists.txt file
# list all molekel moc header files here
############################################

SET( MOC_HEADERS
     MainWindow.h
     widgets/MoleculeRenderingStyleWidget.h
     widgets/WorkspaceTreeWidget.h
     widgets/MoleculeVibrationWidget.h
     widgets/MoleculeTrajectoryWidget.h
     widgets/MoleculeAnimationModeWidget.h
     widgets/TimeStepWidget.h
     widgets/MoleculeElDensSurfaceWidget.h
     widgets/ImagePlaneProbeWidget.h
     widgets/GridDataSurfaceWidget.h
     widgets/SasWidget.h
     widgets/SesWidget.h
     widgets/ViewPropertiesWidget.h
     widgets/ShaderWidget.h
     widgets/DepthPeelingWidget.h
     widgets/AntiAliasingWidget.h
     widgets/SpectrumWidget.h
     dialogs/MoleculeAnimationDialog.h
     dialogs/TimeStepDialog.h
     dialogs/ComputeElDensSurfaceDialog.h
     dialogs/ImagePlaneProbeDialog.h
     dialogs/GridDataSurfaceDialog.h
     dialogs/ExportAnimationDialog.h
     dialogs/SasDialog.h
     dialogs/SesDialog.h
     dialogs/ShadersDialog.h
     dialogs/ViewPropertiesDialog.h
     utility/events/EventFilter.h
     utility/events/EventPlayer.h
     utility/events/EventRecorderWidget.h
     utility/events/EventPlayerWidget.h
     utility/shaders/QGLSLShaderEditorWidget.h
     utility/events/EventPositionTimeWidget.h
     utility/DoubleSliderWidget.h
     utility/FoldableWidget.h
    )