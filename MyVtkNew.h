#pragma once

#include "myvtkresliceimageviewer.h"
#include "myvtkresliceimageviewermeasurements.h"
#include "ui_MyVtkNew.h"

#include <QPoint>
#include <QString>
#include <QVTKOpenGLNativeWidget.h>
#include <QVector>
#include <QWidget>
#include <QtWidgets/QMainWindow>
#include <vtkActor.h>
#include <vtkAngleWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkColorTransferFunction.h>
#include <vtkDICOMImageReader.h>
#include <vtkDistanceWidget.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImagePlaneWidget.h>
#include <vtkImageReader2.h>
#include <vtkImageViewer2.h>
#include <vtkMarchingCubes.h>
#include <vtkOpenGLTextActor.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkRayCastImageDisplayHelper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorWidget.h>
#include <vtkResliceImageViewer.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

class vtkResliceCursorCallback;
class QObject;
class QEvent;

namespace Ui
{
class MyVtkNew;
};

class MyVtkNew : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyVtkNew(QWidget* parent = nullptr);
    ~MyVtkNew() override;

    // 切片中心坐标
    double sliceCenter[3] = {100, 100, 100};

protected:
    // 重写大小改变和重绘事件以更新 VTK 视图
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

public slots:
    // --- 界面按钮与滑块槽函数 (保留原有名称以匹配 UI 隐式关联) ---
    void on_pushButtonOpenDicom_clicked();                // 废弃旧按钮，如果被隐藏可以不管
    void on_pushButtonFor3D_clicked();                    // 执行3D体绘制按钮
    void on_pushButtonReadDicomSeries_clicked();          // 读取 DICOM 序列测试 (原RawSlice)
    void on_pushButtonReadRaw_clicked();                  // 读取 RAW 格式数据
    void on_pushButtonGetWindowLevel_clicked();           // 获取当前的窗宽窗位
    void on_pushButton_clicked();                         // 选择 DICOM 目录主函数 (主要的数据入口)
    void on_pushButtonConvertRawSlicesToDicom_clicked();  // 格式转换功能
    void on_pushButtonForAddAngle_clicked();              // 测角按钮触发
    void on_pushButtonAddDistance_clicked();              // 测距按钮触发

    // 滚动条交互
    void UpdateSliceCenter(int viewIndex, int value);
    void on_horizontalSliderAxial_valueChanged(int value);
    void on_horizontalSliderCoronal_valueChanged(int value);
    void on_horizontalSliderSagittal_valueChanged(int value);
    void on_horizontalSliderVolume_valueChanged(int value);

    // 下拉框改变
    void on_comboBox_currentTextChanged(const QString& text);

    // --- 自定义功能控制槽函数 ---
    void updatePlaneTextEdit(int id, double window, double level);
    static void WindowLevelCallback(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);

    void resliceMode(int mode);
    void thickMode(int mode);
    void SetBlendMode(int m);
    void SetBlendModeToMaxIP();
    void SetBlendModeToMinIP();
    void SetBlendModeToMeanIP();
    void ResetViews();
    void AddDistanceMeasurementToView();                  // 测距按钮统一入口（内部判定目标视图）

public:
    // 更新每个2D视图右下角的窗宽窗位文本
    void UpdateCornerAnnotations();
    void Render();
    int DetermineTargetViewIndex() const;
    int HitTest2DViewByGlobalPos(const QPoint& globalPos) const;
    int GetViewIndexFromObject(const QObject* object) const;
    int GetViewIndexFromWidget(const QWidget* widget) const;
    QWidget* GetViewWidgetByIndex(int viewIndex) const;
    void MarkViewInteracted(int viewIndex);
    void MaybeCancelActiveMeasurementsForView(int sourceViewIndex);
    void MaybeCancelDistanceMeasurementForView(int sourceViewIndex);
    void MaybeCancelAngleMeasurementForView(int sourceViewIndex);
    void CancelActiveDistanceMeasurement();
    void CancelActiveAngleMeasurement();
    void AddDistanceMeasurementToView(int i);
    void AddAngleToView(int i);

private:
    enum class PendingMeasurementTool
    {
        None,
        Distance,
        Angle
    };

    Ui::MyVtkNew* ui;

    // --- 核心可视化组件 ---

    // 2D 视图 (MPR)
    vtkSmartPointer<MyVtkResliceImageViewer> riw[3];      // 三个平面的自定义观察器
    vtkSmartPointer<vtkImagePlaneWidget> planeWidget[3];  // 用于3D窗口中的平面指示器
    vtkSmartPointer<vtkResliceCursorCallback> cbk;        // 十字光标回调联动

    // 测量与标注组件
    vtkSmartPointer<vtkDistanceWidget> DistanceWidget[3];                   // 距离测量组件
    vtkSmartPointer<vtkAngleWidget> AngleWidget[3];                         // 角度测量组件
    QVector<vtkSmartPointer<vtkDistanceWidget>> historyDistanceWidgets[3];  // 历史距离测量集合
    QVector<vtkSmartPointer<vtkAngleWidget>> historyAngleWidgets[3];        // 历史角度测量集合

    vtkSmartPointer<vtkResliceImageViewerMeasurements> ResliceMeasurements;
    vtkSmartPointer<vtkCellPicker> picker;  // 拾取器

    // 文字标注
    vtkSmartPointer<vtkTextActor> textActor[4];             // 视图方向标注
    vtkSmartPointer<vtkTextActor> peopleInforTextActor[4];  // 患者信息标注
    vtkSmartPointer<vtkTextActor> windowLevelTextActor[3];  // 每个2D视图右下角WW/WL
    vtkSmartPointer<vtkOpenGLTextActor> text_axial = nullptr;
    vtkSmartPointer<vtkDistanceWidget> activeDistanceWidget;
    vtkSmartPointer<vtkCallbackCommand> activeDistanceCallbackCommand;
    int activeDistanceViewIndex = -1;
    bool isDistanceArmed = false;
    unsigned long activeDistanceObserverTag = 0;
    vtkSmartPointer<vtkAngleWidget> activeAngleWidget;
    vtkSmartPointer<vtkCallbackCommand> activeAngleCallbackCommand;
    int activeAngleViewIndex = -1;
    bool isAngleArmed = false;
    unsigned long activeAngleObserverTag = 0;
    PendingMeasurementTool pendingMeasurementTool = PendingMeasurementTool::None;
    int lastInteractedViewIndex = 2;

    // 3D 视图
    vtkSmartPointer<vtkRenderer> ren;        // 3D场景渲染器 (对应 widget3D)
    vtkSmartPointer<vtkRenderer> renderer;   // 次级 3D 渲染器 (对应 widget3D_2 等)
    vtkSmartPointer<vtkVolume> volume;       // 体绘制对象
    vtkSmartPointer<vtkActor> surfaceActor;  // 面绘制 Actor 对象
    vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity;
    vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity;  // 体绘制不透明度映射函数

    // 数据源与读取器
    vtkSmartPointer<vtkDICOMImageReader> dicomReader;
    vtkSmartPointer<vtkImageReader2> rawReader;
    vtkSmartPointer<vtkImageData> mainImageData;  // 统一的核心三维体数据缓存

    // 全局图像信息
    int imageDims[3] = {0};
    int windowCenter = 6549;
    int windowWidth = 64962;
    QString dicomDirectory = "";

    // --- 核心逻辑功能私有方法 ---

    // 统一的初始化流水线，传入三维图像数据对象
    void InitializeVisualization(vtkSmartPointer<vtkImageData> imageData);

    // 3D 渲染特定模式方法
    void BoneVolumeRendering();
    void BloodVesselVolumeRendering();
    void SurfaceRendering();

    // 工具类方法
    vtkSmartPointer<vtkPolyData> LoadPointsFromRaw(const std::string& filename);
    void SaveAsRawFile(vtkSmartPointer<vtkPolyData> polyData, const std::string& filename);
    void ConvertRawSlicesToDicom(const std::string& outputFolder, const std::string& patientName,
                                 const std::string& patientID);

    // 遗留方法声明，保留以防实现缺失
    void readRaw(int width, int height, int depth);
    void rawSlice(int width, int height, int depth);
    void setupVTKPipeline(const QString& dicomDirectory);
    void dicomShow(const QString& dicomDirectory);
    void GenerateVirtual3DData(vtkSmartPointer<vtkImageData>& imageData);
    void Expand2DTo3D(vtkSmartPointer<vtkImageData>& imageData2D, vtkSmartPointer<vtkImageData>& imageData3D);
    void readDicomSeries(vtkSmartPointer<vtkImageViewer2>& viewer, QVTKOpenGLNativeWidget* widget,
                         const QString& orientation);
    void Reset2DViewCameras();
    void ResetObliqueViewCameras();
    void SetResliceCursorWidgetsProcessEvents(int enabled);
    void SyncPlaneWidgetsFromResliceCursorPlanes();
    void SyncSliceSlidersFromViews();
    void ValidateInteractionReadiness();
    static void OnDistanceEndInteraction(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);
    static void OnAngleEndInteraction(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);

protected:
    void mouseMoveEvent(QMouseEvent* event);
    bool eventFilter(QObject* watched, QEvent* event) override;
};
