#include <vtkImageData.h>
export module dicom_viewer;

import <string.h>;
import <iostream>;
import <vtkDICOMImageReader.h>;
import <vtkImageActor.h>;
import <vtkImageMapToWindowLevelColors.h>;
import <vtkRenderer.h>;
import <vtkRenderWindow.h>;
import <vtkRenderWindowInteractor.h>;
import <vtkInteractorStyleImage.h>;
import <vtkObjectFactory.h>;


export class CustomInteractorStyle : public vtkInteractorStyleImage {
public:
    static CustomInteractorStyle* New();
    vtkTypeMacro(CustomInteractorStyle, vtkInteractorStyleImage);

    void OnRightButtonDown() override;
    void OnMouseMove() override;
    void OnRightButtonUp() override;
    void SetWindowLevelFilter(vtkImageMapToWindowLevelColors* filter);

private:
    int StartPosition[2] = { 0, 0 };
    double InitialWindow = 400; // Ä¬ÈÏ´°¿í
    double InitialLevel = 40;   // Ä¬ÈÏ´°Î»
    bool Dragging = false;
    vtkImageMapToWindowLevelColors* WindowLevelFilter = nullptr;
};

export void runViewer(const std::string& dicomDirectory);