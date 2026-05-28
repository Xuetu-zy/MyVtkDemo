# MyVtkDemo

MyVtkDemo 是一个基于 `Qt + VTK + DCMTK` 的 Windows 医学影像查看与重建工具，
支持 DICOM/RAW 读取、MPR（Axial/Coronal/Sagittal）、Oblique 重切片、
3D 体绘制以及基础测量（Distance/Angle）。

## 主要功能

- 数据读取
- 支持 DICOM 序列目录读取
- 支持三维 RAW 文件读取（需输入 `width/height/depth`）

- MPR 浏览
- 三视图联动：`Axial`、`Coronal`、`Sagittal`
- 滑块与滚轮切片浏览
- `Reset` 视图重置（含切片/相机/联动状态）

- Oblique 与厚层
- `Oblique Reslice` 倾斜切片
- `Thick Slab` 厚层显示
- Blend mode：`MIP / MinIP / Mean`

- 3D 渲染
- 体绘制预设：`BONE`、`BLOODVESSEL`
- Surface Rendering（等值面）
- 透明度调节

- 测量工具
- Distance 测量（两点）
- Angle 测量（三点）
- 单工具互斥激活，未完成测量跨视图可取消

## 技术栈

- `C++`（MSVC / v143）
- `Qt 6`（Widgets + OpenGL Widgets）
- `VTK 9.6`
- `DCMTK`
- `Visual Studio 2022`（`.sln/.vcxproj`）

## 目录说明

- `MyVtkNew.cpp/.h/.ui`：主界面与主要交互逻辑
- `myvtkresliceimageviewer.cpp/.h`：自定义 Reslice Image Viewer
- `myvtkresliceimageviewermeasurements.cpp/.h`：测量相关支持
- `CustomInteractorStyle.*`、`ResliceInteractorStyle .*`：交互风格扩展
- `VTK/`、`DCMTK/`：项目内依赖头文件与库目录

## 构建环境

建议环境：

- Windows 10/11
- Visual Studio 2022
- Qt 6（与工程配置匹配）
- 已安装并可用的 VTK / DCMTK（本仓库包含工程引用路径）

工程配置（来自 `MyVtkNew.vcxproj`）：

- Solution: `Debug|x64`, `Release|x64`
- Qt install 示例：`msvc2022_64` / `6.8.1_msvc2022_64`
- 关键包含路径：`./VTK/include/vtk-9.6`, `./DCMTK/include`
- 关键库路径：`./VTK/lib`, `./DCMTK/lib`

## 编译与运行

1. 使用 Visual Studio 2022 打开 `MyVtkNew.sln`
2. 选择 `x64` + `Release`（或 `Debug`）
3. 执行 Build
4. 运行生成的可执行文件

如果出现 DLL 缺失，请确认 Qt / VTK / DCMTK 运行时库已在系统 `PATH` 或可执行目录可访问。

## 使用说明（快速）

1. 在主界面点击“打开图像”读取 DICOM 目录，或读取 RAW 并填写尺寸
2. 在 MPR 页面使用三个滑块/滚轮浏览切片
3. 勾选 `Oblique` 进行倾斜切片交互
4. 使用 `Add Distance On View` / `Add Angle On View` 进行标注测量
5. 切换到“体绘制”页面查看 3D 渲染结果

## 已知注意事项

- RAW 导入强依赖尺寸参数，错误尺寸会导致显示异常
- Oblique 模式下建议在重置后确认三视图联动是否与预期一致
- 若用于临床或科研，请在正式流程中增加数据与方向一致性校验

## License

当前仓库未附带明确开源许可证。
如需对外分发或商用，请先补充 LICENSE 并确认第三方依赖许可条款。
