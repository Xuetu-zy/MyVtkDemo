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
#include <vtkImageReslice.h>

class ResliceInteractorStyle :
    public vtkInteractorStyleImage
{
public:
    // 全局记录切片中心（体坐标），初始值可根据需要设定
    double sliceCenter[3] = { 100, 100, 100 }; // 假设体数据尺寸约为200^3

public:
    static ResliceInteractorStyle* New();
    vtkTypeMacro(ResliceInteractorStyle, vtkInteractorStyleImage);

    // 三个 reslice 对象
    vtkImageReslice* AxialReslice;
    vtkImageReslice* SagittalReslice;
    vtkImageReslice* CoronalReslice;

    // 对应三视图的定位线（水平和垂直线，各视图2条）
    vtkActor* AxialHorLine;
    vtkActor* AxialVerLine;
    vtkActor* SagittalHorLine;
    vtkActor* SagittalVerLine;
    vtkActor* CoronalHorLine;
    vtkActor* CoronalVerLine;

    // 假设切片图像尺寸（数据坐标系下），可根据数据实际尺寸设置
    double ImageWidth = 200;
    double ImageHeight = 200;
    void UpdateCrossLines(vtkActor* horLine, vtkActor* verLine);
    virtual void OnLeftButtonDown() override;

};

