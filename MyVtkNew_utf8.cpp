#include "MyVtkNew.h"

#include "CustomInteractorStyle.h"
#include "ResliceInteractorStyle .h"
#include "dcmtk/config/osconfig.h"  // make sure OS specific configuration is included
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"  // dcmGenerateUniqueIdentifier
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

#include <QFileDialog>
#include <QPainter>
#include <QVBoxLayout>
#include <QVTKOpenGLNativeWidget.h>
#include <iostream>
#include <qmessagebox.h>
#include <vtkActor2D.h>
#include <vtkAngleRepresentation2D.h>
#include <vtkAutoInit.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourFilter.h>
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

#pragma execution_character_set("utf-8")

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType)
class vtkResliceCursorCallback : public vtkCommand
{
public:
    static vtkResliceCursorCallback* New() { return new vtkResliceCursorCallback; }
    void Execute(vtkObject* caller, unsigned long ev, void* callData) override
    {
        if (ev == vtkResliceCursorWidget::WindowLevelEvent || ev == vtkCommand::WindowLevelEvent ||
            ev == vtkResliceCursorWidget::ResliceThicknessChangedEvent)
        {
            for (int i = 0; i < 3; i++)
            {
                this->RCW[i]->Render();
            }
            this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
            return;  // 杩欓噷闇€瑕佹敞閲婃帀锛屼笉鐒跺懙鍛靛懙锛岃繑鍥炰簡锛岃繕鎼炰釜姣涚嚎
        }
        vtkImagePlaneWidget* ipw = dynamic_cast<vtkImagePlaneWidget*>(caller);
        if (ipw)
        {
            double* wl = static_cast<double*>(callData);

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
                // If the reslice plane has modified, update it on the 3D widget
                this->IPW[i]->UpdatePlacement();
            }
        }
        for (int i = 0; i < 3; i++)
        {
            this->RCW[i]->Render();
        }
        this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
    }

    vtkResliceCursorCallback() {}
    vtkImagePlaneWidget* IPW[3];
    vtkResliceCursorWidget* RCW[3];
};

MyVtkNew::MyVtkNew(QWidget* parent) : QMainWindow(parent)
{
    ui = new Ui::MyVtkNew();

    ui->setupUi(this);

    // 鑾峰彇 vtkRenderWindow
    // auto renderWindow = ui->centralWidget->GetRenderWindow();
    // renderWindow->AddRenderer(renderer);
}

MyVtkNew::~MyVtkNew() {}

void MyVtkNew::resizeEvent(QResizeEvent* event)
{
    ui->widgetAxial->renderWindow()->Render();     // 閲嶆柊娓叉煋
    ui->widgetCoronal->renderWindow()->Render();   // 閲嶆柊娓叉煋
    ui->widgetSagittal->renderWindow()->Render();  // 閲嶆柊娓叉煋
    ui->widget3D->renderWindow()->Render();        // 閲嶆柊娓叉煋
}

void MyVtkNew::paintEvent(QPaintEvent* event)
{
    // Q_UNUSED(event)
    // QPainter p(this);
    // p.setPen(Qt::NoPen);
    // p.setBrush(Qt::black);
    // p.drawRect(rect());
}

void MyVtkNew::on_pushButtonFor3D_clicked()
{
    setupVTKPipeline(dicomDirectory);
}

void MyVtkNew::on_pushButtonReadDicomSeries_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
    rawSlice(700, 700, 600);
}

void MyVtkNew::on_pushButtonReadRaw_clicked()
{
    int width = 0;
    int height = 0;
    int depth = 0;

    width = ui->lineEditWidth->text().toInt();
    height = ui->lineEditHeight->text().toInt();
    depth = ui->lineEditDepth->text().toInt();

    readRaw(width, height, depth);
}

void MyVtkNew::on_horizontalSliderAxial_valueChanged(int value)
{
    // int numberOfSlices = dicomReader->GetOutput()->GetDimensions()[2];  // 鑾峰彇鍒囩墖鎬绘暟
    int numberOfSlices = rawReader->GetOutput()->GetDimensions()[2];  // 鑾峰彇鍒囩墖鎬绘暟

    if (value < 0)
        value = 0;
    if (value >= numberOfSlices)
        value = numberOfSlices - 1;

    riw[0]->SetSlice(value);
    riw[0]->Render();  // 閲嶆柊娓叉煋瑙嗗浘
}

void MyVtkNew::on_horizontalSliderCoronal_valueChanged(int value)
{
    int numberOfSlices = rawReader->GetOutput()->GetDimensions()[2];  // 鑾峰彇鍒囩墖鎬绘暟
    if (value < 0)
        value = 0;
    if (value >= numberOfSlices)
        value = numberOfSlices - 1;

    riw[1]->SetSlice(value);
    riw[1]->Render();  // 閲嶆柊娓叉煋瑙嗗浘
}

void MyVtkNew::on_horizontalSliderSagittal_valueChanged(int value)
{
    int numberOfSlices = rawReader->GetOutput()->GetDimensions()[2];  // 鑾峰彇鍒囩墖鎬绘暟
    if (value < 0)
        value = 0;
    if (value >= numberOfSlices)
        value = numberOfSlices - 1;

    riw[2]->SetSlice(value);
    riw[2]->Render();  // 閲嶆柊娓叉煋瑙嗗浘
}

void MyVtkNew::on_horizontalSliderVolume_valueChanged(int value)
{
    double doubleValue = value / 20.0;
    vtkNew<vtkPiecewiseFunction> compositeOpacity;
    compositeOpacity->AddPoint(-3024, 0, 0.5, 0.0);
    compositeOpacity->AddPoint(-16, 0, .49, .61);
    compositeOpacity->AddPoint(641, doubleValue, .5, 0.0);
    compositeOpacity->AddPoint(3071, doubleValue, 0.5, 0.0);

    volume->GetProperty()->SetScalarOpacity(compositeOpacity);
    ui->widget3D_2->interactor()->Render();
}

void MyVtkNew::on_pushButtonGetWindowLevel_clicked()
{
    double window = vtkAxial->GetColorWindow();
    double level = vtkAxial->GetColorLevel();
    updatePlaneTextEdit(0, window, level);
    double window2 = vtkCoronal->GetColorWindow();
    double level2 = vtkCoronal->GetColorLevel();
    updatePlaneTextEdit(1, window2, level2);
    double window3 = vtkSagittal->GetColorWindow();
    double level3 = vtkSagittal->GetColorLevel();
    updatePlaneTextEdit(2, window3, level3);
}

void MyVtkNew::on_pushButton_clicked()
{
    dicomDirectory = QFileDialog::getExistingDirectory(this, tr("Select DICOM Folder"));
    dicomReader = vtkSmartPointer<vtkDICOMImageReader>::New();
    dicomReader->SetDirectoryName(dicomDirectory.toStdString().c_str());
    dicomReader->Update();
    dicomReader->GetOutput()->GetDimensions(imageDims);

    double range[2];
    dicomReader->GetOutput()->GetScalarRange(range);  // 鑾峰彇鐏板害鑼冨洿
    QString currentDir = QDir::currentPath();
    QString fontPath = currentDir + "/Fonts/simhei.ttf";

    // 杞崲涓?C 椋庢牸瀛楃涓?
    const char* fontPathCStr = fontPath.toUtf8().constData();

    for (auto i = 0; i < 4; i++)
    {
        textActor[i] = vtkSmartPointer<vtkTextActor>::New();
        textActor[i]->SetDisplayPosition(5, 5);
        textActor[i]->GetTextProperty()->SetFontSize(18);
        textActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        textActor[i]->GetTextProperty()->SetFontFile(fontPathCStr);
        qDebug() << "Font file: " << textActor[i]->GetTextProperty()->GetFontFile();
    }
    textActor[0]->SetInput(QString::fromUtf8("鐭㈢姸").toUtf8());
    textActor[0]->GetTextProperty()->SetColor(0, 1, 0);

    textActor[1]->SetInput(QString::fromUtf8("Coronal").toUtf8());  // 鍐犵姸
    textActor[1]->GetTextProperty()->SetColor(0, 0, 1);

    textActor[2]->SetInput(QString::fromUtf8("Axial").toUtf8());  // 妯埅闈?
    textActor[2]->GetTextProperty()->SetColor(1, 0, 0);

    textActor[3]->SetInput(QString::fromUtf8("3D").toUtf8());
    textActor[3]->GetTextProperty()->SetColor(1, 1, 0);

    for (auto i = 0; i < 4; i++)
    {
        peopleInforTextActor[i] = vtkSmartPointer<vtkTextActor>::New();
        peopleInforTextActor[i]->GetTextProperty()->SetFontSize(14);
        peopleInforTextActor[i]->GetTextProperty()->SetFontFile(fontPathCStr);
        peopleInforTextActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        peopleInforTextActor[i]->SetInput(QString::fromUtf8("鎮ｈ€呭鍚嶏細").toUtf8() + dicomReader->GetPatientName() +
                                          "\r\n" + "UID:" + dicomReader->GetStudyUID());
    }
    peopleInforTextActor[0]->GetTextProperty()->SetColor(0, 1, 0);
    peopleInforTextActor[0]->SetDisplayPosition(5, ui->widgetAxial->height() - 40);
    peopleInforTextActor[1]->GetTextProperty()->SetColor(0, 0, 1);
    peopleInforTextActor[1]->SetDisplayPosition(5, ui->widgetCoronal->height() - 40);
    peopleInforTextActor[2]->GetTextProperty()->SetColor(1, 0, 0);
    peopleInforTextActor[2]->SetDisplayPosition(5, ui->widgetSagittal->height() - 40);
    peopleInforTextActor[3]->GetTextProperty()->SetColor(1, 1, 0);
    peopleInforTextActor[3]->SetDisplayPosition(5, ui->widget3D->height() - 40);

    for (auto i = 0; i < 3; i++)
    {
        riw[i] = vtkSmartPointer<MyVtkResliceImageViewer>::New();
        riw[i]->GetRenderer()->AddActor(textActor[i]);
        riw[i]->GetRenderer()->AddActor(peopleInforTextActor[i]);
    }

    riw[0]->SetRenderWindow(ui->widgetCoronal->renderWindow());
    riw[0]->SetupInteractor(ui->widgetCoronal->renderWindow()->GetInteractor());

    riw[1]->SetRenderWindow(ui->widgetSagittal->renderWindow());
    riw[1]->SetupInteractor(ui->widgetSagittal->renderWindow()->GetInteractor());

    riw[2]->SetRenderWindow(ui->widgetAxial->renderWindow());
    riw[2]->SetupInteractor(ui->widgetAxial->renderWindow()->GetInteractor());

    picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.005);
    ipwProp = vtkSmartPointer<vtkProperty>::New();
    ren = vtkSmartPointer<vtkRenderer>::New();
    ren->AddActor(textActor[3]);
    ren->AddActor(peopleInforTextActor[3]);
    ui->widget3D->renderWindow()->AddRenderer(ren);
    for (auto i = 0; i < 3; i++)
    {
        planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
        planeWidget[i]->SetInteractor(ui->widget3D->interactor());
        planeWidget[i]->SetPicker(picker);
        planeWidget[i]->RestrictPlaneToVolumeOn();
        double color[3] = {0, 0, 0};
        color[i] = 1;
        planeWidget[i]->GetPlaneProperty()->SetColor(color);
        color[0] = 0;
        color[1] = 0;
        color[2] = 0;
        riw[i]->GetRenderer()->SetBackground(color);
        planeWidget[i]->SetTexturePlaneProperty(ipwProp);
        planeWidget[i]->TextureInterpolateOff();
        planeWidget[i]->SetResliceInterpolateToLinear();
        planeWidget[i]->SetInputConnection(dicomReader->GetOutputPort());
        planeWidget[i]->SetPlaneOrientation(i);
        planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
        planeWidget[i]->DisplayTextOn();
        planeWidget[i]->SetDefaultRenderer(ren);
        planeWidget[i]->SetWindowLevel(range[0], range[1]);
        planeWidget[i]->On();
        planeWidget[i]->InteractionOn();
    }
    planeWidget[1]->SetLookupTable(planeWidget[0]->GetLookupTable());
    planeWidget[2]->SetLookupTable(planeWidget[0]->GetLookupTable());

    cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();

    for (auto i = 0; i < 3; i++)
    {
        cbk->IPW[i] = planeWidget[i];
        cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, cbk);
        riw[i]->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, cbk);
        // Make them all share the same color map.
        riw[i]->SetLookupTable(riw[0]->GetLookupTable());
        planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
        // planeWidget[i]->GetColorMap()->SetInput(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
        planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
    }

    for (int i = 0; i < 3; i++)
    {
        vtkResliceCursorLineRepresentation* rep =
            vtkResliceCursorLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());
        rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i);
        rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(riw[0]->GetResliceCursor());

        riw[i]->GetResliceCursorWidget()->On();
        riw[i]->SetInputData(dicomReader->GetOutput());
        riw[i]->SetSliceOrientation(i);
        riw[i]->SetResliceModeToAxisAligned();

        riw[i]->Render();
    }

    for (auto i = 0; i < 3; i++)
    {
        riw[i]->SetThickMode(1);
        riw[i]->Render();
    }
    ui->widgetCoronal->update();
    ui->widgetSagittal->update();
    ui->widgetAxial->update();
    ui->widget3D->update();

    connect(this->ui->resliceModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(resliceMode(int)));
    connect(this->ui->thickModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(thickMode(int)));
    this->ui->thickModeCheckBox->setEnabled(0);

    connect(this->ui->radioButton_Max, SIGNAL(pressed()), this, SLOT(SetBlendModeToMaxIP()));
    connect(this->ui->radioButton_Min, SIGNAL(pressed()), this, SLOT(SetBlendModeToMinIP()));
    connect(this->ui->radioButton_Mean, SIGNAL(pressed()), this, SLOT(SetBlendModeToMeanIP()));
    this->ui->blendModeGroupBox->setEnabled(0);

    connect(this->ui->resetButton, SIGNAL(pressed()), this, SLOT(ResetViews()));
    connect(this->ui->AddDistance1Button, SIGNAL(pressed()), this, SLOT(AddDistanceMeasurementToView1()));
}

void MyVtkNew::on_comboBox_currentTextChanged(const QString& text)
{
    if (text == "BONE")
    {
        BoneVolumeRendering();
    }
    else if (text == "BLOODVESSEL")
    {
        BloodVesselVolumeRendering();
    }
    else if (text == "Surface")
    {
        SurfaceRendering();
    }
}

void MyVtkNew::on_pushButtonConvertRawSlicesToDicom_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"),
                                                    QString(),                 // 榛樿璧峰鐩綍
                                                    QFileDialog::ShowDirsOnly  // 浠呮樉绀烘枃浠跺す
    );
    if (dir.isEmpty())
    {
        return;
    }

    // 灏哘String杞崲涓簊td::string
    std::string outputFolder = dir.toStdString();
    std::string patientName = "鍤ｅ紶";
    std::string patientID = "PID20250401020252616";

    ConvertRawSlicesToDicom(outputFolder, patientName, patientID);
}

void MyVtkNew::on_pushButtonForAddAngle_clicked()
{
    AddAngleToView(1);
}

void MyVtkNew::on_pushButtonOpenDicom_clicked()
{
    // 鍔犺浇 DICOM 鏂囦欢澶?
    dicomDirectory = QFileDialog::getExistingDirectory(this, tr("Select DICOM Folder"));
    if (!dicomDirectory.isEmpty())
    {
        // setupVTKPipeline(dicomDirectory);
        // dicomShow(dicomDirectory);

        dicomReader = vtkSmartPointer<vtkDICOMImageReader>::New();
        dicomReader->SetDirectoryName(dicomDirectory.toStdString().c_str());
        dicomReader->Update();

        vtkSmartPointer<vtkImageViewer2> vtkView = vtkSmartPointer<vtkImageViewer2>::New();
        vtkAxial = vtkView;
        vtkView->SetInputConnection(dicomReader->GetOutputPort());
        vtkSmartPointer<vtkRenderWindowInteractor> renderWindow = vtkSmartPointer<vtkRenderWindowInteractor>::New();
        vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
        renderWindow->SetInteractorStyle(style);
        vtkView->SetSlice(0);
        vtkView->SetSliceOrientationToXZ();
        vtkView->SetRenderWindow(ui->widgetAxial->renderWindow());
        vtkView->SetupInteractor(renderWindow);
        vtkView->Render();
        vtkView->GetRenderer()->SetBackground(0.0, 0.0, 0.0);
        vtkView->GetRenderWindow()->SetWindowName("readSeries");

        vtkSmartPointer<vtkImageViewer2> vtkView2 = vtkSmartPointer<vtkImageViewer2>::New();
        vtkView2->SetInputConnection(dicomReader->GetOutputPort());
        vtkCoronal = vtkView2;
        vtkSmartPointer<vtkRenderWindowInteractor> renderWindow2 = vtkSmartPointer<vtkRenderWindowInteractor>::New();
        vtkSmartPointer<vtkInteractorStyleImage> style2 = vtkSmartPointer<vtkInteractorStyleImage>::New();
        renderWindow2->SetInteractorStyle(style2);
        vtkView2->SetSlice(0);
        vtkView2->SetSliceOrientationToXY();
        vtkView2->SetRenderWindow(ui->widgetCoronal->renderWindow());
        vtkView2->SetupInteractor(renderWindow2);
        vtkView2->Render();
        vtkView2->GetRenderer()->SetBackground(0.0, 0.0, 0.0);

        vtkView2->GetRenderWindow()->SetWindowName("readSeries");

        vtkSmartPointer<vtkImageViewer2> vtkView3 = vtkSmartPointer<vtkImageViewer2>::New();
        vtkView3->SetInputConnection(dicomReader->GetOutputPort());
        vtkSagittal = vtkView3;
        vtkSmartPointer<vtkRenderWindowInteractor> renderWindow3 = vtkSmartPointer<vtkRenderWindowInteractor>::New();
        vtkSmartPointer<vtkInteractorStyleImage> style3 = vtkSmartPointer<vtkInteractorStyleImage>::New();
        renderWindow2->SetInteractorStyle(style3);
        vtkView3->SetSlice(0);
        vtkView3->SetSliceOrientationToYZ();
        vtkView3->SetRenderWindow(ui->widgetSagittal->renderWindow());
        vtkView3->SetupInteractor(renderWindow3);
        vtkView3->Render();
        vtkView3->GetRenderer()->SetBackground(0.0, 0.0, 0.0);
        vtkView3->GetRenderWindow()->SetWindowName("readSeries");

        renderWindow->Start();
        renderWindow2->Start();
        renderWindow3->Start();
    }
}

void MyVtkNew::setupVTKPipeline(const QString& dicomDirectory)
{  // 璇诲彇 DICOM 鍥惧儚
    dicomReader = vtkSmartPointer<vtkDICOMImageReader>::New();
    dicomReader->SetDirectoryName(dicomDirectory.toStdString().c_str());
    dicomReader->Update();
    if (dicomReader->GetOutput()->GetNumberOfPoints() == 0)
    {
        std::cerr << "Error: Failed to load DICOM data!" << std::endl;
    }
    else
    {
        std::cout << "DICOM data loaded successfully." << std::endl;
    }
    vtkSmartPointer<vtkImageData> imageData3D = dicomReader->GetOutput();

    int* dims = imageData3D->GetDimensions();
    std::cout << "Loaded dimensions: " << dims[0] << " x " << dims[1] << " x " << dims[2] << std::endl;

    if (dims[2] <= 1)
    {
        std::cerr << "Error: DICOM data is not 3D!" << std::endl;
        return;
    }

    // 2. 浣跨敤 Marching Cubes 绠楁硶杩涜涓夌淮閲嶅缓
    vtkSmartPointer<vtkMarchingCubes> marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();

    // 鑾峰彇鍥惧儚鏁版嵁鐨勬爣閲忚寖鍥村苟璁剧疆闃堝€?
    double* range = imageData3D->GetScalarRange();
    std::cout << "Scalar range: " << range[0] << " - " << range[1] << std::endl;

    // 浣跨敤涓€间綔涓洪槇鍊?
    double threshold = (range[0] + range[1]) / 2.0;
    marchingCubes->SetInputData(imageData3D);
    marchingCubes->SetValue(0, threshold);
    marchingCubes->Update();

    vtkSmartPointer<vtkPolyData> polyData = marchingCubes->GetOutput();
    // QString filePath = QFileDialog::getSaveFileName(nullptr, "Save Raw File", QString(), "*.raw");

    // if (filePath.isEmpty()) {
    //     std::cerr << "Error: No file selected for saving." << std::endl;
    //     return;
    // }

    // 灏嗛€夋嫨鐨勮矾寰勮浆鎹负 std::string 绫诲瀷
    // std::string rawFileName = filePath.toStdString();

    // 4. 灏?PolyData 涓殑鐐规暟鎹繚瀛樹负 .raw 鏂囦欢
    // SaveAsRawFile(polyData, rawFileName);

    // 3. 鍒涘缓 Mapper 鍜?Actor
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(marchingCubes->GetOutput());
    mapper->ScalarVisibilityOff();  // 绂佺敤棰滆壊鏄犲皠

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 1.0);  // 璁剧疆棰滆壊涓虹櫧鑹?

    // 4. 鍒涘缓 Renderer 鍜?RenderWindow
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);
    renderer->SetBackground(0.1, 0.1, 0.1);  // 璁剧疆鑳屾櫙棰滆壊涓烘繁鐏拌壊

    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(800, 600);

    // 5. 鍒涘缓浜や簰鎺т欢
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    // 6. 鍚姩娓叉煋鍜屼氦浜?
    renderWindow->Render();

    ui->widget3D_2->setRenderWindow(renderWindow);
    renderer->ResetCamera();
    ui->widget3D_2->update();  // 寮哄埗鏇存柊鎺т欢
    renderWindowInteractor->Start();

    return;
}

void MyVtkNew::readDicomSeries(vtkSmartPointer<vtkImageViewer2>& viewer, QVTKOpenGLNativeWidget* widget,
                               const QString& orientation)
{
    // vtkSmartPointer<vtkRenderWindow> renderWindow = widget->renderWindow();

    //// 鑾峰彇 ImageViewer2 鐨勬覆鏌撳櫒锛屼笉闇€瑕佹柊寤烘覆鏌撳櫒
    // vtkSmartPointer<vtkRenderer> renderer = viewer->GetRenderer();  // 浣跨敤 viewer 榛樿鐨勬覆鏌撳櫒
    // renderer->SetBackground(0.0, 0.0, 0.0);  // 璁剧疆鑳屾櫙涓洪粦鑹?
    // renderWindow->AddRenderer(renderer);  // 灏嗘覆鏌撳櫒娣诲姞鍒?RenderWindow

    //// 璁剧疆 ImageViewer2 鐨勮緭鍏ュ拰娓叉煋绐楀彛
    // vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = renderWindow->GetInteractor();
    // viewer->SetupInteractor(renderWindowInteractor);  // 缁戝畾浜や簰鍣?

    // viewer->SetRenderWindow(renderWindow);

    //// 璁剧疆鍒囩墖鏂瑰悜
    // if (orientation == "Axial") {
    //     viewer->SetSliceOrientationToXY(); // Axial 鏂瑰悜
    // }
    // else if (orientation == "Coronal") {
    //     viewer->SetSliceOrientationToXZ(); // Coronal 鏂瑰悜
    // }
    // else if (orientation == "Sagittal") {
    //     viewer->SetSliceOrientationToYZ(); // Sagittal 鏂瑰悜
    // }
}

vtkSmartPointer<vtkPolyData> MyVtkNew::LoadPointsFromRaw(const std::string& filename)
{
    // 鍒涘缓 PolyData 瀵硅薄
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    // 鎵撳紑 .raw 鏂囦欢
    std::ifstream rawFile(filename, std::ios::binary);
    if (!rawFile)
    {
        qDebug() << "Error: Failed to open raw file for reading.";
        return nullptr;
    }

    // 璇诲彇姣忎釜鐐圭殑鍧愭爣鏁版嵁锛堝亣璁炬瘡涓偣鐢变笁涓?double 绫诲瀷鐨勫潗鏍囩粍鎴愶級
    double point[3];
    while (rawFile.read(reinterpret_cast<char*>(point), 3 * sizeof(double)))
    {
        points->InsertNextPoint(point);
    }

    // 灏嗙偣鏁版嵁娣诲姞鍒?PolyData 涓?
    polyData->SetPoints(points);

    return polyData;
}

void MyVtkNew::readRaw(int width, int height, int depth)
{
    // 1. 閫夋嫨 raw 鏂囦欢锛堟暟鎹被鍨嬩负 unsigned short锛?
    // 閫夋嫨鏂囦欢璺緞
    QString rawFilePath =
        QFileDialog::getOpenFileName(nullptr, QObject::tr("Select Raw File"), "", QObject::tr("Raw Files (*.raw)"));
    if (rawFilePath.isEmpty())
    {
        qDebug() << "No file selected!";
        return;
    }

    // 鍋囪鎴戜滑鐭ラ亾鍥惧儚鐨勫昂瀵搞€佸儚绱犻棿璺濆拰鏁版嵁绫诲瀷

    double spacing[3] = {0.25, 0.25, 0.25};  // 鍋囪姣忎釜鍍忕礌鐨勯棿璺濅负 1.0mm

    // 璇诲彇鍘熷 .raw 鏂囦欢
    raw = vtkSmartPointer<vtkImageReader2>::New();
    raw->SetFileName(rawFilePath.toStdString().c_str());
    raw->SetDataExtent(0, width - 1, 0, height - 1, 0, depth - 1);  // 鏁版嵁鐨勮寖鍥?
    raw->SetDataScalarTypeToUnsignedShort();                        // 鍋囪鏁版嵁鏄?unsigned short 绫诲瀷
    raw->SetDataSpacing(spacing);                                   // 璁剧疆鍍忕礌闂磋窛
    raw->SetFileDimensionality(3);                                  // 涓夌淮鏁版嵁.0
    raw->SetDataByteOrderToLittleEndian();
    raw->Update();
    rawReader = vtkSmartPointer<vtkImageReader2>::New();
    rawReader->SetFileName(rawFilePath.toStdString().c_str());
    rawReader->SetDataExtent(0, width - 1, 0, height - 1, 0, depth - 1);  // 鏁版嵁鐨勮寖鍥?
    rawReader->SetDataScalarTypeToUnsignedShort();                        // 鍋囪鏁版嵁鏄?unsigned short 绫诲瀷
    rawReader->SetDataSpacing(spacing);                                   // 璁剧疆鍍忕礌闂磋窛
    rawReader->SetFileDimensionality(3);                                  // 涓夌淮鏁版嵁.0
    rawReader->SetDataByteOrderToLittleEndian();
    rawReader->Update();

    int rawDimensions = rawReader->GetOutput()->GetDimensions()[2];
    ui->horizontalSliderAxial->setMinimum(0);
    ui->horizontalSliderAxial->setMaximum(rawDimensions - 1);
    ui->horizontalSliderCoronal->setMinimum(0);
    ui->horizontalSliderCoronal->setMaximum(rawDimensions - 1);
    ui->horizontalSliderSagittal->setMinimum(0);
    ui->horizontalSliderSagittal->setMaximum(rawDimensions - 1);

    BoneVolumeRendering();

    rawSlice(width, height, depth);
    // iren->Start();
}

void MyVtkNew::rawSlice(int width, int height, int depth)
{
    vtkSmartPointer<vtkImageData> imageData = rawReader->GetOutput();  // 璇诲彇鍒扮殑鍘熷鏁版嵁
    double range[2];
    rawReader->GetOutput()->GetScalarRange(range);  // 鑾峰彇鐏板害鑼冨洿
    int imageDims[3];
    rawReader->GetOutput()->GetDimensions(imageDims);
    for (int i = 0; i < 3; i++)
    {
        riw[i] = vtkSmartPointer<MyVtkResliceImageViewer>::New();
        vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
        riw[i]->SetRenderWindow(renderWindow);
    }
    // this->ui->widgetAxial->setRenderWindow(riw[0]->GetRenderWindow());
    // riw[0]->SetupInteractor(this->ui->widgetAxial->renderWindow()->GetInteractor());

    // this->ui->widgetCoronal->setRenderWindow(riw[1]->GetRenderWindow());
    // riw[1]->SetupInteractor(this->ui->widgetCoronal->renderWindow()->GetInteractor());

    // this->ui->widgetSagittal->setRenderWindow(riw[2]->GetRenderWindow());
    // riw[2]->SetupInteractor(this->ui->widgetSagittal->renderWindow()->GetInteractor());
    for (auto i = 0; i < 4; i++)
    {
        textActor[i] = vtkSmartPointer<vtkTextActor>::New();
        textActor[i]->SetDisplayPosition(5, 5);
        textActor[i]->GetTextProperty()->SetFontSize(14);
        textActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
    }
    textActor[0]->SetInput(QString::fromUtf8("鐭㈢姸").toUtf8());
    textActor[0]->GetTextProperty()->SetColor(0, 1, 0);

    textActor[1]->SetInput(QString::fromUtf8("鍐犵姸").toUtf8());
    textActor[1]->GetTextProperty()->SetColor(0, 0, 1);

    textActor[2]->SetInput(QString::fromUtf8("杞村悜").toUtf8());
    textActor[2]->GetTextProperty()->SetColor(1, 0, 0);

    textActor[3]->SetInput(QString::fromUtf8("3D").toUtf8());
    textActor[3]->GetTextProperty()->SetColor(1, 1, 0);

    for (auto i = 0; i < 4; i++)
    {
        peopleInforTextActor[i] = vtkSmartPointer<vtkTextActor>::New();
        peopleInforTextActor[i]->GetTextProperty()->SetFontSize(14);
        peopleInforTextActor[i]->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        peopleInforTextActor[i]->SetInput(QString::fromUtf8("鎮ｈ€呭鍚嶏細").toUtf8() + "Test" + "\r\n" +
                                          "UID:311261562.001.001.001");
    }
    peopleInforTextActor[0]->GetTextProperty()->SetColor(0, 1, 0);
    peopleInforTextActor[0]->SetDisplayPosition(5, ui->widgetAxial->height() - 40);
    peopleInforTextActor[1]->GetTextProperty()->SetColor(0, 0, 1);
    peopleInforTextActor[1]->SetDisplayPosition(5, ui->widgetCoronal->height() - 40);
    peopleInforTextActor[2]->GetTextProperty()->SetColor(1, 0, 0);
    peopleInforTextActor[2]->SetDisplayPosition(5, ui->widgetSagittal->height() - 40);
    peopleInforTextActor[3]->GetTextProperty()->SetColor(1, 1, 0);
    peopleInforTextActor[3]->SetDisplayPosition(5, ui->widget3D->height() - 40);

    for (auto i = 0; i < 3; i++)
    {
        riw[i] = vtkSmartPointer<MyVtkResliceImageViewer>::New();
        riw[i]->GetRenderer()->AddActor(textActor[i]);
        riw[i]->GetRenderer()->AddActor(peopleInforTextActor[i]);
    }

    riw[0]->SetRenderWindow(ui->widgetAxial->renderWindow());
    riw[0]->SetupInteractor(ui->widgetAxial->renderWindow()->GetInteractor());

    riw[1]->SetRenderWindow(ui->widgetCoronal->renderWindow());
    riw[1]->SetupInteractor(ui->widgetCoronal->renderWindow()->GetInteractor());

    riw[2]->SetRenderWindow(ui->widgetSagittal->renderWindow());
    riw[2]->SetupInteractor(ui->widgetSagittal->renderWindow()->GetInteractor());

    for (int i = 0; i < 3; i++)
    {
        vtkResliceCursorLineRepresentation* rep =
            vtkResliceCursorLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());

        rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i);

        rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetColor(1.0, 0.0, 0.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetLineWidth(2.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetAmbient(1.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetColor(0.0, 1.0, 0.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetLineWidth(2.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetAmbient(1.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetColor(0.0, 0.0, 1.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetLineWidth(2.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetAmbient(1.0);
        rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->SetVisibility(1);
        rep->BuildRepresentation();

        riw[i]->SetInputData(rawReader->GetOutput());
        riw[i]->SetSliceOrientation(i);
        riw[i]->SetResliceModeToAxisAligned();
    }
    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.005);

    vtkSmartPointer<vtkProperty> ipwProp = vtkSmartPointer<vtkProperty>::New();

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();

    vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
    this->ui->widget3D->setRenderWindow(renderWindow);
    this->ui->widget3D->renderWindow()->AddRenderer(ren);
    vtkRenderWindowInteractor* iren = this->ui->widget3D->interactor();

    for (int i = 0; i < 3; i++)
    {
        planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
        planeWidget[i]->SetInteractor(iren);
        planeWidget[i]->SetPicker(picker);
        planeWidget[i]->RestrictPlaneToVolumeOn();
        double color[3] = {0, 0, 0};
        color[i] = 1;
        planeWidget[i]->GetPlaneProperty()->SetColor(color);

        color[0] = 0;
        color[1] = 0;
        color[2] = 0;
        riw[i]->GetRenderer()->SetBackground(color);

        planeWidget[i]->SetTexturePlaneProperty(ipwProp);
        planeWidget[i]->TextureInterpolateOff();
        planeWidget[i]->SetResliceInterpolateToLinear();
        planeWidget[i]->SetInputConnection(rawReader->GetOutputPort());
        planeWidget[i]->SetPlaneOrientation(i);
        planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
        planeWidget[i]->DisplayTextOn();
        planeWidget[i]->SetDefaultRenderer(ren);
        planeWidget[i]->SetWindowLevel(range[0], range[1]);
        planeWidget[i]->On();
        planeWidget[i]->InteractionOn();
    }

    vtkSmartPointer<vtkResliceCursorCallback> cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();

    for (int i = 0; i < 3; i++)
    {
        cbk->IPW[i] = planeWidget[i];
        cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, cbk);
        riw[i]->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, cbk);
        riw[i]->AddObserver(vtkResliceImageViewer::SliceChangedEvent, cbk);
        riw[i]->SetColorWindow(6549.0);
        riw[i]->SetColorLevel(64962.0);

        // Make them all share the same color map.
        riw[i]->SetLookupTable(riw[0]->GetLookupTable());
        planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());

        // planeWidget[i]->GetColorMap()->SetInput(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
        planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
    }

    for (auto i = 0; i < 3; i++)
    {
        riw[i]->GetResliceCursorWidget()->On();
        riw[i]->SetThickMode(1);
        riw[i]->Render();
    }

    ui->widgetAxial->renderWindow()->GetInteractor()->Initialize();
    ui->widgetCoronal->renderWindow()->GetInteractor()->Initialize();
    ui->widgetSagittal->renderWindow()->GetInteractor()->Initialize();

    // Set up action signals and slots

    connect(this->ui->resliceModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(resliceMode(int)));
    connect(this->ui->thickModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(thickMode(int)));
    this->ui->thickModeCheckBox->setEnabled(0);

    connect(this->ui->radioButton_Max, SIGNAL(pressed()), this, SLOT(SetBlendModeToMaxIP()));
    connect(this->ui->radioButton_Min, SIGNAL(pressed()), this, SLOT(SetBlendModeToMinIP()));
    connect(this->ui->radioButton_Mean, SIGNAL(pressed()), this, SLOT(SetBlendModeToMeanIP()));
    this->ui->blendModeGroupBox->setEnabled(0);

    connect(this->ui->resetButton, SIGNAL(pressed()), this, SLOT(ResetViews()));
    connect(this->ui->AddDistance1Button, SIGNAL(pressed()), this, SLOT(AddDistanceMeasurementToView1()));
}

void MyVtkNew::resliceMode(int mode)
{
    this->ui->thickModeCheckBox->setEnabled(mode ? 1 : 0);
    this->ui->blendModeGroupBox->setEnabled(mode ? 1 : 0);

    for (int i = 0; i < 3; i++)
    {
        riw[i]->SetResliceMode(mode ? 1 : 0);
        riw[i]->GetRenderer()->ResetCamera();
        riw[i]->Render();
    }
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
    for (int i = 0; i < 3; i++)
    {
        vtkImageSlabReslice* thickSlabReslice = vtkImageSlabReslice::SafeDownCast(
            vtkResliceCursorThickLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation())
                ->GetReslice());
        thickSlabReslice->SetBlendMode(m);
        riw[i]->Render();
    }
}
void MyVtkNew::SetBlendModeToMaxIP()
{
    this->SetBlendMode(VTK_IMAGE_SLAB_MAX);
}

void MyVtkNew::SetBlendModeToMinIP()
{
    this->SetBlendMode(VTK_IMAGE_SLAB_MIN);
}

void MyVtkNew::SetBlendModeToMeanIP()
{
    this->SetBlendMode(VTK_IMAGE_SLAB_MEAN);
}
void MyVtkNew::ResetViews()
{
    // Reset the reslice image views
    for (int i = 0; i < 3; i++)
    {
        riw[i]->Reset();
    }

    // Also sync the Image plane widget on the 3D top right view with any
    // changes to the reslice cursor.
    for (int i = 0; i < 3; i++)
    {
        vtkPlaneSource* ps = static_cast<vtkPlaneSource*>(planeWidget[i]->GetPolyDataAlgorithm());
        ps->SetNormal(riw[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
        ps->SetCenter(riw[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->planeWidget[i]->UpdatePlacement();
    }

    // Render in response to changes.
    this->Render();
}
void MyVtkNew::Render()
{
    for (int i = 0; i < 3; i++)
    {
        riw[i]->Render();
    }
    this->ui->widget3D->renderWindow()->Render();
}
void MyVtkNew::AddDistanceMeasurementToView1()
{
    this->AddDistanceMeasurementToView(1);
}

void MyVtkNew::AddDistanceMeasurementToView(int i)
{
    // remove existing widgets.
    if (this->DistanceWidget[i])
    {
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    // add new widget
    this->DistanceWidget[i] = vtkSmartPointer<vtkDistanceWidget>::New();
    this->DistanceWidget[i]->SetInteractor(this->riw[i]->GetResliceCursorWidget()->GetInteractor());

    // Set a priority higher than our reslice cursor widget
    this->DistanceWidget[i]->SetPriority(this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

    vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep = vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
    vtkSmartPointer<vtkDistanceRepresentation2D> distanceRep = vtkSmartPointer<vtkDistanceRepresentation2D>::New();
    distanceRep->SetHandleRepresentation(handleRep);
    this->DistanceWidget[i]->SetRepresentation(distanceRep);
    distanceRep->InstantiateHandleRepresentation();
    distanceRep->GetPoint1Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
    distanceRep->GetPoint2Representation()->SetPointPlacer(riw[i]->GetPointPlacer());

    // Add the distance to the list of widgets whose visibility is managed based
    // on the reslice plane by the ResliceImageViewerMeasurements class
    this->riw[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

    this->DistanceWidget[i]->CreateDefaultRepresentation();
    this->DistanceWidget[i]->EnabledOn();
}

void MyVtkNew::AddAngleToView(int i)
{  // remove existing widgets.
    if (this->AngleWidget[i])
    {
        this->AngleWidget[i]->SetEnabled(0);
        this->AngleWidget[i] = nullptr;
    }

    // add new widget
    this->AngleWidget[i] = vtkSmartPointer<vtkAngleWidget>::New();
    this->AngleWidget[i]->SetInteractor(this->riw[i]->GetResliceCursorWidget()->GetInteractor());

    // Set a priority higher than our reslice cursor widget
    this->AngleWidget[i]->SetPriority(this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

    vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep = vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
    vtkSmartPointer<vtkAngleRepresentation2D> angleRep = vtkSmartPointer<vtkAngleRepresentation2D>::New();
    angleRep->SetHandleRepresentation(handleRep);
    angleRep->InstantiateHandleRepresentation();

    angleRep->GetPoint1Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
    angleRep->GetPoint2Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
    angleRep->GetCenterRepresentation()->SetPointPlacer(riw[i]->GetPointPlacer());

    this->AngleWidget[i]->SetRepresentation(angleRep);
    this->riw[i]->GetMeasurements()->AddItem(this->AngleWidget[i]);

    this->AngleWidget[i]->CreateDefaultRepresentation();
    this->AngleWidget[i]->EnabledOn();
}

void MyVtkNew::ConvertRawSlicesToDicom(const std::string& outputFolder, const std::string& patientName,
                                       const std::string& patientID)
{
    QString dirQt = QString::fromStdString(outputFolder) + "/OutPut";
    if (!QDir().exists(dirQt) && !QDir().mkpath(dirQt))
    {
        qWarning() << "鏃犳硶鍒涘缓鐩綍锛? << dirQt;
        return;
    }

    vtkImageData* imageData = rawReader->GetOutput();
    int dims[3];
    imageData->GetDimensions(dims);  // dims = {Columns, Rows, Slices} :contentReference[oaicite:0]{index=0}
    double spacing[3];
    imageData->GetSpacing(spacing);

    // 2. 鐢熸垚 Study 鍜?Series UID
    char studyUID[100] = {0}, seriesUID[100] = {0}, sopUID[100] = {0};
    dcmGenerateUniqueIdentifier(studyUID, SITE_INSTANCE_UID_ROOT);  // :contentReference[oaicite:1]{index=1}
    dcmGenerateUniqueIdentifier(seriesUID, SITE_INSTANCE_UID_ROOT);

    // 3. 閬嶅巻姣忎釜鍒囩墖
    for (int z = 0; z < dims[2]; ++z)
    {
        // 3.1 鎻愬彇鍒囩墖鍍忕礌鏁版嵁
        unsigned short* slicePtr = static_cast<unsigned short*>(imageData->GetScalarPointer(0, 0, z));

        // 3.2 鏋勫缓 DICOM 鏂囦欢缁撴瀯
        DcmFileFormat fileformat;
        DcmDataset* dataset = fileformat.getDataset();

        // 3.3 鐢熸垚鏈垏鐗?SOP Instance UID
        dcmGenerateUniqueIdentifier(sopUID, SITE_INSTANCE_UID_ROOT);  // :contentReference[oaicite:2]{index=2}

        // 3.4 鎻掑叆鎮ｈ€呬笌Study/Series 淇℃伅
        dataset->putAndInsertString(DCM_PatientName, patientName.c_str());
        dataset->putAndInsertString(DCM_PatientID, patientID.c_str());
        dataset->putAndInsertString(DCM_StudyInstanceUID, studyUID);
        dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUID);
        dataset->putAndInsertString(DCM_Modality, "CT");

        // 3.5 鎻掑叆 SOP Class UID 涓?SOP Instance UID
        dataset->putAndInsertString(DCM_SOPClassUID,
                                    UID_SecondaryCaptureImageStorage);  // :contentReference[oaicite:3]{index=3}
        dataset->putAndInsertString(DCM_SOPInstanceUID, sopUID);        // :contentReference[oaicite:4]{index=4}

        // 3.6 鍥惧儚鍍忕礌妯″潡 (Image Pixel Module)
        dataset->putAndInsertUint16(DCM_SamplesPerPixel,
                                    1);  // 鍗曢€氶亾鐏板害鍥撅細SamplesPerPixel = 1 :contentReference[oaicite:5]{index=5}
        dataset->putAndInsertString(DCM_PhotometricInterpretation,
                                    "MONOCHROME2");  // 鐏板害妯″紡锛歁ONOCHROME2 :contentReference[oaicite:6]{index=6}

        // 3.7 鍍忕礌缂栫爜灞炴€?(Pixel Data Encoding)
        dataset->putAndInsertUint16(DCM_Rows, dims[1]);
        dataset->putAndInsertUint16(DCM_Columns, dims[0]);
        dataset->putAndInsertUint16(DCM_BitsAllocated, 16);  // 姣忓儚绱?16 浣?:contentReference[oaicite:7]{index=7}
        dataset->putAndInsertUint16(DCM_BitsStored, 16);     // 鏈夋晥浣嶏細16 :contentReference[oaicite:8]{index=8}
        dataset->putAndInsertUint16(DCM_HighBit,
                                    15);  // HighBit = BitsStored - 1 = 15 :contentReference[oaicite:9]{index=9}
        dataset->putAndInsertUint16(DCM_PixelRepresentation, 0);  // 0 = 鏃犵鍙锋暣鏁?
        dataset->putAndInsertString(DCM_WindowCenter, std::to_string(windowCenter).c_str());
        dataset->putAndInsertString(DCM_WindowWidth, std::to_string(windowWidth).c_str());
        // 浣嶇疆鍜屾柟鍚?
        char imagePos[64];
        sprintf(imagePos, "0\\0\\%.6f", z * spacing[2]);
        dataset->putAndInsertString(DCM_ImagePositionPatient, imagePos);
        dataset->putAndInsertString(DCM_ImageOrientationPatient, "1\\0\\0\\0\\1\\0");
        dataset->putAndInsertString(DCM_SliceThickness, std::to_string(spacing[2]).c_str());
        dataset->putAndInsertString(DCM_PixelSpacing,
                                    (std::to_string(spacing[1]) + "\\" + std::to_string(spacing[0])).c_str());

        // 3.8 鎻掑叆鍍忕礌鏁版嵁
        dataset->putAndInsertUint16Array(
            DCM_PixelData, slicePtr,
            static_cast<unsigned long>(dims[0]) * dims[1]  // PixelData 鏁伴噺 = Rows 脳 Columns
        );

        // 3.9 鐢熸垚骞舵牎楠屾枃浠跺厓淇℃伅澶达紝鐒跺悗鍐欏嚭 DICOM 鏂囦欢
        fileformat.validateMetaInfo(EXS_LittleEndianExplicit);  // 鑷姩濉厖 (0002,0002),(0002,0003),(0002,0010) 绛夋爣璁?
        QString fn = dirQt + QString("/slice%1.dcm").arg(z, 3, 10, QChar('0'));
        fileformat.saveFile(fn.toStdString().c_str(), EXS_LittleEndianExplicit);
    }
}

void MyVtkNew::mouseMoveEvent(QMouseEvent* event) {}

void MyVtkNew::SaveAsRawFile(vtkSmartPointer<vtkPolyData> polyData, const std::string& filename)
{
    // 鑾峰彇 polyData 涓殑鐐规暟鎹?
    vtkSmartPointer<vtkPoints> points = polyData->GetPoints();
    if (!points)
    {
        qDebug() << "Error: PolyData has no points!";
        return;
    }

    // 鑾峰彇鐐圭殑鏁伴噺
    int numPoints = points->GetNumberOfPoints();

    // 鎵撳紑鏂囦欢浠ヤ簩杩涘埗鏂瑰紡鍐欏叆
    std::ofstream rawFile(filename, std::ios::binary);
    if (!rawFile)
    {
        qDebug() << "Error: Failed to open raw file for writing.";
        return;
    }

    // 閬嶅巻姣忎釜鐐瑰苟鍐欏叆鏂囦欢
    for (int i = 0; i < numPoints; ++i)
    {
        double point[3];
        points->GetPoint(i, point);

        // 鍐欏叆鐐圭殑 x, y, z 鍧愭爣
        rawFile.write(reinterpret_cast<char*>(point), 3 * sizeof(double));
    }

    qDebug() << "PolyData successfully saved to " << filename;
}
void MyVtkNew::dicomShow(const QString& dicomDirectory)
{  // 璇诲彇 DICOM 鍥惧儚鏁版嵁
    vtkNew<vtkDICOMImageReader> dicomReader;
    dicomReader->SetDirectoryName(dicomDirectory.toStdString().c_str());
    dicomReader->Update();  // 璇诲彇鏁版嵁

    // 鑾峰彇鍥惧儚鏁版嵁
    vtkSmartPointer<vtkImageData> imageData = dicomReader->GetOutput();
    if (!imageData)
    {
        std::cerr << "Error: Failed to load DICOM image." << std::endl;
        return;
    }

    // 鑾峰彇鍥惧儚鐨勫昂瀵?
    int* dims = imageData->GetDimensions();
    std::cout << "Image Dimensions: " << dims[0] << " x " << dims[1] << " x " << dims[2] << std::endl;

    // 鍒涘缓绐楀绐椾綅鏄犲皠鍣?
    vtkNew<vtkImageMapToWindowLevelColors> windowLevelFilter;
    windowLevelFilter->SetInputData(imageData);
    double range[2];
    imageData->GetScalarRange(range);
    double window = range[1] - range[0];
    double level = (range[1] + range[0]) / 2;
    windowLevelFilter->SetWindow(window);
    windowLevelFilter->SetLevel(level);
    windowLevelFilter->Update();

    // 浣跨敤 vtkImageActor 鏄剧ず鍥惧儚
    vtkNew<vtkImageActor> imageActor;
    imageActor->GetMapper()->SetInputData(windowLevelFilter->GetOutput());
    // imageActor->GetMapper()->SetInputData(imageData);

    // 鍒涘缓娓叉煋鍣ㄥ拰娓叉煋绐楀彛
    vtkNew<vtkRenderer> renderer;

    vtkSmartPointer<vtkRenderWindow> renderWindow = ui->widgetAxial->renderWindow();

    renderWindow->AddRenderer(renderer);

    // 鍒涘缓浜や簰鍣?
    /*  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor ;
      renderWindowInteractor->SetRenderWindow(renderWindow);*/

    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = renderWindow->GetInteractor();

    // 璁剧疆鑷畾涔変氦浜掑櫒鏍峰紡
    vtkNew<CustomInteractorStyle> style;
    style->SetWindowLevelFilter(windowLevelFilter);
    style->SetRenderer(renderer);  // 浼犻€掓覆鏌撳櫒
    renderWindowInteractor->SetInteractorStyle(style);

    // 娣诲姞鍥惧儚婕斿憳鍒版覆鏌撳櫒
    renderer->AddActor(imageActor);
    renderer->ResetCamera();

    // 娓叉煋骞舵樉绀哄浘鍍?
    renderWindow->Render();
    // renderWindowInteractor->Start();
}

void MyVtkNew::GenerateVirtual3DData(vtkSmartPointer<vtkImageData>& imageData)
{
    imageData->SetDimensions(50, 50, 50);
    imageData->AllocateScalars(VTK_FLOAT, 1);

    for (int z = 0; z < 50; z++)
    {
        for (int y = 0; y < 50; y++)
        {
            for (int x = 0; x < 50; x++)
            {
                float* pixel = static_cast<float*>(imageData->GetScalarPointer(x, y, z));
                pixel[0] = static_cast<float>(x + y + z);
            }
        }
    }
    std::cout << "Virtual 3D data generated." << std::endl;
}

void MyVtkNew::Expand2DTo3D(vtkSmartPointer<vtkImageData>& imageData2D, vtkSmartPointer<vtkImageData>& imageData3D)
{
    int* dims2D = imageData2D->GetDimensions();

    // 璁剧疆鏂扮殑 3D 鏁版嵁缁村害
    int zExtent = 50;  // 閲嶅 50 涓垏鐗囷紝妯℃嫙涓夌淮鏁版嵁
    imageData3D->SetDimensions(dims2D[0], dims2D[1], zExtent);
    imageData3D->AllocateScalars(imageData2D->GetScalarType(), imageData2D->GetNumberOfScalarComponents());

    for (int z = 0; z < zExtent; z++)
    {
        for (int y = 0; y < dims2D[1]; y++)
        {
            for (int x = 0; x < dims2D[0]; x++)
            {
                void* srcPixel = imageData2D->GetScalarPointer(x, y, 0);
                void* destPixel = imageData3D->GetScalarPointer(x, y, z);
                memcpy(destPixel, srcPixel, imageData2D->GetScalarSize() * imageData2D->GetNumberOfScalarComponents());
            }
        }
    }
    // 鍒涘缓绐楀绐椾綅杩囨护鍣?

    vtkNew<vtkImageMapToWindowLevelColors> windowLevelFilter;
    windowLevelFilter->SetInputData(imageData3D);
    double range[2];
    imageData3D->GetScalarRange(range);
    double window = range[1] - range[0];
    double level = (range[1] + range[0]) / 2;
    windowLevelFilter->SetWindow(window);
    windowLevelFilter->SetLevel(level);
    windowLevelFilter->Update();

    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);

    // 鍒涘缓浜や簰鍣?
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    // 鍒涘缓鍥惧儚鏌ョ湅鍣?
    vtkSmartPointer<vtkImageViewer2> imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
    imageViewer->SetInputConnection(windowLevelFilter->GetOutputPort());  // 浣跨敤缁忚繃绐楀绐椾綅璋冩暣鐨勫浘鍍忔暟鎹?
    imageViewer->SetRenderWindow(renderWindow);

    // 鏄剧ず绗竴涓垏鐗囷紙榛樿鏄剧ず z = 0 鐨勫垏鐗囷級
    imageViewer->Render();

    // 鍚姩浜や簰寮忔覆鏌?
    renderWindow->Render();
    renderWindowInteractor->Start();

    qDebug() << "2D DICOM data expanded to 3D.";
}

void MyVtkNew::BoneVolumeRendering()
{
    // 鑾峰彇鍥惧儚鏁版嵁
    vtkSmartPointer<vtkImageData> imageData = rawReader->GetOutput();

    int dims[3];
    imageData->GetDimensions(dims);
    qDebug() << tr("鍥惧儚缁存暟锛?) << dims[0] << "" << dims[1] << "" << dims[2];

    double origin[3];
    imageData->GetOrigin(origin);
    qDebug() << tr("鍥惧儚鍘熺偣锛?) << origin[0] << "" << origin[1] << "" << origin[2];

    double spaceing[3];
    imageData->GetSpacing(spaceing);
    qDebug() << tr("鍥惧儚鍘熺偣锛?) << spaceing[0] << "" << spaceing[1] << "" << spaceing[2];

    vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    volumeMapper->SetBlendModeToComposite();
    volumeMapper->SetInputConnection(rawReader->GetOutputPort());
    // volumeMapper->SetVolumeRayCastFunction(rayCastFun);
    // 璁剧疆鍏夌嚎閲囨牱璺濈
    volumeMapper->SetSampleDistance(volumeMapper->GetSampleDistance() * 4);
    // 璁剧疆鍥惧儚閲囨牱姝ラ暱
    volumeMapper->SetAutoAdjustSampleDistances(0);
    volumeMapper->SetImageSampleDistance(1);
    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();  // 鎵撳紑鎴栬€呭叧闂槾褰辨祴璇?
    volumeProperty->SetAmbient(0.4);
    volumeProperty->SetDiffuse(0.6);
    volumeProperty->SetSpecular(0.2);
    compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    compositeOpacity->AddPoint(70, 0.00);
    compositeOpacity->AddPoint(90, 0.40);
    compositeOpacity->AddPoint(180, 0.60);
    volumeProperty->SetScalarOpacity(compositeOpacity);  // 璁剧疆涓嶉€忔槑搴︿紶杈撳嚱鏁?
    volumeGradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    volumeGradientOpacity->AddPoint(10, 0.0);
    volumeGradientOpacity->AddPoint(90, 0.5);
    volumeGradientOpacity->AddPoint(100, 1.0);
    volumeProperty->SetGradientOpacity(volumeGradientOpacity);  // 璁剧疆姊害涓嶉€忔槑搴︽晥鏋滃姣?
    vtkSmartPointer<vtkColorTransferFunction> color = vtkSmartPointer<vtkColorTransferFunction>::New();
    color->AddRGBPoint(0.000, 0.00, 0.00, 0.00);
    color->AddRGBPoint(64.00, 1.00, 0.52, 0.30);
    color->AddRGBPoint(190.0, 1.00, 1.00, 1.00);
    color->AddRGBPoint(220.0, 0.20, 0.20, 0.20);
    volumeProperty->SetColor(color);
    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);
    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
    ren->SetBackground(1.0, 1.0, 1.0);
    ren->AddVolume(volume);
    renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    renWin->AddRenderer(ren);
    renWin->Render();
    ui->horizontalSliderVolume->setMinimum(0);   // 鏈€灏忓€?
    ui->horizontalSliderVolume->setMaximum(20);  // 鏈€澶у€?
    ui->widget3D_2->setRenderWindow(renWin);
    ui->widget3D_2->update();  // 寮哄埗鏇存柊鎺т欢
    ren->ResetCamera();
    renWin->Render();
}

void MyVtkNew::BloodVesselVolumeRendering()
{
    // 鑾峰彇鍥惧儚鏁版嵁
    double range[2];
    rawReader->GetOutput()->GetScalarRange(range);
    qDebug() << "Data HU Range: [" << range[0] << ", " << range[1] << "]";

    renderer = vtkSmartPointer<vtkRenderer>::New();
    auto volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    volumeMapper->SetInputConnection(rawReader->GetOutputPort());

    // 璁剧疆浼犻€掑嚱鏁帮紙棰滆壊鍜屼笉閫忔槑搴︼級
    auto colorTransfer = vtkSmartPointer<vtkColorTransferFunction>::New();
    // 浣庡瘑搴﹀尯鍩燂紙绌烘皵/鑴傝偑/杞粍缁囷級璁句负閫忔槑鎴栫伆鑹?
    colorTransfer->AddRGBPoint(range[0], 0.0, 0.0, 0.0);  // 鏈€浣庡€奸粦鑹?
    colorTransfer->AddRGBPoint(-50, 0.8, 0.8, 0.8);       // 杞粍缁囩伆鑹?
    colorTransfer->AddRGBPoint(80, 0.8, 0.8, 0.8);        // 杩囨浮鍖?

    // 琛€绠★紙鏍稿績鍖洪棿锛?
    colorTransfer->AddRGBPoint(80, 0.5, 0.0, 0.0);  // 鏆楃孩鑹?

    colorTransfer->AddRGBPoint(100, 1.0, 0.0, 0.0);  // 绾㈣壊璧峰
    colorTransfer->AddRGBPoint(200, 1.0, 0.0, 0.0);
    colorTransfer->AddRGBPoint(300, 1.0, 0.0, 0.0);  // 绾㈣壊淇濇寔

    // 楠ㄩ锛堥珮浜庤绠★級
    colorTransfer->AddRGBPoint(301, 1.0, 1.0, 0.0);  // 榛勮壊
    colorTransfer->AddRGBPoint(600, 1.0, 1.0, 1.0);  // 鐧借壊锛堥珮瀵嗗害楠ㄩ锛?

    // 璁剧疆瓒呭嚭鑼冨洿鐨勮涓?
    colorTransfer->SetClamping(true);  // 寮哄埗浣跨敤鏈€杩戠殑绔偣棰滆壊

    auto opacityTransfer = vtkSmartPointer<vtkPiecewiseFunction>::New();
    // 浣庡瘑搴﹂€忔槑
    opacityTransfer->AddPoint(range[0], 0.0);  // 瀹屽叏閫忔槑
    opacityTransfer->AddPoint(-50, 0.0);
    opacityTransfer->AddPoint(80, 0.1);  // 寰急鏄剧ず杞粍缁?

    // 琛€绠″崐閫忔槑
    opacityTransfer->AddPoint(100, 0.0);  // 寮€濮嬫樉绀?
    opacityTransfer->AddPoint(150, 0.5);  // 涓昏琛€绠′笉閫忔槑搴?
    opacityTransfer->AddPoint(300, 0.8);  // 楂樺姣斿害鍖哄煙

    // 楠ㄩ浣庨€忔槑搴︼紙閬垮厤閬尅锛?
    opacityTransfer->AddPoint(301, 0.1);  // 楠ㄩ鍗婇€忔槑
    opacityTransfer->AddPoint(600, 0.3);  // 楂樺瘑搴﹂楠肩暐寰彲瑙?

    // 璁剧疆浣撶Н灞炴€?
    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->ShadeOn();  // 鍚敤鍏夌収
    volumeProperty->SetAmbient(0.4);
    volumeProperty->SetDiffuse(0.6);
    volumeProperty->SetSpecular(0.2);
    volumeProperty->SetColor(colorTransfer);
    volumeProperty->SetScalarOpacity(opacityTransfer);
    volumeProperty->SetInterpolationTypeToLinear();

    // 鍒涘缓浣撶Н瀵硅薄
    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    // 娣诲姞鍒版覆鏌撳櫒
    renderer->AddVolume(volume);
    renderer->ResetCamera();

    ui->horizontalSliderVolume->setMinimum(0);   // 鏈€灏忓€?
    ui->horizontalSliderVolume->setMaximum(20);  // 鏈€澶у€?
    ui->widget3D_2->renderWindow()->AddRenderer(renderer);
    ui->widget3D_2->renderWindow()->Render();
    ui->widget3D_2->update();  // 寮哄埗鏇存柊鎺т欢
    // ren->ResetCamera();
    // renWin->Render();
}

void MyVtkNew::SurfaceRendering()
{
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    std::array<unsigned char, 4> skinColor{{255, 125, 64}};
    colors->SetColor("SkinColor", skinColor.data());

#if 0
    // An isosurface, or contour value of 500 is known to correspond to the
    // skin of the patient.
    auto skinExtractor = vtkSmartPointer<vtkMarchingCubes>::New();
    skinExtractor->SetInputConnection(rawReader->GetOutputPort());
    skinExtractor->SetValue(0, 500);

    // An isosurface, or contour value of 1150 is known to correspond to the
    // bone of the patient.
    auto boneExtractor = vtkSmartPointer<vtkMarchingCubes>::New();
    boneExtractor->SetInputConnection(rawReader->GetOutputPort());
    boneExtractor->SetValue(0, 1150);

    // The triangle stripper is used to create triangle strips from the
    // isosurface; these render much faster on may systems.
    auto skinStripper = vtkSmartPointer<vtkStripper>::New();
    skinStripper->SetInputConnection(skinExtractor->GetOutputPort());

    auto boneStripper = vtkSmartPointer<vtkStripper>::New();
    boneStripper->SetInputConnection(boneExtractor->GetOutputPort());
#else
    // An isosurface, or contour value of 500 is known to correspond to the
    // skin of the patient.
    auto skinExtractor = vtkSmartPointer<vtkContourFilter>::New();
    skinExtractor->SetInputConnection(rawReader->GetOutputPort());
    skinExtractor->SetValue(0, 500);

    // An isosurface, or contour value of 1150 is known to correspond to the
    // bone of the patient.
    auto boneExtractor = vtkSmartPointer<vtkContourFilter>::New();
    boneExtractor->SetInputConnection(rawReader->GetOutputPort());
    boneExtractor->SetValue(0, 1150);

    auto skinNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
    skinNormals->SetInputConnection(skinExtractor->GetOutputPort());
    skinNormals->SetFeatureAngle(60.0);

    auto boneNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
    boneNormals->SetInputConnection(boneExtractor->GetOutputPort());
    boneNormals->SetFeatureAngle(60.0);

    // The triangle stripper is used to create triangle strips from the
    // isosurface; these render much faster on may systems.
    auto skinStripper = vtkSmartPointer<vtkStripper>::New();
    skinStripper->SetInputConnection(skinNormals->GetOutputPort());

    auto boneStripper = vtkSmartPointer<vtkStripper>::New();
    boneStripper->SetInputConnection(boneNormals->GetOutputPort());
#endif

    auto skinMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    skinMapper->SetInputConnection(skinStripper->GetOutputPort());
    skinMapper->ScalarVisibilityOff();

    auto boneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    boneMapper->SetInputConnection(boneStripper->GetOutputPort());
    boneMapper->ScalarVisibilityOff();

    auto skin = vtkSmartPointer<vtkActor>::New();
    skin->SetMapper(skinMapper);
    skin->GetProperty()->SetDiffuseColor(colors->GetColor3d("SkinColor").GetData());
    skin->GetProperty()->SetSpecular(.3);
    skin->GetProperty()->SetSpecularPower(20);
    skin->GetProperty()->SetOpacity(.5);

    auto bone = vtkSmartPointer<vtkActor>::New();
    bone->SetMapper(boneMapper);
    bone->GetProperty()->SetDiffuseColor(colors->GetColor3d("Ivory").GetData());

    // An outline provides context around the data.
    auto outlineData = vtkSmartPointer<vtkOutlineFilter>::New();
    outlineData->SetInputConnection(rawReader->GetOutputPort());

    auto outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    outlineMapper->SetInputConnection(outlineData->GetOutputPort());

    auto outline = vtkSmartPointer<vtkActor>::New();
    outline->SetMapper(outlineMapper);
    outline->GetProperty()->SetColor(1, 0, 0);

    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(skin);
    renderer->AddActor(bone);
    renderer->AddActor(outline);

    auto renWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renWindow->SetSize(640, 480);
    renWindow->AddRenderer(renderer);

    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renWindow);
    ui->widget3D_2->setRenderWindow(renWindow);
    renWindow->Render();
    interactor->Initialize();
    interactor->Start();
}

void MyVtkNew::WindowLevelCallback(vtkObject* caller, unsigned long eventId, void* clientData, void* callData)
{
    int* newEventId = static_cast<int*>(callData);
    if (*newEventId == vtkCommand::EndWindowLevelEvent)
    {
        vtkImageViewer2* viewer = static_cast<vtkImageViewer2*>(caller);
        double window = viewer->GetColorWindow();
        double level = viewer->GetColorLevel();

        MyVtkNew* instance = static_cast<MyVtkNew*>(clientData);
        instance->updatePlaneTextEdit(0, window, level);
    }
}

void MyVtkNew::updatePlaneTextEdit(int id, double window, double level)
{
    // switch (id)
    //{
    // case 0:
    //{
    //     ui->plainTextEdit->insertPlainText(QString("Axial Window: %1, Level: %2").arg(window).arg(level));
    //     break;
    // }
    // case 1:
    //{
    //     ui->plainTextEdit->insertPlainText(QString("Coronal Window: %1, Level: %2").arg(window).arg(level));
    //     break;
    // }
    // case 2:
    //{
    //     ui->plainTextEdit->insertPlainText(QString("Sagittal Window: %1, Level: %2,
    //     %3").arg(window).arg(level).arg("\n")); break;
    // }
    // default:
    //     break;
    // }
}
