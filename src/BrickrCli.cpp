#include "BrickrCli.h"

#include "AssemblyPlugin.h"
#include "LegoCloud.h"
#include "LegoCloudNode.h"
#include "Vector3.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QSvgGenerator>
#include <QTextStream>

#include <iostream>

namespace
{
const int BRICK_PIXEL_SIZE = 20;
}

int BrickrCli::run(const BrickrCliOptions &options)
{
  AssemblyPlugin plugin;

  if(!loadInput(&plugin, options))
    return 1;

  LegoCloudNode *legoCloudNode = plugin.getLegoCloudNode();
  if(!legoCloudNode)
  {
    std::cerr << "Failed to build the LEGO model." << std::endl;
    return 1;
  }

  if(options.autoOptimize)
    plugin.autoOptimize();

  if(options.finalize)
  {
    legoCloudNode->getLegoCloud()->postHollow();
    legoCloudNode->getLegoCloud()->solveBrickNumberLimitation();
    legoCloudNode->getLegoCloud()->merge();
    legoCloudNode->nodeUpdated();
    std::cout << "Finalization done." << std::endl;
  }

  if(options.printStats)
    legoCloudNode->getLegoCloud()->printStats();

  if(!options.exportObjPath.isEmpty())
  {
    std::cout << "Exporting OBJ to: " << qPrintable(options.exportObjPath) << std::endl;
    legoCloudNode->exportToObj(options.exportObjPath);
  }

  if(!options.saveInstructionsPath.isEmpty() && !saveInstructions(legoCloudNode, options.saveInstructionsPath))
    return 1;

  return 0;
}

bool BrickrCli::loadInput(AssemblyPlugin *plugin, const BrickrCliOptions &options)
{
  QFileInfo selectedFileInfo(options.inputPath);

  if(!selectedFileInfo.exists() || !selectedFileInfo.isReadable())
  {
    std::cerr << "Unable to open file: " << options.inputPath.toStdString() << std::endl;
    return false;
  }

  QString binvoxFilePath;
  if(isMeshExtensionSupported(selectedFileInfo.suffix()))
  {
    if(options.voxelizationResolution <= 0)
    {
      std::cerr << "Voxelization resolution must be positive." << std::endl;
      return false;
    }

    const QString scaledFilePath = selectedFileInfo.absolutePath() + "/_" + selectedFileInfo.baseName() + "_scaled.obj";

    {
      QFile scaledFile(scaledFilePath);
      scaledFile.remove();
    }

    if(!scaleMesh(options.inputPath, scaledFilePath))
      return false;

    QFileInfo scaledFileInfo(scaledFilePath);
    binvoxFilePath = selectedFileInfo.absolutePath() + "/" + selectedFileInfo.baseName() + QString::number(options.voxelizationResolution) + ".binvox";
    QFile binvoxFile(binvoxFilePath);
    if(binvoxFile.exists())
      binvoxFile.remove();

    const QString voxelizerPath = locateVoxelizer();
    if(voxelizerPath.isEmpty())
    {
      std::cerr << "Unable to locate a voxelizer. Set BINVOX_PATH to a binvox binary or the Python helper." << std::endl;
      return false;
    }

    QFileInfo voxelizerInfo(voxelizerPath);
    QFile generatedBinvoxFile(scaledFileInfo.absolutePath() + "/" + scaledFileInfo.baseName() + ".binvox");
    if(generatedBinvoxFile.exists())
      generatedBinvoxFile.remove();

    QString command;
#ifdef WIN32
    command = "\"" + voxelizerInfo.absoluteFilePath() + "\" -d " + QString::number(options.voxelizationResolution) + " \"" + scaledFilePath + "\"";
#else
    if(voxelizerInfo.suffix().compare("py", Qt::CaseInsensitive) == 0)
    {
      command = "python3 \"" + voxelizerInfo.absoluteFilePath() + "\" -d " +
                QString::number(options.voxelizationResolution) + " \"" + scaledFilePath + "\" \"" +
                generatedBinvoxFile.fileName() + "\"";
    }
    else
    {
      command = "\"" + voxelizerInfo.absoluteFilePath() + "\" -pb -d " +
                QString::number(options.voxelizationResolution) + " \"" + scaledFilePath + "\"";
    }
#endif

    std::cout << "Running " << qPrintable(command) << std::endl;

    QProcess process;
    process.start(command);
    process.waitForFinished(-1);
    std::cerr << process.readAllStandardError().data() << std::endl;

    if(!generatedBinvoxFile.exists())
    {
      std::cerr << "The mesh could not be voxelized." << std::endl;
      return false;
    }

    generatedBinvoxFile.rename(binvoxFile.fileName());

    QFile scaledFile(scaledFilePath);
    scaledFile.remove();
  }
  else if(selectedFileInfo.suffix().compare("binvox", Qt::CaseInsensitive) == 0)
  {
    binvoxFilePath = options.inputPath;
  }
  else
  {
    std::cerr << "Unsupported input extension: " << selectedFileInfo.suffix().toStdString() << std::endl;
    return false;
  }

  plugin->loadVoxelization(binvoxFilePath);

  LegoCloudNode *legoCloudNode = plugin->getLegoCloudNode();
  if(!legoCloudNode)
    return false;

  if(options.preHollow)
    legoCloudNode->getLegoCloud()->preHollow(options.shellThickness);

  return true;
}

bool BrickrCli::isMeshExtensionSupported(const QString &extension)
{
  return extension.compare("obj", Qt::CaseInsensitive) == 0;
}

bool BrickrCli::scaleMesh(const QString &filePath, const QString &scaledFilePath)
{
  QFile infile(filePath);
  if(!infile.open(QFile::ReadOnly))
  {
    std::cerr << "Unable to read file: " << filePath.toStdString() << std::endl;
    return false;
  }

  QFile outfile(scaledFilePath);
  if(!outfile.open(QFile::WriteOnly | QFile::Truncate))
  {
    std::cerr << "Unable to create scaled file: " << scaledFilePath.toStdString() << std::endl;
    return false;
  }

  QTextStream in(&infile);
  QTextStream out(&outfile);

  while(!in.atEnd())
  {
    QString input = in.readLine();
    if(input.isEmpty() || input[0] == '#')
      continue;

    QTextStream ts(&input);
    QString id;
    ts >> id;
    if(id == "v")
    {
      Vector3 p;
      for(int i = 0; i < 3; ++i)
        ts >> p[i];

      out << "v " << p[0] << " " << 0.83333333 * p[1] << " " << p[2] << "\n";
    }
    else
    {
      out << input << "\n";
    }
  }

  return true;
}

QString BrickrCli::locateVoxelizer()
{
#ifdef WIN32
  QFileInfo voxelizerInfo(QCoreApplication::applicationDirPath() + "/binvox.exe");
  return voxelizerInfo.exists() ? voxelizerInfo.absoluteFilePath() : QString();
#else
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
    qEnvironmentVariable("BINVOX_PATH"),
    appDir + "/binvox",
    appDir + "/resources/binvox",
    appDir + "/../resources/binvox",
    appDir + "/../Resources/binvox"
  };

  foreach(const QString &candidatePath, candidates)
  {
    if(candidatePath.isEmpty())
      continue;

    QFileInfo candidateInfo(candidatePath);
    if(candidateInfo.exists())
      return candidateInfo.absoluteFilePath();
  }

  return QString();
#endif
}

bool BrickrCli::saveInstructions(LegoCloudNode *legoCloudNode, const QString &filePathBase)
{
  QFileInfo fileInfo(filePathBase);
  const bool useSVG = fileInfo.suffix().compare("svg", Qt::CaseInsensitive) == 0;

  if(fileInfo.suffix().isEmpty())
  {
    std::cerr << "Instruction output path must include an extension (.svg, .png, or .jpg)." << std::endl;
    return false;
  }

  QGraphicsScene scene;

  for(int level = 0; level < legoCloudNode->getLegoCloud()->getLevelNumber(); level++)
  {
    legoCloudNode->setRenderLayer(level);
    legoCloudNode->drawInstructions(&scene, !useSVG);

    const int imageSizeX = legoCloudNode->getLegoCloud()->getWidth() * BRICK_PIXEL_SIZE;
    const int imageSizeY = legoCloudNode->getLegoCloud()->getDepth() * BRICK_PIXEL_SIZE;
    const QString filePathLevel = fileInfo.absolutePath() + "/" + fileInfo.baseName() + "_" +
                                  QString::number(level) + "." + fileInfo.completeSuffix();

    std::cout << "Saving: " << filePathLevel.toStdString() << std::endl;

    if(useSVG)
    {
      QSvgGenerator svgGen;
      svgGen.setFileName(filePathLevel);
      svgGen.setSize(QSize(imageSizeX, imageSizeY));
      svgGen.setViewBox(QRect(0, 0, imageSizeX, imageSizeY));

      QPainter painter(&svgGen);
      scene.render(&painter);
    }
    else
    {
      QImage image(QSize(imageSizeX, imageSizeY), QImage::Format_ARGB32_Premultiplied);
      image.fill(QColor("White"));
      QPainter painter(&image);
      painter.setRenderHint(QPainter::Antialiasing);
      scene.render(&painter);
      image.save(filePathLevel);
    }
  }

  return true;
}
