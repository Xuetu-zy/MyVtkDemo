

#include <QApplication>
#include "MyVtkNew.h"
#include "vtkoutputwindow.h"
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2)

VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    MyVtkNew w;
    w.show();
    return a.exec();
}
