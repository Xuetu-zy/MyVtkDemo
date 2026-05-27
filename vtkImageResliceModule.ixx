module;
#include <vtkDICOMImageReader.h>
#include <vtkImageReslice.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkCamera.h>
#include <vtkDataObject.h>
export module vtkImageResliceModule;


//export vtkSmartPointer<vtkImageData> loadDICOMSequence(const std::string& directory);
//

export void MyFunc();

export vtkSmartPointer<vtkImageData> loadDICOMSequence(const std::string& directory)
{
    auto reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName(directory.c_str());
    reader->Update();
    return reader->GetOutput();
}

