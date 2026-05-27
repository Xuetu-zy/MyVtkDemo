#pragma once
#include <vtkDICOMImageReader.h>
#include <vtkImageActor.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkCamera.h>

class CustomInteractorStyle :
    public vtkInteractorStyleImage
{
public:
    static CustomInteractorStyle* New();
    vtkTypeMacro(CustomInteractorStyle, vtkInteractorStyleImage);

    void OnRightButtonDown() override;
    void OnMouseMove() override;
    void OnRightButtonUp() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void SetWindowLevelFilter(vtkSmartPointer<vtkImageMapToWindowLevelColors> filter);
    void SetRenderer(vtkRenderer* renderer);
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;
private:
    int StartPosition[2] = { 0, 0 };
    double InitialWindow = 400;  // 默认窗宽
    double InitialLevel = 40;   // 默认窗位
    bool Dragging = false;       // 控制是否在拖动窗宽窗位
    bool MovingImage = false;    // 控制是否在移动图像
    vtkImageMapToWindowLevelColors* WindowLevelFilter = nullptr;  // 窗宽窗位过滤器
    vtkRenderer* Renderer = nullptr;  // 渲染器，控制图像移动

};

