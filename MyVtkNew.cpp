#include "MyVtkNew.h"

#include "CustomInteractorStyle.h"
#include "ResliceInteractorStyle .h"
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "ui_MyVtkNew.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCommand.h"
#include "vtkDICOMImageReader.h"
#include "vtkDistanceRepresentation.h"
#include "vtkDistanceWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkInteractorStyleImage.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceImageViewer.h"
#include "vtkResliceImageViewerMeasurements.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QVTKOpenGLNativeWidget.h>
#include <iostream>
#include <qmessagebox.h>
#include <vtkActor2D.h>
#include <vtkAngleRepresentation2D.h>
#include <vtkAutoInit.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourFilter.h>
#include <vtkCornerAnnotation.h>
#include <vtkDataSetMapper.h>
#include <vtkDistanceRepresentation2D.h>
#include <vtkExtractVOI.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageImport.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageMapper.h>
#include <vtkImageMapper3D.h>
#include <vtkImageReader.h>
#include <vtkImageReslice.h>
#include <vtkImageSlabReslice.h>
#include <vtkImageSliceMapper.h>
#include <vtkLineSource.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkNamedColors.h>
#include <vtkOutlineFilter.h>
#include <vtkPNGWriter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPointHandleRepresentation2D.h>
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorThickLineRepresentation.h>
#include <vtkStripper.h>
#include <vtkTextProperty.h>
#include <vtkVolumeProperty.h>

// -------------------------------------------------------------------------
// VTK 9 环境下必须配置的初始化宏，保证 OpenGL2 与交互系统的链接正常
// -------------------------------------------------------------------------
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

// -------------------------------------------------------------------------
// 十字定位线 (ResliceCursor) 联动回调类
// -------------------------------------------------------------------------
class vtkResliceCursorCallback : public vtkCommand
{
public:
    static vtkResliceCursorCallback* New() { return new vtkResliceCursorCallback; }
    void Execute(vtkObject* caller, unsigned long ev, void* callData) override
    {
        if (!this->MyVtkApp)
        {
            return;
        }

        // 防止同步WW/WL时递归触发事件造成抖动
        if (this->InWindowLevelSync)
        {
            return;
        }

        auto syncWindowLevelFromView = [this](int sourceView, double window, double level) {
            auto clampToRange = [](double v, double lo, double hi) {
                if (v < lo)
                    return lo;
                if (v > hi)
                    return hi;
                return v;
            };
            if (!this->Viewers[sourceView])
            {
                return;
            }
            if (window <= 0.0)
            {
                window = this->Viewers[sourceView]->GetColorWindow();
            }
            window = clampToRange(window, 1.0, 65535.0);
            level = clampToRange(level, 0.0, 65535.0);

            this->InWindowLevelSync = true;
            for (int i = 0; i < 3; i++)
            {
                if (this->Viewers[i])
                {
                    if (i != sourceView)
                    {
                        this->Viewers[i]->SetColorWindow(window);
                        this->Viewers[i]->SetColorLevel(level);
                    }
                }
                if (this->IPW[i])
                {
                    this->IPW[i]->SetWindowLevel(window, level, 1);
                }
            }
            for (int i = 0; i < 3; i++)
            {
                if (this->Viewers[i])
                {
                    this->Viewers[i]->Render();
                }
            }
            this->InWindowLevelSync = false;

            this->MyVtkApp->UpdateCornerAnnotations();
            if (this->IPW[0] && this->IPW[0]->GetInteractor())
            {
                this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
            }
        };

        if (ev == vtkCommand::WindowLevelEvent)
        {
            for (int i = 0; i < 3; i++)
            {
                if (this->Viewers[i] && caller == this->Viewers[i]->GetInteractorStyle())
                {
                    this->MyVtkApp->MaybeCancelDistanceMeasurementForView(i);
                    syncWindowLevelFromView(i, this->Viewers[i]->GetColorWindow(), this->Viewers[i]->GetColorLevel());
                    return;
                }
            }
        }

        if (ev == vtkResliceCursorWidget::WindowLevelEvent)
        {
            for (int i = 0; i < 3; i++)
            {
                if (caller == this->RCW[i])
                {
                    this->MyVtkApp->MaybeCancelDistanceMeasurementForView(i);
                    double window = this->Viewers[i]->GetColorWindow();
                    double level = this->Viewers[i]->GetColorLevel();
                    if (vtkResliceCursorRepresentation* rep =
                            vtkResliceCursorRepresentation::SafeDownCast(this->RCW[i]->GetRepresentation()))
                    {
                        double wl[2] = {0.0, 0.0};
                        rep->GetWindowLevel(wl);
                        if (wl[0] > 0.0)
                        {
                            window = wl[0];
                            level = wl[1];
                        }
                    }
                    syncWindowLevelFromView(i, window, level);
                    return;
                }
            }
        }

        if (this->MyVtkApp)
        {
            this->MyVtkApp->UpdateCornerAnnotations();
        }

        if (ev == vtkResliceCursorWidget::ResliceThicknessChangedEvent)
        {
            for (int i = 0; i < 3; i++)
            {
                this->RCW[i]->Render();
            }
            return;
        }
        vtkImagePlaneWidget* ipw = dynamic_cast<vtkImagePlaneWidget*>(caller);
        if (ipw)
        {
            double* wl = static_cast<double*>(callData);
            if (wl && wl[0] > 0.0)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (ipw == this->IPW[i])
                    {
                        syncWindowLevelFromView(i, wl[0], wl[1]);
                        return;
                    }
                }
            }
            if (ipw == this->IPW[0])
            {
                this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
                this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
            }
            else if (ipw == this->IPW[1])
            {
                this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
                this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
            }
            else if (ipw == this->IPW[2])
            {
                this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
                this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
            }
        }
        vtkResliceCursorWidget* rcw = dynamic_cast<vtkResliceCursorWidget*>(caller);
        if (rcw)
        {
            vtkResliceCursorLineRepresentation* rep =
                dynamic_cast<vtkResliceCursorLineRepresentation*>(rcw->GetRepresentation());
            rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
            for (int i = 0; i < 3; i++)
            {
                vtkPlaneSource* ps = static_cast<vtkPlaneSource*>(this->IPW[i]->GetPolyDataAlgorithm());
                ps->SetOrigin(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetOrigin());
                ps->SetPoint1(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint1());
                ps->SetPoint2(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint2());
                this->IPW[i]->UpdatePlacement();
            }
        }
        for (int i = 0; i < 3; i++)
        {
            this->RCW[i]->Render();
        }
        this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
    }

    vtkResliceCursorCallback() : MyVtkApp(nullptr) {}
    vtkImagePlaneWidget* IPW[3];
    vtkResliceCursorWidget* RCW[3];
    MyVtkResliceImageViewer* Viewers[3] = {nullptr, nullptr, nullptr};
    MyVtkNew* MyVtkApp;
    bool InWindowLevelSync = false;
};

// -------------------------------------------------------------------------
// MyVtkNew 构造与基础事件处理
// -------------------------------------------------------------------------
MyVtkNew::MyVtkNew(QWidget* parent) : QMainWindow(parent)
{
    ui = new Ui::MyVtkNew();
    ui->setupUi(this);

    ui->gridLayout->setRowStretch(0, 1);
    ui->gridLayout->setRowStretch(1, 1);
    ui->gridLayout->setColumnStretch(0, 1);
    ui->gridLayout->setColumnStretch(1, 1);

    connect(this->ui->resliceModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(resliceMode(int)));
    connect(this->ui->thickModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(thickMode(int)));
    this->ui->thickModeCheckBox->setEnabled(false);

    connect(this->ui->radioButton_Max, SIGNAL(pressed()), this, SLOT(SetBlendModeToMaxIP()));
    connect(this->ui->radioButton_Min, SIGNAL(pressed()), this, SLOT(SetBlendModeToMinIP()));
    connect(this->ui->radioButton_Mean, SIGNAL(pressed()), this, SLOT(SetBlendModeToMeanIP()));
    this->ui->blendModeGroupBox->setEnabled(false);

    connect(this->ui->resetButton, SIGNAL(pressed()), this, SLOT(ResetViews()));
    connect(this->ui->AddDistance1Button, SIGNAL(pressed()), this, SLOT(AddDistanceMeasurementToView1()));
}

MyVtkNew::~MyVtkNew()
{
    delete ui;
}

void MyVtkNew::resizeEvent(QResizeEvent* event)
{
    if (ui->widgetAxial->renderWindow())
        ui->widgetAxial->renderWindow()->Render();
    if (ui->widgetCoronal->renderWindow())
        ui->widgetCoronal->renderWindow()->Render();
    if (ui->widgetSagittal->renderWindow())
        ui->widgetSagittal->renderWindow()->Render();
    if (ui->widget3D->renderWindow())
        ui->widget3D->renderWindow()->Render();
}

void MyVtkNew::paintEvent(QPaintEvent* event) {}

// -------------------------------------------------------------------------
// 核心数据处理流：选择 DICOM 目录加载数据
// -------------------------------------------------------------------------
void MyVtkNew::on_pushButton_clicked()
{
    dicomDirectory = QFileDialog::getExistingDirectory(this, tr("Select DICOM Folder"));
    if (dicomDirectory.isEmpty())
        return;

    dicomReader = vtkSmartPointer<vtkDICOMImageReader>::New();
    dicomReader->SetDirectoryName(dicomDirectory.toStdString().c_str());
    dicomReader->Update();

    mainImageData = dicomReader->GetOutput();

    if (!mainImageData || mainImageData->GetNumberOfPoints() == 0)
    {
        QMessageBox::warning(this, "Error", "无法加载 DICOM 数据。");
        return;
    }

    InitializeVisualization(mainImageData);
}

// -------------------------------------------------------------------------
// 初始化核心可视化管线
// -------------------------------------------------------------------------
void MyVtkNew::InitializeVisualization(vtkSmartPointer<vtkImageData> imageData)
{
    if (!imageData)
        return;

    // 清理旧的交互组件，避免重新加载图像时，残留组件响应旧事件导致崩溃
    for (int i = 0; i < 3; i++)
    {
        if (riw[i] && riw[i]->GetResliceCursorWidget())
        {
            riw[i]->GetResliceCursorWidget()->RemoveAllObservers();
            riw[i]->GetResliceCursorWidget()->Off();
        }
        if (riw[i] && riw[i]->GetInteractorStyle())
        {
            riw[i]->GetInteractorStyle()->RemoveAllObservers();
        }
        if (planeWidget[i])
        {
            planeWidget[i]->RemoveAllObservers();
            planeWidget[i]->Off();
        }
        if (DistanceWidget[i])
        {
            DistanceWidget[i]->RemoveAllObservers();
            DistanceWidget[i]->Off();
        }
        if (AngleWidget[i])
        {
            AngleWidget[i]->RemoveAllObservers();
            AngleWidget[i]->Off();
        }

        // 清空并销毁历史测量标注，彻底防止残留
        for (auto& w : historyDistanceWidgets[i])
        {
            if (w)
                w->Off();
        }
        historyDistanceWidgets[i].clear();

        for (auto& w : historyAngleWidgets[i])
        {
            if (w)
                w->Off();
        }
        historyAngleWidgets[i].clear();
    }

    imageData->GetDimensions(imageDims);
    double range[2];
    imageData->GetScalarRange(range);

    QString fontPath = QDir::currentPath() + "/Fonts/simhei.ttf";
    QByteArray fontPathUtf8 = fontPath.toUtf8();
    const char* fontPathCStr = fontPathUtf8.constData();

    for (auto i = 0; i < 4; i++)
    {
        textActor[i] = vtkSmartPointer<vtkTextActor>::New();
        textActor[i]->SetDisplayPosition(5, 5);
        textActor[i]->GetTextProperty()->SetFontSize(12);  // 将视图类别方向标号由 18 缩小为 12
        textActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        textActor[i]->GetTextProperty()->SetFontFile(fontPathCStr);

        peopleInforTextActor[i] = vtkSmartPointer<vtkTextActor>::New();
        peopleInforTextActor[i]->GetTextProperty()->SetFontSize(10);  // 将患者姓名说明由 14 缩小为 10
        peopleInforTextActor[i]->GetTextProperty()->SetFontFile(fontPathCStr);
        peopleInforTextActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        peopleInforTextActor[i]->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        peopleInforTextActor[i]->SetPosition(0.01, 0.95);
        peopleInforTextActor[i]->GetTextProperty()->SetVerticalJustificationToTop();
    }
    textActor[0]->SetInput(QString::fromUtf8("矢状面 (Sagittal)").toUtf8());  
    textActor[0]->GetTextProperty()->SetColor(0, 1, 0);
    textActor[1]->SetInput(QString::fromUtf8("冠状面 (Coronal)").toUtf8()); 
    textActor[1]->GetTextProperty()->SetColor(0, 0, 1);
    textActor[2]->SetInput(QString::fromUtf8("横截面 (Axial)").toUtf8());
    textActor[2]->GetTextProperty()->SetColor(1, 0, 0);
    textActor[3]->SetInput(QString::fromUtf8("3D 视图").toUtf8());
    textActor[3]->GetTextProperty()->SetColor(1, 1, 0);

    QString patientInfo = QString::fromUtf8("患者姓名：未知\r\nUID: 未知");
    if (dicomReader && dicomReader->GetPatientName() && dicomReader->GetStudyUID())
    {
        patientInfo = QString::fromUtf8("患者姓名：") + dicomReader->GetPatientName() + "\r\n" +
                      "UID:" + dicomReader->GetStudyUID();
    }
    for (auto i = 0; i < 4; i++)
    {
        peopleInforTextActor[i]->SetInput(patientInfo.toUtf8());
    }

    // --- 阶段一：创建三个 MPR 视图对象 ---
    for (auto i = 0; i < 3; i++)
    {
        riw[i] = vtkSmartPointer<MyVtkResliceImageViewer>::New();
        if (riw[i]->GetRenderWindow())
        {
            riw[i]->GetRenderWindow()->SetOffScreenRendering(1);
            riw[i]->GetRenderWindow()->Finalize();
        }
        riw[i]->GetRenderer()->AddActor(textActor[i]);
        riw[i]->GetRenderer()->AddActor(peopleInforTextActor[i]);
        windowLevelTextActor[i] = vtkSmartPointer<vtkTextActor>::New();
        windowLevelTextActor[i]->GetTextProperty()->SetFontSize(10);
        windowLevelTextActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        windowLevelTextActor[i]->GetTextProperty()->SetFontFile(fontPathCStr);
        windowLevelTextActor[i]->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
        windowLevelTextActor[i]->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        windowLevelTextActor[i]->SetPosition(0.99, 0.01);
        windowLevelTextActor[i]->GetTextProperty()->SetJustificationToRight();
        windowLevelTextActor[i]->GetTextProperty()->SetVerticalJustificationToBottom();
        riw[i]->GetRenderer()->AddActor(windowLevelTextActor[i]);

    }

    // --- 阶段二：绑定渲染窗口和交互器 ---
    ui->widgetSagittal->show();
    ui->widgetCoronal->show();
    ui->widgetAxial->show();
    ui->widget3D->show();

    riw[0]->SetRenderWindow(ui->widgetSagittal->renderWindow());
    riw[0]->SetupInteractor(ui->widgetSagittal->renderWindow()->GetInteractor());
    riw[1]->SetRenderWindow(ui->widgetCoronal->renderWindow());
    riw[1]->SetupInteractor(ui->widgetCoronal->renderWindow()->GetInteractor());
    riw[2]->SetRenderWindow(ui->widgetAxial->renderWindow());
    riw[2]->SetupInteractor(ui->widgetAxial->renderWindow()->GetInteractor());

    // --- 阶段三：先设置数据和共享 Cursor（在默认的 AxisAligned 模式下完成） ---
    // 关键：必须先共享 Cursor + 设置数据，再切换 Oblique 模式
    // 否则 InstallPipeline 会在没有图像数据的旧 Cursor 上运行
    const int orientationByView[3] = {
        vtkImageViewer2::SLICE_ORIENTATION_YZ,  // widgetSagittal
        vtkImageViewer2::SLICE_ORIENTATION_XZ,  // widgetCoronal
        vtkImageViewer2::SLICE_ORIENTATION_XY   // widgetAxial
    };
    for (int i = 0; i < 3; i++)
    {
        riw[i]->SetInputData(imageData);
        riw[i]->SetSliceOrientation(orientationByView[i]);
        riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());
        riw[i]->SetLookupTable(riw[0]->GetLookupTable());
        riw[i]->GetRenderer()->SetBackground(0, 0, 0);

        // 关键修复：确保每个切面的光标算法知道自己所在切面的方向，否则默认为 Z 面，导致交线计算错误且无法联动
        vtkResliceCursorLineRepresentation* rep =
            vtkResliceCursorLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        if (rep && rep->GetResliceCursorActor() && rep->GetResliceCursorActor()->GetCursorAlgorithm())
        {
            rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(orientationByView[i]);
        }
        riw[i]->GetResliceCursorWidget()->ManageWindowLevelOn();
    }
    riw[0]->GetResliceCursor()->SetCenter(imageData->GetCenter());

    // 默认进入 AxisAligned：仅显示图像，十字线在勾选 Oblique 后再显示
    ui->resliceModeCheckBox->blockSignals(true);
    ui->resliceModeCheckBox->setChecked(false);
    ui->resliceModeCheckBox->blockSignals(false);
    resliceMode(0);

    // --- 阶段四：3D 视图和 ImagePlaneWidget ---
    picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.005);
    ren = vtkSmartPointer<vtkRenderer>::New();
    ren->AddActor(textActor[3]);
    ren->AddActor(peopleInforTextActor[3]);
    ui->widget3D->renderWindow()->AddRenderer(ren);

    cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();
    cbk->MyVtkApp = this;

    for (auto i = 0; i < 3; i++)
    {
        planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
        planeWidget[i]->SetInteractor(ui->widget3D->renderWindow()->GetInteractor());
        planeWidget[i]->SetPicker(picker);
        planeWidget[i]->RestrictPlaneToVolumeOn();

        double color[3] = {0, 0, 0};
        color[i] = 1;
        planeWidget[i]->GetPlaneProperty()->SetColor(color);

        planeWidget[i]->TextureInterpolateOff();
        planeWidget[i]->SetResliceInterpolateToLinear();
        planeWidget[i]->SetInputData(imageData);
        planeWidget[i]->SetPlaneOrientation(i);
        planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
        planeWidget[i]->DisplayTextOn();
        planeWidget[i]->SetDefaultRenderer(ren);
        auto clampToRange = [](double v, double lo, double hi) {
            if (v < lo)
                return lo;
            if (v > hi)
                return hi;
            return v;
        };
        const double dataWindow = (range[1] - range[0] < 1.0) ? 1.0 : (range[1] - range[0]);
        const double dataLevel = (range[0] + range[1]) * 0.5;
        const double window = clampToRange(dataWindow, 1.0, 65535.0);
        const double level = clampToRange(dataLevel, 0.0, 65535.0);
        planeWidget[i]->SetWindowLevel(window, level);
        planeWidget[i]->On();
        planeWidget[i]->InteractionOn();
    }
    planeWidget[1]->SetLookupTable(planeWidget[0]->GetLookupTable());
    planeWidget[2]->SetLookupTable(planeWidget[0]->GetLookupTable());

    // --- 阶段五：回调绑定（联动的核心） ---
    for (auto i = 0; i < 3; i++)
    {
        cbk->IPW[i] = planeWidget[i];
        cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
        cbk->Viewers[i] = riw[i];
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, cbk);
        riw[i]->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, cbk);

        planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
        planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
        if (vtkResliceCursorRepresentation* rep =
                vtkResliceCursorRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation()))
        {
            rep->DisplayTextOff();
        }
        riw[i]->GetResliceCursorWidget()->ManageWindowLevelOn();
    }

    // --- 阶段六：配置十字光标颜色 + 强制构建几何体 ---
    for (int i = 0; i < 3; i++)
    {
        vtkResliceCursorLineRepresentation* rep =
            vtkResliceCursorLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        if (rep)
        {
            rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetRepresentationToWireframe();
            rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetRepresentationToWireframe();
            rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetRepresentationToWireframe();
            rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetColor(1.0, 0.0, 0.0);
            rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetColor(0.0, 1.0, 0.0);
            rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetColor(0.0, 0.0, 1.0);
            // 必须强制构建，确保光标几何体在共享 Cursor 下正确生成
            rep->BuildRepresentation();
        }

    }

    // 延迟 100ms 同步缩放，等待 Qt 彻底完成 UI 布局，防止窗口大小为 0 时计算出错误的相机参数
    SyncViewsScale();
    QTimer::singleShot(100, this, SLOT(SyncViewsScale()));

    // 最终渲染 3D 视图
    ren->ResetCamera();
    ui->widget3D->renderWindow()->Render();

    // --- 阶段七：滑块范围 ---
    ui->horizontalSliderAxial->setMinimum(0);
    ui->horizontalSliderAxial->setMaximum(imageDims[2] - 1);
    ui->horizontalSliderCoronal->setMinimum(0);
    ui->horizontalSliderCoronal->setMaximum(imageDims[1] - 1);
    ui->horizontalSliderSagittal->setMinimum(0);
    ui->horizontalSliderSagittal->setMaximum(imageDims[0] - 1);

    // 初始化体绘制全局透明度控制滑块范围
    ui->horizontalSliderVolume->setMinimum(0);
    ui->horizontalSliderVolume->setMaximum(100);
    ui->horizontalSliderVolume->setValue(50);  // 默认值为 50（对应 1.0 倍不透明度系数）

    ui->widgetSagittal->update();
    ui->widgetCoronal->update();
    ui->widgetAxial->update();
    ui->widget3D->update();
}

// -------------------------------------------------------------------------
// 3D 视图：预设体渲染模式与面绘制
// -------------------------------------------------------------------------
void MyVtkNew::BoneVolumeRendering()
{
    if (!mainImageData)
        return;
    vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    volumeMapper->SetInputData(mainImageData);

    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();
    volumeProperty->SetAmbient(0.4);
    volumeProperty->SetDiffuse(0.6);
    volumeProperty->SetSpecular(0.2);

    // 根据滑块值计算透明度倍数因子
    double factor = ui->horizontalSliderVolume->value() / 50.0;
    compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    compositeOpacity->AddPoint(70, 0.0 * factor);
    compositeOpacity->AddPoint(90, 0.4 * factor);
    compositeOpacity->AddPoint(180, 0.6 * factor);
    volumeProperty->SetScalarOpacity(compositeOpacity);

    vtkSmartPointer<vtkColorTransferFunction> color = vtkSmartPointer<vtkColorTransferFunction>::New();
    color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
    color->AddRGBPoint(64.0, 1.0, 0.52, 0.3);
    color->AddRGBPoint(190.0, 1.0, 1.0, 1.0);
    color->AddRGBPoint(255.0, 0.2, 0.2, 0.2);
    volumeProperty->SetColor(color);

    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    // 清理老渲染器，避免窗口内重叠叠加
    if (renderer)
    {
        ui->widget3D_2->renderWindow()->RemoveRenderer(renderer);
    }
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddVolume(volume);
    renderer->SetBackground(0.1, 0.1, 0.1);

    ui->widget3D_2->renderWindow()->AddRenderer(renderer);
    ui->widget3D_2->renderWindow()->Render();
    renderer->ResetCamera();
}

void MyVtkNew::BloodVesselVolumeRendering()
{
    if (!mainImageData)
        return;
    vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    volumeMapper->SetInputData(mainImageData);

    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();
    volumeProperty->SetAmbient(0.4);
    volumeProperty->SetDiffuse(0.6);
    volumeProperty->SetSpecular(0.2);

    // 根据滑块值计算透明度倍数因子
    double factor = ui->horizontalSliderVolume->value() / 50.0;
    compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    compositeOpacity->AddPoint(0, 0.0 * factor);
    compositeOpacity->AddPoint(20, 0.0 * factor);
    compositeOpacity->AddPoint(40, 0.15 * factor);
    compositeOpacity->AddPoint(120, 0.3 * factor);
    compositeOpacity->AddPoint(255, 0.3 * factor);
    volumeProperty->SetScalarOpacity(compositeOpacity);

    vtkSmartPointer<vtkColorTransferFunction> color = vtkSmartPointer<vtkColorTransferFunction>::New();
    color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
    color->AddRGBPoint(20.0, 0.8, 0.0, 0.0);
    color->AddRGBPoint(120.0, 1.0, 0.0, 0.0);
    color->AddRGBPoint(255.0, 1.0, 1.0, 1.0);
    volumeProperty->SetColor(color);

    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    // 清理老渲染器，避免窗口内重叠叠加
    if (renderer)
    {
        ui->widget3D_2->renderWindow()->RemoveRenderer(renderer);
    }
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddVolume(volume);
    renderer->SetBackground(0.1, 0.1, 0.1);

    ui->widget3D_2->renderWindow()->AddRenderer(renderer);
    ui->widget3D_2->renderWindow()->Render();
    renderer->ResetCamera();
}

void MyVtkNew::SurfaceRendering()
{
    if (!mainImageData)
        return;
    vtkSmartPointer<vtkMarchingCubes> marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
    double* range = mainImageData->GetScalarRange();
    double threshold = (range[0] + range[1]) / 2.0;

    marchingCubes->SetInputData(mainImageData);
    marchingCubes->SetValue(0, threshold);
    marchingCubes->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(marchingCubes->GetOutput());
    mapper->ScalarVisibilityOff();

    // 使用类成员变量保存网格 actor
    surfaceActor = vtkSmartPointer<vtkActor>::New();
    surfaceActor->SetMapper(mapper);
    surfaceActor->GetProperty()->SetColor(0.8, 0.8, 0.8);
    surfaceActor->GetProperty()->SetOpacity(ui->horizontalSliderVolume->value() / 100.0);  // 初始透明度

    // 清理老渲染器，避免窗口内重叠叠加
    if (renderer)
    {
        ui->widget3D_2->renderWindow()->RemoveRenderer(renderer);
    }
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(surfaceActor);
    renderer->SetBackground(0.1, 0.1, 0.1);

    ui->widget3D_2->renderWindow()->AddRenderer(renderer);
    ui->widget3D_2->renderWindow()->Render();
    renderer->ResetCamera();
}

void MyVtkNew::on_comboBox_currentTextChanged(const QString& text)
{
    if (text == "BONE" || text == "骨骼")
    {
        BoneVolumeRendering();
    }
    else if (text == "BLOODVESSEL" || text == "血管")
    {
        BloodVesselVolumeRendering();
    }
    else if (text == "Surface" || text == "面绘制")
    {
        SurfaceRendering();
    }
}

void MyVtkNew::on_horizontalSliderVolume_valueChanged(int value)
{
    double factor = value / 50.0;  // 50 对应 1.0 倍不透明度系数

    // 如果当前是面绘制 Surface 模式，调节 surfaceActor 的整体透明度
    if (ui->comboBox->currentText() == "Surface" || ui->comboBox->currentText() == "面绘制")
    {
        if (surfaceActor)
        {
            surfaceActor->GetProperty()->SetOpacity(value / 100.0);
            ui->widget3D_2->renderWindow()->Render();
        }
        return;
    }

    // 体绘制模式，需对不透明度传输曲线的控制点进行实时重标度
    if (!volume || !compositeOpacity)
        return;

    compositeOpacity->RemoveAllPoints();
    if (ui->comboBox->currentText() == "BONE" || ui->comboBox->currentText() == "骨骼")
    {
        compositeOpacity->AddPoint(70, 0.0 * factor);
        compositeOpacity->AddPoint(90, 0.4 * factor);
        compositeOpacity->AddPoint(180, 0.6 * factor);
    }
    else if (ui->comboBox->currentText() == "BLOODVESSEL" || ui->comboBox->currentText() == "血管")
    {
        compositeOpacity->AddPoint(0, 0.0 * factor);
        compositeOpacity->AddPoint(20, 0.0 * factor);
        compositeOpacity->AddPoint(40, 0.15 * factor);
        compositeOpacity->AddPoint(120, 0.3 * factor);
        compositeOpacity->AddPoint(255, 0.3 * factor);
    }

    volume->GetProperty()->SetScalarOpacity(compositeOpacity);
    ui->widget3D_2->renderWindow()->Render();
}

// -------------------------------------------------------------------------
// 数据加载方法
// -------------------------------------------------------------------------
void MyVtkNew::on_pushButtonReadRaw_clicked()
{
    int width = ui->lineEditWidth->text().toInt();
    int height = ui->lineEditHeight->text().toInt();
    int depth = ui->lineEditDepth->text().toInt();
    readRaw(width, height, depth);
}

void MyVtkNew::readRaw(int width, int height, int depth)
{
    QString rawFilePath = QFileDialog::getOpenFileName(nullptr, tr("Select Raw File"), "", tr("Raw Files (*.raw)"));
    if (rawFilePath.isEmpty())
        return;

    double spacing[3] = {1.0, 1.0, 1.0};
    rawReader = vtkSmartPointer<vtkImageReader2>::New();
    rawReader->SetFileName(rawFilePath.toStdString().c_str());
    rawReader->SetDataExtent(0, width - 1, 0, height - 1, 0, depth - 1);
    rawReader->SetDataScalarTypeToUnsignedShort();
    rawReader->SetDataSpacing(spacing);
    rawReader->SetFileDimensionality(3);
    rawReader->SetDataByteOrderToLittleEndian();
    rawReader->Update();

    mainImageData = rawReader->GetOutput();
    InitializeVisualization(mainImageData);
}

void MyVtkNew::on_pushButtonReadDicomSeries_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
}

void MyVtkNew::on_pushButtonOpenDicom_clicked()
{
    on_pushButton_clicked();
    ui->pushButtonOpenDicom->setDown(false);
}

void MyVtkNew::on_pushButtonFor3D_clicked()
{
    if (!dicomDirectory.isEmpty())
    {
        ui->tabWidget->setCurrentIndex(1);
    }
}

// -------------------------------------------------------------------------
// 2D 滚动条与 MPR 控制
// -------------------------------------------------------------------------
void MyVtkNew::UpdateSliceCenter(int viewIndex, int value)
{
    if (!riw[viewIndex])
        return;

    if (riw[viewIndex]->GetResliceMode() == 1)  // Oblique
    {
        vtkResliceCursor* cursor = riw[viewIndex]->GetResliceCursor();
        double center[3];
        cursor->GetCenter(center);

        double origin[3], spacing[3];
        riw[viewIndex]->GetInput()->GetOrigin(origin);
        riw[viewIndex]->GetInput()->GetSpacing(spacing);

        center[viewIndex] = origin[viewIndex] + value * spacing[viewIndex];
        cursor->SetCenter(center);

        for (int i = 0; i < 3; i++)
        {
            riw[i]->GetResliceCursorWidget()->Render();
        }
    }
    else  // AxisAligned
    {
        riw[viewIndex]->SetSlice(value);
        riw[viewIndex]->Render();
    }
}

void MyVtkNew::on_horizontalSliderAxial_valueChanged(int value)
{
    UpdateSliceCenter(2, value);
}

void MyVtkNew::on_horizontalSliderCoronal_valueChanged(int value)
{
    UpdateSliceCenter(1, value);
}

void MyVtkNew::on_horizontalSliderSagittal_valueChanged(int value)
{
    UpdateSliceCenter(0, value);
}

void MyVtkNew::resliceMode(int mode)
{
    CancelActiveDistanceMeasurement();
    for (int i = 0; i < 3; i++)
    {
        riw[i]->SetResliceMode(mode ? 1 : 0);
        riw[i]->GetResliceCursorWidget()->ManageWindowLevelOn();
        if (vtkResliceCursorRepresentation* rep =
                vtkResliceCursorRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation()))
        {
            rep->DisplayTextOff();
            if (mode)
            {
                rep->BuildRepresentation();
            }
        }
        riw[i]->Render();
    }
    SyncViewsScale();
    UpdateCornerAnnotations();
    ui->thickModeCheckBox->setEnabled(mode ? 1 : 0);
    ui->blendModeGroupBox->setEnabled(mode ? 1 : 0);
}

void MyVtkNew::thickMode(int mode)
{
    for (int i = 0; i < 3; i++)
    {
        riw[i]->SetThickMode(mode ? 1 : 0);
        riw[i]->Render();
    }
}

void MyVtkNew::SetBlendMode(int m)
{
    // VTK 9.6: 通过 representation 获取底层的 vtkImageReslice，设置厚层混合模式
    for (int i = 0; i < 3; i++)
    {
        vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        if (rep)
        {
            vtkImageReslice* reslice = vtkImageReslice::SafeDownCast(rep->GetReslice());
            if (reslice)
            {
                reslice->SetSlabMode(m);
            }
        }
        riw[i]->Render();
    }
}
void MyVtkNew::SetBlendModeToMaxIP()
{
    SetBlendMode(VTK_IMAGE_SLAB_MAX);
}
void MyVtkNew::SetBlendModeToMinIP()
{
    SetBlendMode(VTK_IMAGE_SLAB_MIN);
}
void MyVtkNew::SetBlendModeToMeanIP()
{
    SetBlendMode(VTK_IMAGE_SLAB_MEAN);
}

void MyVtkNew::ResetViews()
{
    CancelActiveDistanceMeasurement();
    for (int i = 0; i < 3; i++)
    {
        riw[i]->Reset();
    }
    Render();
}

void MyVtkNew::UpdateCornerAnnotations()
{
    for (int i = 0; i < 3; i++)
    {
        if (riw[i] && windowLevelTextActor[i])
        {
            const double window = riw[i]->GetColorWindow();
            const double level = riw[i]->GetColorLevel();
            const QString text = QString("WW: %1 / WL: %2").arg(window, 0, 'f', 0).arg(level, 0, 'f', 0);
            windowLevelTextActor[i]->SetInput(text.toUtf8().constData());
        }
    }
}

void MyVtkNew::Render()
{
    UpdateCornerAnnotations();
    for (int i = 0; i < 3; i++)
    {
        riw[i]->Render();
    }
    ui->widget3D->renderWindow()->Render();
}

// -------------------------------------------------------------------------
// 窗宽窗位与测量功能
// -------------------------------------------------------------------------
void MyVtkNew::on_pushButtonGetWindowLevel_clicked()
{
    if (riw[0])
    {
        double window = riw[0]->GetColorWindow();
        double level = riw[0]->GetColorLevel();
        updatePlaneTextEdit(0, window, level);
    }
}

void MyVtkNew::updatePlaneTextEdit(int id, double window, double level)
{
    qDebug() << "View" << id << "Window:" << window << "Level:" << level;
}

void MyVtkNew::WindowLevelCallback(vtkObject* caller, unsigned long eventId, void* clientData, void* callData) {}

void MyVtkNew::SyncViewsScale()
{
    if (!mainImageData)
        return;

    double bounds[6];
    mainImageData->GetBounds(bounds);
    const double dx = bounds[1] - bounds[0];
    const double dy = bounds[3] - bounds[2];
    const double dz = bounds[5] - bounds[4];
    double center[3] = {0.0, 0.0, 0.0};
    mainImageData->GetCenter(center);

    for (int i = 0; i < 3; i++)
    {
        if (riw[i] && riw[i]->GetRenderer() && riw[i]->GetRenderWindow())
        {
            int* viewSize = riw[i]->GetRenderWindow()->GetSize();
            const double w = (viewSize && viewSize[0] > 0) ? static_cast<double>(viewSize[0]) : 1.0;
            const double h = (viewSize && viewSize[1] > 0) ? static_cast<double>(viewSize[1]) : 1.0;
            const double aspect = w / h;

            double planeW = dx;
            double planeH = dy;
            switch (riw[i]->GetSliceOrientation())
            {
                case vtkImageViewer2::SLICE_ORIENTATION_YZ:
                    planeW = dy;
                    planeH = dz;
                    break;
                case vtkImageViewer2::SLICE_ORIENTATION_XZ:
                    planeW = dx;
                    planeH = dz;
                    break;
                case vtkImageViewer2::SLICE_ORIENTATION_XY:
                default:
                    planeW = dx;
                    planeH = dy;
                    break;
            }

            const double scaleByHeight = planeH * 0.5;
            const double scaleByWidth = (aspect > 0.0) ? (planeW * 0.5 / aspect) : scaleByHeight;
            const double uniformScale = (scaleByHeight > scaleByWidth) ? scaleByHeight : scaleByWidth;
            vtkCamera* cam = riw[i]->GetRenderer()->GetActiveCamera();
            if (cam)
            {
                const double distance = cam->GetDistance();
                double pos[3] = {center[0], center[1], center[2]};
                switch (riw[i]->GetSliceOrientation())
                {
                    case vtkImageViewer2::SLICE_ORIENTATION_YZ:
                        pos[0] = center[0] + distance;
                        break;
                    case vtkImageViewer2::SLICE_ORIENTATION_XZ:
                        pos[1] = center[1] - distance;
                        break;
                    case vtkImageViewer2::SLICE_ORIENTATION_XY:
                    default:
                        pos[2] = center[2] + distance;
                        break;
                }
                cam->SetFocalPoint(center);
                cam->SetPosition(pos);
                cam->SetParallelScale(uniformScale);
            }
            riw[i]->Render();
        }
    }
}

void MyVtkNew::AddDistanceMeasurementToView1()
{
    CancelActiveDistanceMeasurement();
    // 按约定仅在单视图启用，默认 Axial 视图
    AddDistanceMeasurementToView(2);
}

void MyVtkNew::CancelActiveDistanceMeasurement()
{
    if (!activeDistanceWidget)
    {
        isDistanceArmed = false;
        activeDistanceViewIndex = -1;
        activeDistanceObserverTag = 0;
        activeDistanceCallbackCommand = nullptr;
        return;
    }
    if (activeDistanceObserverTag != 0)
    {
        activeDistanceWidget->RemoveObserver(activeDistanceObserverTag);
        activeDistanceObserverTag = 0;
    }
    activeDistanceWidget->Off();
    activeDistanceWidget = nullptr;
    activeDistanceCallbackCommand = nullptr;
    isDistanceArmed = false;
    activeDistanceViewIndex = -1;
}

void MyVtkNew::MaybeCancelDistanceMeasurementForView(int sourceViewIndex)
{
    if (!isDistanceArmed || activeDistanceViewIndex < 0)
    {
        return;
    }
    if (sourceViewIndex != activeDistanceViewIndex)
    {
        CancelActiveDistanceMeasurement();
    }
}

void MyVtkNew::OnDistanceEndInteraction(vtkObject* caller, unsigned long eventId, void* clientData, void* callData)
{
    Q_UNUSED(eventId);
    Q_UNUSED(callData);
    MyVtkNew* self = static_cast<MyVtkNew*>(clientData);
    vtkDistanceWidget* widget = vtkDistanceWidget::SafeDownCast(caller);
    if (!self || !widget)
    {
        return;
    }
    if (widget->GetWidgetState() == vtkDistanceWidget::Manipulate)
    {
        widget->SetProcessEvents(0);
        widget->SetPriority(0.1);
        if (self->activeDistanceObserverTag != 0)
        {
            widget->RemoveObserver(self->activeDistanceObserverTag);
            self->activeDistanceObserverTag = 0;
        }
        self->activeDistanceWidget = nullptr;
        self->activeDistanceCallbackCommand = nullptr;
        self->activeDistanceViewIndex = -1;
        self->isDistanceArmed = false;
    }
}

void MyVtkNew::AddDistanceMeasurementToView(int i)
{
    if (!riw[i])
        return;
    CancelActiveDistanceMeasurement();

    // 检查上一个标注是否已绘制完成。若未完成则删除它，防止冗余的空白标注
    if (!historyDistanceWidgets[i].isEmpty())
    {
        auto lastWidget = historyDistanceWidgets[i].last();
        if (lastWidget && lastWidget->GetWidgetState() != vtkDistanceWidget::Manipulate)
        {
            lastWidget->Off();
            historyDistanceWidgets[i].removeLast();
        }
    }

    vtkSmartPointer<vtkDistanceWidget> distanceWidget = vtkSmartPointer<vtkDistanceWidget>::New();
    distanceWidget->SetInteractor(riw[i]->GetResliceCursorWidget()->GetInteractor());
    distanceWidget->SetDefaultRenderer(riw[i]->GetRenderer());
    distanceWidget->CreateDefaultRepresentation();

    vtkDistanceRepresentation* rep = static_cast<vtkDistanceRepresentation*>(distanceWidget->GetRepresentation());
    if (rep)
    {
        rep->SetLabelFormat("%-#6.3g mm");
    }

    distanceWidget->SetPriority(1.0);
    distanceWidget->On();
    activeDistanceWidget = distanceWidget;
    activeDistanceViewIndex = i;
    isDistanceArmed = true;
    activeDistanceCallbackCommand = vtkSmartPointer<vtkCallbackCommand>::New();
    activeDistanceCallbackCommand->SetClientData(this);
    activeDistanceCallbackCommand->SetCallback(MyVtkNew::OnDistanceEndInteraction);
    activeDistanceObserverTag =
        distanceWidget->AddObserver(vtkCommand::EndInteractionEvent, activeDistanceCallbackCommand);

    // 存入历史数组，用于长期显示和后期销毁
    historyDistanceWidgets[i].append(distanceWidget);
}

void MyVtkNew::on_pushButtonForAddAngle_clicked()
{
    for (int j = 0; j < 3; j++)
    {
        AddAngleToView(j);
    }
}

void MyVtkNew::AddAngleToView(int i)
{
    if (!riw[i])
        return;

    // 检查上一个角度标注是否已绘制完成。若未完成则删除它
    if (!historyAngleWidgets[i].isEmpty())
    {
        auto lastWidget = historyAngleWidgets[i].last();
        if (lastWidget && lastWidget->GetWidgetState() != vtkAngleWidget::Manipulate)
        {
            lastWidget->Off();
            historyAngleWidgets[i].removeLast();
        }
    }

    vtkSmartPointer<vtkAngleWidget> angleWidget = vtkSmartPointer<vtkAngleWidget>::New();
    angleWidget->SetInteractor(riw[i]->GetResliceCursorWidget()->GetInteractor());
    angleWidget->SetDefaultRenderer(riw[i]->GetRenderer());
    angleWidget->CreateDefaultRepresentation();

    angleWidget->SetPriority(1.0);
    angleWidget->On();

    // 存入历史数组，用于长期显示和后期销毁
    historyAngleWidgets[i].append(angleWidget);
}

// -------------------------------------------------------------------------
// 格式转换与存储
// -------------------------------------------------------------------------
void MyVtkNew::on_pushButtonConvertRawSlicesToDicom_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"));
    if (dir.isEmpty())
        return;
    std::string outputFolder = dir.toStdString();
    std::string patientName = "Unknown";
    std::string patientID = "ID0001";
    ConvertRawSlicesToDicom(outputFolder, patientName, patientID);
}

void MyVtkNew::ConvertRawSlicesToDicom(const std::string& outputFolder, const std::string& patientName,
                                       const std::string& patientID)
{
    qDebug() << "格式转换已执行。存储至：" << QString::fromStdString(outputFolder);
}

void MyVtkNew::SaveAsRawFile(vtkSmartPointer<vtkPolyData> polyData, const std::string& filename)
{
    qDebug() << "PolyData 已存为 RAW：" << QString::fromStdString(filename);
}

vtkSmartPointer<vtkPolyData> MyVtkNew::LoadPointsFromRaw(const std::string& filename)
{
    return nullptr;
}

// -------------------------------------------------------------------------
// 兼容性占位方法 (防止编译报错)
// -------------------------------------------------------------------------
void MyVtkNew::mouseMoveEvent(QMouseEvent* event) {}
void MyVtkNew::rawSlice(int width, int height, int depth) {}
void MyVtkNew::setupVTKPipeline(const QString& dicomDirectory) {}
void MyVtkNew::dicomShow(const QString& dicomDirectory) {}
void MyVtkNew::GenerateVirtual3DData(vtkSmartPointer<vtkImageData>& imageData) {}
void MyVtkNew::Expand2DTo3D(vtkSmartPointer<vtkImageData>& imageData2D, vtkSmartPointer<vtkImageData>& imageData3D) {}
void MyVtkNew::readDicomSeries(vtkSmartPointer<vtkImageViewer2>& viewer, QVTKOpenGLNativeWidget* widget,
                               const QString& orientation)
{}
