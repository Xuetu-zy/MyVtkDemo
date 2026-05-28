

#include <QApplication>
#include "MyVtkNew.h"
#include "vtkoutputwindow.h"
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkStringOutputWindow.h>
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2)

VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)
int main(int argc, char *argv[])
{
    vtkObject::GlobalWarningDisplayOff();
    vtkSmartPointer<vtkStringOutputWindow> outputWindow =
        vtkSmartPointer<vtkStringOutputWindow>::New();
    vtkOutputWindow::SetInstance(outputWindow);

    QApplication a(argc, argv);
    MyVtkNew w;
    w.show();
    return a.exec();
}
