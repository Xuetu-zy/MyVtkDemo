
#include <CustomInteractorStyle.h>

vtkStandardNewMacro(CustomInteractorStyle); 


void CustomInteractorStyle::OnRightButtonDown()
{
    this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
    this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
    this->InitialWindow = this->WindowLevelFilter->GetWindow();
    this->InitialLevel = this->WindowLevelFilter->GetLevel();
    this->Dragging = true;
    vtkInteractorStyleImage::OnRightButtonDown();
}


void CustomInteractorStyle::OnMouseMove()
{
    if (this->MovingImage) {
        // 左键移动图像
        int currentPos[2];
        this->Interactor->GetEventPosition(currentPos);

        // 获取图像演员的位置并进行偏移
        vtkActor* imageActor = this->Renderer->GetActors()->GetLastActor(); // 获取最后一个演员，假设它是图像演员
        if (imageActor) {
            double position[3];
            imageActor->GetPosition(position);
            position[0] += currentPos[0] - this->StartPosition[0];  // 更新X坐标
            position[1] += currentPos[1] - this->StartPosition[1];  // 更新Y坐标
            imageActor->SetPosition(position);
        }

        // 更新鼠标位置
        this->StartPosition[0] = currentPos[0];
        this->StartPosition[1] = currentPos[1];

        // 渲染更新
        this->Interactor->GetRenderWindow()->Render();
    }
    if (this->Dragging) {
        int currentPos[2];
        this->Interactor->GetEventPosition(currentPos);

        // 计算窗宽和窗位的调整量
        double deltaWindow = static_cast<double>(currentPos[0] - this->StartPosition[0]);
        double deltaLevel = static_cast<double>(currentPos[1] - this->StartPosition[1]);

        // 更新窗宽和窗位
        this->WindowLevelFilter->SetWindow(this->InitialWindow + deltaWindow);
        this->WindowLevelFilter->SetLevel(this->InitialLevel - deltaLevel);

        // 渲染更新
        this->Interactor->GetRenderWindow()->Render();
    }

    vtkInteractorStyleImage::OnMouseMove();
}

void CustomInteractorStyle::OnRightButtonUp()
{
    this->Dragging = false;
    vtkInteractorStyleImage::OnRightButtonUp();
}

void CustomInteractorStyle::OnLeftButtonDown()
{// 保存开始位置用于移动图像
    this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
    this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
    this->MovingImage = true;
    vtkInteractorStyleImage::OnLeftButtonDown();
}

void CustomInteractorStyle::OnLeftButtonUp()
{
    this->MovingImage = false;
    vtkInteractorStyleImage::OnLeftButtonUp();
}

void CustomInteractorStyle::SetWindowLevelFilter(vtkSmartPointer<vtkImageMapToWindowLevelColors> filter)
{
    this->WindowLevelFilter = filter.Get();
}

void CustomInteractorStyle::SetRenderer(vtkRenderer* renderer)
{
    this->Renderer = renderer;
}

void CustomInteractorStyle::OnMouseWheelForward()
{
    if (this->CurrentRenderer) {
        this->CurrentRenderer->GetActiveCamera()->Zoom(1.2); // 放大
        this->CurrentRenderer->ResetCameraClippingRange();
        this->GetInteractor()->Render();
    }
}

void CustomInteractorStyle::OnMouseWheelBackward()
{
    if (this->CurrentRenderer) {
        this->CurrentRenderer->GetActiveCamera()->Zoom(0.8); // 缩小
        this->CurrentRenderer->ResetCameraClippingRange();
        this->GetInteractor()->Render();
    }
}
