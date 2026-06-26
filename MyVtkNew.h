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
class DcmDataset;
class DcmTagKey;
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
    void UpdateSliceCenter(int viewIndex, int value);  // 更新切片中心坐标
    void on_horizontalSliderAxial_valueChanged(int value);  // 更新轴向切片
    void on_horizontalSliderCoronal_valueChanged(int value);  // 更新冠状切片
    void on_horizontalSliderSagittal_valueChanged(int value);  // 更新矢状切片
    void on_horizontalSliderVolume_valueChanged(int value);    //  更新体绘制透明度

    // 下拉框改变
    void on_comboBox_currentTextChanged(const QString& text);  // 选择体绘制模式

    // --- 自定义功能控制槽函数 ---
    void updatePlaneTextEdit(int id, double window, double level);  // 更新每个2D视图右下角的窗宽窗位文本
    static void WindowLevelCallback(vtkObject* caller, unsigned long eventId, void* clientData,
                                    void* callData);  // 窗宽窗位回调函数

    void resliceMode(int mode);  // 设置切片模式
    void thickMode(int mode);    // 设置厚切模式
    void SetBlendMode(int m);    // 设置混合模式
    void SetBlendModeToMaxIP();  // 设置混合模式为最大密度投影
    void SetBlendModeToMinIP();  // 设置混合模式为最小密度投影
    void SetBlendModeToMeanIP();  // 设置混合模式为平均密度投影
    void ResetViews();                    // 重置所有视图的相机和交互状态
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

    vtkSmartPointer<vtkResliceImageViewerMeasurements> ResliceMeasurements;  // 2D 测量管理器
    vtkSmartPointer<vtkCellPicker> picker;  // 拾取器

    // 文字标注
    vtkSmartPointer<vtkTextActor> textActor[4];             // 视图方向标注
    vtkSmartPointer<vtkTextActor> peopleInforTextActor[4];  // 患者信息标注
    vtkSmartPointer<vtkTextActor> windowLevelTextActor[3];  // 每个2D视图右下角WW/WL
    vtkSmartPointer<vtkOpenGLTextActor> text_axial = nullptr;  // Axial 视图文字标注
    vtkSmartPointer<vtkDistanceWidget> activeDistanceWidget;   // 当前激活的测距组件
    vtkSmartPointer<vtkCallbackCommand> activeDistanceCallbackCommand;  // 当前激活的测距回调命令
    int activeDistanceViewIndex = -1;                                   // 当前激活的测距视图索引
    bool isDistanceArmed = false;                                       // 当前是否处于测距激活状态
    unsigned long activeDistanceObserverTag = 0;                        // 当前激活的测距观察者标签
    vtkSmartPointer<vtkAngleWidget> activeAngleWidget;                  // 当前激活的测角组件
    vtkSmartPointer<vtkCallbackCommand> activeAngleCallbackCommand;     // 当前激活的测角回调命令
    int activeAngleViewIndex = -1;                                      // 当前激活的测角视图索引
    bool isAngleArmed = false;                                          // 当前是否处于测角激活状态
    unsigned long activeAngleObserverTag = 0;                           // 当前激活的测角观察者标签
    PendingMeasurementTool pendingMeasurementTool = PendingMeasurementTool::None;  // 当前待激活的测量工具类型
    int lastInteractedViewIndex = 2;  // 记录最后一次交互的视图索引，默认 Axial

    // 3D 视图
    vtkSmartPointer<vtkRenderer> ren;        // 3D场景渲染器 (对应 widget3D)
    vtkSmartPointer<vtkRenderer> renderer;   // 次级 3D 渲染器 (对应 widget3D_2 等)
    vtkSmartPointer<vtkVolume> volume;       // 体绘制对象
    vtkSmartPointer<vtkActor> surfaceActor;  // 面绘制 Actor 对象
    vtkSmartPointer<vtkPolyData> surfacePolyData;  // Surface 模式生成的网格数据，用于 STL 导出
    vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity;  // 体绘制梯度不透明度映射函数
    vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity;  // 体绘制不透明度映射函数

    // 数据源与读取器
    vtkSmartPointer<vtkDICOMImageReader> dicomReader;  // DICOM 数据读取器
    vtkSmartPointer<vtkImageReader2> rawReader;   // RAW 数据读取器
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
    void BoneVolumeRendering();  // 骨体体绘制
    void BloodVesselVolumeRendering();  // 血管体绘制
    void SurfaceRendering();            // 表面绘制 (Marching Cubes)

    // 工具类方法
    vtkSmartPointer<vtkPolyData> LoadPointsFromRaw(const std::string& filename);  // 从 RAW 文件加载点云数据
    void SaveAsRawFile(vtkSmartPointer<vtkPolyData> polyData,
                       const std::string& filename);  // 将 PolyData 保存为 RAW 文件
    void ConvertRawSlicesToDicom(const std::string& outputFolder, const std::string& patientName,
                                 const std::string& patientID);  // 将 RAW 切片序列转换为 DICOM 文件
    void ShowExportMenu();
    void ExportDicomSeries();
    void ExportCurrentViewPng();
    void ExportSurfaceStl();
    void ExportMeasurementsCsv();
    QVTKOpenGLNativeWidget* SelectExportViewWidget();
    bool WriteRenderWindowToPng(vtkRenderWindow* renderWindow, const QString& fileName);
    QString FormatDicomNumber(double value) const;
    unsigned short ClampToDicomUint16(double value) const;
    QString NormalizeDicomText(const QString& value, const QString& fallback) const;
    QString GenerateDicomUid(const char* prefix) const;
    bool PutDicomString(DcmDataset* dataset, const DcmTagKey& tag, const QString& value,
                        QString& error) const;
    bool PutDicomUint16(DcmDataset* dataset, const DcmTagKey& tag, unsigned short value,
                        QString& error) const;

    // 遗留方法声明，保留以防实现缺失
    void readRaw(int width, int height, int depth);  // 读取 RAW 数据
    void rawSlice(int width, int height, int depth);  // 读取 RAW 数据并切片显示
    void setupVTKPipeline(const QString& dicomDirectory);  // 设置 VTK 渲染管线
    void dicomShow(const QString& dicomDirectory);         // 显示 DICOM 数据
    void GenerateVirtual3DData(vtkSmartPointer<vtkImageData>& imageData);  // 生成虚拟三维数据
    void Expand2DTo3D(vtkSmartPointer<vtkImageData>& imageData2D,
                      vtkSmartPointer<vtkImageData>& imageData3D);  // 将二维数据扩展为三维数据
    void readDicomSeries(vtkSmartPointer<vtkImageViewer2>& viewer, QVTKOpenGLNativeWidget* widget,
                         const QString& orientation);        // 读取 DICOM 序列并显示
    void Reset2DViewCameras();                               // 重置 2D 视图相机
    void ResetObliqueViewCameras();                          // 重置斜切视图相机
    void SetResliceCursorWidgetsProcessEvents(int enabled);  // 设置 ResliceCursorWidget 是否响应事件
    void SyncPlaneWidgetsFromResliceCursorPlanes();  // 同步平面小部件位置到 ResliceCursor
    void SyncSliceSlidersFromViews();                        // 同步切片滑块位置到视图
    void ValidateInteractionReadiness();             // 验证交互准备状态
    static void OnDistanceEndInteraction(vtkObject* caller, unsigned long eventId, void* clientData,
                                         void* callData);  // 测距交互结束回调
    static void OnAngleEndInteraction(vtkObject* caller, unsigned long eventId, void* clientData,
                                      void* callData);  // 测角交互结束回调

protected:
    void mouseMoveEvent(QMouseEvent* event);
    bool eventFilter(QObject* watched, QEvent* event) override;
};
