#ifndef MYVTKRESLICEIMAGEVIEWERMEASUREMENTS_H
#define MYVTKRESLICEIMAGEVIEWERMEASUREMENTS_H

#include "vtkInteractionImageModule.h"
#include "vtkObject.h"

class MyVtkResliceImageViewer;
class vtkAbstractWidget;
class vtkCallbackCommand;
class vtkCollection;
class vtkDistanceWidget;
class vtkAngleWidget;
class vtkBiDimensionalWidget;
class vtkHandleRepresentation;
class vtkHandleWidget;
class vtkCaptionWidget;
class vtkContourWidget;
class vtkSeedWidget;
// 2D 测量管理器类，负责管理所有测量组件的显示和交互
class MyVtkResliceImageViewerMeasurements : public vtkObject
{
public:
    static MyVtkResliceImageViewerMeasurements* New();
    vtkTypeMacro(MyVtkResliceImageViewerMeasurements,vtkObject);
    void PrintSelf(ostream& os,vtkIndent indent) override;
    virtual void Render();
    virtual void AddItem(vtkAbstractWidget*w);
    virtual void RemoveItem(vtkAbstractWidget*w);
    virtual void RemoveAllItems();
    vtkSetClampMacro(ProcessEvents, vtkTypeBool, 0, 1);
    vtkGetMacro(ProcessEvents,vtkTypeBool);
    vtkBooleanMacro(ProcessEvents, vtkTypeBool);
    vtkSetMacro(Tolerance, double);
    vtkGetMacro(Tolerance, double);
    virtual void SetResliceImageViewer(MyVtkResliceImageViewer*i);
    vtkGetObjectMacro(ResliceImageViewer, MyVtkResliceImageViewer);
    virtual void Update();
protected:
    MyVtkResliceImageViewerMeasurements();  
    ~MyVtkResliceImageViewerMeasurements() override;
    bool IsItemOnReslicedPlane(vtkAbstractWidget* w);  // 判断测量组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkDistanceWidget* w);  // 判断测距组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkAngleWidget* w);     // 判断测角组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkBiDimensionalWidget* w);  // 判断双向测量组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkCaptionWidget* w);        // 判断文字标注组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkContourWidget* w);        // 判断轮廓组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkSeedWidget* w);           // 判断种子组件是否在当前切片平面上
    bool IsWidgetOnReslicedPlane(vtkHandleWidget* w);         // 判断手柄组件是否在当前切片平面上
    bool IsPointOnReslicedPlane(vtkHandleRepresentation* h);  // 判断手柄表示是否在当前切片平面上
    bool IsPositionOnReslicedPlane(double p[3]);              // 判断三维坐标是否在当前切片平面上
    static void ProcessEventsHandler(vtkObject* object, unsigned long event, void* clientdata,
                                     void* calldata);  // 事件处理回调函数
    MyVtkResliceImageViewer* ResliceImageViewer;       // 关联的 ResliceImageViewer 对象
    vtkCollection* WidgetCollection;                   // 存储测量组件的集合
    vtkCallbackCommand* EventCallbackCommand;          // 事件回调命令对象
    vtkTypeBool ProcessEvents;                         // 是否处理事件的标志
    double Tolerance;       // 容差值，用于判断测量组件是否在当前切片平面上
private:
    MyVtkResliceImageViewerMeasurements(const MyVtkResliceImageViewerMeasurements&) = delete;  // 禁止拷贝构造函数
    void operator=(MyVtkResliceImageViewerMeasurements&) = delete;                             // 禁止赋值操作符
};

#endif // MYVTKRESLICEIMAGEVIEWERMEASUREMENTS_H
