#ifndef BRICKR_CLI_H
#define BRICKR_CLI_H

#include <QString>

class AssemblyPlugin;
class LegoCloudNode;

struct BrickrCliOptions
{
  QString inputPath;
  int voxelizationResolution;
  bool preHollow;
  int shellThickness;
  bool autoOptimize;
  bool finalize;
  bool printStats;
  QString exportObjPath;
  QString saveInstructionsPath;

  BrickrCliOptions()
    : voxelizationResolution(30),
      preHollow(false),
      shellThickness(2),
      autoOptimize(false),
      finalize(false),
      printStats(false)
  {
  }
};

class BrickrCli
{
public:
  static int run(const BrickrCliOptions &options);

private:
  static bool loadInput(AssemblyPlugin *plugin, const BrickrCliOptions &options);
  static bool isMeshExtensionSupported(const QString &extension);
  static bool scaleMesh(const QString &filePath, const QString &scaledFilePath);
  static QString locateVoxelizer();
  static bool saveInstructions(LegoCloudNode *legoCloudNode, const QString &filePathBase);
};

#endif
