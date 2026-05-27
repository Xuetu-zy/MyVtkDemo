
#include "ResliceInteractorStyle .h"
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>

vtkStandardNewMacro(ResliceInteractorStyle);



// 更新某个视图中的十字线
void ResliceInteractorStyle::UpdateCrossLines(vtkActor* horLine, vtkActor* verLine)
{    // 创建水平线：从 (0, SliceCenter[1]) 到 (ImageWidth, SliceCenter[1])
    vtkSmartPointer<vtkLineSource> lineSourceHor = vtkSmartPointer<vtkLineSource>::New();
    lineSourceHor->SetPoint1(0, sliceCenter[1], 0);
    lineSourceHor->SetPoint2(ImageWidth, sliceCenter[1], 0);
    lineSourceHor->Update();
    vtkSmartPointer<vtkPolyDataMapper> mapperHor = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapperHor->SetInputConnection(lineSourceHor->GetOutputPort());
    horLine->SetMapper(mapperHor);

    // 创建垂直线：从 (SliceCenter[0], 0) 到 (SliceCenter[0], ImageHeight)
    vtkSmartPointer<vtkLineSource> lineSourceVer = vtkSmartPointer<vtkLineSource>::New();
    lineSourceVer->SetPoint1(sliceCenter[0], 0, 0);
    lineSourceVer->SetPoint2(sliceCenter[0], ImageHeight, 0);
    lineSourceVer->Update();
    vtkSmartPointer<vtkPolyDataMapper> mapperVer = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapperVer->SetInputConnection(lineSourceVer->GetOutputPort());
    verLine->SetMapper(mapperVer);
}

void ResliceInteractorStyle::OnLeftButtonDown()
{
    int clickPos[2];
    this->GetInteractor()->GetEventPosition(clickPos);
    int* size = this->GetInteractor()->GetRenderWindow()->GetSize();
    // 简单线性映射：将屏幕坐标映射到数据坐标（这里假设屏幕大小和数据尺寸成比例）
    sliceCenter[0] = (clickPos[0] / static_cast<double>(size[0])) * ImageWidth;
    sliceCenter[1] = (clickPos[1] / static_cast<double>(size[1])) * ImageHeight;
    // sliceCenter[2] 保持不变（或根据需要更新）

    // 更新三个切片的原点
    if (AxialReslice)
    {
        AxialReslice->SetResliceAxesOrigin(sliceCenter);
        AxialReslice->Update();
    }
    if (CoronalReslice)
    {
        CoronalReslice->SetResliceAxesOrigin(sliceCenter);
        CoronalReslice->Update();
    }
    if (SagittalReslice)
    {
        SagittalReslice->SetResliceAxesOrigin(sliceCenter);
        SagittalReslice->Update();
    }

    // 更新各视图定位线
    UpdateCrossLines(AxialHorLine, AxialVerLine);
    UpdateCrossLines(CoronalHorLine, CoronalVerLine);
    UpdateCrossLines(SagittalHorLine, SagittalVerLine);

    this->GetInteractor()->Render();
    vtkInteractorStyleImage::OnLeftButtonDown();
}

